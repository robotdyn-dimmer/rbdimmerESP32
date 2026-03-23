/**
 * @file rbdimmer_curves.h
 * @brief Brightness curve lookup tables and level-to-delay conversion
 * @internal
 *
 * Pure math module — no hardware dependencies.
 * Manages pre-computed lookup tables for three curve types and
 * converts a brightness percentage to a microsecond firing delay.
 */

#ifndef RBDIMMER_CURVES_H
#define RBDIMMER_CURVES_H

#include <stdint.h>
#include "rbdimmerESP32.h"   // rbdimmer_curve_t, RBDIMMER_MIN_DELAY_US, RBDIMMER_DEFAULT_PULSE_WIDTH_US

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pre-compute all brightness curve lookup tables.
 * Must be called once during rbdimmer_init().
 */
void rbdimmer_curves_init(void);

/**
 * @brief Convert brightness level to TRIAC firing delay.
 *
 * @param level_percent  Brightness 0-100%
 * @param half_cycle_us  Mains half-cycle duration in microseconds
 * @param curve_type     Selected brightness curve
 * @return               Delay in microseconds (clamped to safe range)
 */
uint32_t rbdimmer_curves_level_to_delay(uint8_t level_percent,
                                         uint32_t half_cycle_us,
                                         rbdimmer_curve_t curve_type);

#ifdef __cplusplus
}
#endif

#endif /* RBDIMMER_CURVES_H */
