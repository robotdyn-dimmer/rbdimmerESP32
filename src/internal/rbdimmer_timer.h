/**
 * @file rbdimmer_timer.h
 * @brief TRIAC pulse timer state machine for dimmer channels
 * @internal
 *
 * Owns: delay_timer_callback, pulse_timer_callback (ISR-dispatched).
 * Provides lifecycle helpers: create and delete both timers for a channel.
 */

#ifndef RBDIMMER_TIMER_H
#define RBDIMMER_TIMER_H

#include "rbdimmerESP32.h"    // rbdimmer_err_t
#include "rbdimmer_types.h"   // rbdimmer_channel_t (full struct)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create delay and pulse timers for a channel.
 *
 * Both timers use ESP_TIMER_ISR dispatch to avoid FreeRTOS scheduler latency.
 * Stores timer handles in channel->delay_timer and channel->pulse_timer.
 *
 * @param channel  Fully initialised channel struct (gpio_pin must be set)
 * @return RBDIMMER_OK or RBDIMMER_ERR_TIMER_FAILED
 */
rbdimmer_err_t rbdimmer_timer_create(rbdimmer_channel_t* channel);

/**
 * @brief Stop and delete both timers for a channel.
 * Safe to call if timers were never started.
 */
void rbdimmer_timer_delete(rbdimmer_channel_t* channel);

#ifdef __cplusplus
}
#endif

#endif /* RBDIMMER_TIMER_H */
