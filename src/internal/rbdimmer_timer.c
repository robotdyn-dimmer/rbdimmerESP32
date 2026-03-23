/**
 * @file rbdimmer_timer.c
 * @brief TRIAC pulse timer state machine
 * @internal
 *
 * Two one-shot timers per channel, both ESP_TIMER_ISR dispatched:
 *   delay_timer  — zero-cross → TRIAC gate HIGH
 *   pulse_timer  — TRIAC gate HIGH → LOW (fixed pulse width)
 *
 * Sequential chain (Fix 1.2): pulse_timer is started from delay_timer
 * callback, never from the ISR, to guarantee constant pulse width.
 *
 * Both callbacks are IRAM_ATTR (Fix 1.1 + Fix 1.5).
 */

#include "rbdimmer_timer.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "esp_idf_version.h"

// ESP_TIMER_ISR dispatch (used below) was introduced in ESP-IDF 5.0.
// Arduino Core 3.x is based on ESP-IDF 5.x — earlier cores are not supported.
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
  #error "rbdimmerESP32 requires ESP-IDF >= 5.0.0 (Arduino Core >= 3.0)"
#endif

// ---------------------------------------------------------------------------
// ISR-context callbacks
// ---------------------------------------------------------------------------

static void IRAM_ATTR delay_timer_callback(void* arg) {
    rbdimmer_channel_t* channel = (rbdimmer_channel_t*)arg;

    if (channel == NULL || channel->timer_state != TIMER_STATE_DELAY) {
        return;
    }

    // Fire TRIAC
    gpio_set_level((gpio_num_t)channel->gpio_pin, 1);
    channel->timer_state = TIMER_STATE_PULSE_ON;

    // Start pulse timer — guarantees fixed pulse width regardless of jitter
    esp_timer_start_once(channel->pulse_timer, RBDIMMER_DEFAULT_PULSE_WIDTH_US);
}

static void IRAM_ATTR pulse_timer_callback(void* arg) {
    rbdimmer_channel_t* channel = (rbdimmer_channel_t*)arg;

    if (channel == NULL || channel->timer_state != TIMER_STATE_PULSE_ON) {
        return;
    }

    // End TRIAC pulse
    gpio_set_level((gpio_num_t)channel->gpio_pin, 0);
    channel->timer_state = TIMER_STATE_IDLE;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

rbdimmer_err_t rbdimmer_timer_create(rbdimmer_channel_t* channel) {
    // delay timer: zero-cross offset → TRIAC gate HIGH
    esp_timer_create_args_t delay_args = {
        .callback = &delay_timer_callback,
        .arg = channel,
        .dispatch_method = ESP_TIMER_ISR,
        .name = "dimmer_delay",
        .skip_unhandled_events = false
    };
    if (esp_timer_create(&delay_args, &channel->delay_timer) != ESP_OK) {
        return RBDIMMER_ERR_TIMER_FAILED;
    }

    // pulse timer: TRIAC gate HIGH → LOW
    esp_timer_create_args_t pulse_args = {
        .callback = &pulse_timer_callback,
        .arg = channel,
        .dispatch_method = ESP_TIMER_ISR,
        .name = "dimmer_pulse",
        .skip_unhandled_events = false
    };
    if (esp_timer_create(&pulse_args, &channel->pulse_timer) != ESP_OK) {
        esp_timer_delete(channel->delay_timer);
        channel->delay_timer = NULL;
        return RBDIMMER_ERR_TIMER_FAILED;
    }

    return RBDIMMER_OK;
}

void rbdimmer_timer_delete(rbdimmer_channel_t* channel) {
    if (channel->delay_timer) {
        esp_timer_stop(channel->delay_timer);
        esp_timer_delete(channel->delay_timer);
        channel->delay_timer = NULL;
    }
    if (channel->pulse_timer) {
        esp_timer_stop(channel->pulse_timer);
        esp_timer_delete(channel->pulse_timer);
        channel->pulse_timer = NULL;
    }
}
