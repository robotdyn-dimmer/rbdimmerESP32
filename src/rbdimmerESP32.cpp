/**
 * @file rbdimmerESP32.cpp
 * @brief Facade: wires subsystem modules together and exposes the public API
 *
 * After Sprint 2 modularisation this file is intentionally thin:
 *   - rbdimmer_init / rbdimmer_deinit   — orchestrate module lifecycle
 *   - rbdimmer_register_zero_cross      — input validation + delegation
 *   - rbdimmer_get_frequency            — direct delegation
 *   - rbdimmer_set_callback             — direct delegation
 *   - rbdimmer_set_level_transition     — FreeRTOS smooth-transition task
 *                                         (Story 2.6: will move to rbdimmer_transition.c)
 *
 * All channel lifecycle and control functions live in rbdimmer_channel.c.
 * Zero-cross detection lives in rbdimmer_zerocross.c.
 * Timer state machine lives in rbdimmer_timer.c.
 * Brightness curves live in rbdimmer_curves.c.
 *
 * @author dev@rbdimmer.com
 * @version 1.0.0
 * @see https://rbdimmer.com
 * @copyright Copyright (c) 2024 RBDimmer
 * @license MIT License
 */

#include "rbdimmerESP32.h"
#include "internal/rbdimmer_zerocross.h"
#include "internal/rbdimmer_curves.h"
#include "internal/rbdimmer_channel.h"
#include <esp_log.h>

#define TAG "RBDIMMER"

// ---------------------------------------------------------------------------
// Library lifecycle
// ---------------------------------------------------------------------------

rbdimmer_err_t rbdimmer_init(void) {
    rbdimmer_zc_init();
    rbdimmer_channel_manager_init();   // also registers ZC phase-trigger
    rbdimmer_curves_init();
    ESP_LOGI(TAG, "RBDimmer library initialized");
    return RBDIMMER_OK;
}

rbdimmer_err_t rbdimmer_deinit(void) {
    rbdimmer_channel_manager_deinit(); // deletes all channels first
    rbdimmer_zc_deinit();
    ESP_LOGI(TAG, "RBDimmer library deinitialized");
    return RBDIMMER_OK;
}

// ---------------------------------------------------------------------------
// Zero-cross registration
// ---------------------------------------------------------------------------

rbdimmer_err_t rbdimmer_register_zero_cross(uint8_t pin, uint8_t phase,
                                              uint16_t frequency) {
    if (frequency != 0 &&
        (frequency < RBDIMMER_FREQUENCY_MIN ||
         frequency > RBDIMMER_FREQUENCY_MAX)) {
        ESP_LOGW(TAG, "Frequency out of recommended range, using auto-detect");
        frequency = RBDIMMER_DEFAULT_FREQUENCY;
    }
    rbdimmer_err_t err = rbdimmer_zc_register(pin, phase, frequency);
    if (err != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Zero-cross registration failed: %d", err);
        return err;
    }
    ESP_LOGI(TAG, "Zero-cross registered on pin %d phase %d", pin, phase);
    return RBDIMMER_OK;
}

// ---------------------------------------------------------------------------
// Zero-cross queries (thin delegation)
// ---------------------------------------------------------------------------

uint16_t rbdimmer_get_frequency(uint8_t phase) {
    return rbdimmer_zc_get_frequency(phase);
}

rbdimmer_err_t rbdimmer_set_callback(uint8_t phase,
                                      void (*callback)(void*),
                                      void* user_data) {
    return rbdimmer_zc_set_callback(phase, callback, user_data);
}

// rbdimmer_set_level_transition() implemented in rbdimmer_transition.c
