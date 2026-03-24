# rbdimmerESP32 - Professional AC Dimmer Library for ESP32

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Arduino IDE](https://img.shields.io/badge/Arduino%20IDE-Compatible-green.svg)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-Compatible-red.svg)](https://github.com/espressif/esp-idf)

> **v2.0.0 Breaking Changes**: Internal source layout changed to `src/internal/` modules. Examples restructured to per-sketch directories (Arduino IDE 2.x). `Kconfig.txt` renamed to `Kconfig`. Public API unchanged — no user code changes required.

A professional, high-performance AC dimmer control library for ESP32 microcontrollers. Designed for precision timing, hardware efficiency, multi-phase and multi-channel operation with support for various load types including incandescent bulbs, LED dimmers, and motor control applications.

## Key Features

### Hardware Optimization
- **Hardware Timer Integration**: Uses ESP32's high-resolution timers for microsecond precision
- **Interrupt-Driven Architecture**: Minimal CPU overhead with hardware interrupt handling
- **Multi-Phase Support**: Up to 4 independent AC phases with individual zero-crossing detection
- **Automatic Frequency Detection**: Supports both 50Hz and 60Hz mains with auto-detection
- **IRAM_ATTR ISR Placement**: All timing-critical paths in IRAM for deterministic latency
- **ESP_TIMER_ISR Dispatch**: Timer callbacks dispatched from ISR context for minimum jitter
- **Zero-Cross Noise Gate**: Hardware-validated debounce filter eliminates false triggers from TRIAC switching spikes

### Advanced Control
- **Multiple Brightness Curves**: Linear, RMS-compensated, and logarithmic curves for different loads
- **Smooth Transitions**: Non-blocking brightness transitions using FreeRTOS tasks
- **Multi-Channel Operation**: Control up to 8 independent dimmer channels simultaneously
- **Real-Time Synchronization**: Phase-locked operation with mains frequency

### Professional Features
- **Comprehensive Error Handling**: Detailed error codes and diagnostic information
- **Thread-Safe Design**: Full FreeRTOS compatibility with proper resource management
- **Extensive Documentation**: Complete Doxygen documentation with examples
- **Cross-Platform Support**: Works with Arduino IDE, PlatformIO, and ESP-IDF

## Modular Architecture

v2.0.0 replaces the monolithic single-file implementation with six internal modules:

| Module | Responsibility |
|--------|---------------|
| `rbdimmer_zerocross` | ZC GPIO ISR, frequency measurement, noise gate |
| `rbdimmer_channel` | Channel state, ZC phase dispatch, two-pass ISR |
| `rbdimmer_timer` | esp_timer create/start/stop wrappers |
| `rbdimmer_curves` | Level → delay conversion (LINEAR, RMS, LOG) |
| `rbdimmer_transition` | FreeRTOS task-based smooth fade |
| `rbdimmer_types` | Shared structs and enums |
| `rbdimmer_hal` | GPIO/timer HAL shims for Arduino/ESP-IDF portability |

> **Note**: The public header (`rbdimmerESP32.h`) API is unchanged — user code does not need modification.

## Quick Start

### Arduino IDE Installation

1. Download the library from [GitHub](https://github.com/robotdyn-dimmer/rbdimmerESP32)
2. In Arduino IDE: **Sketch** → **Include Library** → **Add .ZIP Library**
3. Select the downloaded ZIP file
4. Include in your sketch:

```cpp
#include <rbdimmerESP32.h>
```

### Basic Usage Example

```cpp
#include <rbdimmerESP32.h>

rbdimmer_channel_t* dimmer_channel;

void setup() {
    Serial.begin(115200);

    // Initialize the library
    rbdimmer_init();

    // Register zero-crossing detector on pin 2, phase 0, auto-detect frequency
    rbdimmer_register_zero_cross(2, 0, 0);

    // Configure dimmer channel
    rbdimmer_config_t config = {
        .gpio_pin = 4,              // Dimmer output pin
        .phase = 0,                 // Use phase 0
        .initial_level = 0,         // Start at 0% brightness
        .curve_type = RBDIMMER_CURVE_RMS  // RMS-compensated curve
    };

    // Create the channel
    rbdimmer_create_channel(&config, &dimmer_channel);

    Serial.println("RBDimmer initialized successfully");
}

void loop() {
    // Fade from 0% to 100% over 3 seconds
    rbdimmer_set_level_transition(dimmer_channel, 100, 3000);
    delay(4000);

    // Fade from 100% to 0% over 2 seconds
    rbdimmer_set_level_transition(dimmer_channel, 0, 2000);
    delay(3000);
}
```

## Hardware Requirements

### Supported Boards
- **ESP32 DevKit**: All variants (ESP32-WROOM-32, ESP32-WROVER, etc.)
- **ESP32-S2**: Single-core ESP32 variant with USB OTG
- **ESP32-S3**: Dual-core with enhanced AI capabilities and USB
- **ESP32-C3**: RISC-V based, ultra-low power consumption
- **ESP32-C6**: Wi-Fi 6 support with 802.11ax
- **ESP32-H2**: Dedicated for Thread/Zigbee applications
- **ESP32-P4**: High-performance variant with enhanced processing
- **ESP32-C2**: Cost-optimized with essential connectivity
- **Compatible Third-Party Boards**: Wemos D1 Mini ESP32, NodeMCU-32S, etc.

### Framework Compatibility Matrix

| ESP32 Chip | Arduino Core | ESP-IDF | ESPHome |
|------------|-------------|---------|---------|
| **ESP32** | ✅ 3.x | ✅ 5.3+ | ✅ 2023.x+ |
| **ESP32-S2** | ✅ 3.x | ✅ 5.3+ | ✅ 2023.x+ |
| **ESP32-S3** | ✅ 3.x | ✅ 5.3+ | ✅ 2023.x+ |
| **ESP32-C3** | ✅ 3.x | ✅ 5.3+ | ✅ 2023.x+ |
| **ESP32-C6** | ✅ 3.x | ✅ 5.3+ | ⏳ Coming |
| **ESP32-H2** | ✅ 3.x | ✅ 5.3+ | ✅ 2024.x+ |
| **ESP32-P4** | ⏳ Coming | ✅ 5.3+ | ⏳ Coming |
| **ESP32-C2** | ✅ 3.x | ✅ 5.3+ | ⚠️ Limited |

### Dimmer Hardware
This library is designed to work with **pre-built dimmer modules** that include:
- Zero-crossing detection circuit
- TRIAC or solid-state relay driver
- Optical isolation for safety
- Appropriate heat sinking

⚠️ **Safety Warning**: AC mains voltage is dangerous. Only use certified dimmer modules with proper isolation.

### Wiring Example
```
ESP32 Pin 2  ←→  Zero-Cross Detector Output
ESP32 Pin 4  ←→  TRIAC Gate Control Input
ESP32 GND    ←→  Dimmer Module GND (isolated side)
ESP32 3.3V   ←→  Dimmer Module VCC (if required)
```

## Documentation

| Document | Description |
|----------|-------------|
| [API Reference](API.md) | Complete API documentation |
| [Hardware Guide](HARDWARE.md) | Wiring diagrams and safety |
| [Troubleshooting](TROUBLESHOOTING.md) | Common problems and solutions |
| [Examples](examples/README.md) | All example sketches explained |
| [Changelog](CHANGELOG.md) | Version history |

### Platform-Specific Guides
- **Arduino Guide**: [Arduino Framework Setup and Examples](https://www.rbdimmer.com/docs/dimmers-universal-library-for-esp32-arduino-guide-and-examples)
- **ESP-IDF Guide**: [ESP-IDF Framework Setup and Examples](https://www.rbdimmer.com/docs/dimmers-universal-library-for-esp32-esp-idf-fraimwork-c-guide-and-examples)
- **Complete Library Overview**: [Comprehensive Documentation](https://www.rbdimmer.com/docs/universal-library-for-esp32)

## Advanced Features

### Multiple Brightness Curves

Choose the optimal curve for your load type and desired visual response:

```cpp
// For incandescent bulbs (smooth, power-linear dimming)
rbdimmer_set_curve(channel, RBDIMMER_CURVE_RMS);

// For LED loads (perceptually linear brightness)
rbdimmer_set_curve(channel, RBDIMMER_CURVE_LOGARITHMIC);

// For motor control (linear power control)
rbdimmer_set_curve(channel, RBDIMMER_CURVE_LINEAR);
```

**Detailed Curve Characteristics:**

1. **Linear (RBDIMMER_CURVE_LINEAR)**
   - Uniform change in phase angle delay
   - Direct percentage to delay conversion
   - Suitable for motor control and simple applications
   - ⚠️ Note: Perceived brightness is not visually linear

2. **RMS (RBDIMMER_CURVE_RMS)**
   - Compensates for RMS characteristics of sinusoidal AC
   - Provides linear power change: Power ∝ Level²
   - Ideal for incandescent bulbs and resistive loads
   - Based on: `angle = arccos(√(level/100))`

3. **Logarithmic (RBDIMMER_CURVE_LOGARITHMIC)**
   - Compensates for logarithmic brightness perception
   - Provides visually linear brightness changes
   - Recommended for LED lighting and displays
   - Based on: `log₁₀(1 + 9×(level/100))`

### Performance Optimization

#### Hardware Timer Utilization
- **ESP-Timer Integration**: Uses ESP32's high-resolution software timers
- **Microsecond Precision**: ±1-10μs timing accuracy under normal load
- **Resource Efficient**: 2 timers per channel (delay + pulse width)
- **IRAM Placement**: Critical ISR code placed in IRAM for speed

#### Interrupt Optimization
```cpp
// Zero-cross interrupt characteristics
static void IRAM_ATTR zero_cross_isr_handler(void* arg) {
    // Minimal code in ISR context
    // Uses pre-calculated lookup tables
    // Defers heavy processing to main tasks
}
```

#### Memory Optimization
- **Pre-calculated Tables**: Brightness curves calculated at initialization
- **Efficient Caching**: Parameters cached to reduce repeated calculations
- **Minimal RAM Usage**: ~200 bytes per channel + ~1KB global overhead
- **Flash Optimization**: ~32KB total flash footprint

### Debug and Logging System

Enable comprehensive logging for development and troubleshooting:

```cpp
// Enable detailed logging
#define RBDIMMER_DEBUG_LOG 1

// Check system performance
void monitor_performance() {
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("CPU frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Zero-cross frequency: %d Hz\n", rbdimmer_get_frequency(0));

    // Monitor timing accuracy
    uint32_t delay_us = rbdimmer_get_delay(channel);
    Serial.printf("Current delay: %d μs\n", delay_us);
}
```

### Multi-Channel Control

```cpp
rbdimmer_channel_t* channels[4];

// Create multiple channels on different phases
for(int i = 0; i < 4; i++) {
    rbdimmer_register_zero_cross(zc_pins[i], i, 0);

    rbdimmer_config_t config = {
        .gpio_pin = dimmer_pins[i],
        .phase = i,
        .initial_level = 0,
        .curve_type = RBDIMMER_CURVE_RMS
    };

    rbdimmer_create_channel(&config, &channels[i]);
}
```

### Real-Time Callbacks

```cpp
void zero_cross_callback(void* user_data) {
    // Called on every zero-crossing - perfect for synchronization
    static uint32_t cross_count = 0;
    cross_count++;

    // Implement custom effects, measurements, etc.
    if(cross_count % 100 == 0) {
        Serial.printf("Frequency: %d Hz\n", rbdimmer_get_frequency(0));
    }
}

// Register the callback
rbdimmer_set_callback(0, zero_cross_callback, NULL);
```

## Kconfig Parameters

Four tunable parameters for ESP-IDF builds (Arduino uses compile-time defaults):

| Parameter | Default | Description |
|-----------|---------|-------------|
| `CONFIG_RBDIMMER_ZC_DEBOUNCE_US` | 3000 µs | Noise gate window after valid ZC edge |
| `CONFIG_RBDIMMER_MIN_DELAY_US` | 100 µs | Minimum ZC→TRIAC delay |
| `CONFIG_RBDIMMER_LEVEL_MIN` | 3 % | Levels below this → OFF |
| `CONFIG_RBDIMMER_LEVEL_MAX` | 99 % | Levels above this → capped |

## Use Cases

### Home Automation
- Smart lighting control with smooth dimming
- Integration with IoT platforms (Home Assistant, OpenHAB)
- Voice control and smartphone apps
- Energy monitoring and optimization

### Commercial Applications
- Stage and theatrical lighting
- Restaurant and hospitality ambiance
- Retail display illumination
- Industrial process control

### Educational Projects
- Power electronics demonstrations
- AC phase control experiments
- Real-time systems learning
- Embedded programming education

### Maker Projects
- Custom lamp controllers
- Halloween/Christmas light effects
- Music-synchronized lighting
- Temperature-based fan control

## Common Issues and Quick Solutions

### General Flickering or Unstable Brightness
**Possible Causes:**
- Incorrect AC Neutral/Line connections
- Zero-crossing detector signal issues
- TRIAC switching spikes re-triggering ZC detection
- High CPU load or interrupt conflicts

**Quick Fixes:**
- The v2.0.0 zero-cross noise gate (`ZC_DEBOUNCE_US = 3000µs`) filters TRIAC spike re-triggers automatically
- Try different curve for your load type:
```cpp
rbdimmer_set_curve(channel, RBDIMMER_CURVE_RMS);    // For incandescent
rbdimmer_set_curve(channel, RBDIMMER_CURVE_LOGARITHMIC); // For LEDs

// Check zero-crossing detection
uint16_t freq = rbdimmer_get_frequency(0);
if (freq == 0) {
    Serial.println("Zero-crossing not detected - check wiring");
}
```

### Flickering at 100% Brightness
**Cause:** Minimum delay too short, or level exceeding safe range.

**Fix in v2.0.0:** `MIN_DELAY_US` raised from 50 to 100µs. Levels at or above 100% are mapped to `LEVEL_MAX` (99%), preventing full-on glitches.

### Multi-Channel Sync Drift
**Cause:** Channels armed at slightly different times within a single ZC event.

**Fix in v2.0.0:** Two-pass ISR — Pass 1 resets all GPIOs, Pass 2 arms all timers. This guarantees all channels start from the same baseline on every zero-crossing.

### Flickering Below 3% Brightness
**Cause:** TRIAC cannot reliably latch at very low conduction angles.

**Fix in v2.0.0:** Levels below `LEVEL_MIN` (3%) are treated as OFF, eliminating erratic flicker at the bottom of the dimming range.

### Load-Specific Brightness Issues
**At 50% setting, lamp appears too bright/dim:**
```cpp
// Test different curves to find optimal response
rbdimmer_curve_t curves[] = {RBDIMMER_CURVE_LINEAR,
                            RBDIMMER_CURVE_RMS,
                            RBDIMMER_CURVE_LOGARITHMIC};

for(int i = 0; i < 3; i++) {
    rbdimmer_set_curve(channel, curves[i]);
    rbdimmer_set_level(channel, 50);
    delay(3000); // Observe result
}
```

### No Dimmer Response
**Check zero-crossing detection:**
```cpp
void diagnose_zero_cross() {
    if (rbdimmer_get_frequency(0) == 0) {
        Serial.println("Zero-cross signal not detected");
        Serial.println("Check: Pin connection, dimmer power, signal quality");
    }
}
```

## CI Build Matrix

Automated CI ensures every commit builds cleanly across all supported platforms:

- **Arduino**: `arduino/compile-sketches@v1`, Core 3.x, 4 examples × 5 chips (ESP32, S2, S3, C3, C6)
- **ESP-IDF**: `espressif/esp-idf-ci-action@v1`, IDF v5.3/v5.4/v5.5 × 5 chips = 15 jobs
- **test_app/**: Minimal ESP-IDF project for compile-time API surface verification

## Supported Platforms

### Arduino Framework
- **Arduino IDE 2.x**
- **PlatformIO** with Arduino framework
- Seamless integration with Arduino libraries

### ESP-IDF Framework
- **ESP-IDF 5.3+** with CMake support
- Professional development features

### Development Tools
- **Visual Studio Code** with PlatformIO extension
- **CLion** with ESP-IDF plugin
- **Eclipse CDT** with ESP-IDF tools
- Command-line development support

## Performance Characteristics

| Feature | Specification | Notes |
|---------|---------------|-------|
| **Timing Precision** | ±1-10 microseconds | Depends on system load |
| **Max Channels** | 8 simultaneous | 2 timers per channel |
| **Max Phases** | 4 independent | GPIO and memory limited |
| **Frequency Range** | 45-65 Hz auto-detect | 50/60Hz primary support |
| **Response Time** | < 20ms (one AC cycle) | Next zero-crossing |
| **CPU Overhead** | < 1% (interrupt-driven) | IRAM-optimized ISR |
| **Memory Usage** | ~8KB RAM, ~32KB Flash | Scales with channels |
| **Timer Resolution** | 1 microsecond | ESP-Timer based |
| **Jitter Tolerance** | ±10-50μs under load | FreeRTOS dependent |

### ESP-Timer Architecture Benefits
- **Software Implementation**: Allows numerous concurrent timers
- **Queue-Based Execution**: Ordered by trigger time for efficiency
- **Low Overhead**: Single hardware timer serves all software timers
- **ISR Dispatch**: Timer callbacks dispatched directly from ISR context for minimum jitter

### Performance Limitations
- **Timer Jitter**: ±10-50μs possible under high system load
- **Callback Constraints**: Should not block execution for extended periods
- **Memory Scaling**: Each channel requires ~200 bytes RAM
- **GPIO Limitations**: Available pins vary by ESP32 variant

## Library Comparison

| Feature | rbdimmerESP32 | Other Libraries |
|---------|---------------|-----------------|
| **Hardware Timers** | ✅ Native ESP32 timers | ❌ Software delays |
| **Multi-Phase** | ✅ Up to 4 phases | ❌ Single phase only |
| **Frequency Detection** | ✅ Automatic | ❌ Manual configuration |
| **Curve Types** | ✅ 3 built-in + custom | ❌ Linear only |
| **Thread Safety** | ✅ FreeRTOS compatible | ❌ Basic implementation |
| **Error Handling** | ✅ Comprehensive | ❌ Limited |
| **Documentation** | ✅ Professional docs | ❌ Basic examples |

## Contributing

We welcome contributions from the community! Please read our [Contributing Guide](CONTRIBUTING.md) for:

- Code style guidelines
- Testing procedures
- Pull request process
- Issue reporting templates
- Development environment setup

## Examples

| Example | Platform | Description | Complexity |
|---------|----------|-------------|------------|
| [BasicDimmer](examples/arduino/BasicDimmer/) | Arduino | Simple on/off dimming | Beginner |
| [BasicTransition](examples/arduino/BasicTransition/) | Arduino | Smooth brightness transitions | Beginner |
| [MultiDimmer](examples/arduino/MultiDimmer/) | Arduino | Multiple channel control | Intermediate |
| [ZCCallBack](examples/arduino/ZCCallBack/) | Arduino | Zero-crossing callbacks | Advanced |
| [BasicDimmer.c](examples/esp_idf/basic_dimmer/) | ESP-IDF | C-based dimming control | Intermediate |
| [MultiDimmer.c](examples/esp_idf/multi_dimmer/) | ESP-IDF | Multi-channel with C | Advanced |
| [ZCCallBack.c](examples/esp_idf/zc_callback/) | ESP-IDF | Advanced interrupt handling | Expert |
| [ESPHome Basic](examples/esphome/basic_dimmer.yaml) | ESPHome | YAML configuration setup | Beginner |
| [ESPHome Advanced](examples/esphome/multi_dimmer.yaml) | ESPHome | Multi-channel YAML config | Intermediate |

## Support & Community

### Official Resources
- **Website**: [https://rbdimmer.com](https://rbdimmer.com)
- **Documentation**: [https://www.rbdimmer.com/docs/universal-library-for-esp32](https://www.rbdimmer.com/docs/universal-library-for-esp32)
- **Hardware Catalog**: [https://www.rbdimmer.com/dimmers-pricing](https://www.rbdimmer.com/dimmers-pricing)
- **Project Gallery**: [https://www.rbdimmer.com/blog/dimmers-projects-5](https://www.rbdimmer.com/blog/dimmers-projects-5)

### Community Support
- **Forum**: [https://forum.rbdimmer.com](https://forum.rbdimmer.com)
- **GitHub Issues**: [Report bugs and request features](https://github.com/robotdyn-dimmer/rbdimmerESP32/issues)
- **GitHub Discussions**: [Community discussions and Q&A](https://github.com/robotdyn-dimmer/rbdimmerESP32/discussions)

### Professional Support
For commercial applications, enterprise support, and custom development:
- **Email**: support@rbdimmer.com
- **Technical Consultation**: Available for complex projects
- **Custom Development**: Tailored solutions for specific requirements

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

```
MIT License

Copyright (c) 2024 RBDimmer Team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
```

## Version Information

- **Current Version**: 2.0.0
- **Release Date**: 2026-03-23
- **Minimum ESP-IDF**: 5.3+
- **Arduino ESP32 Core**: 3.x
- **Arduino IDE**: 2.x
- **PlatformIO**: 6.0+

## Documentation in Other Languages

Full documentation is available in 5 languages on the RBDimmer website:

- [Русский (Russian)](https://www.rbdimmer.com/ru/docs/universal-library-for-esp32)
- [Français (French)](https://www.rbdimmer.com/fr/docs/universal-library-for-esp32)
- [Deutsch (German)](https://www.rbdimmer.com/de/docs/universal-library-for-esp32)
- [Español (Spanish)](https://www.rbdimmer.com/es/docs/universal-library-for-esp32)
- [Italiano (Italian)](https://www.rbdimmer.com/it/docs/universal-library-for-esp32)

## Related Projects

- **Hardware Modules**: [RBDimmer Hardware Catalog](https://www.rbdimmer.com/dimmers-pricing)
- **Application Examples**: [Project Gallery](https://www.rbdimmer.com/blog/dimmers-projects-5)
- **Technical Documentation**: [Knowledge Base](https://www.rbdimmer.com/docs/universal-library-for-esp32)

---

**Made with ❤️ by the RBDimmer Team**

*Professional AC dimmer control for the maker community*
