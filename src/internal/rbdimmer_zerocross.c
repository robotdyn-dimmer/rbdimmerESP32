/**
 * @file rbdimmer_zerocross.c
 * @brief Zero-crossing detection, frequency measurement, and ISR management
 * @internal
 */

#include "rbdimmer_zerocross.h"
#include "rbdimmer_hal.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Module-private state
// ---------------------------------------------------------------------------

// Minimum time between two valid ZC edges (µs).  Configurable via Kconfig
// (RBDIMMER_ZC_DEBOUNCE_US); falls back to 3000 us for Arduino builds.
#ifdef CONFIG_RBDIMMER_ZC_DEBOUNCE_US
#  define ZC_DEBOUNCE_US  CONFIG_RBDIMMER_ZC_DEBOUNCE_US
#else
#  define ZC_DEBOUNCE_US  3000
#endif

// O(1) ISR lookup: gpio_num → index in zero_cross_manager.zero_cross[]
static DRAM_ATTR int8_t gpio_to_phase_map[GPIO_NUM_MAX];
static volatile bool gpio_phase_map_initialized = false;

// DRAM_ATTR: read by find_by_pin() and zero_cross_isr_handler() in ISR context.
static DRAM_ATTR struct {
    rbdimmer_zero_cross_t zero_cross[RBDIMMER_MAX_PHASES];
    uint8_t count;
    bool isr_installed;
} zero_cross_manager;

// Phase-trigger callback registered by the channel layer.
// Called from ISR context on every zero-crossing.
// DRAM_ATTR: function pointer read in ISR context.
static DRAM_ATTR rbdimmer_zc_phase_trigger_t phase_trigger_cb = NULL;

// ---------------------------------------------------------------------------
// Internal helpers (IRAM_ATTR — called from ISR)
// ---------------------------------------------------------------------------

static IRAM_ATTR rbdimmer_zero_cross_t* find_by_pin(uint8_t pin) {
    if (pin >= GPIO_NUM_MAX || !gpio_phase_map_initialized) {
        return NULL;
    }
    int8_t idx = gpio_to_phase_map[pin];
    if (idx < 0 || idx >= (int8_t)zero_cross_manager.count) {
        return NULL;
    }
    return &zero_cross_manager.zero_cross[idx];
}

// No ESP_LOG inside — called from ISR, format strings live in flash
static IRAM_ATTR void measure_frequency(rbdimmer_zero_cross_t* zc,
                                         uint32_t current_time) {
    if (zc->frequency_measured) {
        return;
    }
    if (zc->last_cross_time > 0) {
        uint32_t period_us = current_time - zc->last_cross_time;
        // Noise filter: valid half-cycle range (5ms .. 15ms covers 33..100 Hz)
        if (period_us > 5000 && period_us < 15000) {
            zc->total_period_us += period_us;
            zc->measurement_count++;
            if (zc->measurement_count >= 50) {
                uint32_t avg = zc->total_period_us / zc->measurement_count;
                if (avg >= 9000 && avg <= 11000) {        // 50 Hz ±10%
                    zc->frequency = 50;
                    zc->half_cycle_us = 10000;
                    zc->frequency_measured = true;
                } else if (avg >= 7500 && avg <= 9166) {  // 60 Hz ±10%
                    zc->frequency = 60;
                    zc->half_cycle_us = 8333;
                    zc->frequency_measured = true;
                } else {
                    // Unknown — reset and retry
                    zc->frequency = 0;
                    zc->frequency_measured = false;
                    zc->measurement_count = 0;
                    zc->total_period_us = 0;
                }
            }
        }
    }
    zc->last_cross_time = current_time;
}

// ---------------------------------------------------------------------------
// GPIO ISR handler
// ---------------------------------------------------------------------------

static void IRAM_ATTR zero_cross_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t)arg;
    rbdimmer_zero_cross_t* zc = find_by_pin((uint8_t)gpio_num);

    if (zc == NULL || !zc->is_active) {
        return;
    }

    uint32_t now = (uint32_t)esp_timer_get_time();

    if (!zc->frequency_measured) {
        // Auto-measure frequency until determined
        measure_frequency(zc, now);
    } else {
        // Noise gate: reject edges closer than ZC_DEBOUNCE_US.
        // Eliminates TRIAC-induced spikes and optocoupler bounce that would
        // otherwise reset the delay timer mid-half-cycle → missed pulses → flicker.
        uint32_t elapsed = now - zc->last_cross_time;
        if (zc->last_cross_time > 0 && elapsed < ZC_DEBOUNCE_US) {
            return;
        }
        zc->last_cross_time = now;
    }

    // User zero-cross callback (must be IRAM_ATTR if provided)
    if (zc->callback) {
        zc->callback(zc->user_data);
    }

    // Notify channel layer: fire all channels on this phase
    if (phase_trigger_cb) {
        phase_trigger_cb(zc->phase);
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void rbdimmer_zc_set_phase_trigger(rbdimmer_zc_phase_trigger_t cb) {
    phase_trigger_cb = cb;
}

void rbdimmer_zc_init(void) {
    memset(&zero_cross_manager, 0, sizeof(zero_cross_manager));
    memset(gpio_to_phase_map, -1, sizeof(gpio_to_phase_map));
    gpio_phase_map_initialized = true;
}

rbdimmer_err_t rbdimmer_zc_register(uint8_t pin, uint8_t phase,
                                      uint16_t frequency) {
    if (phase >= RBDIMMER_MAX_PHASES || !RBDIMMER_HAL_IS_INPUT_GPIO(pin)) {
        return RBDIMMER_ERR_INVALID_ARG;
    }

    // Check for duplicate phase
    for (int i = 0; i < zero_cross_manager.count; i++) {
        if (zero_cross_manager.zero_cross[i].phase == phase) {
            return RBDIMMER_ERR_ALREADY_EXIST;
        }
    }

    if (zero_cross_manager.count >= RBDIMMER_MAX_PHASES) {
        return RBDIMMER_ERR_NO_MEMORY;
    }

    // Configure GPIO input
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    if (gpio_config(&io_conf) != ESP_OK) {
        return RBDIMMER_ERR_GPIO_FAILED;
    }

    // Install ISR service once (ESP_INTR_FLAG_IRAM: ISR must be IRAM-resident)
    if (!zero_cross_manager.isr_installed) {
        esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            return RBDIMMER_ERR_GPIO_FAILED;
        }
        zero_cross_manager.isr_installed = true;
    }

    if (gpio_isr_handler_add((gpio_num_t)pin, zero_cross_isr_handler,
                              (void*)(uint32_t)pin) != ESP_OK) {
        return RBDIMMER_ERR_GPIO_FAILED;
    }

    // Fill ZC descriptor
    rbdimmer_zero_cross_t* zc =
        &zero_cross_manager.zero_cross[zero_cross_manager.count];
    zc->pin = pin;
    zc->phase = phase;
    zc->frequency = frequency;
    zc->half_cycle_us = (frequency > 0) ? (1000000 / (2 * frequency)) : 10000;
    zc->last_cross_time = 0;
    zc->callback = NULL;
    zc->user_data = NULL;
    zc->is_active = true;
    zc->frequency_measured = false;
    zc->measurement_count = 0;
    zc->total_period_us = 0;

    // Register O(1) lookup entry
    gpio_to_phase_map[pin] = (int8_t)zero_cross_manager.count;
    zero_cross_manager.count++;

    return RBDIMMER_OK;
}

void rbdimmer_zc_deinit(void) {
    for (int i = 0; i < zero_cross_manager.count; i++) {
        gpio_isr_handler_remove(
            (gpio_num_t)zero_cross_manager.zero_cross[i].pin);
    }
    if (zero_cross_manager.isr_installed) {
        gpio_uninstall_isr_service();
        zero_cross_manager.isr_installed = false;
    }
    memset(&zero_cross_manager, 0, sizeof(zero_cross_manager));
    memset(gpio_to_phase_map, -1, sizeof(gpio_to_phase_map));
    gpio_phase_map_initialized = false;
}

rbdimmer_zero_cross_t* rbdimmer_zc_get_by_phase(uint8_t phase) {
    for (int i = 0; i < zero_cross_manager.count; i++) {
        if (zero_cross_manager.zero_cross[i].phase == phase) {
            return &zero_cross_manager.zero_cross[i];
        }
    }
    return NULL;
}

uint16_t rbdimmer_zc_get_frequency(uint8_t phase) {
    rbdimmer_zero_cross_t* zc = rbdimmer_zc_get_by_phase(phase);
    return zc ? zc->frequency : 0;
}

rbdimmer_err_t rbdimmer_zc_set_callback(uint8_t phase,
                                          void (*callback)(void*),
                                          void* user_data) {
    rbdimmer_zero_cross_t* zc = rbdimmer_zc_get_by_phase(phase);
    if (zc == NULL) {
        return RBDIMMER_ERR_NOT_FOUND;
    }
    zc->callback = callback;
    zc->user_data = user_data;
    return RBDIMMER_OK;
}
