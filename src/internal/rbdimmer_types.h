/**
 * @file rbdimmer_types.h
 * @brief Internal type definitions for rbdimmerESP32
 * @internal
 *
 * All internal structs, enums, and typedefs used by the implementation.
 * NOT part of the public API -- do not include from user code.
 */

#ifndef RBDIMMER_TYPES_H
#define RBDIMMER_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"   // TaskHandle_t
#include "freertos/task.h"
#include "rbdimmerESP32.h"       // rbdimmer_curve_t, rbdimmer_channel_t (opaque forward decl)

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Timer state machine
// ---------------------------------------------------------------------------

typedef enum {
    TIMER_STATE_IDLE,        // Waiting for zero-crossing
    TIMER_STATE_DELAY,       // Delay timer running, waiting to fire TRIAC
    TIMER_STATE_PULSE_ON,    // Pulse active, waiting to turn off
} timer_state_t;

// ---------------------------------------------------------------------------
// Zero-crossing detector (one per AC phase)
// ---------------------------------------------------------------------------

typedef struct {
    uint8_t pin;                      // GPIO pin of the ZC detector
    uint8_t phase;                    // Phase number (0..RBDIMMER_MAX_PHASES-1)
    uint16_t frequency;               // Measured mains frequency in Hz
    uint32_t half_cycle_us;           // Half-cycle duration in microseconds
    uint32_t last_cross_time;         // Timestamp of last zero-crossing (us)
    void (*callback)(void*);          // User callback (rising edge)
    void* user_data;                  // User data passed to callback
    bool is_active;                   // Active flag

    // Frequency auto-measurement
    bool frequency_measured;          // True once frequency is determined
    uint8_t measurement_count;        // Number of half-periods accumulated
    uint32_t total_period_us;         // Sum of half-periods for averaging
} rbdimmer_zero_cross_t;

// ---------------------------------------------------------------------------
// Dimmer channel (implements opaque rbdimmer_channel_t from public header)
// ---------------------------------------------------------------------------

struct rbdimmer_channel_s {
    uint8_t gpio_pin;                          // Output GPIO pin (TRIAC gate)
    uint8_t phase;                             // Phase this channel belongs to

    // Fields shared between task and ISR context — must be volatile so the
    // compiler does not cache them in a register across context boundaries.
    volatile uint8_t  level_percent;           // Current brightness (0-100)
    uint8_t prev_level_percent;                // Previous brightness (change detection, task-only)
    volatile uint32_t current_delay;           // Firing delay [µs]: written by task, read by ISR
    volatile bool     is_active;               // Enable flag: written by task, read by ISR
    bool needs_update;                         // Delay recalc pending (task-only)
    rbdimmer_curve_t curve_type;               // Brightness curve (task-only)
    esp_timer_handle_t delay_timer;            // One-shot: zero-cross → TRIAC fire
    esp_timer_handle_t pulse_timer;            // One-shot: TRIAC fire → pulse end
    volatile timer_state_t timer_state;        // FSM state: read/written by ISR callbacks

    // W4: active transition task handle (NULL when idle).
    // rbdimmer_set_level_transition() checks this before spawning a new task
    // to prevent two tasks fighting over the same channel.
    TaskHandle_t transition_task;              // NULL = no transition in progress
};

#ifdef __cplusplus
}
#endif

#endif /* RBDIMMER_TYPES_H */
