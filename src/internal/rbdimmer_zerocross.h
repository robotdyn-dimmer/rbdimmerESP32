/**
 * @file rbdimmer_zerocross.h
 * @brief Zero-crossing detection, frequency measurement, and ISR management
 * @internal
 *
 * Owns: gpio_to_phase_map, zero_cross_manager, GPIO ISR handler.
 * The ISR notifies the channel layer via a registered phase-trigger callback.
 */

#ifndef RBDIMMER_ZEROCROSS_H
#define RBDIMMER_ZEROCROSS_H

#include <stdint.h>
#include <stdbool.h>
#include "rbdimmerESP32.h"       // rbdimmer_err_t, RBDIMMER_MAX_PHASES
#include "rbdimmer_types.h"      // rbdimmer_zero_cross_t

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Phase-trigger callback
// Called from ISR context on every zero-crossing for the given phase.
// The registered function MUST be IRAM_ATTR.
// ---------------------------------------------------------------------------

typedef void (*rbdimmer_zc_phase_trigger_t)(uint8_t phase);

/**
 * @brief Register a phase-trigger callback.
 *
 * Called once during library init by the channel layer.
 * The callback is invoked from the GPIO ISR for every detected zero-crossing.
 * The callback MUST be IRAM_ATTR.
 *
 * @param cb  Callback function (IRAM_ATTR required)
 */
void rbdimmer_zc_set_phase_trigger(rbdimmer_zc_phase_trigger_t cb);

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/** @brief Reset internal state. Call during rbdimmer_init(). */
void rbdimmer_zc_init(void);

/**
 * @brief Register a zero-crossing detector on a GPIO pin.
 *
 * Configures the GPIO, installs the ISR service (once), adds the handler,
 * and stores the ZC descriptor.
 *
 * @param pin        GPIO pin connected to zero-cross detector
 * @param phase      Phase number (0 .. RBDIMMER_MAX_PHASES-1)
 * @param frequency  Mains frequency in Hz (0 = auto-detect)
 * @return RBDIMMER_OK or error code
 */
rbdimmer_err_t rbdimmer_zc_register(uint8_t pin, uint8_t phase, uint16_t frequency);

/** @brief Remove all ISR handlers and uninstall ISR service. */
void rbdimmer_zc_deinit(void);

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

/**
 * @brief Look up a ZC descriptor by phase number.
 * @return Pointer to descriptor, or NULL if not found.
 */
rbdimmer_zero_cross_t* rbdimmer_zc_get_by_phase(uint8_t phase);

/**
 * @brief Return measured mains frequency for a phase.
 * @return Frequency in Hz, or 0 if not yet measured.
 */
uint16_t rbdimmer_zc_get_frequency(uint8_t phase);

/**
 * @brief Set user zero-cross callback for a phase.
 * @note  Callback runs in ISR context — must be IRAM_ATTR.
 */
rbdimmer_err_t rbdimmer_zc_set_callback(uint8_t phase,
                                         void (*callback)(void*),
                                         void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* RBDIMMER_ZEROCROSS_H */
