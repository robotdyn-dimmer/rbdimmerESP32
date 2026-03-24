# Changelog

All notable changes to the rbdimmerESP32 library will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/), and this project adheres to [Semantic Versioning](https://semver.org/).

## [2.0.0] - 2026-03-23

### Overview
Complete architectural rewrite. The monolithic single-file implementation has been replaced with a fully modular internal architecture. All timing-critical paths now use IRAM_ATTR placement and ESP_TIMER_ISR dispatch. Hardware validation on a 4-channel board revealed and resolved three independent flickering issues that were not reproducible in simulation.

### Breaking Changes
- Internal source layout changed from single `rbdimmerESP32.cpp` to `src/internal/` modules. Direct inclusion of internal headers is no longer supported.
- Examples restructured from flat `.ino`/`.c` files to per-sketch directories (required for Arduino IDE 2.x library manager).
- `Kconfig.txt` removed — ESP-IDF component config now uses the standard `Kconfig` filename. Projects that referenced `Kconfig.txt` must update.
- **Public header (`rbdimmerESP32.h`) API is unchanged** — user code does not need to be modified.

### Added
- **Modular Architecture** — implementation split into seven internal modules:
  - `rbdimmer_zerocross` — ZC GPIO ISR, frequency measurement, noise gate
  - `rbdimmer_channel` — Channel state, ZC phase dispatch, two-pass ISR
  - `rbdimmer_timer` — esp_timer create/start/stop wrappers
  - `rbdimmer_curves` — Level → delay conversion (LINEAR, RMS, LOG)
  - `rbdimmer_transition` — FreeRTOS task-based smooth fade
  - `rbdimmer_types` — Shared structs and enums
  - `rbdimmer_hal` — GPIO/timer HAL shims for Arduino/ESP-IDF portability
- **Kconfig parameters** for ESP-IDF tuning:
  - `CONFIG_RBDIMMER_ZC_DEBOUNCE_US` (default 3000 µs) — noise gate window
  - `CONFIG_RBDIMMER_MIN_DELAY_US` (default 100 µs) — minimum ZC→TRIAC delay
  - `CONFIG_RBDIMMER_LEVEL_MIN` (default 3%) — levels below → OFF
  - `CONFIG_RBDIMMER_LEVEL_MAX` (default 99%) — levels above → capped
- **CI Build Matrix** (`.github/workflows/build.yml`):
  - Arduino: `arduino/compile-sketches@v1`, Core 3.x, 4 examples × 5 chips (ESP32, S2, S3, C3, C6)
  - ESP-IDF: `espressif/esp-idf-ci-action@v1`, IDF v5.3 / v5.4 / v5.5 × 5 chips = 15 jobs
  - `test_app/` — minimal ESP-IDF project for compile-time API surface verification

### Fixed
- **General flickering at all brightness levels** — TRIAC switching spike re-triggered ZC ISR mid half-cycle. Added noise gate: any ZC edge within `ZC_DEBOUNCE_US` (3000 µs) of previous valid edge is discarded.
- **Flickering at 100% brightness** — at 50 µs delay the AC voltage (~5V) was below TRIAC latching threshold; plus `esp_timer` ISR dispatch from level-3 GPIO ISR made sub-100 µs delays unpredictable. `MIN_DELAY_US` raised from 50 → 100 µs; levels ≥ 100% mapped to `LEVEL_MAX` (99%).
- **Multi-channel synchronization** — per-channel phase offsets caused by sequential GPIO reset + timer arm in single loop. Two-pass ISR: Pass 1 sets all GPIOs LOW, Pass 2 arms all delay timers.
- **Flickering at levels below 3%** — AC voltage too low for reliable TRIAC latching near end of half-cycle. Levels below `LEVEL_MIN` (3%) return delay = 0 (channel OFF).

### Changed
- All ISR handlers now use `IRAM_ATTR` placement
- Timer dispatch changed to `ESP_TIMER_ISR` for minimum jitter
- Minimum supported ESP-IDF version: 5.3 (was 4.4)
- Arduino ESP32 Core minimum: 3.x (was 2.0.0)

## [1.0.0] - 2024-01-01

### Added
- Initial release
- Single-file implementation
- Support for ESP32, ESP32-S2, ESP32-S3, ESP32-C3
- Three brightness curves: LINEAR, RMS, LOGARITHMIC
- Multi-channel and multi-phase support
- Smooth transitions via FreeRTOS tasks
- Arduino IDE, PlatformIO, and ESP-IDF support
