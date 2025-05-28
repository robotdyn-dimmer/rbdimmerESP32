/**
 * @file ZCCallBack.ino
 * @brief Advanced Zero-Cross Callback Example for ESP32 AC Dimmer
 * 
 * This advanced example demonstrates how to use zero-cross callback functions
 * with the rbdimmerESP32 library. It shows proper interrupt handling with
 * FreeRTOS, allowing you to synchronize other operations with AC mains
 * frequency while maintaining real-time dimmer control.
 * 
 * @section Concepts Key Concepts Demonstrated
 * - Registering callback functions for zero-cross events
 * - Proper ISR (Interrupt Service Routine) handling
 * - FreeRTOS queue communication from ISR to tasks
 * - Frequency measurement and monitoring
 * - Synchronized effects with AC mains
 * - Real-time event counting and timing
 * 
 * @section UseCase Real-World Use Cases
 * - Synchronizing multiple devices to mains frequency
 * - Creating flicker-free video recording environments
 * - Power quality monitoring and analysis
 * - Precise timing for industrial control
 * - Music visualization synchronized to mains
 * - Creating stroboscopic effects
 * 
 * @section Hardware Hardware Requirements
 * - ESP32 development board (any variant)
 * - RBDimmer AC dimmer module
 * - AC load (incandescent bulb recommended)
 * - LED on GPIO 2 (built-in) for visual zero-cross indication
 * - Proper isolation between AC mains and ESP32
 * 
 * @section Wiring Wiring Connections
 * - Zero-Cross Pin: GPIO 18 -> Dimmer ZC output
 * - Dimmer Pin: GPIO 19 -> Dimmer PWM input  
 * - Built-in LED: GPIO 2 (usually onboard)
 * - VCC: 3.3V (ESP32) -> Dimmer VCC
 * - GND: GND (ESP32) -> Dimmer GND
 * 
 * @section Expected Expected Behavior
 * 1. Dimmer maintains steady 60% brightness
 * 2. Built-in LED flashes at mains frequency (50/60 Hz)
 * 3. Serial output shows zero-cross statistics
 * 4. Demonstrates non-blocking callback operation
 * 5. Shows frequency stability monitoring
 * 
 * @section SerialOutput Serial Monitor Output Example
 * @code
 * === RBDimmer Zero-Cross Callback Example ===
 * Library initialized
 * Zero-cross detector registered
 * Callback registered successfully
 * Dimmer channel created at 60%
 * 
 * Zero-Cross Statistics (every second):
 * - Frequency: 50 Hz (measured)
 * - Zero-crosses: 100 (last second)
 * - Total crosses: 1523
 * - Period stability: ±0.1%
 * - Callback latency: <5μs
 * 
 * [Task] Processing zero-cross event #1523
 * [Task] Timestamp: 15234 ms
 * [Task] Period: 10.00 ms
 * @endcode
 * 
 * @section Theory Understanding Zero-Cross Detection
 * Zero-cross detection is fundamental to AC dimmer control. The AC mains
 * voltage crosses zero twice per cycle (once positive-to-negative, once
 * negative-to-positive). These points are critical because:
 * 
 * 1. TRIACs can only be safely triggered after zero-cross
 * 2. It provides a timing reference for phase control
 * 3. It allows synchronization with mains frequency
 * 4. It enables flicker-free operation
 * 
 * @section Safety ISR Safety and Best Practices
 * Interrupt Service Routines must be fast and non-blocking:
 * - No delays or blocking calls
 * - Minimal processing
 * - Use FreeRTOS queues for complex operations
 * - Keep variables volatile when shared with ISR
 * - Use IRAM_ATTR for ISR functions
 * 
 * @author dev@rbdimmer.com
 * @version 1.0.0
 * @date 2024
 * 
 * @see Dimmers website: https://rbdimmer.com
 * @see Library repository: https://github.com/robotdyn-dimmer/rbdimmerESP32
 * @see Dimmers catalog: https://www.rbdimmer.com/dimmers-pricing
 * @see Dimmers documentation: https://www.rbdimmer.com/knowledge/article/45
 * @see Library documentation: https://www.rbdimmer.com/knowledge/article/59
 * @see Dimmers projects: https://www.rbdimmer.com/blog/dimmers-projects-5
 * @see Support and community: https://www.rbdimmer.com/forum
 * 
 * @copyright Copyright (c) 2024 RBDimmer
 * @license MIT License
 */

#include <Arduino.h>
#include "rbdimmerESP32.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/** @name Hardware Configuration
 *  @brief Pin definitions and system constants
 *  @{
 */
#define ZERO_CROSS_PIN 18    ///< GPIO for zero-cross detector input
#define DIMMER_PIN 19        ///< GPIO for dimmer control output
#define LED_PIN 2            ///< Built-in LED for visual indication
#define PHASE_NUM 0          ///< AC phase number (0 for single-phase)
/** @} */

/** @name System Configuration
 *  @brief FreeRTOS and timing parameters
 *  @{
 */
#define QUEUE_SIZE 10        ///< Size of event queue
#define TASK_STACK_SIZE 4096 ///< Stack size for processing task
#define TASK_PRIORITY 5      ///< Priority of processing task
/** @} */

/**
 * @brief Structure for zero-cross event data
 * 
 * This structure contains all information about a zero-cross event
 * that needs to be passed from ISR to processing task
 */
typedef struct {
    uint32_t timestamp;      ///< Time of zero-cross in microseconds
    uint32_t count;          ///< Sequential event number
    uint32_t period;         ///< Period since last zero-cross
} ZeroCrossEvent_t;

/**
 * @brief Global variables for system state
 * @{
 */
rbdimmer_channel_t* dimmer = NULL;           ///< Dimmer channel handle
QueueHandle_t zeroCrossQueue = NULL;         ///< Queue for ISR->Task communication
TaskHandle_t processingTaskHandle = NULL;    ///< Handle for processing task

volatile uint32_t totalZeroCrosses = 0;      ///< Total zero-cross count
volatile uint32_t lastZeroCrossTime = 0;     ///< Last zero-cross timestamp
volatile uint32_t zeroCrossesPerSecond = 0;  ///< Current frequency counter

// Statistics for monitoring
volatile uint32_t minPeriod = UINT32_MAX;   ///< Minimum period observed
volatile uint32_t maxPeriod = 0;             ///< Maximum period observed
volatile uint32_t totalPeriod = 0;           ///< Sum of all periods
volatile uint32_t periodCount = 0;           ///< Number of periods measured
/** @} */

/**
 * @brief Zero-cross callback function (runs in ISR context)
 * 
 * This function is called by the rbdimmer library on every zero-cross
 * detection. It runs in interrupt context, so it must be fast and
 * non-blocking. Complex processing is deferred to a FreeRTOS task.
 * 
 * @param arg User-provided argument (unused in this example)
 * 
 * @warning This runs in ISR context - keep it minimal!
 */
void IRAM_ATTR zeroCrossCallback(void* arg) {
    uint32_t currentTime = micros();
    uint32_t period = 0;
    
    // Calculate period since last zero-cross
    if (lastZeroCrossTime > 0) {
        period = currentTime - lastZeroCrossTime;
        
        // Update statistics (safe atomic operations)
        if (period < minPeriod) minPeriod = period;
        if (period > maxPeriod) maxPeriod = period;
        totalPeriod += period;
        periodCount++;
    }
    
    lastZeroCrossTime = currentTime;
    totalZeroCrosses++;
    
    // Create event data
    ZeroCrossEvent_t event = {
        .timestamp = currentTime,
        .count = totalZeroCrosses,
        .period = period
    };
    
    // Send event to processing task via queue
    // Using FromISR version for interrupt safety
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(zeroCrossQueue, &event, &xHigherPriorityTaskWoken);
    
    // Request context switch if high priority task was woken
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
    
    // Quick visual indication (safe GPIO operation)
    static bool ledState = false;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
}

/**
 * @brief FreeRTOS task for processing zero-cross events
 * 
 * This task receives zero-cross events from the ISR via a queue
 * and performs complex processing that would be inappropriate
 * in interrupt context.
 * 
 * @param pvParameters Task parameters (unused)
 */
void zeroCrossProcessingTask(void* pvParameters) {
    ZeroCrossEvent_t event;
    uint32_t eventsThisSecond = 0;
    uint32_t lastSecondTime = millis();
    
    Serial.println("[Task] Zero-cross processing task started");
    
    // Task main loop
    for (;;) {
        // Wait for event from queue (block indefinitely)
        if (xQueueReceive(zeroCrossQueue, &event, portMAX_DELAY) == pdTRUE) {
            eventsThisSecond++;
            
            // Detailed processing can happen here safely
            // For example: logging, calculations, network updates
            
            // Every 100th event, print detailed info
            if (event.count % 100 == 0) {
                Serial.printf("[Task] Processing event #%lu\n", event.count);
                Serial.printf("[Task] Timestamp: %lu ms\n", event.timestamp / 1000);
                if (event.period > 0) {
                    Serial.printf("[Task] Period: %.2f ms\n", event.period / 1000.0);
                    float frequency = 1000000.0 / (event.period * 2); // Hz
                    Serial.printf("[Task] Instantaneous frequency: %.2f Hz\n", frequency);
                }
            }
            
            // Update per-second statistics
            uint32_t currentTime = millis();
            if (currentTime - lastSecondTime >= 1000) {
                zeroCrossesPerSecond = eventsThisSecond;
                eventsThisSecond = 0;
                lastSecondTime = currentTime;
            }
        }
    }
}

/**
 * @brief Print comprehensive system statistics
 * 
 * Displays detailed information about zero-cross detection,
 * frequency stability, and system performance
 */
void printStatistics() {
    Serial.println("\n========== Zero-Cross Statistics ==========");
    
    // Frequency information
    uint16_t measuredFreq = rbdimmer_get_frequency(PHASE_NUM);
    Serial.printf("Library measured frequency: %d Hz\n", measuredFreq);
    Serial.printf("Zero-crosses per second: %lu\n", zeroCrossesPerSecond);
    Serial.printf("Total zero-crosses: %lu\n", totalZeroCrosses);
    
    // Period statistics
    if (periodCount > 0) {
        float avgPeriod = (float)totalPeriod / periodCount;
        float avgFreq = 1000000.0 / (avgPeriod * 2);
        
        Serial.printf("\nPeriod Statistics:\n");
        Serial.printf("  Average: %.2f ms (%.2f Hz)\n", 
                     avgPeriod / 1000.0, avgFreq);
        Serial.printf("  Minimum: %.2f ms\n", minPeriod / 1000.0);
        Serial.printf("  Maximum: %.2f ms\n", maxPeriod / 1000.0);
        
        // Calculate stability (variation from average)
        float variation = ((maxPeriod - minPeriod) / avgPeriod) * 100;
        Serial.printf("  Stability: ±%.2f%%\n", variation / 2);
    }
    
    // Dimmer status
    Serial.printf("\nDimmer Status:\n");
    Serial.printf("  Brightness: %d%%\n", rbdimmer_get_level(dimmer));
    Serial.printf("  Active: %s\n", rbdimmer_is_active(dimmer) ? "Yes" : "No");
    
    // System health
    Serial.printf("\nSystem Health:\n");
    Serial.printf("  Queue spaces used: %d/%d\n", 
                 uxQueueMessagesWaiting(zeroCrossQueue), QUEUE_SIZE);
    Serial.printf("  Task stack high water mark: %d bytes\n",
                 uxTaskGetStackHighWaterMark(processingTaskHandle));
    Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
    
    Serial.println("==========================================\n");
}

/**
 * @brief Arduino setup function
 * 
 * Initializes all components of the zero-cross callback demonstration:
 * - LED for visual indication
 * - FreeRTOS queue and task
 * - Dimmer library and hardware
 * - Zero-cross callback registration
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        delay(10);
    }
    
    Serial.println("\n=== RBDimmer Zero-Cross Callback Example ===");
    
    // Initialize LED for visual indication
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED initialized for zero-cross visualization");
    
    // Create FreeRTOS queue for ISR to task communication
    zeroCrossQueue = xQueueCreate(QUEUE_SIZE, sizeof(ZeroCrossEvent_t));
    if (zeroCrossQueue == NULL) {
        Serial.println("ERROR: Failed to create event queue!");
        while (1) delay(1000);
    }
    Serial.println("FreeRTOS queue created");
    
    // Create processing task
    BaseType_t taskCreated = xTaskCreate(
        zeroCrossProcessingTask,      // Task function
        "ZeroCrossTask",              // Task name
        TASK_STACK_SIZE,              // Stack size in bytes
        NULL,                         // Parameters
        TASK_PRIORITY,                // Priority
        &processingTaskHandle         // Task handle
    );
    
    if (taskCreated != pdPASS) {
        Serial.println("ERROR: Failed to create processing task!");
        while (1) delay(1000);
    }
    Serial.println("Processing task created");
    
    // Initialize dimmer library
    if (rbdimmer_init() != RBDIMMER_OK) {
        Serial.println("ERROR: Failed to initialize dimmer library!");
        while (1) delay(1000);
    }
    Serial.println("Dimmer library initialized");
    
    // Register zero-cross detector
    if (rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0) != RBDIMMER_OK) {
        Serial.println("ERROR: Failed to register zero-cross detector!");
        while (1) delay(1000);
    }
    Serial.println("Zero-cross detector registered");
    
    // Register our callback function
    if (rbdimmer_set_callback(PHASE_NUM, zeroCrossCallback, NULL) != RBDIMMER_OK) {
        Serial.println("ERROR: Failed to register callback!");
        while (1) delay(1000);
    }
    Serial.println("Callback registered successfully");
    
    // Create dimmer channel
    rbdimmer_config_t config = {
        .gpio_pin = DIMMER_PIN,
        .phase = PHASE_NUM,
        .initial_level = 60,  // 60% brightness
        .curve_type = RBDIMMER_CURVE_RMS
    };
    
    if (rbdimmer_create_channel(&config, &dimmer) != RBDIMMER_OK) {
        Serial.println("ERROR: Failed to create dimmer channel!");
        while (1) delay(1000);
    }
    Serial.println("Dimmer channel created at 60% brightness");
    
    // Wait for frequency detection
    delay(500);
    
    Serial.println("\nSetup complete! Monitoring zero-cross events...");
    Serial.println("Watch the LED - it should flash at mains frequency");
    Serial.println("Statistics will be printed every 5 seconds\n");
}

/**
 * @brief Arduino main loop
 * 
 * Monitors system operation and prints statistics periodically.
 * Demonstrates that the main loop continues running normally
 * while callbacks and dimmer control happen in the background.
 */
void loop() {
    static unsigned long lastStatsTime = 0;
    static unsigned long lastDimmerChange = 0;
    
    // Print statistics every 5 seconds
    if (millis() - lastStatsTime >= 5000) {
        lastStatsTime = millis();
        printStatistics();
    }
    
    // Demonstrate that dimmer can be changed while callbacks run
    if (millis() - lastDimmerChange >= 10000) {
        lastDimmerChange = millis();
        
        // Cycle through different brightness levels
        static uint8_t brightness = 60;
        brightness = (brightness == 60) ? 30 : ((brightness == 30) ? 90 : 60);
        
        Serial.printf("Changing brightness to %d%%\n", brightness);
        rbdimmer_set_level_transition(dimmer, brightness, 2000);
    }
    
    // Monitor for any system issues
    if (uxQueueMessagesWaiting(zeroCrossQueue) > QUEUE_SIZE - 2) {
        Serial.println("WARNING: Queue nearly full - processing may be too slow!");
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}

/**
 * @example ZCCallBack.ino
 * 
 * This advanced example demonstrates professional use of zero-cross
 * callbacks with the rbdimmerESP32 library. Key learning points:
 * 
 * 1. **ISR Safety**: The callback runs in interrupt context, so all
 *    processing must be minimal and non-blocking
 * 
 * 2. **FreeRTOS Integration**: Shows proper pattern for deferring
 *    complex processing from ISR to task using queues
 * 
 * 3. **Frequency Monitoring**: Demonstrates how to measure and monitor
 *    mains frequency stability in real-time
 * 
 * 4. **Non-Interference**: The callback system doesn't interfere with
 *    normal dimmer operation - everything runs concurrently
 * 
 * 5. **Debugging Support**: Comprehensive statistics help diagnose
 *    system performance and timing issues
 * 
 * This pattern is essential for:
 * - Power quality monitoring systems
 * - Synchronized multi-device installations  
 * - Flicker-free environments for video
 * - Industrial control applications
 * - Advanced lighting effects
 * 
 * @note The LED flashing at mains frequency provides immediate visual
 *       confirmation that zero-cross detection is working correctly.
 */