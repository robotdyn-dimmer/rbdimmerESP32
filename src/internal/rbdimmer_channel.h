/**
 * @file rbdimmer_channel.h
 * @brief Dimmer channel lifecycle, control, and zero-cross firing
 * @internal
 *
 * Owns: dimmer_manager, on_zero_cross_phase (ISR phase-trigger),
 *       all public channel API implementations.
 * Exposes: module init/deinit called by the rbdimmerESP32.cpp facade.
 */

#ifndef RBDIMMER_CHANNEL_H
#define RBDIMMER_CHANNEL_H

#include "rbdimmerESP32.h"    // rbdimmer_err_t, public types
#include "rbdimmer_types.h"   // rbdimmer_channel_t (full struct)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise channel manager and register the zero-cross phase trigger.
 *
 * Must be called after rbdimmer_zc_init() so that the zero-cross module is
 * ready to accept the phase-trigger callback.
 *
 * @return RBDIMMER_OK (always succeeds)
 */
rbdimmer_err_t rbdimmer_channel_manager_init(void);

/**
 * @brief Delete all channels and reset the channel manager.
 *
 * Equivalent to calling rbdimmer_delete_channel() for every registered channel.
 * Safe to call even if no channels have been created.
 */
void rbdimmer_channel_manager_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* RBDIMMER_CHANNEL_H */
