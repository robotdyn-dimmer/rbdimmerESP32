# rbdimmerESP32 API Reference

## Table of Contents

1. [Overview](#overview)
2. [Data Types](#data-types)
3. [Constants](#constants)
4. [Initialization Functions](#initialization-functions)
5. [Zero-Cross Management](#zero-cross-management) 
6. [Channel Management](#channel-management)
7. [Level Control](#level-control)
8. [Configuration Functions](#configuration-functions)
9. [Information Retrieval](#information-retrieval)
10. [Callback Functions](#callback-functions)
11. [Utility Functions](#utility-functions)
12. [Error Handling](#error-handling)
13. [Code Examples](#code-examples)

## Overview

The rbdimmerESP32 library provides a comprehensive API for controlling AC dimmers with ESP32 microcontrollers. The API is designed with safety, performance, and ease of use in mind, featuring hardware timer integration and interrupt-driven operation.

### Key Design Principles

- **Thread-Safe**: All functions are safe to call from multiple FreeRTOS tasks
- **Hardware Optimized**: Uses ESP32 hardware timers for microsecond precision
- **Resource Managed**: Automatic cleanup and error handling
- **Extensible**: Support for multiple phases and channels

## Data Types

### Opaque Types

#### `rbdimmer_channel_t`
```c
typedef struct rbdimmer_channel_s rbdimmer_channel_t;
```
Opaque handle representing a dimmer channel. All channel operations use this handle.

**Usage Notes:**
- Created by `rbdimmer_create_channel()`
- Must be properly destroyed with `rbdimmer_delete_channel()`
- Cannot be copied or serialized

### Configuration Structures

#### `rbdimmer_config_t`
```c
typedef struct {
    uint8_t gpio_pin;                 // Output signal pin
    uint8_t phase;                    // Phase number (for multi-phase systems)
    uint8_t initial_level;            // Initial level percentage (0-100)
    rbdimmer_curve_t curve_type;      // Level curve type
} rbdimmer_config_t;
```

Configuration structure for creating dimmer channels.

**Fields:**
- `gpio_pin`: GPIO pin connected to the dimmer's gate control (0-39)
- `phase`: Phase identifier (0-3) - must be registered first
- `initial_level`: Starting brightness level (0-100%)
- `curve_type`: Brightness curve algorithm

**Example:**
```c
rbdimmer_config_t config = {
    .gpio_pin = 4,
    .phase = 0,
    .initial_level = 50,
    .curve_type = RBDIMMER_CURVE_RMS
};
```

### Enumerations

#### `rbdimmer_curve_t`
```c
typedef enum {
    RBDIMMER_CURVE_LINEAR,        // Linear curve (no RMS consideration)
    RBDIMMER_CURVE_RMS,           // RMS-compensated curve
    RBDIMMER_CURVE_LOGARITHMIC,   // Logarithmic curve (for LEDs)
    RBDIMMER_CURVE_CUSTOM         // Custom curve (reserved)
} rbdimmer_curve_t;
```

Defines the brightness curve type for power calculation.

**Curve Types:**
- **LINEAR**: Direct percentage to phase angle conversion
- **RMS**: Power-compensated for resistive loads (incandescent bulbs)
- **LOGARITHMIC**: Perceptually linear for LED loads
- **CUSTOM**: Reserved for future custom curve implementation

#### `rbdimmer_err_t`
```c
typedef enum {
    RBDIMMER_OK = 0,                  // Operation completed successfully
    RBDIMMER_ERR_INVALID_ARG,         // Invalid argument
    RBDIMMER_ERR_NO_MEMORY,           // Memory allocation failed
    RBDIMMER_ERR_NOT_FOUND,           // Object not found
    RBDIMMER_ERR_ALREADY_EXIST,       // Object already exists
    RBDIMMER_ERR_TIMER_FAILED,        // Timer initialization failed
    RBDIMMER_ERR_GPIO_FAILED          // GPIO initialization failed
} rbdimmer_err_t;
```

Error codes returned by library functions.

#### `timer_state_t`
```c
typedef enum {
    TIMER_STATE_IDLE,        // Waiting for zero-crossing
    TIMER_STATE_DELAY,       // Waiting for delay to finish
    TIMER_STATE_PULSE_ON,    // Pulse is active, waiting to turn off
} timer_state_t;
```

Internal timer state enumeration (used internally by the library).

## Constants

### System Limits
```c
#define RBDIMMER_MAX_PHASES 4                 // Maximum number of phases
#define RBDIMMER_MAX_CHANNELS 8               // Maximum number of channels
```

### Timing Constants
```c
#define RBDIMMER_DEFAULT_PULSE_WIDTH_US 50    // Default pulse width in microseconds
#define RBDIMMER_MIN_DELAY_US 50              // Minimum delay for safe triac operation
```

### Frequency Constants
```c
#define RBDIMMER_DEFAULT_FREQUENCY 0          // Auto-detect frequency
#define RBDIMMER_FREQUENCY_MIN 45             // Minimum allowed frequency
#define RBDIMMER_FREQUENCY_MAX 65             // Maximum allowed frequency
#define RBDIMMER_MEASURE_CYCLES 10            // Number of cycles for measurement
```

## Initialization Functions

### `rbdimmer_init()`
```c
rbdimmer_err_t rbdimmer_init(void);
```

Initializes the rbdimmerESP32 library. Must be called before any other library functions.

**Returns:**
- `RBDIMMER_OK`: Initialization successful
- `RBDIMMER_ERR_NO_MEMORY`: Memory allocation failed

**Example:**
```c
void setup() {
    rbdimmer_err_t err = rbdimmer_init();
    if (err != RBDIMMER_OK) {
        Serial.println("Failed to initialize rbdimmer library");
        return;
    }
    Serial.println("rbdimmer initialized successfully");
}
```

**Notes:**
- Initializes internal data structures
- Prepares lookup tables for brightness curves
- Must be called from main task/setup function

### `rbdimmer_deinit()`
```c
rbdimmer_err_t rbdimmer_deinit(void);
```

Deinitializes the library and releases all resources.

**Returns:**
- `RBDIMMER_OK`: Deinitialization successful

**Example:**
```c
void cleanup() {
    rbdimmer_deinit();
    Serial.println("Library deinitialized");
}
```

**Notes:**
- Automatically deletes all channels
- Removes all interrupt handlers
- Releases GPIO resources
- Call before system restart or deep sleep

## Zero-Cross Management

### `rbdimmer_register_zero_cross()`
```c
rbdimmer_err_t rbdimmer_register_zero_cross(uint8_t pin, uint8_t phase, uint16_t frequency);
```

Registers a zero-crossing detector for AC synchronization.

**Parameters:**
- `pin`: GPIO pin connected to zero-cross detector (0-39)
- `phase`: Phase identifier (0-3)
- `frequency`: Expected mains frequency in Hz (0 for auto-detect, 50, 60)

**Returns:**
- `RBDIMMER_OK`: Registration successful
- `RBDIMMER_ERR_INVALID_ARG`: Invalid pin or phase number
- `RBDIMMER_ERR_ALREADY_EXIST`: Phase already registered
- `RBDIMMER_ERR_NO_MEMORY`: Maximum phases reached
- `RBDIMMER_ERR_GPIO_FAILED`: GPIO configuration failed

**Example:**
```c
// Auto-detect frequency on phase 0
rbdimmer_err_t err = rbdimmer_register_zero_cross(2, 0, 0);
if (err == RBDIMMER_OK) {
    Serial.println("Zero-cross detector registered on pin 2");
}

// Fixed 60Hz frequency on phase 1
rbdimmer_register_zero_cross(3, 1, 60);
```

**Notes:**
- Must be called before creating channels on the same phase
- Pin will be configured as input with interrupt on rising edge
- Frequency 0 enables automatic detection (recommended)
- Each phase requires a separate zero-cross detector

## Channel Management

### `rbdimmer_create_channel()`
```c
rbdimmer_err_t rbdimmer_create_channel(rbdimmer_config_t* config, rbdimmer_channel_t** channel);
```

Creates a new dimmer channel with specified configuration.

**Parameters:**
- `config`: Pointer to configuration structure
- `channel`: Pointer to store the created channel handle

**Returns:**
- `RBDIMMER_OK`: Channel created successfully
- `RBDIMMER_ERR_INVALID_ARG`: Invalid configuration or NULL pointers
- `RBDIMMER_ERR_NOT_FOUND`: Referenced phase not registered
- `RBDIMMER_ERR_NO_MEMORY`: Memory allocation failed or max channels reached
- `RBDIMMER_ERR_GPIO_FAILED`: GPIO configuration failed
- `RBDIMMER_ERR_TIMER_FAILED`: Timer creation failed

**Example:**
```c
rbdimmer_channel_t* my_channel;
rbdimmer_config_t config = {
    .gpio_pin = 4,
    .phase = 0,
    .initial_level = 0,
    .curve_type = RBDIMMER_CURVE_RMS
};

rbdimmer_err_t err = rbdimmer_create_channel(&config, &my_channel);
if (err == RBDIMMER_OK) {
    Serial.println("Channel created successfully");
}
```

**Notes:**
- GPIO pin will be configured as output
- Channel starts in active state
- Two hardware timers are allocated per channel
- Initial level is applied immediately

### `rbdimmer_delete_channel()`
```c
rbdimmer_err_t rbdimmer_delete_channel(rbdimmer_channel_t* channel);
```

Deletes a dimmer channel and releases all associated resources.

**Parameters:**
- `channel`: Channel handle to delete

**Returns:**
- `RBDIMMER_OK`: Channel deleted successfully
- `RBDIMMER_ERR_INVALID_ARG`: NULL channel handle
- `RBDIMMER_ERR_NOT_FOUND`: Channel not found in manager

**Example:**
```c
rbdimmer_err_t err = rbdimmer_delete_channel(my_channel);
if (err == RBDIMMER_OK) {
    my_channel = NULL; // Prevent accidental reuse
    Serial.println("Channel deleted successfully");
}
```

**Notes:**
- Stops all running timers
- Sets GPIO output to LOW
- Releases allocated memory
- Channel handle becomes invalid after deletion

## Level Control

### `rbdimmer_set_level()`
```c
rbdimmer_err_t rbdimmer_set_level(rbdimmer_channel_t* channel, uint8_t level_percent);
```

Sets the brightness level of a dimmer channel immediately.

**Parameters:**
- `channel`: Target channel handle
- `level_percent`: Brightness level (0-100%)

**Returns:**
- `RBDIMMER_OK`: Level set successfully
- `RBDIMMER_ERR_INVALID_ARG`: NULL channel handle

**Example:**
```c
// Set to 50% brightness
rbdimmer_set_level(my_channel, 50);

// Turn off
rbdimmer_set_level(my_channel, 0);

// Full brightness
rbdimmer_set_level(my_channel, 100);
```

**Notes:**
- Level changes take effect on the next zero-crossing
- Values above 100 are clamped to 100
- Level 0 = fully off, Level 100 = fully on
- Thread-safe - can be called from any task

### `rbdimmer_set_level_transition()`
```c
rbdimmer_err_t rbdimmer_set_level_transition(rbdimmer_channel_t* channel, uint8_t level_percent, uint32_t transition_ms);
```

Sets the brightness level with a smooth transition over time.

**Parameters:**
- `channel`: Target channel handle
- `level_percent`: Target brightness level (0-100%)
- `transition_ms`: Transition duration in milliseconds

**Returns:**
- `RBDIMMER_OK`: Transition started successfully
- `RBDIMMER_ERR_INVALID_ARG`: NULL channel handle
- `RBDIMMER_ERR_NO_MEMORY`: Failed to create transition task

**Example:**
```c
// Fade to 75% over 3 seconds
rbdimmer_set_level_transition(my_channel, 75, 3000);

// Quick fade to off over 500ms
rbdimmer_set_level_transition(my_channel, 0, 500);
```

**Notes:**
- Creates a FreeRTOS task for smooth transitions
- Non-blocking - returns immediately
- Minimum transition time is 50ms
- Transitions shorter than 50ms use immediate setting
- Multiple transitions can run simultaneously on different channels

## Configuration Functions

### `rbdimmer_set_curve()`
```c
rbdimmer_err_t rbdimmer_set_curve(rbdimmer_channel_t* channel, rbdimmer_curve_t curve_type);
```

Sets the brightness curve type for a channel.

**Parameters:**
- `channel`: Target channel handle
- `curve_type`: Desired curve type

**Returns:**
- `RBDIMMER_OK`: Curve set successfully
- `RBDIMMER_ERR_INVALID_ARG`: NULL channel handle

**Example:**
```c
// For incandescent bulbs
rbdimmer_set_curve(my_channel, RBDIMMER_CURVE_RMS);

// For LED strips
rbdimmer_set_curve(my_channel, RBDIMMER_CURVE_LOGARITHMIC);

// For motor control
rbdimmer_set_curve(my_channel, RBDIMMER_CURVE_LINEAR);
```

**Notes:**
- Change takes effect on next zero-crossing
- Different curves optimize for different load types
- Can be changed during operation without restart

### `rbdimmer_set_active()`
```c
rbdimmer_err_t rbdimmer_set_active(rbdimmer_channel_t* channel, bool active);
```

Enables or disables a dimmer channel.

**Parameters:**
- `channel`: Target channel handle
- `active`: true to enable, false to disable

**Returns:**
- `RBDIMMER_OK`: State changed successfully
- `RBDIMMER_ERR_INVALID_ARG`: NULL channel handle

**Example:**
```c
// Enable channel
rbdimmer_set_active(my_channel, true);

// Disable channel (output goes to 0)
rbdimmer_set_active(my_channel, false);
```

**Notes:**
- Disabled channels consume no CPU time
- Output is immediately set to LOW when disabled
- Channel configuration is preserved when disabled
- Re-enabling resumes previous operation

## Information Retrieval

### `rbdimmer_get_level()`
```c
uint8_t rbdimmer_get_level(rbdimmer_channel_t* channel);
```

Gets the current brightness level of a channel.

**Parameters:**
- `channel`: Channel handle to query

**Returns:**
- Current brightness level (0-100%), or 0 if channel is NULL

**Example:**
```c
uint8_t current_level = rbdimmer_get_level(my_channel);
Serial.printf("Current brightness: %d%%\n", current_level);
```

### `rbdimmer_get_frequency()`
```c
uint16_t rbdimmer_get_frequency(uint8_t phase);
```

Gets the measured mains frequency for a specific phase.

**Parameters:**
- `phase`: Phase number to query (0-3)

**Returns:**
- Measured frequency in Hz, or 0 if not measured yet or phase not found

**Example:**
```c
uint16_t freq = rbdimmer_get_frequency(0);
if (freq > 0) {
    Serial.printf("Mains frequency: %d Hz\n", freq);
} else {
    Serial.println("Frequency not measured yet");
}
```

**Notes:**
- Returns 0 during initial measurement period
- Frequency detection takes about 20 AC cycles
- Useful for power quality monitoring

### `rbdimmer_is_active()`
```c
bool rbdimmer_is_active(rbdimmer_channel_t* channel);
```

Checks if a channel is currently active.

**Parameters:**
- `channel`: Channel handle to query

**Returns:**
- `true` if channel is active, `false` if inactive or NULL

**Example:**
```c
if (rbdimmer_is_active(my_channel)) {
    Serial.println("Channel is active");
} else {
    Serial.println("Channel is inactive");
}
```

### `rbdimmer_get_curve()`
```c
rbdimmer_curve_t rbdimmer_get_curve(rbdimmer_channel_t* channel);
```

Gets the current curve type of a channel.

**Parameters:**
- `channel`: Channel handle to query

**Returns:**
- Current curve type, or `RBDIMMER_CURVE_LINEAR` if channel is NULL

**Example:**
```c
rbdimmer_curve_t curve = rbdimmer_get_curve(my_channel);
switch(curve) {
    case RBDIMMER_CURVE_LINEAR:
        Serial.println("Using linear curve");
        break;
    case RBDIMMER_CURVE_RMS:
        Serial.println("Using RMS curve");
        break;
    case RBDIMMER_CURVE_LOGARITHMIC:
        Serial.println("Using logarithmic curve");
        break;
}
```

### `rbdimmer_get_delay()`
```c
uint32_t rbdimmer_get_delay(rbdimmer_channel_t* channel);
```

Gets the current delay setting in microseconds.

**Parameters:**
- `channel`: Channel handle to query

**Returns:**
- Current delay in microseconds, or 0 if channel is NULL

**Example:**
```c
uint32_t delay_us = rbdimmer_get_delay(my_channel);
Serial.printf("Current delay: %d microseconds\n", delay_us);
```

**Notes:**
- Useful for debugging and optimization
- Delay varies with brightness level and curve type
- Measured from zero-crossing to TRIAC trigger

## Callback Functions

### `rbdimmer_set_callback()`
```c
rbdimmer_err_t rbdimmer_set_callback(uint8_t phase, void (*callback)(void*), void* user_data);
```

Sets a callback function to be called on zero-crossing events.

**Parameters:**
- `phase`: Phase number (0-3)
- `callback`: Function to call on zero-crossing (NULL to disable)
- `user_data`: User data pointer passed to callback

**Returns:**
- `RBDIMMER_OK`: Callback set successfully
- `RBDIMMER_ERR_NOT_FOUND`: Phase not registered

**Callback Function Signature:**
```c
void zero_cross_callback(void* user_data);
```

**Example:**
```c
typedef struct {
    uint32_t cross_count;
    unsigned long last_time;
} callback_data_t;

callback_data_t cb_data = {0, 0};

void my_zero_cross_callback(void* user_data) {
    callback_data_t* data = (callback_data_t*)user_data;
    data->cross_count++;
    data->last_time = millis();
    
    // Note: Keep ISR code minimal and fast!
    if (data->cross_count % 100 == 0) {
        // Schedule a task to print statistics
        // Don't use Serial.print() directly in ISR!
    }
}

// Register the callback
rbdimmer_set_callback(0, my_zero_cross_callback, &cb_data);
```

**Important Notes:**
- Callback runs in interrupt context (ISR)
- Keep callback code minimal and fast
- No blocking operations (delays, prints, etc.)
- Use FreeRTOS queues for communication with tasks
- Callback is called on every zero-crossing (100-120 times per second)

## Utility Functions

### `rbdimmer_update_all()`
```c
rbdimmer_err_t rbdimmer_update_all(void);
```

Forces an immediate update of all active channels.

**Returns:**
- `RBDIMMER_OK`: All channels updated successfully

**Example:**
```c
// After changing multiple channel parameters
rbdimmer_update_all();
```

**Notes:**
- Normally not needed - channels update automatically
- Useful after bulk configuration changes
- Recalculates timing for all active channels

## Error Handling

### Error Code Descriptions

| Error Code | Description | Common Causes |
|------------|-------------|---------------|
| `RBDIMMER_OK` | Success | Operation completed normally |
| `RBDIMMER_ERR_INVALID_ARG` | Invalid argument | NULL pointers, out-of-range values |
| `RBDIMMER_ERR_NO_MEMORY` | Memory allocation failed | Insufficient heap memory |
| `RBDIMMER_ERR_NOT_FOUND` | Object not found | Phase not registered, channel not found |
| `RBDIMMER_ERR_ALREADY_EXIST` | Object already exists | Phase already registered |
| `RBDIMMER_ERR_TIMER_FAILED` | Timer operation failed | Hardware timer unavailable |
| `RBDIMMER_ERR_GPIO_FAILED` | GPIO operation failed | Invalid pin, pin already in use |

### Error Handling Best Practices

```c
rbdimmer_err_t err;
rbdimmer_channel_t* channel;

// Always check return values
err = rbdimmer_init();
if (err != RBDIMMER_OK) {
    Serial.printf("Initialization failed: %d\n", err);
    return;
}

// Handle specific error conditions
err = rbdimmer_register_zero_cross(2, 0, 0);
switch (err) {
    case RBDIMMER_OK:
        Serial.println("Zero-cross registered successfully");
        break;
    case RBDIMMER_ERR_ALREADY_EXIST:
        Serial.println("Phase already registered - continuing");
        break;
    case RBDIMMER_ERR_GPIO_FAILED:
        Serial.println("GPIO configuration failed - check wiring");
        return;
    default:
        Serial.printf("Unexpected error: %d\n", err);
        return;
}
```

## Code Examples

### Complete Basic Setup

```c
#include <rbdimmerESP32.h>

rbdimmer_channel_t* dimmer;

void setup() {
    Serial.begin(115200);
    
    // Initialize library
    if (rbdimmer_init() != RBDIMMER_OK) {
        Serial.println("Library initialization failed");
        return;
    }
    
    // Register zero-cross detector
    if (rbdimmer_register_zero_cross(2, 0, 0) != RBDIMMER_OK) {
        Serial.println("Zero-cross registration failed");
        return;
    }
    
    // Create dimmer channel
    rbdimmer_config_t config = {
        .gpio_pin = 4,
        .phase = 0,
        .initial_level = 0,
        .curve_type = RBDIMMER_CURVE_RMS
    };
    
    if (rbdimmer_create_channel(&config, &dimmer) != RBDIMMER_OK) {
        Serial.println("Channel creation failed");
        return;
    }
    
    Serial.println("Setup complete");
}

void loop() {
    // Fade up over 2 seconds
    rbdimmer_set_level_transition(dimmer, 100, 2000);
    delay(3000);
    
    // Fade down over 1 second
    rbdimmer_set_level_transition(dimmer, 0, 1000);
    delay(2000);
}
```

### Multi-Channel Control

```c
#include <rbdimmerESP32.h>

#define NUM_CHANNELS 3
rbdimmer_channel_t* channels[NUM_CHANNELS];
uint8_t dimmer_pins[] = {4, 5, 6};

void setup() {
    Serial.begin(115200);
    
    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 0);
    
    // Create multiple channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
        rbdimmer_config_t config = {
            .gpio_pin = dimmer_pins[i],
            .phase = 0,
            .initial_level = 0,
            .curve_type = RBDIMMER_CURVE_RMS
        };
        
        rbdimmer_create_channel(&config, &channels[i]);
    }
    
    Serial.println("Multi-channel setup complete");
}

void loop() {
    // Sequential fade effect
    for (int i = 0; i < NUM_CHANNELS; i++) {
        rbdimmer_set_level_transition(channels[i], 100, 1000);
        delay(500);
    }
    
    delay(2000);
    
    // All off together
    for (int i = 0; i < NUM_CHANNELS; i++) {
        rbdimmer_set_level_transition(channels[i], 0, 1000);
    }
    
    delay(2000);
}
```

### Advanced Callback Usage

```c
#include <rbdimmerESP32.h>

typedef struct {
    uint32_t zero_cross_count;
    uint16_t frequency;
    bool frequency_stable;
} system_stats_t;

system_stats_t stats = {0, 0, false};
rbdimmer_channel_t* dimmer;

void zero_cross_isr(void* user_data) {
    system_stats_t* s = (system_stats_t*)user_data;
    s->zero_cross_count++;
    
    // Check frequency every 100 crossings (avoid frequent updates)
    if (s->zero_cross_count % 100 == 0) {
        s->frequency = rbdimmer_get_frequency(0);
        s->frequency_stable = (s->frequency == 50 || s->frequency == 60);
    }
}

void setup() {
    Serial.begin(115200);
    
    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 0);
    
    // Set up callback for monitoring
    rbdimmer_set_callback(0, zero_cross_isr, &stats);
    
    rbdimmer_config_t config = {
        .gpio_pin = 4,
        .phase = 0,
        .initial_level = 50,
        .curve_type = RBDIMMER_CURVE_RMS
    };
    
    rbdimmer_create_channel(&config, &dimmer);
}

void loop() {
    // Print statistics every 5 seconds
    static unsigned long last_print = 0;
    if (millis() - last_print > 5000) {
        Serial.printf("Zero crossings: %d\n", stats.zero_cross_count);
        Serial.printf("Frequency: %d Hz\n", stats.frequency);
        Serial.printf("Frequency stable: %s\n", stats.frequency_stable ? "Yes" : "No");
        Serial.printf("Current level: %d%%\n", rbdimmer_get_level(dimmer));
        Serial.println("---");
        
        last_print = millis();
    }
    
    delay(100);
}
```

## Performance Considerations

### Memory Usage
- Each channel uses approximately 200 bytes of RAM
- Global library overhead: ~1KB RAM
- Flash memory usage: ~32KB

### Timing Precision
- Zero-crossing detection: ±10 microseconds
- TRIAC triggering: ±1 microsecond
- Frequency measurement: ±0.1 Hz

### CPU Overhead
- Interrupt handling: <50 microseconds per zero-crossing
- Background processing: <1% CPU usage
- Transition tasks: Minimal impact on other operations

### Limitations
- Maximum 8 channels per ESP32
- Maximum 4 independent phases
- Minimum pulse width: 50 microseconds
- Maximum transition time: Limited by available memory for tasks

---

*This API reference covers all public functions and data types in rbdimmerESP32 v1.0.0*