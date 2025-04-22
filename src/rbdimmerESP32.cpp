/**
 * @file rbdimmerESP32.cpp
 * @brief Implementation of the ESP32 AC Dimmer Library using esp_timer and hardware interrupts
 */

 #include "rbdimmerESP32.h"
 #include <esp_log.h>
 #include <string.h>
 #include "driver/gpio.h"
 #include "esp_intr_alloc.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "math.h"
 #include "esp_timer.h"
 
 #define TAG "RBDIMMER"
 
 // Structures
 typedef struct {
     uint8_t pin;                      // Zero-cross detector pin
     uint8_t phase;                    // Phase number
     uint16_t frequency;               // Mains frequency in Hz
     uint32_t half_cycle_us;           // Half-cycle duration in microseconds
     uint32_t last_cross_time;         // Time of last zero-crossing
     void (*callback)(void*);          // Callback for rising edge only
     void* user_data;                  // User data for callbacks
     bool is_active;                   // Active state flag
     
     // Оптимизация измерения частоты
     bool frequency_measured;          // Флаг, указывающий что частота определена
     uint8_t measurement_count;        // Счетчик измерений
     uint32_t total_period_us;         // Суммарный период для усреднения
 } rbdimmer_zero_cross_t;
 
 struct rbdimmer_channel_s {
     uint8_t gpio_pin;                 // Output pin
     uint8_t phase;                    // Reference to phase
     uint8_t level_percent;            // Current level percentage (0-100)
     uint8_t prev_level_percent;       // Previous level percentage
     uint32_t current_delay;           // Current delay in microseconds
     bool is_active;                   // Active state flag
     bool needs_update;                // Update flag
     rbdimmer_curve_t curve_type;      // Level curve type
     esp_timer_handle_t delay_timer;   // Timer for delay
     esp_timer_handle_t pulse_timer;   // Timer for pulse duration
     timer_state_t timer_state;        // Current timer state
 };
 
 // Managers
 static struct {
     rbdimmer_zero_cross_t zero_cross[RBDIMMER_MAX_PHASES];
     uint8_t count;
     bool isr_installed;
 } zero_cross_manager = {
     .count = 0,
     .isr_installed = false
 };
 
 static struct {
     rbdimmer_channel_t* channels[RBDIMMER_MAX_CHANNELS];
     uint8_t count;
 } dimmer_manager = {
     .count = 0
 };
 
 // Tables for level to delay conversion
 static uint8_t level_to_delay_table_linear[101];
 static uint8_t level_to_delay_table_rms[101];
 static uint8_t level_to_delay_table_log[101];
 
 // Forward declarations for internal functions
 static void init_level_tables(void);
 static rbdimmer_zero_cross_t* find_zero_cross_by_phase(uint8_t phase);
 static rbdimmer_zero_cross_t* find_zero_cross_by_pin(uint8_t pin);
 static void IRAM_ATTR zero_cross_isr_handler(void* arg);
 static uint32_t level_to_delay(uint8_t level_percent, uint32_t half_cycle_us, rbdimmer_curve_t curve_type);
 static void measure_frequency(rbdimmer_zero_cross_t* zc, uint32_t current_time);
 static void update_channel_delay(rbdimmer_channel_t* channel);
 static void IRAM_ATTR delay_timer_callback(void* arg);
 static void IRAM_ATTR pulse_timer_callback(void* arg);
 
 // Initialize the RBDimmer library
 rbdimmer_err_t rbdimmer_init(void) {
     // Initialize managers
     memset(zero_cross_manager.zero_cross, 0, sizeof(zero_cross_manager.zero_cross));
     zero_cross_manager.count = 0;
     zero_cross_manager.isr_installed = false;
     
     memset(dimmer_manager.channels, 0, sizeof(dimmer_manager.channels));
     dimmer_manager.count = 0;
     
     // Initialize level tables
     init_level_tables();
     
     ESP_LOGI(TAG, "RBDimmer library initialized");
     return RBDIMMER_OK;
 }
 
 // Register a zero-cross detector
 rbdimmer_err_t rbdimmer_register_zero_cross(uint8_t pin, uint8_t phase, uint16_t frequency) {
     if (phase >= RBDIMMER_MAX_PHASES) {
         ESP_LOGE(TAG, "Phase number out of range");
         return RBDIMMER_ERR_INVALID_ARG;
     }
     
     // Проверяем корректность номера GPIO пина
     if (pin >= GPIO_NUM_MAX) {
         ESP_LOGE(TAG, "GPIO pin number out of range");
         return RBDIMMER_ERR_INVALID_ARG;
     }
     
     gpio_num_t gpio_pin = static_cast<gpio_num_t>(pin);
     
     if (frequency < RBDIMMER_FREQUENCY_MIN || frequency > RBDIMMER_FREQUENCY_MAX) {
         ESP_LOGW(TAG, "Frequency out of recommended range, using default");
         frequency = RBDIMMER_DEFAULT_FREQUENCY;
         
         // Если DEFAULT_FREQUENCY = 0, переключаемся в режим автоизмерения
         if (frequency == 0) {
             ESP_LOGI(TAG, "Auto-measurement mode enabled");
         }
     }
     
     // Check if phase already exists
     for (int i = 0; i < zero_cross_manager.count; i++) {
         if (zero_cross_manager.zero_cross[i].phase == phase) {
             ESP_LOGE(TAG, "Phase %d already registered", phase);
             return RBDIMMER_ERR_ALREADY_EXIST;
         }
     }
     
     if (zero_cross_manager.count >= RBDIMMER_MAX_PHASES) {
         ESP_LOGE(TAG, "Maximum number of phases reached");
         return RBDIMMER_ERR_NO_MEMORY;
     }
     
     // Configure GPIO for interrupt
     gpio_config_t io_conf = {
         .pin_bit_mask = (1ULL << pin),
         .mode = GPIO_MODE_INPUT,
         .pull_up_en = GPIO_PULLUP_DISABLE,
         .pull_down_en = GPIO_PULLDOWN_DISABLE,
         .intr_type = GPIO_INTR_POSEDGE // Trigger on rising edge
     };
     
     esp_err_t err = gpio_config(&io_conf);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "GPIO configuration failed: %d", err);
         return RBDIMMER_ERR_GPIO_FAILED;
     }
     
     // Install ISR service if not already done
     if (!zero_cross_manager.isr_installed) {
         err = gpio_install_isr_service(0);
         if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
             ESP_LOGE(TAG, "ISR service installation failed: %d", err);
             return RBDIMMER_ERR_GPIO_FAILED;
         }
         zero_cross_manager.isr_installed = true;
     }
     
     // Add ISR handler
     err = gpio_isr_handler_add(gpio_pin, zero_cross_isr_handler, (void*)((uint32_t)pin));
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "ISR handler addition failed: %d", err);
         return RBDIMMER_ERR_GPIO_FAILED;
     }
     
     // Initialize zero-cross detector
     rbdimmer_zero_cross_t* zc = &zero_cross_manager.zero_cross[zero_cross_manager.count++];
     zc->pin = pin;
     zc->phase = phase;
     zc->frequency = frequency;
     if (frequency > 0) {
         zc->half_cycle_us = 1000000 / (2 * frequency);
     } else {
         // При frequency=0 устанавливаем значение для 50 Гц, будет обновлено после измерения
         zc->half_cycle_us = 10000; // 10 мс (предполагая 50 Гц)
     }
     zc->last_cross_time = 0;
     zc->callback = NULL;
     zc->user_data = NULL;
     zc->is_active = true;
     
     // Инициализация полей для измерения частоты
     zc->frequency_measured = false;
     zc->measurement_count = 0;
     zc->total_period_us = 0;
     
     ESP_LOGI(TAG, "Zero-cross detector registered on pin %d for phase %d", pin, phase);
     return RBDIMMER_OK;
 }
 
 // Create a dimmer channel
 rbdimmer_err_t rbdimmer_create_channel(rbdimmer_config_t* config, rbdimmer_channel_t** channel) {
     if (config == NULL || channel == NULL) {
         ESP_LOGE(TAG, "NULL parameters");
         return RBDIMMER_ERR_INVALID_ARG;
     }
     
     // Проверяем корректность номера GPIO пина
     if (config->gpio_pin >= GPIO_NUM_MAX) {
         ESP_LOGE(TAG, "GPIO pin number out of range: %d", config->gpio_pin);
         return RBDIMMER_ERR_INVALID_ARG;
     }
     
     // Check if phase exists
     rbdimmer_zero_cross_t* zc = find_zero_cross_by_phase(config->phase);
     if (zc == NULL) {
         ESP_LOGE(TAG, "Phase %d not registered", config->phase);
         return RBDIMMER_ERR_NOT_FOUND;
     }
     
     // Allocate memory for the channel
     rbdimmer_channel_t* new_channel = (rbdimmer_channel_t*)malloc(sizeof(rbdimmer_channel_t));
     if (new_channel == NULL) {
         ESP_LOGE(TAG, "Memory allocation failed");
         return RBDIMMER_ERR_NO_MEMORY;
     }
     
     // Configure GPIO for output
     gpio_config_t io_conf = {
         .pin_bit_mask = (1ULL << config->gpio_pin),
         .mode = GPIO_MODE_OUTPUT,
         .pull_up_en = GPIO_PULLUP_DISABLE,
         .pull_down_en = GPIO_PULLDOWN_DISABLE,
         .intr_type = GPIO_INTR_DISABLE
     };
     
     esp_err_t err = gpio_config(&io_conf);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "GPIO configuration failed: %d", err);
         free(new_channel);
         return RBDIMMER_ERR_GPIO_FAILED;
     }
     
     gpio_set_level((gpio_num_t)config->gpio_pin, 0); // Initialize to LOW
     
     // Configure the delay timer
     esp_timer_create_args_t delay_timer_args = {
         .callback = &delay_timer_callback,
         .arg = new_channel,
         .name = "dimmer_delay"
     };
     
     err = esp_timer_create(&delay_timer_args, &new_channel->delay_timer);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "Failed to create delay timer: %d", err);
         free(new_channel);
         return RBDIMMER_ERR_TIMER_FAILED;
     }
     
     // Configure the pulse timer
     esp_timer_create_args_t pulse_timer_args = {
         .callback = &pulse_timer_callback,
         .arg = new_channel,
         .name = "dimmer_pulse"
     };
     
     err = esp_timer_create(&pulse_timer_args, &new_channel->pulse_timer);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "Failed to create pulse timer: %d", err);
         esp_timer_delete(new_channel->delay_timer);
         free(new_channel);
         return RBDIMMER_ERR_TIMER_FAILED;
     }
     
     // Initialize channel data
     new_channel->gpio_pin = config->gpio_pin;
     new_channel->phase = config->phase;
     new_channel->level_percent = config->initial_level > 100 ? 100 : config->initial_level;
     new_channel->prev_level_percent = 255; // Force update on first run
     new_channel->curve_type = config->curve_type;
     new_channel->is_active = true;
     new_channel->needs_update = true;
     new_channel->timer_state = TIMER_STATE_IDLE;
     
     // Calculate initial delay
     new_channel->current_delay = level_to_delay(
         new_channel->level_percent,
         zc->half_cycle_us,
         new_channel->curve_type
     );
     
     ESP_LOGI(TAG, "Initial delay: %d us, half-cycle: %d us", new_channel->current_delay, zc->half_cycle_us);
     
     // Add channel to manager
     if (dimmer_manager.count < RBDIMMER_MAX_CHANNELS) {
         dimmer_manager.channels[dimmer_manager.count++] = new_channel;
     } else {
         ESP_LOGE(TAG, "Maximum number of channels reached");
         esp_timer_delete(new_channel->delay_timer);
         esp_timer_delete(new_channel->pulse_timer);
         free(new_channel);
         return RBDIMMER_ERR_NO_MEMORY;
     }
     
     // Return the channel handle
     *channel = new_channel;
     
     ESP_LOGI(TAG, "Dimmer channel created on pin %d, phase %d", 
         config->gpio_pin, config->phase);
     return RBDIMMER_OK;
 }
 
 // Set channel level
 rbdimmer_err_t rbdimmer_set_level(rbdimmer_channel_t* channel, uint8_t level_percent) {
     if (channel == NULL) {
         return RBDIMMER_ERR_INVALID_ARG;
     }
     
     if (level_percent > 100) {
         level_percent = 100;
     }
     
     // Only update if the value changed
     if (channel->level_percent != level_percent) {
         channel->prev_level_percent = channel->level_percent;
         channel->level_percent = level_percent;
         channel->needs_update = true;
         
         // Immediately update channel delay if active
         if (channel->is_active) {
             update_channel_delay(channel);
         }
     }
     
     return RBDIMMER_OK;
 }
 
 // Структура параметров перехода для передачи в задачу
 typedef struct {
     rbdimmer_channel_t* channel;      // Канал диммера
     uint8_t start_level;              // Начальная яркость
     uint8_t target_level;             // Целевая яркость
     uint32_t transition_ms;           // Время перехода в мс
     uint32_t step_ms;                 // Время между шагами
     TaskHandle_t task_handle;         // Хэндлер задачи для автоудаления
 } transition_params_t;
 
 // Задача для плавного изменения яркости
 static void level_transition_task(void* pvParameters) {
     transition_params_t* params = (transition_params_t*)pvParameters;
     
     uint8_t current = params->start_level;
     uint8_t target = params->target_level;
     int8_t step = (target > current) ? 1 : -1;
     uint32_t steps = abs(target - current);
     uint32_t step_time = (steps > 0) ? (params->transition_ms / steps) : params->transition_ms;
     
     // Убедиться, что шаг не меньше минимального времени
     if (step_time < params->step_ms) {
         step_time = params->step_ms;
     }
     
     // Выполняем шаги изменения яркости
     while (current != target) {
         // Обновляем яркость
         rbdimmer_set_level(params->channel, current);
         
         // Продвигаемся к цели
         current += step;
         
         // Проверяем, не перескочили ли цель
         if ((step > 0 && current > target) || (step < 0 && current < target)) {
             current = target;
         }
         
         // Ждем до следующего шага
         vTaskDelay(pdMS_TO_TICKS(step_time));
     }
     
     // Финальное обновление (на всякий случай)
     rbdimmer_set_level(params->channel, target);
     
     // Очищаем ресурсы
     free(params);
     
     // Удаляем текущую задачу
     vTaskDelete(NULL);
 }
 
 // Обновленная функция плавного изменения яркости
 rbdimmer_err_t rbdimmer_set_level_transition(rbdimmer_channel_t* channel, uint8_t level_percent, uint32_t transition_ms) {
     if (channel == NULL) {
         return RBDIMMER_ERR_INVALID_ARG;
     }
     
     if (level_percent > 100) {
         level_percent = 100;
     }
     
     // Если текущая яркость уже равна целевой, ничего не делаем
     if (channel->level_percent == level_percent) {
         return RBDIMMER_OK;
     }
     
     // Если время перехода слишком маленькое, меняем мгновенно
     if (transition_ms < 50) {
         return rbdimmer_set_level(channel, level_percent);
     }
     
     // Создаем структуру параметров
     transition_params_t* params = (transition_params_t*)malloc(sizeof(transition_params_t));
     if (params == NULL) {
         ESP_LOGE(TAG, "Failed to allocate memory for transition parameters");
         return RBDIMMER_ERR_NO_MEMORY;
     }
     
     // Заполняем параметры
     params->channel = channel;
     params->start_level = channel->level_percent;
     params->target_level = level_percent;
     params->transition_ms = transition_ms;
     params->step_ms = 20; // Минимальное время между шагами (50 Гц)
     
     // Создаем задачу
     BaseType_t task_created = xTaskCreate(
         level_transition_task,
         "dimmer_transition",
         2048,
         params,
         5,
         &params->task_handle
     );
     
     if (task_created != pdPASS) {
         ESP_LOGE(TAG, "Failed to create transition task");
         free(params);
         return RBDIMMER_ERR_NO_MEMORY;
     }
     
     return RBDIMMER_OK;
 }
 
 // Set level curve type
 rbdimmer_err_t rbdimmer_set_curve(rbdimmer_channel_t* channel, rbdimmer_curve_t curve_type) {
     if (channel == NULL) {
         return RBDIMMER_ERR_INVALID_ARG;
     }
     
     if (curve_type != channel->curve_type) {
         channel->curve_type = curve_type;
         channel->needs_update = true;
         
         ESP_LOGI(TAG, "Setting curve type to %d", curve_type);
         
         // Immediately update channel delay if active
         if (channel->is_active) {
             update_channel_delay(channel);
         }
     }
     
     return RBDIMMER_OK;
 }
 
 // Enable or disable a channel
 rbdimmer_err_t rbdimmer_set_active(rbdimmer_channel_t* channel, bool active) {
     if (channel == NULL) {
         return RBDIMMER_ERR_INVALID_ARG;
     }
     
     if (channel->is_active != active) {
         channel->is_active = active;
         channel->needs_update = true;
         
         ESP_LOGI(TAG, "Setting channel active state to %d", active);
         
         // If deactivating, ensure output is low and stop any running timers
         if (!active) {
             gpio_set_level((gpio_num_t)channel->gpio_pin, 0);
             esp_timer_stop(channel->delay_timer);
             esp_timer_stop(channel->pulse_timer);
             channel->timer_state = TIMER_STATE_IDLE;
         }
     }
     
     return RBDIMMER_OK;
 }
 
 // Get current channel level
 uint8_t rbdimmer_get_level(rbdimmer_channel_t* channel) {
     if (channel == NULL) {
         return 0;
     }
     
     return channel->level_percent;
 }
 
 // Get measured mains frequency for specified phase
 uint16_t rbdimmer_get_frequency(uint8_t phase) {
     rbdimmer_zero_cross_t* zc = find_zero_cross_by_phase(phase);
     if (zc == NULL) {
         return 0;
     }
     
     return zc->frequency;
 }
 
 // Set callback function for zero-cross events
 rbdimmer_err_t rbdimmer_set_callback(uint8_t phase, void (*callback)(void*), void* user_data) {
     rbdimmer_zero_cross_t* zc = find_zero_cross_by_phase(phase);
     if (zc == NULL) {
         return RBDIMMER_ERR_NOT_FOUND;
     }
     
     zc->callback = callback;
     zc->user_data = user_data;
     return RBDIMMER_OK;
 }
 
 // Force update of all channels
 rbdimmer_err_t rbdimmer_update_all(void) {
     for (int i = 0; i < dimmer_manager.count; i++) {
         if (dimmer_manager.channels[i]->is_active) {
             update_channel_delay(dimmer_manager.channels[i]);
         }
     }
     return RBDIMMER_OK;
 }
 
 // Delete a dimmer channel and release resources
 rbdimmer_err_t rbdimmer_delete_channel(rbdimmer_channel_t* channel) {
     if (channel == NULL) {
         return RBDIMMER_ERR_INVALID_ARG;
     }
     
     // Find channel in manager
     int index = -1;
     for (int i = 0; i < dimmer_manager.count; i++) {
         if (dimmer_manager.channels[i] == channel) {
             index = i;
             break;
         }
     }
     
     if (index == -1) {
         return RBDIMMER_ERR_NOT_FOUND;
     }
     
     // Stop and delete timers
     esp_timer_stop(channel->delay_timer);
     esp_timer_delete(channel->delay_timer);
     esp_timer_stop(channel->pulse_timer);
     esp_timer_delete(channel->pulse_timer);
     
     // Ensure GPIO is low
     gpio_set_level((gpio_num_t)channel->gpio_pin, 0);
     
     // Remove from manager and shift remaining channels
     for (int i = index; i < dimmer_manager.count - 1; i++) {
         dimmer_manager.channels[i] = dimmer_manager.channels[i + 1];
     }
     dimmer_manager.count--;
     
     // Free memory
     free(channel);
     
     return RBDIMMER_OK;
 }
 
 // Deinitialize the RBDimmer library
 rbdimmer_err_t rbdimmer_deinit(void) {
     // Delete all channels
     while (dimmer_manager.count > 0) {
         rbdimmer_delete_channel(dimmer_manager.channels[0]);
     }
     
     // Remove ISR handlers for all zero-cross detectors
     for (int i = 0; i < zero_cross_manager.count; i++) {
         gpio_isr_handler_remove((gpio_num_t)zero_cross_manager.zero_cross[i].pin);
     }
     
     // Uninstall ISR service
     if (zero_cross_manager.isr_installed) {
         gpio_uninstall_isr_service();
         zero_cross_manager.isr_installed = false;
     }
     
     // Reset manager data
     memset(zero_cross_manager.zero_cross, 0, sizeof(zero_cross_manager.zero_cross));
     zero_cross_manager.count = 0;
     
     ESP_LOGI(TAG, "RBDimmer library deinitialized");
     return RBDIMMER_OK;
 }
 
 // Check if a channel is active
 bool rbdimmer_is_active(rbdimmer_channel_t* channel) {
     if (channel == NULL) {
         return false;
     }
     return channel->is_active;
 }
 
 // Get the curve type of a channel
 rbdimmer_curve_t rbdimmer_get_curve(rbdimmer_channel_t* channel) {
     if (channel == NULL) {
         return RBDIMMER_CURVE_LINEAR;
     }
     return channel->curve_type;
 }
 
 // Get the current delay setting of a channel
 uint32_t rbdimmer_get_delay(rbdimmer_channel_t* channel) {
     if (channel == NULL) {
         return 0;
     }
     return channel->current_delay;
 }
 
 //-----------------------------------------------------------------------------
 // Internal functions
 //-----------------------------------------------------------------------------
 
 // Initialize level tables
 static void init_level_tables(void) {
     // Linear table (simply inverted percentages)
     for (int i = 0; i <= 100; i++) {
         level_to_delay_table_linear[i] = 100 - i;
     }
     
     // RMS compensation table (based on RMS effect of sine wave)
     for (int i = 0; i <= 100; i++) {
         float level_normalized = i / 100.0f;
         if (level_normalized <= 0.0f) {
             level_to_delay_table_rms[i] = 100;
         } else if (level_normalized >= 1.0f) {
             level_to_delay_table_rms[i] = 0;
         } else {
             // Angle calculation based on RMS power: angle = arccos(sqrt(level))
             float angle_rad = acos(sqrt(level_normalized));
             float delay_percent = angle_rad / M_PI;
             level_to_delay_table_rms[i] = round(delay_percent * 100.0f);
         }
     }
     
     // Logarithmic table (better for LED loads)
     for (int i = 0; i <= 100; i++) {
         float level_normalized = i / 100.0f;
         if (level_normalized <= 0.0f) {
             level_to_delay_table_log[i] = 100;
         } else if (level_normalized >= 1.0f) {
             level_to_delay_table_log[i] = 0;
         } else {
             // Logarithmic scale (approximately perceivable as linear by human eye)
             float log_value = log10(1.0f + 9.0f * level_normalized) / log10(10.0f);
             float delay_percent = 1.0f - log_value;
             level_to_delay_table_log[i] = round(delay_percent * 100.0f);
         }
     }
 }
 
 // Find zero-cross detector by phase
 static rbdimmer_zero_cross_t* find_zero_cross_by_phase(uint8_t phase) {
     for (int i = 0; i < zero_cross_manager.count; i++) {
         if (zero_cross_manager.zero_cross[i].phase == phase) {
             return &zero_cross_manager.zero_cross[i];
         }
     }
     return NULL;
 }
 
 // Find zero-cross detector by pin
 static rbdimmer_zero_cross_t* find_zero_cross_by_pin(uint8_t pin) {
     for (int i = 0; i < zero_cross_manager.count; i++) {
         if (zero_cross_manager.zero_cross[i].pin == pin) {
             return &zero_cross_manager.zero_cross[i];
         }
     }
     return NULL;
 }
 
 // Convert level percentage to delay
 static uint32_t level_to_delay(uint8_t level_percent, uint32_t half_cycle_us, rbdimmer_curve_t curve_type) {
     if (level_percent >= 100) {
         return RBDIMMER_MIN_DELAY_US; // Минимальная задержка для макс. яркости
     }
     
     if (level_percent <= 0) {
         return half_cycle_us - RBDIMMER_DEFAULT_PULSE_WIDTH_US; // Почти полная задержка
     }
     
     uint8_t delay_percent;
     
     switch (curve_type) {
         case RBDIMMER_CURVE_RMS:
             delay_percent = level_to_delay_table_rms[level_percent];
             break;
         case RBDIMMER_CURVE_LOGARITHMIC:
             delay_percent = level_to_delay_table_log[level_percent];
             break;
         case RBDIMMER_CURVE_LINEAR:
         default:
             delay_percent = level_to_delay_table_linear[level_percent];
             break;
     }
     
     // Преобразуем процент задержки в микросекунды
     uint32_t delay_us = (half_cycle_us * delay_percent) / 100;
     
     // Применяем ограничения
     if (delay_us < RBDIMMER_MIN_DELAY_US) {
         delay_us = RBDIMMER_MIN_DELAY_US;
     }
     
     if (delay_us > half_cycle_us - RBDIMMER_DEFAULT_PULSE_WIDTH_US) {
         delay_us = half_cycle_us - RBDIMMER_DEFAULT_PULSE_WIDTH_US;
     }
     
     return delay_us;
 }
 
 // Measure mains frequency based on zero-cross events
 static void measure_frequency(rbdimmer_zero_cross_t* zc, uint32_t current_time) {
     // Если частота уже определена, ничего не делаем
     if (zc->frequency_measured) {
         return;
     }
     
     if (zc->last_cross_time > 0) {
         uint32_t period_us = current_time - zc->last_cross_time;
         
         // Отбрасываем слишком короткие или длинные периоды как шум
         if (period_us > 5000 && period_us < 15000) {
             zc->total_period_us += period_us;
             zc->measurement_count++;
             
             // После сбора данных за 10 полных циклов (20 полупериодов)
             if (zc->measurement_count >= 20) {
                 // Вычисляем средний период
                 uint32_t avg_half_cycle_us = zc->total_period_us / zc->measurement_count;
                 
                 // Проверяем соответствие распространенным частотам
                 // Допуск ±10% от ожидаемых значений
                 
                 // 50 Гц -> полупериод = 10000 мкс (±1000 мкс)
                 if (avg_half_cycle_us >= 9000 && avg_half_cycle_us <= 11000) {
                     zc->frequency = 50;
                     zc->half_cycle_us = 10000;
                     zc->frequency_measured = true;
                 } 
                 // 60 Гц -> полупериод = 8333 мкс (±833 мкс)
                 else if (avg_half_cycle_us >= 7500 && avg_half_cycle_us <= 9166) {
                     zc->frequency = 60;
                     zc->half_cycle_us = 8333;
                     zc->frequency_measured = true;
                 } 
                 // Неизвестная частота
                 else {
                     // Сообщаем об ошибке и сбрасываем счетчик для повторного измерения
                     ESP_LOGE(TAG, "Unknown mains frequency detected! Average half-cycle: %d us", avg_half_cycle_us);
                     zc->frequency = 0;
                     zc->frequency_measured = false;
                     zc->measurement_count = 0;
                     zc->total_period_us = 0;
                 }
                 
                 if (zc->frequency_measured) {
                     ESP_LOGI(TAG, "Frequency measurement complete: %d Hz (half-cycle: %d us)", 
                             zc->frequency, zc->half_cycle_us);
                 }
             }
         }
     }
     
     zc->last_cross_time = current_time;
 }
 
 // Update channel's delay based on level and curve
 static void update_channel_delay(rbdimmer_channel_t* channel) {
     if (!channel->needs_update) {
         return;
     }
     
     // Find corresponding zero-cross
     rbdimmer_zero_cross_t* zc = find_zero_cross_by_phase(channel->phase);
     if (zc == NULL) {
         return;
     }
     
     // Calculate new delay
     uint32_t new_delay = level_to_delay(
         channel->level_percent,
         zc->half_cycle_us,
         channel->curve_type
     );
     
     // Update if different
     if (new_delay != channel->current_delay) {
         channel->current_delay = new_delay;
     }
     
     channel->needs_update = false;
 }
 
 // Callback for delay timer
 static void IRAM_ATTR delay_timer_callback(void* arg) {
     rbdimmer_channel_t* channel = (rbdimmer_channel_t*)arg;
     
     if (channel == NULL || channel->timer_state != TIMER_STATE_DELAY) {
         return;
     }
     
     // Set output pin high to start pulse
     gpio_set_level((gpio_num_t)channel->gpio_pin, 1);
     
     // Update state
     channel->timer_state = TIMER_STATE_PULSE_ON;
 }
 
 // Callback for pulse timer
 static void IRAM_ATTR pulse_timer_callback(void* arg) {
     rbdimmer_channel_t* channel = (rbdimmer_channel_t*)arg;
     
     if (channel == NULL || channel->timer_state != TIMER_STATE_PULSE_ON) {
         return;
     }
     
     // Set output pin low to end pulse
     gpio_set_level((gpio_num_t)channel->gpio_pin, 0);
     
     // Reset state
     channel->timer_state = TIMER_STATE_IDLE;
 }
 
 // Zero-cross interrupt handler
 static void IRAM_ATTR zero_cross_isr_handler(void* arg) {
     uint32_t gpio_num = (uint32_t)arg;
     
     // Find zero-cross detector
     rbdimmer_zero_cross_t* zc = find_zero_cross_by_pin(gpio_num);
     
     if (zc == NULL || !zc->is_active) {
         return;
     }
     
     // Измеряем частоту, если она еще не определена
     if (!zc->frequency_measured) {
         uint32_t current_time = micros();
         measure_frequency(zc, current_time);
     }
     
     // Вызываем callback если он зарегистрирован
     if (zc->callback) {
         zc->callback(zc->user_data);
     }
     
     // Запускаем таймеры для всех каналов на этой фазе
     for (int i = 0; i < dimmer_manager.count; i++) {
         rbdimmer_channel_t* channel = dimmer_manager.channels[i];
 
         if (channel->is_active && channel->phase == zc->phase) {
             // Запускаем таймеры только если канал в исходном состоянии
             if (channel->timer_state == TIMER_STATE_IDLE) {
                 // Останавливаем любые запущенные таймеры (для безопасности)
                 esp_timer_stop(channel->delay_timer);
                 esp_timer_stop(channel->pulse_timer);
                 
                 // Запускаем таймер задержки (включит выход)
                 esp_timer_start_once(channel->delay_timer, channel->current_delay);
                 
                 // Запускаем таймер импульса (выключит выход)
                 esp_timer_start_once(channel->pulse_timer, 
                     channel->current_delay + RBDIMMER_DEFAULT_PULSE_WIDTH_US);
                 
                 // Обновляем состояние
                 channel->timer_state = TIMER_STATE_DELAY;
             }
         }
     }
 }
