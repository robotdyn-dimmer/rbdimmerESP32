
# ESP-IDF Guide & Examples

Dimmers universal library for ESP32. ESP-IDF framework C guide and examples.

:::info
This guide is updated for **rbdimmerESP32 v2.0.0** (March 2026). Public API is unchanged — existing code works without modification.
:::

Before start, read library overview: [Universal library for ESP32](https://www.rbdimmer.com/docs/universal-library-for-esp32)

:::info
**Download library from GitHub:** [rbdimmerESP32](https://github.com/robotdyn-dimmer/rbdimmerESP32)
:::

## Requirements and Compatibility

- **Minimum ESP-IDF version:** 5.3
- Supported chips: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6
- The library uses standard `Kconfig` for build configuration (renamed from `Kconfig.txt` in v2.0.0)

## New in v2.0.0

Version 2.0.0 is a major internal rewrite. The **public API is fully backward-compatible** — no changes to your application code are required.

**Internal improvements:**

- **Modular architecture** — the codebase is split into 7 internal modules (phase engine, channel manager, curve tables, ISR core, etc.). The single public header `rbdimmerESP32.h` remains the only include you need.
- **All ISRs use `IRAM_ATTR`** and timers use `ESP_TIMER_ISR` dispatch for deterministic sub-microsecond timing.
- **Zero-cross noise gate** — a configurable debounce window rejects electrical noise and false triggers on the zero-cross input. Default: 3000 us.
- **Two-pass ISR for multi-channel sync** — when multiple channels share one phase, the ISR pre-sorts them by delay and fires TRIAC pulses in a single consolidated pass, eliminating timing jitter between channels.
- **`Kconfig` build configuration** — the file is now named `Kconfig` (standard ESP-IDF convention; previously `Kconfig.txt`).

**New Kconfig parameters:**

| Parameter | Default | Description |
|---|---|---|
| `CONFIG_RBDIMMER_ZC_DEBOUNCE_US` | 3000 | Zero-cross debounce window in microseconds |
| `CONFIG_RBDIMMER_MIN_DELAY_US` | 100 | Minimum TRIAC firing delay in microseconds |
| `CONFIG_RBDIMMER_LEVEL_MIN` | 3 | Minimum dimming level (%). Values below this are treated as OFF |
| `CONFIG_RBDIMMER_LEVEL_MAX` | 99 | Maximum dimming level (%) |

## Installation

### Using CMake with ESP-IDF

1. Download the `rbdimmerESP32` library from GitHub repository:

```bash
git clone https://github.com/your-username/rbdimmerESP32 components/rbdimmer
```

1. Configure your project's `CMakeLists.txt` to include the library:

```cmake
# Main project CMakeLists.txt
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(your_project_name)
```

1. Add component dependency in your application's `CMakeLists.txt`:

```cmake
# App CMakeLists.txt
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES rbdimmer
)
```

1. The library's `CMakeLists.txt` and `Kconfig` are included automatically when placed in `components/rbdimmer/`. The component `CMakeLists.txt` registers sources, includes, and dependencies:

```cmake
# components/rbdimmer/CMakeLists.txt
idf_component_register(
    SRCS "rbdimmerESP32.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_timer freertos
)
```

:::info
In v2.0.0 the Kconfig file is named `Kconfig` (standard ESP-IDF convention). If you are upgrading from an earlier version that used `Kconfig.txt`, rename it to `Kconfig`.
:::

## Kconfig Configuration

The library exposes tuning parameters through the ESP-IDF menuconfig system. To adjust them:

```bash
idf.py menuconfig
# Navigate to: Component config → RBDimmer Configuration
```

Available options:

- **Zero-Cross Debounce (us)** — `CONFIG_RBDIMMER_ZC_DEBOUNCE_US` (default 3000). Increase if you see false zero-cross triggers from electrical noise.
- **Minimum TRIAC Delay (us)** — `CONFIG_RBDIMMER_MIN_DELAY_US` (default 100). Prevents the TRIAC from firing too close to the zero-cross, which can cause flicker at high brightness.
- **Level Min (%)** — `CONFIG_RBDIMMER_LEVEL_MIN` (default 3). Levels below this threshold are treated as OFF to avoid unstable TRIAC behavior.
- **Level Max (%)** — `CONFIG_RBDIMMER_LEVEL_MAX` (default 99). Caps the maximum firing angle.

## Hardware Connection

Instructions for connecting the dimmer to the microcontroller and AC load:

- Connect the **Zero-Cross Pin** to any GPIO that has ISR functionality. Check your ESP32 chip documentation
- Connect the **Dimmer Pin** to any GPIO
- **VCC** to 3.3V (for ESP32, VCC = 3.3V)
- **GND** to GND

:::info
For detailed hardware connection guides, please refer to: [Hardware Connection](https://www.rbdimmer.com/docs/hardware-connection)
:::

## Basic Example (ESP-IDF / C)

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rbdimmerESP32.h"

static const char *TAG = "DIMMER_EXAMPLE";

// Pins
#define ZERO_CROSS_PIN  18   // Zero-Cross pin
#define DIMMER_PIN      19   // Dimming control pin
#define PHASE_NUM       0    // Phase N (0 for single phase)

// Global variables. Dimmer object
rbdimmer_channel_t* dimmer_channel = NULL;

void app_main(void)
{
    ESP_LOGI(TAG, "AC Dimmer Test");

    // Dimmer lib init
    if (rbdimmer_init() != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to initialize AC Dimmer library");
        return;
    }

    // Zero-cross detector and phase registry
    if (rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0) != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to register zero-cross detector");
        return;
    }

    // Dimmer channel. Configuration data structure.
    rbdimmer_config_t config_channel = {
        .gpio_pin = DIMMER_PIN,
        .phase = PHASE_NUM,
        .initial_level = 50,  // Initial dimming level 50%
        .curve_type = RBDIMMER_CURVE_RMS  // Level Curve Selection. RMS-curve
    };

    if (rbdimmer_create_channel(&config_channel, &dimmer_channel) != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to create dimmer channel");
        return;
    }

    ESP_LOGI(TAG, "AC Dimmer initialized successfully");

    // Main loop
    while (1) {
        // dimming from 10% to 90% with step 10
        for (int brightness = 10; brightness <= 90; brightness += 10) {
            ESP_LOGI(TAG, "Setting brightness to %d%%", brightness);
            rbdimmer_set_level(dimmer_channel, brightness);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        // Smooth transition from current level to 0 level in 5 sec
        ESP_LOGI(TAG, "Smooth transition to 0%%");
        rbdimmer_set_level_transition(dimmer_channel, 0, 5000);
        vTaskDelay(6000 / portTICK_PERIOD_MS); // delay 6 sec

        // Smooth transition from current level (0) to 100 level in 5 sec
        ESP_LOGI(TAG, "Smooth transition to 100%%");
        rbdimmer_set_level_transition(dimmer_channel, 100, 5000);
        vTaskDelay(6000 / portTICK_PERIOD_MS); // delay 6 sec
    }
}
```

## API Reference

### Library Operation

**Preparation:**

1. Initialize the library using `rbdimmer_init()`
2. Register the zero-crossing detector using `rbdimmer_register_zero_cross()`
3. Create a dimmer channel using `rbdimmer_create_channel()`

**Dimming Control:**

- Set the dimming level with `rbdimmer_set_level()`. The dimming level is set in the range of 0(OFF) ~ 100(ON)
- Smooth dimming level transition with `rbdimmer_set_level_transition()`. Smooth transition from the current level to the set level over a period of time (in milliseconds, 1s=1000ms)

:::info
For a detailed explanation of how dimmers work, visit: [AC Dimmer Operating Principles](https://www.rbdimmer.com/blog/diy-insights-1/ac-dimmer-based-on-zero-cross-detector-and-triac-operating-principles-and-applications-5)
:::

### Data Structures

#### rbdimmer_config_t

```c
typedef struct {
    uint8_t gpio_pin;                 // Dimmer GPIO
    uint8_t phase;                    // Phase number
    uint8_t initial_level;            // Initial dimming level
    rbdimmer_curve_t curve_type;      // Level Curve type
} rbdimmer_config_t;
```

### Enumerations

#### rbdimmer_curve_t

Types of level curves:

```c
typedef enum {
    RBDIMMER_CURVE_LINEAR,      // Linear curve
    RBDIMMER_CURVE_RMS,         // RMS-compensated curve (for incandescent bulbs)
    RBDIMMER_CURVE_LOGARITHMIC  // Logarithmic curve (for dimmable LED)
} rbdimmer_curve_t;
```

#### rbdimmer_err_t

Library function responses:

```c
typedef enum {
    RBDIMMER_OK = 0,            // Successful execution
    RBDIMMER_ERR_INVALID_ARG,   // Invalid argument
    RBDIMMER_ERR_NO_MEMORY,     // Not enough memory
    RBDIMMER_ERR_NOT_FOUND,     // Object not found
    RBDIMMER_ERR_ALREADY_EXIST, // Object already exists
    RBDIMMER_ERR_TIMER_FAILED,  // Timer initialization error
    RBDIMMER_ERR_GPIO_FAILED    // GPIO initialization error
} rbdimmer_err_t;
```

### Constants and Macros

In v2.0.0, most tuning parameters have moved to Kconfig (see [Kconfig Configuration](#kconfig-configuration) above). The following constants remain in `rbdimmerESP32.h`:

```c
#define RBDIMMER_MAX_PHASES 4                 // Maximum number of phases
#define RBDIMMER_MAX_CHANNELS 8               // Maximum number of channels
#define RBDIMMER_DEFAULT_PULSE_WIDTH_US 50    // Pulse width (us)
```

The following are now configurable via `idf.py menuconfig`:

```c
// Kconfig defaults (override via menuconfig):
// CONFIG_RBDIMMER_ZC_DEBOUNCE_US  = 3000   // Zero-cross debounce (us)
// CONFIG_RBDIMMER_MIN_DELAY_US    = 100    // Minimum TRIAC delay (us)
// CONFIG_RBDIMMER_LEVEL_MIN       = 3      // Minimum level (%)
// CONFIG_RBDIMMER_LEVEL_MAX       = 99     // Maximum level (%)
```

:::warning
We do not recommend changing `RBDIMMER_DEFAULT_PULSE_WIDTH_US`, as this relates to the hardware characteristics of the dimmer.
:::

### Functions

#### Initialization and Setup

```c
// Initialize the library
rbdimmer_err_t rbdimmer_init(void);

// Register a zero-cross detector
rbdimmer_err_t rbdimmer_register_zero_cross(uint8_t pin, uint8_t phase, uint16_t frequency);

// Create a dimmer channel
rbdimmer_err_t rbdimmer_create_channel(rbdimmer_config_t* config, rbdimmer_channel_t** channel);

// Set callback function for zero-cross events
rbdimmer_err_t rbdimmer_set_callback(uint8_t phase, void (*callback)(void*), void* user_data);
```

#### Dimming Control

```c
// Set dimming level
rbdimmer_err_t rbdimmer_set_level(rbdimmer_channel_t* channel, uint8_t level_percent);

// Set brightness with smooth transition
rbdimmer_err_t rbdimmer_set_level_transition(rbdimmer_channel_t* channel, uint8_t level_percent, uint32_t transition_ms);

// Set brightness curve type
rbdimmer_err_t rbdimmer_set_curve(rbdimmer_channel_t* channel, rbdimmer_curve_t curve_type);

// Activate/deactivate channel
rbdimmer_err_t rbdimmer_set_active(rbdimmer_channel_t* channel, bool active);
```

#### Information Queries

```c
// Get current channel brightness
uint8_t rbdimmer_get_level(rbdimmer_channel_t* channel);

// Get measured mains frequency for the specified phase
uint16_t rbdimmer_get_frequency(uint8_t phase);

// Check if channel is active
bool rbdimmer_is_active(rbdimmer_channel_t* channel);

// Get channel curve type
rbdimmer_curve_t rbdimmer_get_curve(rbdimmer_channel_t* channel);

// Get current channel delay
uint32_t rbdimmer_get_delay(rbdimmer_channel_t* channel);
```

## Step-by-Step Guide

### Project Structure

```text
your_project/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   └── main.c
└── components/
    └── rbdimmer/
        ├── CMakeLists.txt
        ├── Kconfig
        ├── include/
        │   └── rbdimmer.h
        └── rbdimmerESP32.c
```

### Implementation Steps

1. Define library and pins in your `main.c` file:

```c
#include "rbdimmer.h"

// Pins
#define ZERO_CROSS_PIN  18   // Zero-Cross pin
#define DIMMER_PIN      19   // Dimming control pin
#define PHASE_NUM       0    // Phase N (0 for single phase)
```

1. Create dimmer object (one for each dimmer):

```c
rbdimmer_channel_t* dimmer_channel = NULL;
```

1. Initialize dimmer library:

```c
rbdimmer_init();
```

1. Register zero-cross detector and phase:

```c
rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);
```

1. Configure dimmer channel and create it:

```c
rbdimmer_config_t config_channel = {
    .gpio_pin = DIMMER_PIN,
    .phase = PHASE_NUM,
    .initial_level = 50,  // Initial dimming level 50%
    .curve_type = RBDIMMER_CURVE_RMS  // Level Curve Selection. RMS-curve
};

rbdimmer_create_channel(&config_channel, &dimmer_channel);
```

1. Control dimming:

```c
// Set specific level
rbdimmer_set_level(dimmer_channel, level);

// Smooth transition
rbdimmer_set_level_transition(dimmer_channel, 0, 5000);
```

:::tip
The smooth transition function creates a transition by breaking it into multiple small steps using a FreeRTOS task. During the transition, the main code continues to execute.
:::

## Advanced Examples

### Multi-Channel Dimmer Systems

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rbdimmer.h"

#define ZERO_CROSS_PIN  18
#define DIMMER_PIN_1    19
#define DIMMER_PIN_2    21
#define PHASE_NUM       0

static const char *TAG = "DIMMER_EXAMPLE";
rbdimmer_channel_t* channel1 = NULL;
rbdimmer_channel_t* channel2 = NULL;

void app_main(void)
{
    // Initialize library
    rbdimmer_init();

    // Register zero-cross detector (one per phase)
    rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);

    // Create first channel (incandescent bulbs)
    rbdimmer_config_t config1 = {
        .gpio_pin = DIMMER_PIN_1,
        .phase = PHASE_NUM,
        .initial_level = 50,
        .curve_type = RBDIMMER_CURVE_RMS
    };
    rbdimmer_create_channel(&config1, &channel1);

    // Create second channel (dimmable LED lighting)
    rbdimmer_config_t config2 = {
        .gpio_pin = DIMMER_PIN_2,
        .phase = PHASE_NUM,
        .initial_level = 50,
        .curve_type = RBDIMMER_CURVE_LOGARITHMIC
    };
    rbdimmer_create_channel(&config2, &channel2);

    // Main control loop
    while (1) {
        // Control channels independently
        rbdimmer_set_level(channel1, 75);
        rbdimmer_set_level(channel2, 25);
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        rbdimmer_set_level(channel1, 25);
        rbdimmer_set_level(channel2, 75);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
```

### Using Zero-Cross Interrupt Callback Functions

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rbdimmer.h"

#define ZERO_CROSS_PIN  18
#define DIMMER_PIN      19
#define LED_PIN         2  // Built-in LED for zero-cross visualization
#define PHASE_NUM       0

static const char *TAG = "DIMMER_CALLBACK";
rbdimmer_channel_t* dimmer = NULL;
QueueHandle_t zero_cross_queue;

// Simple message for our queue
typedef struct {
    uint32_t timestamp;
} ZeroCrossEvent_t;

// Callback function for zero-cross events
void zero_cross_callback(void* arg)
{
    ZeroCrossEvent_t event;
    event.timestamp = esp_timer_get_time() / 1000; // Current time in ms

    // Send to queue from ISR
    BaseType_t higher_priority_task_woken = pdFALSE;
    xQueueSendFromISR(zero_cross_queue, &event, &higher_priority_task_woken);

    if (higher_priority_task_woken) {
        portYIELD_FROM_ISR();
    }
}

// Task to process zero-cross events
void zero_cross_processing_task(void *pvParameters)
{
    ZeroCrossEvent_t event;

    while (1) {
        if (xQueueReceive(zero_cross_queue, &event, portMAX_DELAY)) {
            // Toggle LED to visualize zero-crossing
            gpio_set_level(LED_PIN, !gpio_get_level(LED_PIN));

            // Additional processing can be done here safely
            ESP_LOGI(TAG, "Zero-cross event at time: %lu ms", event.timestamp);
        }
    }
}

void app_main(void)
{
    // Setup LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    // Create the queue
    zero_cross_queue = xQueueCreate(10, sizeof(ZeroCrossEvent_t));
    if (zero_cross_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
        return;
    }

    // Create the task to process zero-cross events
    BaseType_t task_created = xTaskCreate(
        zero_cross_processing_task,
        "ZeroCrossTask",
        2048,
        NULL,
        5,
        NULL
    );

    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        return;
    }

    // Initialize dimmer
    rbdimmer_init();
    rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);

    // Register callback
    rbdimmer_set_callback(PHASE_NUM, zero_cross_callback, NULL);

    // Create dimmer channel
    rbdimmer_config_t config = {
        .gpio_pin = DIMMER_PIN,
        .phase = PHASE_NUM,
        .initial_level = 60,
        .curve_type = RBDIMMER_CURVE_RMS
    };

    rbdimmer_create_channel(&config, &dimmer);
    ESP_LOGI(TAG, "Dimmer with callback initialized");

    // Main loop - print frequency information
    while (1) {
        uint16_t frequency = rbdimmer_get_frequency(PHASE_NUM);
        ESP_LOGI(TAG, "Detected frequency: %u Hz", frequency);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
```

### Multi-Phase Systems

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rbdimmer.h"

#define ZERO_CROSS_PIN_PHASE_A  18
#define ZERO_CROSS_PIN_PHASE_B  19
#define ZERO_CROSS_PIN_PHASE_C  21

#define DIMMER_PIN_PHASE_A      22
#define DIMMER_PIN_PHASE_B      23
#define DIMMER_PIN_PHASE_C      25

#define PHASE_A  0
#define PHASE_B  1
#define PHASE_C  2

static const char *TAG = "DIMMER_MULTIPHASE";
rbdimmer_channel_t* channel_a = NULL;
rbdimmer_channel_t* channel_b = NULL;
rbdimmer_channel_t* channel_c = NULL;

void app_main(void)
{
    // Initialize library
    rbdimmer_init();

    // Register zero-cross detectors for each phase
    rbdimmer_register_zero_cross(ZERO_CROSS_PIN_PHASE_A, PHASE_A, 0);
    rbdimmer_register_zero_cross(ZERO_CROSS_PIN_PHASE_B, PHASE_B, 0);
    rbdimmer_register_zero_cross(ZERO_CROSS_PIN_PHASE_C, PHASE_C, 0);

    // Create channels for each phase
    rbdimmer_config_t config_a = {
        .gpio_pin = DIMMER_PIN_PHASE_A,
        .phase = PHASE_A,
        .initial_level = 50,
        .curve_type = RBDIMMER_CURVE_RMS
    };
    rbdimmer_create_channel(&config_a, &channel_a);

    rbdimmer_config_t config_b = {
        .gpio_pin = DIMMER_PIN_PHASE_B,
        .phase = PHASE_B,
        .initial_level = 50,
        .curve_type = RBDIMMER_CURVE_RMS
    };
    rbdimmer_create_channel(&config_b, &channel_b);

    rbdimmer_config_t config_c = {
        .gpio_pin = DIMMER_PIN_PHASE_C,
        .phase = PHASE_C,
        .initial_level = 50,
        .curve_type = RBDIMMER_CURVE_RMS
    };
    rbdimmer_create_channel(&config_c, &channel_c);

    ESP_LOGI(TAG, "Multi-phase dimmer system initialized");

    // Main control loop
    while (1) {
        // Control phases with different levels
        ESP_LOGI(TAG, "Setting phase A: 75%%, phase B: 50%%, phase C: 25%%");
        rbdimmer_set_level(channel_a, 75);
        rbdimmer_set_level(channel_b, 50);
        rbdimmer_set_level(channel_c, 25);
        vTaskDelay(3000 / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "Setting phase A: 25%%, phase B: 50%%, phase C: 75%%");
        rbdimmer_set_level(channel_a, 25);
        rbdimmer_set_level(channel_b, 50);
        rbdimmer_set_level(channel_c, 75);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
```

## Operation Monitoring and Debugging

```c
void print_dimmer_status(rbdimmer_channel_t* channel, uint8_t phase)
{
    ESP_LOGI(TAG, "=== Dimmer Status ===");
    ESP_LOGI(TAG, "Mains frequency: %d Hz", rbdimmer_get_frequency(phase));
    ESP_LOGI(TAG, "Brightness: %d%%", rbdimmer_get_level(channel));
    ESP_LOGI(TAG, "Active: %s", rbdimmer_is_active(channel) ? "Yes" : "No");
    ESP_LOGI(TAG, "Curve type: %d", rbdimmer_get_curve(channel));
    ESP_LOGI(TAG, "Delay: %d us", rbdimmer_get_delay(channel));
    ESP_LOGI(TAG, "====================");
}
```

## Troubleshooting

### General

- If the dimmer doesn't work correctly, check your hardware connections, especially the zero-cross detector
- Ensure that the zero-cross pin is connected to a GPIO that supports interrupts
- Use `ESP_LOG` functions to monitor the operation in real-time
- For multi-channel systems, ensure that each dimmer channel has a separate GPIO pin
- The library supports frequency auto-detection. If you know the mains frequency in your region (typically 50Hz or 60Hz), you can set it explicitly for better initial performance

### Flickering and stability issues (v2.0.0 fixes)

**Random flickering or false triggers:** The zero-cross noise gate (`CONFIG_RBDIMMER_ZC_DEBOUNCE_US`, default 3000 us) filters out electrical noise on the zero-cross line. If you still see random flicker, try increasing the debounce value via `idf.py menuconfig`.

**Flickering at 100% (full brightness):** The minimum TRIAC delay (`CONFIG_RBDIMMER_MIN_DELAY_US`, default 100 us) prevents the TRIAC from firing too close to the zero-cross edge. The v2.0.0 default of 100 us resolves the flicker that occurred with the previous 50 us default.

**Unstable behavior below 3%:** Levels below `CONFIG_RBDIMMER_LEVEL_MIN` (default 3%) are now treated as OFF. The TRIAC cannot reliably sustain conduction at very low firing angles, so the library clamps to off rather than producing erratic output.

**Multi-channel jitter:** When multiple channels share the same phase, v2.0.0 uses a two-pass ISR that pre-sorts channels by delay and fires them in sequence within a single interrupt. This eliminates the timing jitter that could occur when channels had similar delay values in earlier versions.

## Continuous Integration

The library is tested in CI against the following matrix:

- **ESP-IDF versions:** v5.3, v5.4, v5.5
- **Target chips:** ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6

This ensures that every commit builds cleanly across the full range of supported configurations.

## Changelog

For a complete list of changes, see [CHANGELOG.md](https://github.com/robotdyn-dimmer/rbdimmerESP32/blob/main/CHANGELOG.md).

