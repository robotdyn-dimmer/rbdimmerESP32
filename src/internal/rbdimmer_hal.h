/**
 * @file rbdimmer_hal.h
 * @brief Chip-specific capability map and GPIO validation helpers
 * @internal
 *
 * This file is the single source of truth for hardware capability information
 * across the ESP32 family.  It does NOT implement separate code paths per chip —
 * ESP-IDF already abstracts all initialisation differences at the API level.
 * Instead it:
 *   1. Documents per-chip limits (GPIO count, output pins, core count)
 *   2. Provides validated GPIO-check macros used by channel and zerocross modules
 *   3. Emits compile-time warnings for chip-specific concerns
 *
 * -------------------------------------------------------------------------
 * Chip quick-reference (ESP-IDF v5.x, verified against soc_caps.h)
 * -------------------------------------------------------------------------
 *
 * | Chip      | GPIO total | OUTPUT GPIO     | Cores | esp_timer HW |
 * |-----------|------------|-----------------|-------|--------------|
 * | ESP32     |     40     | 34  (GPIO 0-33) |   2   | TG0 LAC      |
 * | ESP32-S2  |     47     | 46  (GPIO 0-45) |   1   | SYSTIMER     |
 * | ESP32-S3  |     49     | 49  (GPIO 0-48) |   2   | SYSTIMER     |
 * | ESP32-C3  |     22     | 22  (GPIO 0-21) |   1   | SYSTIMER     |
 * | ESP32-C6  |     31     | 31  (GPIO 0-30) |   1   | SYSTIMER     |
 *
 * INPUT-ONLY GPIO pins (cannot be used as TRIAC output):
 *   ESP32:    GPIO 34, 35, 36, 37, 38, 39
 *   ESP32-S2: GPIO 46
 *   Others:   none — all GPIO support output
 *
 * -------------------------------------------------------------------------
 * esp_timer notes
 * -------------------------------------------------------------------------
 * esp_timer is a software layer on top of a hardware timer:
 *   - ESP32:   TG0 LAC (Timer Group 0 Load-Access timer)
 *   - Others:  SYSTIMER (64-bit system timer)
 * There is NO fixed per-channel hardware resource limit.
 * Each dimmer channel consumes 2 esp_timer handles (delay + pulse timer),
 * allocated from heap.  Practical limit is heap size and IRAM budget.
 *
 * General-purpose timers (gptimer): NOT used by this library.
 *   ESP32/S2/S3: 4 timers (2 groups × 2)
 *   ESP32-C3/C6: 2 timers (1 group × 2)
 *
 * -------------------------------------------------------------------------
 * Single-core notes (ESP32-S2, C3, C6)
 * -------------------------------------------------------------------------
 * On single-core chips CONFIG_FREERTOS_UNICORE is set automatically by ESP-IDF.
 * xTaskCreatePinnedToCore(task, name, stack, params, pri, handle, 0) is safe
 * and equivalent to xTaskCreate — the transition task pin (Fix 1.6) is correct
 * on all chips without any conditional compilation.
 *
 * Cross-core race condition (Fix 1.6) cannot occur on single-core chips,
 * but the pinning incurs no cost and keeps the code uniform.
 */

#ifndef RBDIMMER_HAL_H
#define RBDIMMER_HAL_H

#include "driver/gpio.h"    /* GPIO_IS_VALID_OUTPUT_GPIO, GPIO_IS_VALID_GPIO  */
#include "soc/soc_caps.h"   /* SOC_CPU_CORES_NUM, SOC_GPIO_PIN_COUNT          */

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// GPIO validation — delegate to IDF chip-specific output/input masks
// ---------------------------------------------------------------------------

/**
 * @brief True if @p pin can be used as TRIAC trigger output on this chip.
 *
 * Equivalent to IDF's GPIO_IS_VALID_OUTPUT_GPIO() but implemented with an
 * upper-bound check instead of `>= 0` to avoid -Wtype-limits when @p pin is
 * uint8_t (an unsigned type that is always >= 0).
 *
 * Automatically excludes:
 *   - GPIO 34-39  on ESP32   (input-only pins)
 *   - GPIO 46     on ESP32-S2 (input-only)
 *   - Out-of-range numbers on all chips
 */
#define RBDIMMER_HAL_IS_OUTPUT_GPIO(pin) \
    (((pin) < (uint8_t)SOC_GPIO_PIN_COUNT) && \
     ((((uint64_t)1U << (pin)) & (uint64_t)SOC_GPIO_VALID_OUTPUT_GPIO_MASK) != 0U))

/**
 * @brief True if @p pin can be used for zero-cross input + GPIO interrupt.
 *
 * On all ESP32-family chips every valid GPIO supports input mode and external
 * interrupts (POSEDGE / NEGEDGE / ANYEDGE).
 */
#define RBDIMMER_HAL_IS_INPUT_GPIO(pin) \
    (((pin) < (uint8_t)SOC_GPIO_PIN_COUNT) && \
     ((((uint64_t)1U << (pin)) & (uint64_t)SOC_GPIO_VALID_GPIO_MASK) != 0U))

// ---------------------------------------------------------------------------
// Core count (compile-time constant from soc_caps.h)
// ---------------------------------------------------------------------------

/** Number of CPU cores on this chip (1 or 2). */
#define RBDIMMER_HAL_CPU_CORES   SOC_CPU_CORES_NUM

/** 1 on single-core chips (ESP32-S2, C3, C6), 0 on dual-core (ESP32, S3). */
#if SOC_CPU_CORES_NUM == 1
  #define RBDIMMER_HAL_SINGLE_CORE 1
#else
  #define RBDIMMER_HAL_SINGLE_CORE 0
#endif

// ---------------------------------------------------------------------------
// Compile-time advisory checks
// ---------------------------------------------------------------------------

/*
 * ESP32-C3 has 22 GPIO total; flash typically occupies GPIO 11-17, leaving
 * at most ~14 free pins.  With 1 ZC input, that allows at most 13 output
 * channels — warn if the configured maximum looks unrealistic.
 */
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  #if defined(CONFIG_RBDIMMER_MAX_CHANNELS) && CONFIG_RBDIMMER_MAX_CHANNELS > 10
    #warning "rbdimmerESP32: ESP32-C3 has 22 GPIO total. " \
             "RBDIMMER_MAX_CHANNELS > 10 may exceed available pins."
  #endif
#endif

/*
 * ESP32-C6 has 31 GPIO total; GPIO 24-30 are used by USB/LP peripherals in
 * most board configurations, leaving ~23 free pins.
 */
#if defined(CONFIG_IDF_TARGET_ESP32C6)
  #if defined(CONFIG_RBDIMMER_MAX_CHANNELS) && CONFIG_RBDIMMER_MAX_CHANNELS > 16
    #warning "rbdimmerESP32: ESP32-C6 has 31 GPIO total. " \
             "RBDIMMER_MAX_CHANNELS > 16 may exceed available pins."
  #endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* RBDIMMER_HAL_H */
