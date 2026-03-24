
# Arduino Guide & Examples
> [!NOTE]
> **Library GitHub:** [https://github.com/robotdyn-dimmer/rbdimmerESP32](https://github.com/robotdyn-dimmer/rbdimmerESP32)


> [!NOTE]
> This guide is updated for **rbdimmerESP32 v2.0.0** (March 2026). Public API is unchanged -- existing code works without modification.

Before start, read library overview: [Universal library for ESP32](https://www.rbdimmer.com/docs/universal-library-for-esp32)

> [!NOTE]
> **Download library from GitHub:** [https://github.com/robotdyn-dimmer/rbdimmerESP32](https://github.com/robotdyn-dimmer/rbdimmerESP32)

## New in v2.0.0

- **Modular internal architecture** -- the library internals have been reorganized into separate modules. The public API is unchanged; existing sketches compile and run without modification.
- **Zero-cross noise gate** -- eliminates false TRIAC spike re-triggers that previously caused flicker on some loads.
- **Two-pass ISR for multi-channel synchronization** -- all channels on the same phase are now sorted and fired in a single half-cycle, removing inter-channel timing jitter.
- **IRAM_ATTR on all timing-critical paths** -- ISR handlers and related functions are placed in IRAM to avoid flash-cache misses during time-sensitive operations.
- **Four new Kconfig parameters** -- configurable via `idf.py menuconfig` for ESP-IDF projects. Arduino builds use compile-time defaults (no action needed).

## Requirements

- **Arduino ESP32 Core 3.x** (tested with 3.0+)
- Arduino IDE 2.x
- Examples are restructured to per-sketch directories for Arduino IDE 2.x compatibility

## Library Installation

### Arduino IDE

1. Download the `rbdimmerESP32` library as a ZIP archive
2. In Arduino IDE, select: **Sketch > Include Library > Add .ZIP Library**
3. Select the downloaded ZIP file
4. Restart Arduino IDE to complete the installation

### Library Operation Explanation

**Preparation:**

1. The library is initialized using `rbdimmer_init()`
2. The zero-crossing detector is registered using `rbdimmer_register_zero_cross()`
3. The dimmer channel is created using `rbdimmer_create_channel()`

**Dimming Control:**

- Setting the dimming level with `rbdimmer_set_level()`. The dimming level is set in the range of 0(OFF) ~ 100(ON)
- Smooth dimming level transition with `rbdimmer_set_level_transition()`. Smooth transition from the current level to the set level over a period of time (in milliseconds, 1s=1000ms)

> [!NOTE]
> How a dimmer works - article in our blog: [AC Dimmer Operating Principles](https://www.rbdimmer.com/blog/diy-insights-1/ac-dimmer-based-on-zero-cross-detector-and-triac-operating-principles-and-applications-5)

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

Constants in the `rbdimmerESP32.h` file. You can modify these parameters:

```c
#define RBDIMMER_MAX_PHASES 4                 // Maximum number of phases
#define RBDIMMER_MAX_CHANNELS 8               // Maximum number of channels
#define RBDIMMER_DEFAULT_PULSE_WIDTH_US 50    // Pulse width (us)
#define RBDIMMER_MIN_DELAY_US 100             // Minimum delay (us) — raised from 50 in v2.0.0
```

> [!IMPORTANT]
> We do not recommend changing `RBDIMMER_DEFAULT_PULSE_WIDTH_US`, as this relates to the hardware characteristics of the dimmer.

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

## Step-by-Step Guideline

Implementation steps for initial library, registry phase, zero-cross detector, dimming channel and dimming control:

### 1. Define library and pins

```cpp
#include "rbdimmerESP32.h"

// Pins
#define ZERO_CROSS_PIN  18   // Zero-Cross pin
#define DIMMER_PIN      19   // Dimming control pin
#define PHASE_NUM       0    // Phase N (0 for single phase)
```

### 2. Create dimmer object

For each dimmer, create an object:

```cpp
rbdimmer_channel_t* dimmer_channel = NULL;
```

### 3. Dimmer library initialization

The function returns a status:

```cpp
rbdimmer_init();
```

### 4. Zero-cross detector and phase registry

The function returns a status:

```cpp
rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);
```

### 5. Dimmer channel configuration and creation

```cpp
rbdimmer_config_t config_channel = {
    .gpio_pin = DIMMER_PIN,
    .phase = PHASE_NUM,
    .initial_level = 50,  // Initial dimming level 50%
    .curve_type = RBDIMMER_CURVE_RMS  // Level Curve Selection. RMS-curve
};

rbdimmer_create_channel(&config_channel, &dimmer_channel);
```

### 6. Dimming operation

```cpp
rbdimmer_set_level(dimmer_channel, level);
```

### 7. Dimming smooth transitions

```cpp
rbdimmer_set_level_transition(dimmer_channel, 0, 5000);
```

> [!TIP]
> The function creates a smooth transition by breaking it into multiple small steps. The function uses a FreeRTOS task; during the transition, the main code continues to execute.

## Solutions

### Multi-Channel Dimmer Systems

The library supports multiple independent dimming channels. The number of channels is limited in the library settings in the `rbdimmerESP32.h` file. Each dimming channel must have a separate output pin.

> [!NOTE]
> In v2.0.0, multi-channel setups benefit from the new **two-pass ISR** that sorts and fires all channels on the same phase within a single half-cycle, eliminating inter-channel timing jitter.

Example of creating a two-channel system:

```cpp
#define ZERO_CROSS_PIN  18
#define DIMMER_PIN_1    19
#define DIMMER_PIN_2    21
#define PHASE_NUM       0

rbdimmer_channel_t* channel1 = NULL;
rbdimmer_channel_t* channel2 = NULL;

void setup() {
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
}

void loop() {
  // Control channels independently
  rbdimmer_set_level(channel1, 75);
  rbdimmer_set_level(channel2, 25);
  delay(2000);

  rbdimmer_set_level(channel1, 25);
  rbdimmer_set_level(channel2, 75);
  delay(2000);
}
```

### Using Interrupt Callback Functions

Callback functions allow you to synchronize your code with zero-crossing events. This is useful for tasks requiring precise synchronization with the AC network.

Example of registration and FreeRTOS task handler:

```cpp
// Callback function for zero-cross events
void zero_cross_callback(void* arg) {
  // process zero-cross events
  digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Flashing with ZC frequency

  // Any code
}

void setup() {
  // ... Lib inits ...

  // callback-function registry
  rbdimmer_set_callback(PHASE_NUM, zero_cross_callback, NULL);

  // ... any code ...
}
```

> [!IMPORTANT]
> Do not use heavy code in the callback function. We recommend using FreeRTOS task calls.

### Multi-Phase Systems

For three-phase systems, a separate zero-crossing detector must be registered for each phase:

```cpp
#define ZERO_CROSS_PIN_PHASE_A  18
#define ZERO_CROSS_PIN_PHASE_B  19
#define ZERO_CROSS_PIN_PHASE_C  21

#define DIMMER_PIN_PHASE_A      22
#define DIMMER_PIN_PHASE_B      23
#define DIMMER_PIN_PHASE_C      25

#define PHASE_A  0
#define PHASE_B  1
#define PHASE_C  2

void setup() {
  // Lib init
  rbdimmer_init();

  // ZC detect registry for each phase
  rbdimmer_register_zero_cross(ZERO_CROSS_PIN_PHASE_A, PHASE_A, 0);
  rbdimmer_register_zero_cross(ZERO_CROSS_PIN_PHASE_B, PHASE_B, 0);
  rbdimmer_register_zero_cross(ZERO_CROSS_PIN_PHASE_C, PHASE_C, 0);

  // Create dimming channels for each phase
  // ... dimming code ...
}
```

### Operation Monitoring

For debugging, you can use the built-in library functions:

```cpp
void printDimmerStatus(rbdimmer_channel_t* channel) {
  Serial.println("=== Dimmer Status ===");
  Serial.printf("Mains frequency: %d Hz\n", rbdimmer_get_frequency(0));
  Serial.printf("Brightness: %d%%\n", rbdimmer_get_level(channel));
  Serial.printf("Active: %s\n", rbdimmer_is_active(channel) ? "Yes" : "No");
  Serial.printf("Curve type: %d\n", rbdimmer_get_curve(channel));
  Serial.printf("Delay: %d us\n", rbdimmer_get_delay(channel));
  Serial.println("====================");
}
```

## Troubleshooting: Flickering

v2.0.0 includes several targeted fixes for common flickering issues:

| Symptom | Cause | v2.0.0 fix |
|---------|-------|------------|
| General flickering / random spikes | TRIAC turn-on spike re-triggers the zero-cross detector | **Zero-cross noise gate**: ZC_DEBOUNCE_US = 3000 us blanking window after each zero-cross, ignoring false edges |
| Flickering at 100% brightness | Firing delay too close to the next zero-cross | **MIN_DELAY_US raised to 100 us**; levels >= 100% are mapped to 99% internally so the TRIAC always receives a proper gate pulse |
| Flickering below 3% brightness | TRIAC cannot sustain current at very small conduction angles | Levels below **LEVEL_MIN** are treated as fully OFF, avoiding unreliable partial firing |
| Multi-channel jitter / flicker | Channels fired with independent timers causing scheduling conflicts | **Two-pass ISR**: all channels on the same phase are sorted by delay and fired sequentially in one timer chain within each half-cycle |

If you still experience flickering after upgrading, check that your zero-cross hardware signal is clean (no ringing) and that your load is compatible with leading-edge (TRIAC) phase-cut dimming.

## Basic Examples for AC Dimmer Library

Example sketches are located in per-sketch directories under `examples/arduino/`:

| Example | Path |
|---------|------|
| Basic Dimmer Control | `examples/arduino/BasicDimmer/BasicDimmer.ino` |
| Brightness Transition | `examples/arduino/BasicTransition/BasicTransition.ino` |
| Multiple Dimmer Channels | `examples/arduino/MultiDimmer/MultiDimmer.ino` |
| Zero-Cross Callback | `examples/arduino/ZCCallBack/ZCCallBack.ino` |

### Basic Dimmer Control

Description: The simplest example showing how to control an AC dimmer with a fixed brightness level.

File: `examples/arduino/BasicDimmer/BasicDimmer.ino`

```cpp
#include <Arduino.h>
#include "rbdimmerESP32.h"

#define ZERO_CROSS_PIN 18
#define DIMMER_PIN 19
#define PHASE_NUM 0

rbdimmer_channel_t* dimmer = NULL;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize the library
  rbdimmer_init();

  // Register zero-cross detector
  rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);

  // Create dimmer channel with RMS curve (best for incandescent bulbs)
  rbdimmer_config_t config = {
    .gpio_pin = DIMMER_PIN,
    .phase = PHASE_NUM,
    .initial_level = 50,  // Start at 50% brightness
    .curve_type = RBDIMMER_CURVE_RMS
  };

  rbdimmer_create_channel(&config, &dimmer);
  Serial.println("Dimmer initialized at 50% brightness");
}

void loop() {
  // Nothing needed in the loop - dimmer maintains its state
  delay(1000);
}
```

### Brightness Transition

Shows how to create smooth transitions between brightness levels.

File: `examples/arduino/BasicTransition/BasicTransition.ino`

```cpp
#include <Arduino.h>
#include "rbdimmerESP32.h"

#define ZERO_CROSS_PIN 18
#define DIMMER_PIN 19
#define PHASE_NUM 0

rbdimmer_channel_t* dimmer = NULL;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize dimmer
  rbdimmer_init();
  rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);

  rbdimmer_config_t config = {
    .gpio_pin = DIMMER_PIN,
    .phase = PHASE_NUM,
    .initial_level = 0,  // Start with light off
    .curve_type = RBDIMMER_CURVE_RMS
  };

  rbdimmer_create_channel(&config, &dimmer);
  Serial.println("Dimmer initialized");
}

void loop() {
  // Fade up to 100% over 3 seconds
  Serial.println("Fading up...");
  rbdimmer_set_level_transition(dimmer, 100, 3000);
  delay(4000);  // Wait for transition + 1 second

  // Fade down to 10% over 3 seconds
  Serial.println("Fading down...");
  rbdimmer_set_level_transition(dimmer, 10, 3000);
  delay(4000);  // Wait for transition + 1 second
}
```

### Multiple Dimmer Channels

Controls two separate dimmer channels independently.

File: `examples/arduino/MultiDimmer/MultiDimmer.ino`

```cpp
#include <Arduino.h>
#include "rbdimmerESP32.h"

#define ZERO_CROSS_PIN 18
#define DIMMER_PIN_1 19
#define DIMMER_PIN_2 21
#define PHASE_NUM 0

rbdimmer_channel_t* dimmer1 = NULL;
rbdimmer_channel_t* dimmer2 = NULL;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize dimmer library
  rbdimmer_init();
  rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);

  // Create first dimmer channel (for incandescent bulb)
  rbdimmer_config_t config1 = {
    .gpio_pin = DIMMER_PIN_1,
    .phase = PHASE_NUM,
    .initial_level = 50,
    .curve_type = RBDIMMER_CURVE_RMS
  };
  rbdimmer_create_channel(&config1, &dimmer1);

  // Create second dimmer channel (for LED light)
  rbdimmer_config_t config2 = {
    .gpio_pin = DIMMER_PIN_2,
    .phase = PHASE_NUM,
    .initial_level = 50,
    .curve_type = RBDIMMER_CURVE_LOGARITHMIC  // Better for LEDs
  };
  rbdimmer_create_channel(&config2, &dimmer2);

  Serial.println("Two dimmer channels initialized");
}

void loop() {
  // Alternate level between channels
  Serial.println("Channel 1: 80%, Channel 2: 20%");
  rbdimmer_set_level(dimmer1, 80);
  rbdimmer_set_level(dimmer2, 20);
  delay(3000);

  Serial.println("Channel 1: 20%, Channel 2: 80%");
  rbdimmer_set_level(dimmer1, 20);
  rbdimmer_set_level(dimmer2, 80);
  delay(3000);
}
```

### Zero-Cross Callback

Demonstrates using a callback function with a FreeRTOS task to safely process zero-crossing events, which can be used for synchronizing with the AC waveform without adding code execution delay to the interrupt handler.

File: `examples/arduino/ZCCallBack/ZCCallBack.ino`

```cpp
#include <Arduino.h>
#include "rbdimmerESP32.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define ZERO_CROSS_PIN 18
#define DIMMER_PIN 19
#define LED_PIN 2  // Onboard LED for zero-cross visualization
#define PHASE_NUM 0

rbdimmer_channel_t* dimmer = NULL;
uint32_t zeroCount = 0;

// FreeRTOS components
QueueHandle_t zeroCrossQueue;
TaskHandle_t zeroCrossTaskHandle;

// Simple message type for our queue
typedef struct {
  uint32_t timestamp;
} ZeroCrossEvent_t;

// Callback function for zero-cross events (runs in ISR context)
void zeroCrossCallback(void* arg) {
  // Create event
  ZeroCrossEvent_t event;
  event.timestamp = millis();

  // Send to queue from ISR
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(zeroCrossQueue, &event, &xHigherPriorityTaskWoken);

  // If a higher priority task was woken, request context switch
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

// Task to process zero-cross events
void zeroCrossProcessingTask(void* parameter) {
  ZeroCrossEvent_t event;

  // Task loop - will run forever
  for (;;) {
    // Wait for an item from the queue
    if (xQueueReceive(zeroCrossQueue, &event, portMAX_DELAY)) {
      // Process the event (now we're in task context, not ISR)

      // Toggle LED to visualize zero-crossing
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));

      // Count zero-crossing events
      zeroCount++;

      // Additional processing can be done here safely
      // This doesn't affect the zero-cross interrupt timing
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Setup LED for visual zero-cross indication
  pinMode(LED_PIN, OUTPUT);

  // Create the queue to send events from ISR to task
  zeroCrossQueue = xQueueCreate(10, sizeof(ZeroCrossEvent_t));
  if (zeroCrossQueue == NULL) {
    Serial.println("Error creating the queue");
    while (1); // Stop execution on error
  }

  // Create the task to process zero-cross events
  BaseType_t xReturned = xTaskCreate(
    zeroCrossProcessingTask,  // Task function
    "ZeroCrossTask",          // Task name
    2048,                     // Stack size (bytes)
    NULL,                     // No parameters needed
    5,                        // Medium priority
    &zeroCrossTaskHandle      // Task handle
  );

  if (xReturned != pdPASS) {
    Serial.println("Error creating the task");
    while (1); // Stop execution on error
  }

  // Initialize dimmer
  rbdimmer_init();
  rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);

  // Register zero-cross callback
  rbdimmer_set_callback(PHASE_NUM, zeroCrossCallback, NULL);

  // Create dimmer channel
  rbdimmer_config_t config = {
    .gpio_pin = DIMMER_PIN,
    .phase = PHASE_NUM,
    .initial_level = 60,
    .curve_type = RBDIMMER_CURVE_RMS
  };

  rbdimmer_create_channel(&config, &dimmer);
  Serial.println("Dimmer with zero-cross callback and task processing initialized");
}

void loop() {
  // Print zero-cross statistics every second
  static unsigned long lastPrint = 0;

  if (millis() - lastPrint >= 1000) {
    uint16_t frequency = rbdimmer_get_frequency(PHASE_NUM);

    Serial.printf("Zero-cross count: %u, Detected frequency: %u Hz\n",
                  zeroCount, frequency);

    lastPrint = millis();
  }

  delay(10);
}
```

> [!TIP]
> This implementation significantly improves the original example by:
>
> - Keeping the ISR (Interrupt Service Routine) extremely short - it only sends a message to a queue
> - Moving all processing logic to a dedicated FreeRTOS task
> - Using proper FreeRTOS mechanisms for safe communication between ISR and task
> - Preventing any timing issues in the zero-crossing detection by separating the interrupt handling from the processing
>
> This approach follows best practices for real-time systems where interrupt handlers should be as short as possible to avoid affecting system timing and responsiveness.

---

See [CHANGELOG.md](https://github.com/robotdyn-dimmer/rbdimmerESP32/blob/main/CHANGELOG.md) for the full list of changes in each release.

