#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rbdimmerESP32.h"

#define ZERO_CROSS_PIN  18
#define DIMMER_PIN_1    19
#define DIMMER_PIN_2    21
#define PHASE_NUM       0

static const char *TAG = "DIMMER_EXAMPLE";
rbdimmer_channel_t* channel1 = NULL;
rbdimmer_channel_t* channel2 = NULL;

void app_main(void)
{
    // Initialize library
    rbdimmer_init();
    
    // Register zero-cross detector (one per phase)
    rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);
    
    // Create first channel (incandescent bulbs)
    rbdimmer_config_t config1 = {
        .gpio_pin = DIMMER_PIN_1,
        .phase = PHASE_NUM,
        .initial_level = 50,
        .curve_type = RBDIMMER_CURVE_RMS
    };
    rbdimmer_create_channel(&config1, &channel1);
    
    // Create second channel (dimmable LED lighting)
    rbdimmer_config_t config2 = {
        .gpio_pin = DIMMER_PIN_2,
        .phase = PHASE_NUM,
        .initial_level = 50,
        .curve_type = RBDIMMER_CURVE_LOGARITHMIC
    };
    rbdimmer_create_channel(&config2, &channel2);
    
    // Main control loop
    while (1) {
        // Control channels independently
        rbdimmer_set_level(channel1, 75);
        rbdimmer_set_level(channel2, 25);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        
        rbdimmer_set_level(channel1, 25);
        rbdimmer_set_level(channel2, 75);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
