/**
 * @file main.c
 * @brief Minimal build-verification app for rbdimmerESP32 CI.
 *
 * Not intended to run on hardware — just exercises the public API surface
 * so that every chip × framework combination is compile-tested in CI.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rbdimmerESP32.h"

#define TAG "CI"

void app_main(void)
{
    ESP_LOGI(TAG, "rbdimmerESP32 build test — version check OK");
    ESP_LOGI(TAG, "ZC_DEBOUNCE_US  : %d", CONFIG_RBDIMMER_ZC_DEBOUNCE_US);
    ESP_LOGI(TAG, "MIN_DELAY_US    : %d", CONFIG_RBDIMMER_MIN_DELAY_US);
    ESP_LOGI(TAG, "LEVEL_MIN       : %d", CONFIG_RBDIMMER_LEVEL_MIN);
    ESP_LOGI(TAG, "LEVEL_MAX       : %d", CONFIG_RBDIMMER_LEVEL_MAX);

    rbdimmer_err_t err = rbdimmer_init();
    ESP_LOGI(TAG, "rbdimmer_init: %d", (int)err);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
