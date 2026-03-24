# rbdimmerESP32 Troubleshooting Guide

> Updated for v2.0.0 -- includes fixes for four flickering issues discovered during 4-channel hardware validation.

## Table of Contents

1. [Quick Diagnostic Checklist](#quick-diagnostic-checklist)
2. [Compilation Issues](#compilation-issues)
3. [Runtime Errors](#runtime-errors)
4. [Hardware Connection Problems](#hardware-connection-problems)
5. [Performance Issues](#performance-issues)
6. [Dimming and Flickering Problems (v2.0.0 Fixes)](#dimming-and-flickering-problems)
7. [Frequency Detection Issues](#frequency-detection-issues)
8. [Multi-Channel Problems](#multi-channel-problems)
9. [Kconfig Tuning](#kconfig-tuning)
10. [Safety and Hardware Failures](#safety-and-hardware-failures)
11. [Debugging Tools and Techniques](#debugging-tools-and-techniques)
12. [Frequently Asked Questions](#frequently-asked-questions)
13. [Getting Additional Help](#getting-additional-help)

## Quick Diagnostic Checklist

Before diving into detailed troubleshooting, run through this quick checklist:

### Basic System Check

- [ ] ESP32 board selected correctly in IDE
- [ ] ESP-IDF 5.3 or later (if using ESP-IDF framework)
- [ ] Library properly installed and included
- [ ] All wiring connections secure
- [ ] Power supply adequate (3.3V, >500mA)
- [ ] Dimmer module powered and operational
- [ ] AC load compatible with dimming
- [ ] Safety procedures followed

### Quick Test Code

Run this minimal test to verify basic functionality:

```cpp
#include <rbdimmerESP32.h>

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("=== RBDimmer Quick Diagnostic ===");

    // Test 1: Library initialization
    rbdimmer_err_t err = rbdimmer_init();
    Serial.printf("1. Library Init: %s (%d)\n",
                  (err == RBDIMMER_OK) ? "OK" : "FAILED", err);

    // Test 2: Zero-cross registration
    err = rbdimmer_register_zero_cross(2, 0, 50);
    Serial.printf("2. Zero-Cross Registration: %s (%d)\n",
                  (err == RBDIMMER_OK) ? "OK" : "FAILED", err);

    // Test 3: Channel creation
    rbdimmer_channel_t* channel;
    rbdimmer_config_t config = {4, 0, 0, RBDIMMER_CURVE_LINEAR};
    err = rbdimmer_create_channel(&config, &channel);
    Serial.printf("3. Channel Creation: %s (%d)\n",
                  (err == RBDIMMER_OK) ? "OK" : "FAILED", err);

    if (err == RBDIMMER_OK) {
        Serial.println("Basic functionality working");
        Serial.println("Check hardware connections for full operation");
    } else {
        Serial.println("Basic functionality failed");
        Serial.println("Check installation and wiring");
    }
}

void loop() {
    delay(1000);
}
```

## Compilation Issues

### Problem: Library Not Found

**Error Messages:**
```
fatal error: rbdimmerESP32.h: No such file or directory
```

**Solutions:**

#### Arduino IDE
1. **Verify Installation**:
   - Check **File** -> **Examples** -> **rbdimmerESP32**
   - If not visible, library not properly installed

2. **Reinstall Library**:
   ```
   Sketch -> Include Library -> Manage Libraries
   Search "rbdimmerESP32" -> Install
   ```

3. **Manual Installation Check**:
   - Library should be in: `~/Documents/Arduino/libraries/rbdimmerESP32/`
   - Ensure `rbdimmerESP32.h` is in `src/` folder

#### Problem: Compilation Errors

**Error Messages:**
```
error: 'micros' was not declared in this scope
error: 'digitalRead' was not declared in this scope
```

**Solutions:**

1. **Check Board Selection**:
   - Must be ESP32 board type
   - Arduino IDE: **Tools** -> **Board** -> **ESP32**

2. **Update ESP32 Core**:
   - Arduino IDE: **Tools** -> **Board** -> **Boards Manager**
   - Search "ESP32" -> Update to latest version
   - ESP-IDF users: v5.3 or later required

3. **Check Framework**:

**Error Messages:**
```
error: conflicting declaration of C function
```

**Solutions:**

1. **Check Multiple Includes**:
   - Only include `rbdimmerESP32.h` once per file
   - Check for conflicts with other dimmer libraries

2. **Clean Build**:
   - Delete build folder and recompile

### Problem: Linker Errors

**Error Messages:**
```
undefined reference to `rbdimmer_init'
```

**Solutions:**

1. **ESP-IDF Component Setup**:
   ```cmake
   # In main/CMakeLists.txt
   idf_component_register(
       SRCS "main.c"
       INCLUDE_DIRS "."
       REQUIRES rbdimmerESP32
   )
   ```

   The library uses a modular architecture with its own `Kconfig` file. ESP-IDF will automatically pick up the Kconfig options when the component is registered correctly.

### Problem: Library Initialization Fails

**Symptoms:**
- `rbdimmer_init()` returns non-zero error code
- System doesn't respond to commands

**Diagnostic Code:**
```cpp
void diagnose_init() {
    Serial.println("Diagnosing initialization...");

    rbdimmer_err_t err = rbdimmer_init();
    switch(err) {
        case RBDIMMER_OK:
            Serial.println("Initialization successful");
            break;
        case RBDIMMER_ERR_NO_MEMORY:
            Serial.println("Memory allocation failed");
            Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
            break;
        default:
            Serial.printf("Unknown error: %d\n", err);
    }
}
```

**Solutions:**

1. **Memory Issues**:
   - Check available heap memory
   - Reduce other memory usage
   - Increase heap size if possible

2. **Multiple Initializations**:
   - Call `rbdimmer_init()` only once
   - Check for duplicate calls

### Problem: Zero-Cross Registration Fails

**Error Codes and Solutions:**

#### `RBDIMMER_ERR_INVALID_ARG`
```cpp
// Check pin number validity
void check_pin_validity() {
    uint8_t test_pin = 2;
    if (test_pin >= GPIO_NUM_MAX) {
        Serial.printf("Invalid pin number: %d\n", test_pin);
    }

    // Check if pin is available
    if (test_pin == 0 || test_pin == 1 || test_pin == 3) {
        Serial.println("Warning: Using boot/serial pin");
    }
}
```

**Solutions:**
- Use GPIO pins 2, 4, 5, 12-15, 25-27
- Avoid pins 0, 1, 3 (boot/serial)
- Avoid pins 6-11 (flash memory)

#### `RBDIMMER_ERR_ALREADY_EXIST`
```cpp
// Check for duplicate phase registration
for(int phase = 0; phase < 4; phase++) {
    rbdimmer_err_t err = rbdimmer_register_zero_cross(2+phase, phase, 0);
    if (err == RBDIMMER_ERR_ALREADY_EXIST) {
        Serial.printf("Phase %d already registered\n", phase);
    }
}
```

**Solutions:**
- Check for duplicate `rbdimmer_register_zero_cross()` calls
- Use `rbdimmer_deinit()` to reset if needed

### Problem: Channel Creation Fails

**Diagnostic Code:**
```cpp
void diagnose_channel_creation() {
    rbdimmer_config_t config = {4, 0, 0, RBDIMMER_CURVE_LINEAR};
    rbdimmer_channel_t* channel;

    rbdimmer_err_t err = rbdimmer_create_channel(&config, &channel);

    switch(err) {
        case RBDIMMER_OK:
            Serial.println("Channel created successfully");
            break;
        case RBDIMMER_ERR_NOT_FOUND:
            Serial.printf("Phase %d not registered\n", config.phase);
            break;
        case RBDIMMER_ERR_NO_MEMORY:
            Serial.println("No memory or max channels reached");
            Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
            break;
        case RBDIMMER_ERR_GPIO_FAILED:
            Serial.printf("GPIO %d configuration failed\n", config.gpio_pin);
            break;
        case RBDIMMER_ERR_TIMER_FAILED:
            Serial.println("Timer allocation failed");
            break;
        default:
            Serial.printf("Unknown error: %d\n", err);
    }
}
```

**Solutions:**

1. **Phase Not Registered**:
   - Register zero-cross detector first
   - Verify phase number matches

2. **Memory Issues**:
   - Maximum 8 channels supported
   - Each channel uses ~200 bytes RAM
   - Free up memory if needed

3. **GPIO Issues**:
   - Try different GPIO pin
   - Check for pin conflicts with other code

## Hardware Connection Problems

### Problem: No Zero-Cross Detection

**Symptoms:**
- `rbdimmer_get_frequency()` returns 0
- No dimming response

**Diagnostic Code:**
```cpp
void diagnose_zero_cross() {
    Serial.println("Zero-cross diagnostic starting...");

    // Test 1: Pin state monitoring
    pinMode(2, INPUT);
    Serial.println("Monitoring zero-cross pin for 10 seconds...");

    int high_count = 0, low_count = 0;
    unsigned long start_time = millis();

    while(millis() - start_time < 10000) {
        if(digitalRead(2)) {
            high_count++;
        } else {
            low_count++;
        }
        delayMicroseconds(100);
    }

    Serial.printf("High readings: %d, Low readings: %d\n", high_count, low_count);

    if(high_count == 0) {
        Serial.println("Pin always LOW - check connections");
    } else if(low_count == 0) {
        Serial.println("Pin always HIGH - check connections");
    } else {
        Serial.println("Pin changing states - wiring OK");
    }

    // Test 2: Frequency measurement
    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 0);

    Serial.println("Measuring frequency for 30 seconds...");
    for(int i = 0; i < 30; i++) {
        delay(1000);
        uint16_t freq = rbdimmer_get_frequency(0);
        Serial.printf("Frequency: %d Hz\n", freq);

        if(freq > 0) {
            Serial.println("Frequency detected successfully");
            break;
        }
    }
}
```

**Solutions:**

1. **Check Physical Connections**:
   - Verify zero-cross output pin connection
   - Check ground connection between ESP32 and dimmer module
   - Ensure dimmer module is powered

2. **Test Dimmer Module**:
   - Use multimeter to check zero-cross output (DC voltage)
   - Should show ~3.3V pulses at mains frequency
   - If no signal, dimmer module may be faulty

3. **Try Different Pin**:
   ```cpp
   // Test different GPIO pins
   uint8_t test_pins[] = {2, 4, 5, 12, 13, 14, 15};
   for(int i = 0; i < sizeof(test_pins); i++) {
       Serial.printf("Testing pin %d\n", test_pins[i]);
       // Test each pin...
   }
   ```

### Problem: TRIAC Not Switching

**Symptoms:**
- Load doesn't respond to dimming commands
- Always full on or full off

**Diagnostic Code:**
```cpp
void diagnose_triac_control() {
    Serial.println("TRIAC control diagnostic...");

    // Test 1: GPIO output test
    pinMode(4, OUTPUT);
    Serial.println("Manual GPIO test - watch for LED/scope");

    for(int i = 0; i < 10; i++) {
        digitalWrite(4, HIGH);
        Serial.println("GPIO HIGH");
        delay(500);
        digitalWrite(4, LOW);
        Serial.println("GPIO LOW");
        delay(500);
    }

    // Test 2: Library control test
    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 50); // Fixed frequency for test

    rbdimmer_config_t config = {4, 0, 0, RBDIMMER_CURVE_LINEAR};
    rbdimmer_channel_t* channel;
    rbdimmer_create_channel(&config, &channel);

    Serial.println("Testing dimmer levels...");
    for(int level = 0; level <= 100; level += 25) {
        rbdimmer_set_level(channel, level);
        Serial.printf("Level: %d%%, Delay: %d us\n",
                      level, rbdimmer_get_delay(channel));
        delay(2000);
    }
}
```

**Solutions:**

1. **Check Gate Control Connection**:
   - Verify GPIO pin to dimmer gate input
   - Ensure correct polarity
   - Check for loose connections

2. **Test with LED Indicator**:
   ```cpp
   // Add LED to gate control pin
   pinMode(4, OUTPUT);
   // LED should pulse with dimming
   ```

3. **Dimmer Module Issues**:
   - Check module power supply
   - Verify module is designed for 3.3V logic
   - Test with different dimmer module

### Problem: Erratic Dimming Behavior

**Symptoms:**
- Inconsistent brightness levels
- Flickering or jumping
- Random on/off switching

**Diagnostic Code:**
```cpp
void diagnose_stability() {
    Serial.println("Stability diagnostic...");

    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 0);

    rbdimmer_config_t config = {4, 0, 50, RBDIMMER_CURVE_RMS};
    rbdimmer_channel_t* channel;
    rbdimmer_create_channel(&config, &channel);

    // Monitor frequency stability
    Serial.println("Monitoring frequency stability...");
    uint16_t freq_readings[60];

    for(int i = 0; i < 60; i++) {
        delay(1000);
        freq_readings[i] = rbdimmer_get_frequency(0);
        Serial.printf("Freq[%d]: %d Hz\n", i, freq_readings[i]);
    }

    // Analyze stability
    uint16_t min_freq = 1000, max_freq = 0;
    for(int i = 0; i < 60; i++) {
        if(freq_readings[i] > 0) {
            if(freq_readings[i] < min_freq) min_freq = freq_readings[i];
            if(freq_readings[i] > max_freq) max_freq = freq_readings[i];
        }
    }

    Serial.printf("Frequency range: %d - %d Hz\n", min_freq, max_freq);
    if(max_freq - min_freq > 2) {
        Serial.println("Frequency unstable - check power quality");
    } else {
        Serial.println("Frequency stable");
    }
}
```

**Solutions:**

1. **Power Supply Issues**:
   - Check ESP32 power supply stability
   - Use quality power adapter (>500mA)
   - Add power supply filtering capacitors

2. **Electrical Interference**:
   - Separate low-voltage and AC wiring
   - Add ferrite cores on cables
   - Use shielded cables if necessary

3. **Load Issues**:
   - Test with different load type
   - Check load compatibility
   - Verify load current within dimmer rating

4. **Zero-Cross Noise (v2.0.0)**:
   - See [Dimming and Flickering Problems](#dimming-and-flickering-problems) below for ZC debounce tuning

## Performance Issues

### Problem: Timing Inaccuracy

**Symptoms:**
- Dimming levels don't match expected brightness
- Timing measurements show variations

**Diagnostic Code:**
```cpp
void diagnose_timing() {
    Serial.println("Timing accuracy diagnostic...");

    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 50);

    rbdimmer_config_t config = {4, 0, 0, RBDIMMER_CURVE_LINEAR};
    rbdimmer_channel_t* channel;
    rbdimmer_create_channel(&config, &channel);

    // Test timing at different levels
    for(int level = 10; level <= 90; level += 10) {
        rbdimmer_set_level(channel, level);
        uint32_t delay_us = rbdimmer_get_delay(channel);
        uint16_t freq = rbdimmer_get_frequency(0);
        uint32_t half_cycle_us = 1000000 / (2 * freq);

        Serial.printf("Level: %d%%, Delay: %d us, Half-cycle: %d us, Ratio: %.1f%%\n",
                      level, delay_us, half_cycle_us,
                      (float)delay_us / half_cycle_us * 100.0);

        delay(1000);
    }
}
```

**Solutions:**

1. **Check System Load**:
   ```cpp
   void check_system_load() {
       unsigned long start = millis();
       // Run normal operations for 10 seconds
       while(millis() - start < 10000) {
           // Your normal loop code here
           delay(10);
       }

       unsigned long actual_time = millis() - start;
       Serial.printf("Expected: 10000ms, Actual: %dms\n", actual_time);
       if(actual_time > 10100) {
           Serial.println("System overloaded - optimize code");
       }
   }
   ```

2. **Optimize Interrupt Handling**:
   - Keep ISR code minimal
   - Avoid Serial.print() in callbacks
   - Use FreeRTOS queues for data transfer

3. **Check CPU Clock**:
   ```cpp
   void check_cpu_clock() {
       Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
       if(ESP.getCpuFreqMHz() < 240) {
           Serial.println("Consider increasing CPU frequency");
       }
   }
   ```

### Problem: Memory Issues

**Symptoms:**
- System crashes or resets
- Channel creation fails
- Erratic behavior

**Diagnostic Code:**
```cpp
void diagnose_memory() {
    Serial.println("Memory diagnostic...");

    Serial.printf("Free heap at start: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Minimum free heap: %d bytes\n", ESP.getMinFreeHeap());
    Serial.printf("Heap size: %d bytes\n", ESP.getHeapSize());

    rbdimmer_init();
    Serial.printf("After init: %d bytes\n", ESP.getFreeHeap());

    // Create maximum channels
    rbdimmer_register_zero_cross(2, 0, 50);
    rbdimmer_channel_t* channels[8];

    for(int i = 0; i < 8; i++) {
        rbdimmer_config_t config = {4+i, 0, 0, RBDIMMER_CURVE_LINEAR};
        rbdimmer_err_t err = rbdimmer_create_channel(&config, &channels[i]);

        if(err == RBDIMMER_OK) {
            Serial.printf("Channel %d created, Free heap: %d bytes\n",
                          i, ESP.getFreeHeap());
        } else {
            Serial.printf("Channel %d failed: %d\n", i, err);
            break;
        }
    }

    if(ESP.getFreeHeap() < 10000) {
        Serial.println("WARNING: Low memory");
    }
}
```

**Solutions:**

1. **Reduce Memory Usage**:
   - Limit number of channels
   - Optimize string usage
   - Use PROGMEM for constants

2. **Check for Memory Leaks**:
   ```cpp
   void monitor_memory() {
       static unsigned long last_check = 0;
       static uint32_t last_free_heap = 0;

       if(millis() - last_check > 5000) {
           uint32_t current_heap = ESP.getFreeHeap();
           Serial.printf("Heap: %d (change: %d)\n",
                         current_heap, (int32_t)current_heap - last_free_heap);

           last_free_heap = current_heap;
           last_check = millis();
       }
   }
   ```

## Dimming and Flickering Problems

v2.0.0 addresses four distinct flickering issues discovered during 4-channel hardware validation. Each has a specific root cause and fix.

### Problem: General Flickering at All Brightness Levels

**Root cause:** TRIAC switching injects a voltage spike on the zero-cross optocoupler pin. This spike re-triggers the ZC ISR mid half-cycle, causing the library to compute incorrect timing and fire the TRIAC at the wrong moment.

**Fix (v2.0.0):** The `zero_cross_isr_handler` now implements a noise gate. Any ZC edge that arrives within `ZC_DEBOUNCE_US` (default 3000 us) of the previous valid edge is discarded. This eliminates false zero-cross detections caused by TRIAC switching noise.

**Tuning:** If you still see flickering on electrically noisy setups, increase the debounce window. If frequency auto-detection is slow or fails, decrease it.

```cpp
// ESP-IDF: set via menuconfig -> Component config -> RBDimmer
// Or as a compile-time define:
#define CONFIG_RBDIMMER_ZC_DEBOUNCE_US 4000  // default is 3000
```

See the [Kconfig Tuning](#kconfig-tuning) section for details.

### Problem: Flickering at 100% Brightness

**Root cause:** At 100% brightness the computed delay was 50 us. At that point in the AC cycle the instantaneous voltage is only around 5V -- below the TRIAC latching current threshold. Additionally, dispatching a timer ISR from within a GPIO ISR via `esp_timer` made sub-100 us delays unpredictable.

**Fix (v2.0.0):** Two changes:
- `MIN_DELAY_US` raised from 50 to 100 us, ensuring the TRIAC fires at a point where the AC voltage is high enough for reliable latching.
- Levels at or above 100% are mapped to `LEVEL_MAX` (default 99%), which avoids the problematic near-zero delay entirely.

**Tuning:** Adjust via Kconfig if your hardware can reliably latch at a shorter delay:

```cpp
#define CONFIG_RBDIMMER_MIN_DELAY_US 100     // default 100
#define CONFIG_RBDIMMER_LEVEL_MAX    99      // default 99
```

### Problem: Multi-Channel Synchronization -- Channels Firing at Different Offsets

**Root cause:** In v1.x, the ZC ISR processed all channels in a single loop: for each channel it reset the GPIO LOW and then armed the delay timer. Because each channel's GPIO reset happened at a slightly different time within that loop iteration, channels that should have fired simultaneously appeared offset from each other.

**Fix (v2.0.0):** The ZC ISR now uses a two-pass approach:
- **Pass 1:** Sets all channel GPIOs LOW (resets the TRIAC gate signals).
- **Pass 2:** Arms all delay timers.

This ensures all GPIO resets happen as close together as possible, and timer arming does not interleave with GPIO operations.

### Problem: Flickering at Levels Below 3%

**Root cause:** At very low brightness levels (below 3%), the computed delay fires near the end of the half-cycle where the AC voltage is too low for reliable TRIAC latching. The TRIAC either fails to latch or latches intermittently, producing visible flicker.

**Fix (v2.0.0):** Levels below `LEVEL_MIN` (default 3%) now return `delay=0`, which turns the channel OFF rather than attempting unreliable low-level dimming.

**Tuning:**

```cpp
#define CONFIG_RBDIMMER_LEVEL_MIN 3  // default 3
```

If your load and TRIAC combination can reliably latch at lower levels, you can reduce this value.

### Problem: No Response to Level Changes

**Diagnostic Steps:**

1. **Verify Channel State**:
   ```cpp
   void check_channel_state() {
       if(!rbdimmer_is_active(channel)) {
           Serial.println("Channel is inactive");
           rbdimmer_set_active(channel, true);
       }

       uint8_t level = rbdimmer_get_level(channel);
       Serial.printf("Current level: %d%%\n", level);

       uint32_t delay_us = rbdimmer_get_delay(channel);
       Serial.printf("Current delay: %d us\n", delay_us);
   }
   ```

2. **Test Manual Level Changes**:
   ```cpp
   void test_level_changes() {
       Serial.println("Testing level changes...");

       for(int level = 0; level <= 100; level += 10) {
           rbdimmer_set_level(channel, level);
           delay(500);

           uint8_t actual = rbdimmer_get_level(channel);
           if(actual != level) {
               Serial.printf("Level mismatch: set %d, got %d\n", level, actual);
           }
       }
   }
   ```

### Problem: Incorrect Brightness Response

**Solutions:**

1. **Try Different Curves**:
   ```cpp
   void test_curves() {
       rbdimmer_curve_t curves[] = {
           RBDIMMER_CURVE_LINEAR,
           RBDIMMER_CURVE_RMS,
           RBDIMMER_CURVE_LOGARITHMIC
       };

       const char* names[] = {"Linear", "RMS", "Logarithmic"};

       for(int i = 0; i < 3; i++) {
           Serial.printf("Testing %s curve...\n", names[i]);
           rbdimmer_set_curve(channel, curves[i]);

           rbdimmer_set_level(channel, 50);
           delay(2000);

           Serial.printf("Delay at 50%%: %d us\n", rbdimmer_get_delay(channel));
       }
   }
   ```

2. **Check Load Compatibility**:
   - Resistive loads: Use RMS curve
   - LED loads: Use Logarithmic curve
   - Motor loads: Use Linear curve

## Frequency Detection Issues

### Problem: Wrong Frequency Detection

**Diagnostic Code:**
```cpp
void diagnose_frequency_detection() {
    Serial.println("Frequency detection diagnostic...");

    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 0); // Auto-detect

    // Monitor detection process
    uint16_t readings[100];
    for(int i = 0; i < 100; i++) {
        delay(500);
        readings[i] = rbdimmer_get_frequency(0);
        Serial.printf("Reading %d: %d Hz\n", i, readings[i]);

        if(readings[i] > 0) {
            Serial.printf("Frequency detected at reading %d\n", i);
            break;
        }
    }

    // Analyze final frequency
    uint16_t final_freq = rbdimmer_get_frequency(0);
    if(final_freq == 50) {
        Serial.println("Detected 50Hz mains");
    } else if(final_freq == 60) {
        Serial.println("Detected 60Hz mains");
    } else if(final_freq == 0) {
        Serial.println("No frequency detected");
    } else {
        Serial.printf("Unusual frequency: %d Hz\n", final_freq);
    }
}
```

**Solutions:**

1. **Force Known Frequency**:
   ```cpp
   // If auto-detection fails, use known frequency
   rbdimmer_register_zero_cross(2, 0, 50); // Force 50Hz
   rbdimmer_register_zero_cross(2, 0, 60); // Force 60Hz
   ```

2. **Check Zero-Cross Signal Quality**:
   - Signal should be clean digital pulses
   - Check for electrical noise
   - Verify dimmer module specifications

3. **ZC Debounce Too Aggressive**:
   - If `ZC_DEBOUNCE_US` is set too high, legitimate ZC edges may be discarded
   - For 60Hz mains the half-cycle is ~8333 us; debounce must be well below this
   - Default 3000 us works for both 50Hz and 60Hz

## Multi-Channel Problems

### Problem: Channel Interference

**Symptoms:**
- Channels affecting each other
- Synchronized flickering

**Diagnostic Code:**
```cpp
void diagnose_multi_channel() {
    Serial.println("Multi-channel diagnostic...");

    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 50);

    // Create multiple channels
    rbdimmer_channel_t* channels[4];
    uint8_t pins[] = {4, 5, 18, 19};

    for(int i = 0; i < 4; i++) {
        rbdimmer_config_t config = {pins[i], 0, 0, RBDIMMER_CURVE_LINEAR};
        rbdimmer_create_channel(&config, &channels[i]);
    }

    // Test individual control
    Serial.println("Testing individual channel control...");
    for(int ch = 0; ch < 4; ch++) {
        Serial.printf("Activating channel %d only\n", ch);

        for(int i = 0; i < 4; i++) {
            rbdimmer_set_level(channels[i], (i == ch) ? 50 : 0);
        }

        delay(2000);
    }

    // Test simultaneous control
    Serial.println("Testing simultaneous control...");
    for(int level = 0; level <= 100; level += 25) {
        for(int i = 0; i < 4; i++) {
            rbdimmer_set_level(channels[i], level);
        }
        Serial.printf("All channels set to %d%%\n", level);
        delay(1000);
    }
}
```

**Solutions:**

1. **Check GPIO Pin Assignments**:
   - Ensure unique pins for each channel
   - Avoid conflicting pin usage

2. **Power Supply Capacity**:
   - Multiple channels increase current draw
   - Use adequate power supply

3. **Multi-Channel Sync Issues (v2.0.0)**:
   - v2.0.0 uses a two-pass ZC ISR to eliminate inter-channel offset
   - If channels still appear offset, verify you are running v2.0.0
   - Check that all channels are registered on the same phase if they share a ZC signal

## Kconfig Tuning

v2.0.0 exposes four parameters via Kconfig (ESP-IDF `menuconfig`) or as compile-time defines for Arduino. These control the flickering fixes and should only be changed if the defaults do not suit your hardware.

### ZC_DEBOUNCE_US -- Zero-Cross Debounce Window

```
CONFIG_RBDIMMER_ZC_DEBOUNCE_US (default: 3000)
```

The minimum time in microseconds between two valid zero-cross edges. Any edge arriving sooner is treated as noise and discarded.

**When to adjust:**
- **Increase** if you still see general flickering (TRIAC noise getting through). Try 4000-5000.
- **Decrease** if frequency auto-detection is unreliable or slow. Do not go below 1500 for 60Hz mains.
- Must be significantly less than half the AC period (10000 us at 50Hz, 8333 us at 60Hz).

### MIN_DELAY_US -- Minimum TRIAC Firing Delay

```
CONFIG_RBDIMMER_MIN_DELAY_US (default: 100)
```

The shortest delay allowed between a zero-cross event and the TRIAC gate pulse. Prevents firing at near-zero AC voltage where TRIAC latching is unreliable.

**When to adjust:**
- **Increase** if you see flickering at high brightness levels. Try 150-200.
- **Decrease** only if your TRIAC and load combination reliably latches at very short delays. Not recommended below 80.

### LEVEL_MAX -- Maximum Effective Dimming Level

```
CONFIG_RBDIMMER_LEVEL_MAX (default: 99)
```

Levels at or above this value are clamped to this percentage. Prevents the computed delay from falling below `MIN_DELAY_US`.

**When to adjust:**
- Normally leave at 99. Setting to 100 effectively disables the max-level clamp and may cause flickering at full brightness.

### LEVEL_MIN -- Minimum Effective Dimming Level

```
CONFIG_RBDIMMER_LEVEL_MIN (default: 3)
```

Levels below this value result in the channel being turned OFF (delay=0) rather than attempting to fire the TRIAC near the end of the half-cycle.

**When to adjust:**
- **Decrease** if your TRIAC and load can reliably dim below 3%. Test carefully.
- **Increase** if you see flickering at low brightness. Try 5.

### Setting Parameters

**ESP-IDF (menuconfig):**
```
idf.py menuconfig
-> Component config -> RBDimmer ESP32
```

**Arduino (compile-time defines):**
```ini
# platformio.ini
build_flags =
    -DCONFIG_RBDIMMER_ZC_DEBOUNCE_US=3000
    -DCONFIG_RBDIMMER_MIN_DELAY_US=100
    -DCONFIG_RBDIMMER_LEVEL_MAX=99
    -DCONFIG_RBDIMMER_LEVEL_MIN=3
```

```cpp
// Or in code, before including the library header:
#define CONFIG_RBDIMMER_ZC_DEBOUNCE_US 3000
#define CONFIG_RBDIMMER_MIN_DELAY_US   100
#define CONFIG_RBDIMMER_LEVEL_MAX      99
#define CONFIG_RBDIMMER_LEVEL_MIN      3
#include <rbdimmerESP32.h>
```

## Safety and Hardware Failures

### CRITICAL: Safety Issues

**Immediate Actions for Safety Problems:**

1. **Burning Smell or Smoke**:
   - Immediately disconnect power
   - Do not attempt to diagnose while powered
   - Check all connections when safe

2. **Electrical Shock**:
   - Disconnect power immediately
   - Check for proper isolation
   - Verify dimmer module integrity

3. **Overheating**:
   - Check current ratings
   - Verify adequate heat sinking
   - Reduce load if necessary

### Hardware Failure Diagnosis

**Systematic Approach:**

1. **Visual Inspection**:
   - Check for burnt components
   - Look for loose connections
   - Verify proper mounting

2. **Electrical Testing** (Power Off Only):
   - Continuity checks
   - Insulation resistance
   - Component values

3. **Replacement Testing**:
   - Swap suspected components
   - Test with known good modules
   - Isolate problem components

## Debugging Tools and Techniques

### Serial Monitor Debugging

**Enhanced Debug Output:**
```cpp
#define DEBUG_LEVEL 3 // 0=none, 1=error, 2=warning, 3=info

void debug_print(int level, const char* message) {
    if(level <= DEBUG_LEVEL) {
        const char* prefixes[] = {"", "[ERROR]", "[WARN]", "[INFO]"};
        Serial.printf("%s %s\n", prefixes[level], message);
    }
}

void detailed_status() {
    debug_print(3, "=== System Status ===");
    debug_print(3, String("Free heap: " + String(ESP.getFreeHeap())).c_str());
    debug_print(3, String("CPU freq: " + String(ESP.getCpuFreqMHz()) + "MHz").c_str());
    debug_print(3, String("Frequency: " + String(rbdimmer_get_frequency(0)) + "Hz").c_str());

    if(channel) {
        debug_print(3, String("Channel level: " + String(rbdimmer_get_level(channel)) + "%").c_str());
        debug_print(3, String("Channel delay: " + String(rbdimmer_get_delay(channel)) + "us").c_str());
        debug_print(3, String("Channel active: " + String(rbdimmer_is_active(channel) ? "Yes" : "No")).c_str());
    }
}
```

### Oscilloscope Analysis

**Key Measurements:**
1. **Zero-cross signal**: Should be clean 50/60Hz pulses
2. **Gate control**: Precise timing relative to zero-cross
3. **Load voltage**: Phase-cut waveform
4. **Load current**: Should follow voltage pattern

### Logic Analyzer

**For Digital Signal Analysis:**
- Zero-cross timing
- Gate control timing
- Multi-channel synchronization (verify two-pass ISR eliminates offset)

## Frequently Asked Questions

### Q: Why is my dimmer flickering?

**A:** v2.0.0 fixes the four most common flickering causes. Check which pattern matches yours:
1. **Flickering at all levels** -- ZC noise from TRIAC switching. Fixed by ZC debounce. See [General Flickering](#problem-general-flickering-at-all-brightness-levels).
2. **Flickering at 100%** -- TRIAC fails to latch at near-zero delay. Fixed by MIN_DELAY_US increase. See [Flickering at 100%](#problem-flickering-at-100-brightness).
3. **Flickering below 3%** -- AC voltage too low at end of half-cycle. Fixed by LEVEL_MIN cutoff. See [Flickering Below 3%](#problem-flickering-at-levels-below-3).
4. **Multi-channel offset/flicker** -- ISR loop ordering. Fixed by two-pass ISR. See [Multi-Channel Sync](#problem-multi-channel-synchronization----channels-firing-at-different-offsets).

If none of these match, also check:
5. **Load incompatibility** -- Try resistive load for testing
6. **Power supply issues** -- Check ESP32 power stability
7. **Electrical interference** -- Separate AC and DC wiring
8. **Wrong curve type** -- Try different curve settings

### Q: Can I use this with LED lights?

**A:** Yes, but with limitations:
- Use only "dimmable" rated LED bulbs
- Try `RBDIMMER_CURVE_LOGARITHMIC`
- Some LEDs may flicker at low levels -- consider increasing `LEVEL_MIN`
- Test compatibility first

### Q: Why does frequency detection fail?

**A:** Possible reasons:
1. **No zero-cross signal** -- Check wiring
2. **Wrong dimmer module** -- Verify ZC output
3. **Electrical noise** -- Improve signal quality
4. **ZC debounce too aggressive** -- Try reducing `ZC_DEBOUNCE_US`
5. **Software issue** -- Try fixed frequency first

### Q: How many channels can I use?

**A:** Technical limits:
- Maximum 8 channels per ESP32
- Each channel uses ~200 bytes RAM
- All channels on same phase share zero-crossing
- Power supply must support all channels
- v2.0.0 two-pass ISR ensures proper synchronization across all channels

### Q: Is it safe to use with motors?

**A:** With precautions:
- Use motor-rated dimmer modules
- Consider soft-start requirements
- Monitor current and temperature
- Use appropriate curve (usually Linear)

### Q: Can I control 220V loads?

**A:** Yes, if dimmer module supports it:
- Verify dimmer voltage rating
- Use appropriate safety measures
- Follow local electrical codes
- Consider professional installation

### Q: What ESP-IDF version is required?

**A:** ESP-IDF 5.3 or later is required for v2.0.0. The library uses Kconfig for parameter configuration, which integrates with the ESP-IDF `menuconfig` system. Arduino users can set parameters via compile-time defines instead.

## Getting Additional Help

### Before Asking for Help

Please gather this information:

1. **Hardware Setup**:
   - ESP32 board type
   - Dimmer module model
   - Load type and power
   - Wiring diagram

2. **Software Configuration**:
   - IDE and version
   - Library version (v2.0.0 or earlier)
   - ESP-IDF version (if applicable)
   - Complete code (minimal example)
   - Error messages
   - Kconfig / compile-time parameter overrides

3. **Problem Description**:
   - Expected behavior
   - Actual behavior
   - Steps to reproduce
   - When problem started

### Support Channels

1. **GitHub Issues**: [Report bugs and problems](https://github.com/robotdyn-dimmer/rbdimmerESP32/issues)
2. **Community Forum**: [Discussion and help](https://www.rbdimmer.com/forum)
3. **Email Support**: support@rbdimmer.com
4. **Documentation**: [Complete guides](https://www.rbdimmer.com/docs/universal-library-for-esp32)

### Creating Good Issue Reports

**Template for Issue Reports:**

```markdown
## Problem Description
Brief description of the issue

## Hardware Setup
- ESP32 Board: [e.g., ESP32 DevKit V1]
- Dimmer Module: [e.g., RobotDyn AC Light Dimmer]
- Load: [e.g., 100W incandescent bulb]
- Wiring: [pin connections]

## Software Environment
- IDE: [Arduino IDE 2.x]
- ESP32 Core / ESP-IDF: [version]
- Library Version: [2.0.0]
- Kconfig overrides: [list any non-default values]

## Expected Behavior
What should happen

## Actual Behavior
What actually happens

## Code Example
\```cpp
// Minimal code that reproduces the problem
\```

## Additional Information
- Error messages
- Serial output
- Oscilloscope traces (if available)
```

---

**Remember**: Most problems have simple solutions. Work through the diagnostics systematically, and don't hesitate to ask for help with detailed information about your setup.

*Troubleshooting guide for rbdimmerESP32 v2.0.0*
