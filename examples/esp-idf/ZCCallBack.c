#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rbdimmerESP32.h"

#define ZERO_CROSS_PIN  18
#define DIMMER_PIN      19
#define LED_PIN         2  // Built-in LED for zero-cross visualization
#define PHASE_NUM       0

static const char *TAG = "DIMMER_CALLBACK";
rbdimmer_channel_t* dimmer = NULL;
QueueHandle_t zero_cross_queue;

// Simple message for our queue
typedef struct {
    uint32_t timestamp;
} ZeroCrossEvent_t;

// Callback function for zero-cross events
void zero_cross_callback(void* arg)
{
    ZeroCrossEvent_t event;
    event.timestamp = esp_timer_get_time() / 1000; // Current time in ms
    
    // Send to queue from ISR
    BaseType_t higher_priority_task_woken = pdFALSE;
    xQueueSendFromISR(zero_cross_queue, &event, &higher_priority_task_woken);
    
    if (higher_priority_task_woken) {
        portYIELD_FROM_ISR();
    }
}

// Task to process zero-cross events
void zero_cross_processing_task(void *pvParameters)
{
    ZeroCrossEvent_t event;
    
    while (1) {
        if (xQueueReceive(zero_cross_queue, &event, portMAX_DELAY)) {
            // Toggle LED to visualize zero-crossing
            gpio_set_level(LED_PIN, !gpio_get_level(LED_PIN));
            
            // Additional processing can be done here safely
            ESP_LOGI(TAG, "Zero-cross event at time: %lu ms", event.timestamp);
        }
    }
}

void app_main(void)
{
    // Setup LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    // Create the queue 
    zero_cross_queue = xQueueCreate(10, sizeof(ZeroCrossEvent_t));
    if (zero_cross_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
        return;
    }
    
    // Create the task to process zero-cross events
    BaseType_t task_created = xTaskCreate(
        zero_cross_processing_task,
        "ZeroCrossTask",
        2048,
        NULL,
        5,
        NULL
    );
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        return;
    }
    
    // Initialize dimmer
    rbdimmer_init();
    rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);
    
    // Register callback
    rbdimmer_set_callback(PHASE_NUM, zero_cross_callback, NULL);
    
    // Create dimmer channel
    rbdimmer_config_t config = {
        .gpio_pin = DIMMER_PIN,
        .phase = PHASE_NUM,
        .initial_level = 60,
        .curve_type = RBDIMMER_CURVE_RMS
    };
    
    rbdimmer_create_channel(&config, &dimmer);
    ESP_LOGI(TAG, "Dimmer with callback initialized");
    
    // Main loop - print frequency information
    while (1) {
        uint16_t frequency = rbdimmer_get_frequency(PHASE_NUM);
        ESP_LOGI(TAG, "Detected frequency: %u Hz", frequency);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
