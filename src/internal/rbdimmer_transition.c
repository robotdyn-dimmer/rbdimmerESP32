/**
 * @file rbdimmer_transition.c
 * @brief Smooth brightness transition via FreeRTOS task
 * @internal
 *
 * Implements rbdimmer_set_level_transition() (public API, declared in rbdimmerESP32.h).
 *
 * Fix 1.6: transition task pinned to CPU0 — same core as GPIO ISR and
 * esp_timer callbacks — to avoid cross-core race on channel->current_delay.
 * On single-core chips (ESP32-C3/S2/C6) pinning to CPU0 is a no-op.
 */

#include "rbdimmer_transition.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>

#define TAG "RBDIMMER"

// Transition task parameters — use Kconfig values if available (ESP-IDF build),
// fall back to compile-time defaults for Arduino builds.
#ifdef CONFIG_RBDIMMER_TRANSITION_TASK_STACK_SIZE
  #define TRANSITION_STACK_SIZE  CONFIG_RBDIMMER_TRANSITION_TASK_STACK_SIZE
#else
  #define TRANSITION_STACK_SIZE  2048
#endif

#ifdef CONFIG_RBDIMMER_TRANSITION_TASK_PRIORITY
  #define TRANSITION_TASK_PRIO   CONFIG_RBDIMMER_TRANSITION_TASK_PRIORITY
#else
  #define TRANSITION_TASK_PRIO   5
#endif

// ---------------------------------------------------------------------------
// Internal types
// ---------------------------------------------------------------------------

typedef struct {
    rbdimmer_channel_t* channel;
    uint8_t  start_level;
    uint8_t  target_level;
    uint32_t transition_ms;
    uint32_t step_ms;
} transition_params_t;

// ---------------------------------------------------------------------------
// FreeRTOS task
// ---------------------------------------------------------------------------

static void level_transition_task(void* pvParameters) {
    transition_params_t* params = (transition_params_t*)pvParameters;
    rbdimmer_channel_t*  ch     = params->channel;

    uint8_t  current   = params->start_level;
    uint8_t  target    = params->target_level;
    int8_t   step      = (target > current) ? 1 : -1;
    uint32_t steps     = (uint32_t)abs((int)target - (int)current);
    uint32_t step_time = (steps > 0) ? (params->transition_ms / steps)
                                      : params->transition_ms;
    if (step_time < params->step_ms) {
        step_time = params->step_ms;
    }

    while (current != target) {
        rbdimmer_set_level(ch, current);
        current += step;
        if ((step > 0 && current > target) ||
            (step < 0 && current < target)) {
            current = target;
        }
        vTaskDelay(pdMS_TO_TICKS(step_time));
    }
    rbdimmer_set_level(ch, target);

    // W4: self-clear handle so the channel knows no transition is running.
    ch->transition_task = NULL;
    free(params);
    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

rbdimmer_err_t rbdimmer_set_level_transition(rbdimmer_channel_t* channel,
                                               uint8_t level_percent,
                                               uint32_t transition_ms) {
    if (channel == NULL) {
        return RBDIMMER_ERR_INVALID_ARG;
    }
    if (level_percent > 100) {
        level_percent = 100;
    }
    if (channel->level_percent == level_percent) {
        return RBDIMMER_OK;
    }
    if (transition_ms < 50) {
        return rbdimmer_set_level(channel, level_percent);
    }

    // W4: cancel any transition already running on this channel so only one
    // task drives the channel at a time.  vTaskDelete() on a running task is
    // safe — FreeRTOS cleans up the TCB at the next scheduler tick.
    if (channel->transition_task != NULL) {
        vTaskDelete(channel->transition_task);
        channel->transition_task = NULL;
    }

    transition_params_t* params =
        (transition_params_t*)malloc(sizeof(transition_params_t));
    if (params == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for transition parameters");
        return RBDIMMER_ERR_NO_MEMORY;
    }

    params->channel       = channel;
    params->start_level   = channel->level_percent;
    params->target_level  = level_percent;
    params->transition_ms = transition_ms;
    params->step_ms       = 20;

    TaskHandle_t task_handle = NULL;
    BaseType_t created = xTaskCreatePinnedToCore(
        level_transition_task,
        "dimmer_transition",
        TRANSITION_STACK_SIZE,
        params,
        TRANSITION_TASK_PRIO,
        &task_handle,
        0   // CPU0: same core as zero_cross ISR and timer callbacks (Fix 1.6)
    );
    if (created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create transition task");
        free(params);
        return RBDIMMER_ERR_NO_MEMORY;
    }

    // W4: store handle so next call can cancel this task if needed.
    channel->transition_task = task_handle;
    return RBDIMMER_OK;
}
