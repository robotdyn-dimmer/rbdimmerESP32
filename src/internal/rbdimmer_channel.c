/**
 * @file rbdimmer_channel.c
 * @brief Dimmer channel lifecycle, control, and zero-cross firing
 * @internal
 *
 * Owns dimmer_manager and on_zero_cross_phase (ISR phase-trigger).
 * Implements all public channel API declared in rbdimmerESP32.h:
 *   create/delete, set_level, set_active, set_curve, getters, update_all.
 */

#include "rbdimmer_channel.h"
#include "rbdimmer_hal.h"
#include "rbdimmer_zerocross.h"
#include "rbdimmer_timer.h"
#include "rbdimmer_curves.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <stdlib.h>
#include <string.h>

#define TAG "RBDIMMER"

// ---------------------------------------------------------------------------
// Module-private state
// ---------------------------------------------------------------------------

// DRAM_ATTR: read by on_zero_cross_phase() in GPIO ISR context.
// Without it, a cache-miss during flash write (NVS/OTA) could cause an exception.
static DRAM_ATTR struct {
    rbdimmer_channel_t* channels[RBDIMMER_MAX_CHANNELS];
    uint8_t count;
} dimmer_manager;

// Spinlock protecting dimmer_manager.channels[] and .count.
// Task side:  portENTER_CRITICAL / portEXIT_CRITICAL
// (ISR side does not acquire this lock — see delete_channel comments)
// DRAM_ATTR required because portMUX is touched in ISR-adjacent paths.
static DRAM_ATTR portMUX_TYPE dimmer_spinlock = portMUX_INITIALIZER_UNLOCKED;

// Forward declaration
static void update_channel_delay(rbdimmer_channel_t* channel);

// ---------------------------------------------------------------------------
// ISR phase-trigger
// ---------------------------------------------------------------------------

// Called from zero-cross ISR for every zero-crossing on a given phase.
// Two-pass design:
//   Pass 1 — immediately stop all timers and drive all TRIAC GPIOs LOW so
//             every channel on this phase is reset at the same ZC instant.
//   Pass 2 — arm the delay timers now that all outputs are safely deasserted.
// IRAM_ATTR: runs directly in GPIO ISR context.
static IRAM_ATTR void on_zero_cross_phase(uint8_t phase) {
    // Pass 1: GPIO LOW for all active channels on this phase
    for (int i = 0; i < dimmer_manager.count; i++) {
        rbdimmer_channel_t* channel = dimmer_manager.channels[i];
        if (channel->is_active && channel->phase == phase) {

            esp_timer_stop(channel->delay_timer);
            esp_timer_stop(channel->pulse_timer);
            gpio_set_level((gpio_num_t)channel->gpio_pin, 0);
            channel->timer_state = TIMER_STATE_IDLE;
        }
    }
    // Pass 2: arm delay timers
    for (int i = 0; i < dimmer_manager.count; i++) {
        rbdimmer_channel_t* channel = dimmer_manager.channels[i];
        if (channel->is_active && channel->phase == phase) {
            if (channel->current_delay > 0) {
                esp_timer_start_once(channel->delay_timer, channel->current_delay);
                channel->timer_state = TIMER_STATE_DELAY;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Module lifecycle
// ---------------------------------------------------------------------------

rbdimmer_err_t rbdimmer_channel_manager_init(void) {
    memset(&dimmer_manager, 0, sizeof(dimmer_manager));
    rbdimmer_zc_set_phase_trigger(on_zero_cross_phase);
    return RBDIMMER_OK;
}

void rbdimmer_channel_manager_deinit(void) {
    while (dimmer_manager.count > 0) {
        rbdimmer_delete_channel(dimmer_manager.channels[0]);
    }
}

// ---------------------------------------------------------------------------
// Public API — channel lifecycle
// ---------------------------------------------------------------------------

rbdimmer_err_t rbdimmer_create_channel(rbdimmer_config_t* config,
                                        rbdimmer_channel_t** channel) {
    if (config == NULL || channel == NULL) {
        ESP_LOGE(TAG, "NULL parameters");
        return RBDIMMER_ERR_INVALID_ARG;
    }

    // W6: check capacity BEFORE allocating any resources
    if (dimmer_manager.count >= RBDIMMER_MAX_CHANNELS) {
        ESP_LOGE(TAG, "Maximum number of channels reached (%d)", RBDIMMER_MAX_CHANNELS);
        return RBDIMMER_ERR_NO_MEMORY;
    }

    if (!RBDIMMER_HAL_IS_OUTPUT_GPIO(config->gpio_pin)) {
        ESP_LOGE(TAG, "GPIO %d is not a valid output pin on this chip "
                 "(e.g. GPIO34-39 are input-only on ESP32)", config->gpio_pin);
        return RBDIMMER_ERR_INVALID_ARG;
    }

    // W3: reject duplicate GPIO — two channels on the same pin would fight
    // over the TRIAC gate and produce undefined hardware behaviour.
    for (int i = 0; i < dimmer_manager.count; i++) {
        if (dimmer_manager.channels[i]->gpio_pin == config->gpio_pin) {
            ESP_LOGE(TAG, "GPIO %d is already used by channel %d",
                     config->gpio_pin, i);
            return RBDIMMER_ERR_ALREADY_EXIST;
        }
    }

    rbdimmer_zero_cross_t* zc = rbdimmer_zc_get_by_phase(config->phase);
    if (zc == NULL) {
        ESP_LOGE(TAG, "Phase %d not registered", config->phase);
        return RBDIMMER_ERR_NOT_FOUND;
    }

    rbdimmer_channel_t* new_channel =
        (rbdimmer_channel_t*)malloc(sizeof(rbdimmer_channel_t));
    if (new_channel == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return RBDIMMER_ERR_NO_MEMORY;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask   = (1ULL << config->gpio_pin),
        .mode           = GPIO_MODE_OUTPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_DISABLE
    };
    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "GPIO configuration failed");
        free(new_channel);
        return RBDIMMER_ERR_GPIO_FAILED;
    }

    gpio_set_level((gpio_num_t)config->gpio_pin, 0);

    if (rbdimmer_timer_create(new_channel) != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to create timers");
        free(new_channel);
        return RBDIMMER_ERR_TIMER_FAILED;
    }

    new_channel->gpio_pin          = config->gpio_pin;
    new_channel->phase             = config->phase;
    new_channel->level_percent     = config->initial_level > 100 ? 100
                                                                  : config->initial_level;
    new_channel->prev_level_percent = 255; // force update on first run
    new_channel->curve_type        = config->curve_type;
    new_channel->is_active         = true;
    new_channel->needs_update      = true;
    new_channel->timer_state       = TIMER_STATE_IDLE;
    new_channel->transition_task   = NULL;
    new_channel->current_delay     = rbdimmer_curves_level_to_delay(
        new_channel->level_percent,
        zc->half_cycle_us,
        new_channel->curve_type
    );

    ESP_LOGI(TAG, "Initial delay: %"PRIu32" us, half-cycle: %"PRIu32" us",
             new_channel->current_delay, (uint32_t)zc->half_cycle_us);

    // C4/C5 fix: write pointer then increment count inside a critical section.
    // portENTER_CRITICAL disables interrupts on this core (prevents ISR) and
    // acquires the spinlock (prevents concurrent task access from other core).
    // It also provides a full CPU memory barrier on both Xtensa and RISC-V,
    // which a plain compiler barrier (__asm__ volatile) does not.
    portENTER_CRITICAL(&dimmer_spinlock);
    dimmer_manager.channels[dimmer_manager.count] = new_channel;
    dimmer_manager.count++;
    portEXIT_CRITICAL(&dimmer_spinlock);

    *channel = new_channel;

    ESP_LOGI(TAG, "Dimmer channel created on pin %d, phase %d",
             config->gpio_pin, config->phase);
    return RBDIMMER_OK;
}

rbdimmer_err_t rbdimmer_delete_channel(rbdimmer_channel_t* channel) {
    if (channel == NULL) {
        return RBDIMMER_ERR_INVALID_ARG;
    }

    // Step 1: Deactivate — on_zero_cross_phase() checks is_active before
    // touching timers, so after this write the ISR will skip this channel
    // on every subsequent zero-crossing.
    channel->is_active = false;

    // Step 2: Stop timers so no further callbacks can fire.
    // If the ISR already started a timer cycle before seeing is_active=false,
    // delay_timer_callback will check timer_state==TIMER_STATE_DELAY and
    // find TIMER_STATE_IDLE (set below), returning without effect.
    esp_timer_stop(channel->delay_timer);
    esp_timer_stop(channel->pulse_timer);
    gpio_set_level((gpio_num_t)channel->gpio_pin, 0);
    channel->timer_state = TIMER_STATE_IDLE;

    // Step 3: Locate channel in manager (array not yet modified, safe to read).
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

    // Step 4: Compact array inside critical section.
    // portENTER_CRITICAL disables interrupts on this core (prevents the GPIO
    // ISR from running here) and acquires the spinlock (prevents a concurrent
    // task on the other core from modifying the array at the same time).
    portENTER_CRITICAL(&dimmer_spinlock);
    for (int i = index; i < (int)dimmer_manager.count - 1; i++) {
        dimmer_manager.channels[i] = dimmer_manager.channels[i + 1];
    }
    dimmer_manager.count--;
    portEXIT_CRITICAL(&dimmer_spinlock);

    // Step 5: Delete timer handles and free memory outside the critical section
    // (esp_timer_delete may call into the FreeRTOS heap allocator).
    rbdimmer_timer_delete(channel);
    free(channel);
    return RBDIMMER_OK;
}

// ---------------------------------------------------------------------------
// Public API — control
// ---------------------------------------------------------------------------

rbdimmer_err_t rbdimmer_set_level(rbdimmer_channel_t* channel,
                                   uint8_t level_percent) {
    if (channel == NULL) {
        return RBDIMMER_ERR_INVALID_ARG;
    }
    if (level_percent > 100) {
        level_percent = 100;
    }
    if (channel->level_percent != level_percent) {
        channel->prev_level_percent = channel->level_percent;
        channel->level_percent      = level_percent;
        channel->needs_update       = true;
        if (channel->is_active) {
            update_channel_delay(channel);
        }
    }
    return RBDIMMER_OK;
}

rbdimmer_err_t rbdimmer_set_curve(rbdimmer_channel_t* channel,
                                   rbdimmer_curve_t curve_type) {
    if (channel == NULL) {
        return RBDIMMER_ERR_INVALID_ARG;
    }
    if (curve_type != channel->curve_type) {
        channel->curve_type   = curve_type;
        channel->needs_update = true;
        ESP_LOGI(TAG, "Setting curve type to %d", curve_type);
        if (channel->is_active) {
            update_channel_delay(channel);
        }
    }
    return RBDIMMER_OK;
}

rbdimmer_err_t rbdimmer_set_active(rbdimmer_channel_t* channel, bool active) {
    if (channel == NULL) {
        return RBDIMMER_ERR_INVALID_ARG;
    }
    if (channel->is_active != active) {
        channel->is_active    = active;
        channel->needs_update = true;
        ESP_LOGI(TAG, "Setting channel active state to %d", active);
        if (!active) {
            gpio_set_level((gpio_num_t)channel->gpio_pin, 0);
            esp_timer_stop(channel->delay_timer);
            esp_timer_stop(channel->pulse_timer);
            channel->timer_state = TIMER_STATE_IDLE;
        }
    }
    return RBDIMMER_OK;
}

rbdimmer_err_t rbdimmer_update_all(void) {
    for (int i = 0; i < dimmer_manager.count; i++) {
        if (dimmer_manager.channels[i]->is_active) {
            update_channel_delay(dimmer_manager.channels[i]);
        }
    }
    return RBDIMMER_OK;
}

// ---------------------------------------------------------------------------
// Public API — getters
// ---------------------------------------------------------------------------

uint8_t rbdimmer_get_level(rbdimmer_channel_t* channel) {
    if (channel == NULL) return 0;
    return channel->level_percent;
}

bool rbdimmer_is_active(rbdimmer_channel_t* channel) {
    if (channel == NULL) return false;
    return channel->is_active;
}

rbdimmer_curve_t rbdimmer_get_curve(rbdimmer_channel_t* channel) {
    if (channel == NULL) return RBDIMMER_CURVE_LINEAR;
    return channel->curve_type;
}

uint32_t rbdimmer_get_delay(rbdimmer_channel_t* channel) {
    if (channel == NULL) return 0;
    return channel->current_delay;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static void update_channel_delay(rbdimmer_channel_t* channel) {
    if (!channel->needs_update) {
        return;
    }
    rbdimmer_zero_cross_t* zc = rbdimmer_zc_get_by_phase(channel->phase);
    if (zc == NULL) {
        return;
    }
    uint32_t new_delay = rbdimmer_curves_level_to_delay(
        channel->level_percent,
        zc->half_cycle_us,
        channel->curve_type
    );
    if (new_delay != channel->current_delay) {
        channel->current_delay = new_delay;
    }
    channel->needs_update = false;
}
