/**
 * @file rbdimmer_transition.h
 * @brief Smooth brightness transition via FreeRTOS task
 * @internal
 *
 * Implements rbdimmer_set_level_transition() declared in rbdimmerESP32.h.
 * The transition task is pinned to CPU0 (Fix 1.6) to avoid cross-core races
 * with the GPIO ISR and esp_timer callbacks.
 */

#ifndef RBDIMMER_TRANSITION_H
#define RBDIMMER_TRANSITION_H

#include "rbdimmerESP32.h"    // rbdimmer_err_t, rbdimmer_channel_t
#include "rbdimmer_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// rbdimmer_set_level_transition() is declared in rbdimmerESP32.h (public API).
// No additional internal symbols need to be exported from this module.

#ifdef __cplusplus
}
#endif

#endif /* RBDIMMER_TRANSITION_H */
