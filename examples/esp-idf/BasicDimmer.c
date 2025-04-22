#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rbdimmerESP32.h"

static const char *TAG = "DIMMER_EXAMPLE";

// Pins
#define ZERO_CROSS_PIN  18   // Zero-Cross pin
#define DIMMER_PIN      19   // Dimming control pin
#define PHASE_NUM       0    // Phase N (0 for single phase)

// Global variables. Dimmer object
rbdimmer_channel_t* dimmer_channel = NULL;

void app_main(void)
{
    ESP_LOGI(TAG, "AC Dimmer Test");
    
    // Dimmer lib init
    if (rbdimmer_init() != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to initialize AC Dimmer library");
        return;
    }
    
    // Zero-cross detector and phase registry
    if (rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0) != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to register zero-cross detector");
        return;
    }
    
    // Dimmer channel. Configuration data structure.
    rbdimmer_config_t config_channel = {
        .gpio_pin = DIMMER_PIN,
        .phase = PHASE_NUM,
        .initial_level = 50,  // Initial dimming level 50%
        .curve_type = RBDIMMER_CURVE_RMS  // Level Curve Selection. RMS-curve
    };
    
    if (rbdimmer_create_channel(&config_channel, &dimmer_channel) != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to create dimmer channel");
        return;
    }
    
    ESP_LOGI(TAG, "AC Dimmer initialized successfully");
    
    // Main loop
    while (1) {
        // dimming from 10% to 90% with step 10
        for (int brightness = 10; brightness <= 90; brightness += 10) {
            ESP_LOGI(TAG, "Setting brightness to %d%%", brightness);
            rbdimmer_set_level(dimmer_channel, brightness);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
        
        // Smooth transition from current level to 0 level in 5 sec
        ESP_LOGI(TAG, "Smooth transition to 0%%");
        rbdimmer_set_level_transition(dimmer_channel, 0, 5000);
        vTaskDelay(6000 / portTICK_PERIOD_MS); // delay 6 sec
        
        // Smooth transition from current level (0) to 100 level in 5 sec
        ESP_LOGI(TAG, "Smooth transition to 100%%");
        rbdimmer_set_level_transition(dimmer_channel, 100, 5000);
        vTaskDelay(6000 / portTICK_PERIOD_MS); // delay 6 sec
    }
}
