/**
 * @file ZCCallBack.c
 * @brief Advanced Zero-Cross Callback Example for ESP-IDF Framework
 * 
 * This sophisticated example demonstrates professional interrupt handling
 * and real-time synchronization using the rbdimmerESP32 library's callback
 * system in pure ESP-IDF environment. It showcases the critical patterns
 * for building systems that require precise synchronization with AC mains.
 * 
 * @section ISRConcepts Critical ISR Concepts in ESP-IDF
 * This example demonstrates several advanced concepts:
 * - IRAM-safe interrupt handlers for minimal latency
 * - FreeRTOS queue communication from ISR context
 * - Proper ISR-to-task communication patterns
 * - Real-time frequency measurement and monitoring
 * - Resource-safe interrupt handling
 * 
 * @section Architecture System Architecture Overview
 * The callback system implements a three-layer architecture:
 * 1. Hardware Layer: Zero-cross detection generates interrupts
 * 2. ISR Layer: Minimal processing, capture critical data
 * 3. Task Layer: Complex processing in normal context
 * 
 * This separation ensures real-time performance while allowing
 * complex operations like logging, network communication, or
 * calculations that would be unsafe in interrupt context.
 * 
 * @section UseCases Professional Use Cases
 * - Power quality monitoring and analysis
 * - Synchronized multi-device control systems
 * - Flicker-free environments for video production
 * - Precision timing for industrial processes
 * - Grid frequency monitoring and logging
 * - Phase-locked lighting effects
 * 
 * @section Hardware Hardware Requirements
 * - ESP32 development board
 * - RBDimmer AC dimmer module
 * - AC load (incandescent bulb recommended)
 * - LED on GPIO 2 for visual feedback
 * - Oscilloscope (optional, for timing verification)
 * 
 * @section Wiring Wiring Connections
 * - GPIO 18 -> Dimmer ZC output (zero-cross detection)
 * - GPIO 19 -> Dimmer PWM input (control signal)
 * - GPIO 2 -> Built-in LED (visual indicator)
 * - 3.3V -> Dimmer VCC
 * - GND -> Dimmer GND
 * 
 * @section Expected Expected Behavior and Output
 * @code
 * I (325) DIMMER_CALLBACK: === RBDimmer Zero-Cross Callback ESP-IDF Example ===
 * I (335) DIMMER_CALLBACK: Creating FreeRTOS queue for ISR communication...
 * I (345) DIMMER_CALLBACK: Creating zero-cross processing task...
 * I (355) RBDIMMER: RBDimmer library initialized
 * I (365) RBDIMMER: Zero-cross detector registered on pin 18
 * I (375) DIMMER_CALLBACK: Callback registered successfully
 * I (385) RBDIMMER: Dimmer channel created on pin 19
 * I (395) DIMMER_CALLBACK: System initialized, LED should flash at mains frequency
 * I (405) DIMMER_CALLBACK: [Task] Zero-cross processing task started
 * I (1405) DIMMER_CALLBACK: Detected frequency: 50 Hz
 * I (1405) DIMMER_CALLBACK: Zero-cross count: 100 (last second)
 * I (1415) DIMMER_CALLBACK: Period stability: ±0.15%
 * I (2405) DIMMER_CALLBACK: [Task] Processed event #200, period: 10.01 ms
 * @endcode
 * 
 * @section Safety Critical Safety Considerations
 * When working with mains-synchronized systems:
 * - Always maintain proper electrical isolation
 * - Never connect AC mains directly to microcontroller pins
 * - Use optically isolated zero-cross detectors
 * - Implement watchdog timers for fault detection
 * - Test with low-voltage AC during development
 * 
 * @author dev@rbdimmer.com
 * @version 1.0.0
 * @date 2024
 * 
 * @see Dimmers website: https://rbdimmer.com
 * @see Library repository: https://github.com/robotdyn-dimmer/rbdimmerESP32
 * @see Dimmers catalog: https://www.rbdimmer.com/dimmers-pricing
 * @see Dimmers documentation: https://www.rbdimmer.com/knowledge/article/45
 * @see Library documentation: https://www.rbdimmer.com/knowledge/article/59
 * @see Dimmers projects: https://www.rbdimmer.com/blog/dimmers-projects-5
 * @see Support and community: https://www.rbdimmer.com/forum
 * 
 * @copyright Copyright (c) 2024 RBDimmer
 * @license MIT License
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_attr.h"
#include "rbdimmerESP32.h"

/** @brief Tag for ESP-IDF logging system */
static const char *TAG = "DIMMER_CALLBACK";

/** @name Hardware Configuration
 *  @brief GPIO assignments and hardware parameters
 *  @{
 */
#define ZERO_CROSS_PIN  18   ///< GPIO for zero-cross detector input
#define DIMMER_PIN      19   ///< GPIO for dimmer control output
#define LED_PIN         2    ///< Built-in LED for visual feedback
#define PHASE_NUM       0    ///< AC phase number (single-phase)
/** @} */

/** @name System Configuration
 *  @brief FreeRTOS and timing parameters
 *  @{
 */
#define QUEUE_LENGTH    20   ///< Zero-cross event queue size
#define TASK_STACK_SIZE 4096 ///< Processing task stack in bytes
#define TASK_PRIORITY   5    ///< Processing task priority (1-25)
#define STATS_INTERVAL_MS 1000 ///< Statistics print interval
/** @} */

/** @name Timing Analysis Parameters
 *  @brief Constants for frequency analysis and stability monitoring
 *  @{
 */
#define NOMINAL_FREQ_50HZ  50.0f    ///< Nominal 50Hz frequency
#define NOMINAL_FREQ_60HZ  60.0f    ///< Nominal 60Hz frequency
#define FREQ_TOLERANCE     0.5f     ///< Frequency tolerance in Hz
#define STABILITY_WINDOW   100      ///< Number of samples for stability
/** @} */

/**
 * @brief Zero-cross event data structure
 * 
 * Contains all information captured during a zero-cross event
 * that needs to be passed from ISR to processing task
 */
typedef struct {
    uint64_t timestamp_us;  ///< Event timestamp in microseconds
    uint32_t event_count;   ///< Sequential event number
    uint32_t period_us;     ///< Period since last event (microseconds)
    uint8_t gpio_level;     ///< GPIO level at interrupt (for debugging)
} zero_cross_event_t;

/**
 * @brief System statistics structure
 * 
 * Maintains running statistics for system monitoring and analysis
 */
typedef struct {
    uint32_t total_events;          ///< Total zero-cross events
    uint32_t events_per_second;     ///< Current event rate
    uint64_t last_event_time;       ///< Last event timestamp
    float min_period_ms;            ///< Minimum period observed
    float max_period_ms;            ///< Maximum period observed
    float avg_period_ms;            ///< Average period
    float frequency_hz;             ///< Calculated frequency
    float stability_percent;        ///< Period stability metric
    uint32_t queue_overflows;       ///< Number of queue overflow events
} system_stats_t;

/** @name Global Variables
 *  @brief System-wide state and handles
 *  @{
 */
static QueueHandle_t zero_cross_queue = NULL;     ///< ISR to task communication queue
static TaskHandle_t processing_task = NULL;        ///< Processing task handle
static rbdimmer_channel_t* dimmer = NULL;         ///< Dimmer channel handle
static system_stats_t stats = {0};                ///< System statistics
static portMUX_TYPE stats_mutex = portMUX_INITIALIZER_UNLOCKED; ///< Statistics protection
/** @} */

/**
 * @brief Initialize GPIO for LED indicator
 * 
 * Configures the built-in LED for visual zero-cross indication
 * 
 * @return ESP_OK on success
 */
static esp_err_t init_indicator_led(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED GPIO");
        return ret;
    }
    
    gpio_set_level(LED_PIN, 0);
    ESP_LOGI(TAG, "LED indicator initialized on GPIO %d", LED_PIN);
    return ESP_OK;
}

/**
 * @brief Zero-cross callback function (ISR context)
 * 
 * This function is called by the rbdimmer library on every zero-cross
 * detection. It runs in interrupt context with strict limitations:
 * - Must be in IRAM (IRAM_ATTR)
 * - Cannot call most ESP-IDF functions
 * - Must complete quickly (target: <10μs)
 * - Can only use ISR-safe FreeRTOS functions
 * 
 * @param arg User-provided argument (unused in this example)
 * 
 * @warning This runs in ISR context - keep it minimal!
 */
static void IRAM_ATTR zero_cross_callback(void* arg)
{
    static uint64_t last_timestamp = 0;
    static uint32_t event_counter = 0;
    
    // Capture timestamp as early as possible for accuracy
    uint64_t current_time = esp_timer_get_time();
    
    // Calculate period (0 on first call)
    uint32_t period = 0;
    if (last_timestamp != 0) {
        period = current_time - last_timestamp;
    }
    last_timestamp = current_time;
    
    // Increment event counter
    event_counter++;
    
    // Toggle LED for visual indication (direct register access for speed)
    static bool led_state = false;
    led_state = !led_state;
    gpio_set_level(LED_PIN, led_state);
    
    // Create event data
    zero_cross_event_t event = {
        .timestamp_us = current_time,
        .event_count = event_counter,
        .period_us = period,
        .gpio_level = gpio_get_level(ZERO_CROSS_PIN)
    };
    
    // Send to queue with zero timeout (non-blocking)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result = xQueueSendFromISR(zero_cross_queue, &event, &xHigherPriorityTaskWoken);
    
    // Track queue overflows
    if (result != pdTRUE) {
        // Queue full - increment overflow counter
        // Note: We can't use mutex in ISR, so this is a simple increment
        stats.queue_overflows++;
    }
    
    // Yield to higher priority task if necessary
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief Update system statistics
 * 
 * Processes zero-cross event data to maintain running statistics
 * for frequency analysis and stability monitoring
 * 
 * @param event Pointer to zero-cross event data
 */
static void update_statistics(const zero_cross_event_t* event)
{
    portENTER_CRITICAL(&stats_mutex);
    
    stats.total_events = event->event_count;
    
    if (event->period_us > 0) {
        float period_ms = event->period_us / 1000.0f;
        
        // Update min/max
        if (stats.min_period_ms == 0 || period_ms < stats.min_period_ms) {
            stats.min_period_ms = period_ms;
        }
        if (period_ms > stats.max_period_ms) {
            stats.max_period_ms = period_ms;
        }
        
        // Update average (simple moving average)
        if (stats.avg_period_ms == 0) {
            stats.avg_period_ms = period_ms;
        } else {
            stats.avg_period_ms = (stats.avg_period_ms * 0.95f) + (period_ms * 0.05f);
        }
        
        // Calculate frequency (Hz = 1000ms / (period_ms * 2))
        stats.frequency_hz = 1000.0f / (stats.avg_period_ms * 2.0f);
        
        // Calculate stability (percentage variation from average)
        float variation = stats.max_period_ms - stats.min_period_ms;
        stats.stability_percent = (variation / stats.avg_period_ms) * 100.0f;
    }
    
    stats.last_event_time = event->timestamp_us;
    
    portEXIT_CRITICAL(&stats_mutex);
}

/**
 * @brief Zero-cross processing task
 * 
 * This FreeRTOS task receives events from the ISR queue and performs
 * all complex processing that cannot be done in interrupt context.
 * This includes logging, statistics calculation, and any operations
 * that might block or take significant time.
 * 
 * @param pvParameters Task parameters (unused)
 */
static void zero_cross_processing_task(void* pvParameters)
{
    zero_cross_event_t event;
    uint32_t events_this_second = 0;
    uint64_t last_second_time = esp_timer_get_time();
    
    ESP_LOGI(TAG, "[Task] Zero-cross processing task started");
    
    while (1) {
        // Wait for event from queue (blocking)
        if (xQueueReceive(zero_cross_queue, &event, portMAX_DELAY) == pdTRUE) {
            events_this_second++;
            
            // Update statistics
            update_statistics(&event);
            
            // Detailed logging every 100 events
            if (event.event_count % 100 == 0) {
                ESP_LOGI(TAG, "[Task] Processed event #%lu, period: %.2f ms",
                        event.event_count, event.period_us / 1000.0f);
            }
            
            // Update events per second counter
            uint64_t current_time = esp_timer_get_time();
            if (current_time - last_second_time >= 1000000) { // 1 second
                portENTER_CRITICAL(&stats_mutex);
                stats.events_per_second = events_this_second;
                portEXIT_CRITICAL(&stats_mutex);
                
                events_this_second = 0;
                last_second_time = current_time;
            }
            
            // Additional processing can be added here:
            // - Network reporting
            // - Data logging to SD card
            // - Trigger other system events
            // - Complex calculations
        }
    }
}

/**
 * @brief Print comprehensive system statistics
 * 
 * Displays detailed information about zero-cross detection,
 * frequency analysis, and system performance metrics
 */
static void print_statistics(void)
{
    // Copy stats with mutex protection
    system_stats_t local_stats;
    portENTER_CRITICAL(&stats_mutex);
    memcpy(&local_stats, &stats, sizeof(system_stats_t));
    portEXIT_CRITICAL(&stats_mutex);
    
    ESP_LOGI(TAG, "========== Zero-Cross Callback Statistics ==========");
    
    // Frequency analysis
    ESP_LOGI(TAG, "Frequency Analysis:");
    ESP_LOGI(TAG, "  Measured frequency: %.2f Hz", local_stats.frequency_hz);
    ESP_LOGI(TAG, "  Library reported: %u Hz", rbdimmer_get_frequency(PHASE_NUM));
    ESP_LOGI(TAG, "  Events per second: %lu", local_stats.events_per_second);
    ESP_LOGI(TAG, "  Total events: %lu", local_stats.total_events);
    
    // Timing analysis
    ESP_LOGI(TAG, "Timing Analysis:");
    ESP_LOGI(TAG, "  Average period: %.3f ms", local_stats.avg_period_ms);
    ESP_LOGI(TAG, "  Min period: %.3f ms", local_stats.min_period_ms);
    ESP_LOGI(TAG, "  Max period: %.3f ms", local_stats.max_period_ms);
    ESP_LOGI(TAG, "  Stability: ±%.2f%%", local_stats.stability_percent / 2.0f);
    
    // Determine mains standard
    float freq_diff_50 = fabs(local_stats.frequency_hz - NOMINAL_FREQ_50HZ);
    float freq_diff_60 = fabs(local_stats.frequency_hz - NOMINAL_FREQ_60HZ);
    const char* standard = (freq_diff_50 < freq_diff_60) ? "50Hz" : "60Hz";
    float deviation = (freq_diff_50 < freq_diff_60) ? freq_diff_50 : freq_diff_60;
    ESP_LOGI(TAG, "  Mains standard: %s (deviation: %.3f Hz)", standard, deviation);
    
    // System health
    ESP_LOGI(TAG, "System Health:");
    ESP_LOGI(TAG, "  Queue usage: %u/%d", uxQueueMessagesWaiting(zero_cross_queue), QUEUE_LENGTH);
    ESP_LOGI(TAG, "  Queue overflows: %lu", local_stats.queue_overflows);
    ESP_LOGI(TAG, "  Task stack watermark: %u bytes", uxTaskGetStackHighWaterMark(processing_task));
    ESP_LOGI(TAG, "  Free heap: %d bytes", esp_get_free_heap_size());
    
    // Dimmer status
    ESP_LOGI(TAG, "Dimmer Status:");
    ESP_LOGI(TAG, "  Brightness: %u%%", rbdimmer_get_level(dimmer));
    ESP_LOGI(TAG, "  Active: %s", rbdimmer_is_active(dimmer) ? "Yes" : "No");
    
    ESP_LOGI(TAG, "===================================================");
}

/**
 * @brief Main application entry point
 * 
 * Initializes all components of the advanced callback demonstration:
 * - GPIO for LED indicator
 * - FreeRTOS queue and task for event processing
 * - Dimmer library with callback registration
 * - Periodic statistics reporting
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== RBDimmer Zero-Cross Callback ESP-IDF Example ===");
    ESP_LOGI(TAG, "Demonstrating professional interrupt handling patterns");
    
    // Initialize LED indicator
    if (init_indicator_led() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LED");
        return;
    }
    
    // Create queue for ISR to task communication
    ESP_LOGI(TAG, "Creating FreeRTOS queue for ISR communication...");
    zero_cross_queue = xQueueCreate(QUEUE_LENGTH, sizeof(zero_cross_event_t));
    if (zero_cross_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return;
    }
    
    // Create processing task
    ESP_LOGI(TAG, "Creating zero-cross processing task...");
    BaseType_t ret = xTaskCreate(
        zero_cross_processing_task,
        "ZeroCrossProc",
        TASK_STACK_SIZE,
        NULL,
        TASK_PRIORITY,
        &processing_task
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create processing task");
        vQueueDelete(zero_cross_queue);
        return;
    }
    
    // Initialize dimmer library
    if (rbdimmer_init() != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to initialize dimmer library");
        vTaskDelete(processing_task);
        vQueueDelete(zero_cross_queue);
        return;
    }
    
    // Register zero-cross detector
    if (rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0) != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to register zero-cross detector");
        return;
    }
    
    // Register callback BEFORE creating dimmer channels
    ESP_LOGI(TAG, "Registering zero-cross callback...");
    if (rbdimmer_set_callback(PHASE_NUM, zero_cross_callback, NULL) != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to register callback");
        return;
    }
    ESP_LOGI(TAG, "Callback registered successfully");
    
    // Create dimmer channel
    rbdimmer_config_t config = {
        .gpio_pin = DIMMER_PIN,
        .phase = PHASE_NUM,
        .initial_level = 60,
        .curve_type = RBDIMMER_CURVE_RMS
    };
    
    if (rbdimmer_create_channel(&config, &dimmer) != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to create dimmer channel");
        return;
    }
    
    ESP_LOGI(TAG, "System initialized, LED should flash at mains frequency");
    ESP_LOGI(TAG, "Statistics will be printed every %d ms", STATS_INTERVAL_MS);
    
    // Main loop - print statistics and demonstrate dimmer control
    TickType_t last_stats_time = xTaskGetTickCount();
    TickType_t last_dimmer_change = xTaskGetTickCount();
    uint8_t brightness_sequence[] = {60, 30, 90, 60};
    int sequence_index = 0;
    
    while (1) {
        TickType_t current_time = xTaskGetTickCount();
        
        // Print statistics periodically
        if ((current_time - last_stats_time) * portTICK_PERIOD_MS >= STATS_INTERVAL_MS) {
            print_statistics();
            last_stats_time = current_time;
        }
        
        // Change dimmer brightness periodically to show non-interference
        if ((current_time - last_dimmer_change) * portTICK_PERIOD_MS >= 10000) {
            sequence_index = (sequence_index + 1) % (sizeof(brightness_sequence) / sizeof(brightness_sequence[0]));
            uint8_t new_brightness = brightness_sequence[sequence_index];
            
            ESP_LOGI(TAG, "Changing brightness to %u%% (demonstrating non-blocking operation)", new_brightness);
            rbdimmer_set_level_transition(dimmer, new_brightness, 2000);
            
            last_dimmer_change = current_time;
        }
        
        // Check for system issues
        if (stats.queue_overflows > 0) {
            ESP_LOGW(TAG, "Queue overflows detected: %lu (consider increasing queue size)", stats.queue_overflows);
            // Reset counter after warning
            portENTER_CRITICAL(&stats_mutex);
            stats.queue_overflows = 0;
            portEXIT_CRITICAL(&stats_mutex);
        }
        
        // Small delay to prevent watchdog
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @example ZCCallBack.c
 * 
 * This advanced ESP-IDF example demonstrates professional patterns for
 * handling hardware interrupts and real-time synchronization. Key concepts:
 * 
 * 1. **ISR Safety**: Shows proper IRAM_ATTR usage and ISR-safe operations
 *    to maintain microsecond-precision timing
 * 
 * 2. **FreeRTOS Patterns**: Demonstrates the standard ISR-to-task
 *    communication pattern using queues for deferred processing
 * 
 * 3. **Real-time Analysis**: Implements comprehensive frequency and
 *    stability monitoring suitable for power quality applications
 * 
 * 4. **Non-interference**: Proves that callback processing doesn't
 *    affect normal dimmer operation
 * 
 * 5. **Professional Monitoring**: Shows how to implement system health
 *    monitoring and performance metrics
 * 
 * This example is essential for applications requiring:
 * - Precise synchronization with AC mains
 * - Power quality monitoring
 * - Multi-device coordination
 * - Flicker-free video environments
 * - Industrial timing applications
 * 
 * The patterns demonstrated here form the foundation for building
 * sophisticated real-time control systems that maintain microsecond
 * precision while performing complex processing tasks.
 */