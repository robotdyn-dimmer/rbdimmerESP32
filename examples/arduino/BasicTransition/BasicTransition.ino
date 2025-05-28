/**
 * @file BasicTransition.ino
 * @brief Smooth Brightness Transitions Example for ESP32 AC Dimmer
 * 
 * This example demonstrates how to create smooth brightness transitions using
 * the rbdimmerESP32 library. It showcases the built-in transition functionality
 * that uses FreeRTOS tasks to gradually change brightness levels over time,
 * creating pleasant fade effects for lighting applications.
 * 
 * @section Features Key Features Demonstrated
 * - Smooth fade-in and fade-out effects
 * - Concurrent operation (main loop continues during transitions)
 * - Different transition speeds
 * - Automatic handling of overlapping transitions
 * 
 * @section Hardware Hardware Requirements
 * - ESP32 development board (any variant)
 * - RBDimmer AC dimmer module
 * - AC load (incandescent bulb shows transitions best)
 * - Proper isolation between AC mains and ESP32
 * 
 * @section Wiring Wiring Connections
 * - Zero-Cross Pin: GPIO 18 -> Dimmer ZC output
 * - Dimmer Pin: GPIO 19 -> Dimmer PWM input
 * - VCC: 3.3V (ESP32) -> Dimmer VCC
 * - GND: GND (ESP32) -> Dimmer GND
 * 
 * @section Expected Expected Behavior
 * The connected light will continuously cycle through these transitions:
 * 1. Fade from 0% to 100% over 3 seconds
 * 2. Hold at 100% for 1 second
 * 3. Fade from 100% to 20% over 2 seconds
 * 4. Hold at 20% for 1 second
 * 5. Quick fade to 80% over 0.5 seconds
 * 6. Hold at 80% for 1 second
 * 7. Fade to 0% over 2 seconds
 * 8. Hold at 0% for 1 second
 * 9. Repeat cycle
 * 
 * @section SerialOutput Serial Monitor Output
 * @code
 * === RBDimmer Smooth Transitions Example ===
 * Library initialized successfully
 * Zero-cross detector registered
 * Dimmer channel created
 * Starting transition demonstration...
 * 
 * Starting: Fade up to 100% (3 seconds)
 * Transition started, main loop continues...
 * Current brightness: 15%
 * Current brightness: 33%
 * Current brightness: 67%
 * Current brightness: 89%
 * Transition complete! Now at 100%
 * 
 * Starting: Fade down to 20% (2 seconds)
 * Transition started, main loop continues...
 * ...
 * @endcode
 * 
 * @section Theory Understanding Smooth Transitions
 * The smooth transition feature works by:
 * 1. Creating a FreeRTOS task when transition is requested
 * 2. The task calculates intermediate brightness steps
 * 3. Updates are applied at regular intervals (typically 20-50ms)
 * 4. Main program continues executing during the transition
 * 5. Multiple transitions can be queued or override each other
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

/** @name Pin Configuration
 *  @brief Hardware pin assignments
 *  @{
 */
#define ZERO_CROSS_PIN 18  ///< GPIO for zero-cross detection input
#define DIMMER_PIN 19      ///< GPIO for dimmer control output
#define PHASE_NUM 0        ///< AC phase number (0 for single-phase)
/** @} */

/**
 * @brief Dimmer channel handle
 * 
 * Global pointer to the dimmer channel used throughout the example
 */
rbdimmer_channel_t* dimmer = NULL;

/**
 * @brief Transition sequence states
 * 
 * Enumeration to track which transition we're currently performing
 * in the demonstration cycle
 */
enum TransitionState {
    TRANS_FADE_UP,      ///< Fading from 0% to 100%
    TRANS_FADE_DOWN,    ///< Fading from 100% to 20%
    TRANS_FADE_QUICK,   ///< Quick fade to 80%
    TRANS_FADE_OFF      ///< Fading to 0%
};

/**
 * @brief Current state in the transition sequence
 */
TransitionState currentState = TRANS_FADE_UP;

/**
 * @brief Timestamp of when current transition started
 */
unsigned long transitionStartTime = 0;

/**
 * @brief Duration of current transition in milliseconds
 */
unsigned long transitionDuration = 0;

/**
 * @brief Flag to track if transition is in progress
 */
bool transitionInProgress = false;

/**
 * @brief Arduino setup function
 * 
 * Initializes the dimmer system:
 * - Serial communication for debugging
 * - RBDimmer library initialization
 * - Zero-cross detector setup
 * - Dimmer channel creation
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    
    // Wait for serial connection (useful for native USB boards)
    while (!Serial && millis() < 3000) {
        delay(10);
    }
    
    // Welcome message
    Serial.println("\n=== RBDimmer Smooth Transitions Example ===");
    
    // Initialize the dimmer library
    if (rbdimmer_init() != RBDIMMER_OK) {
        Serial.println("ERROR: Failed to initialize dimmer library!");
        while (1) delay(1000);
    }
    Serial.println("Library initialized successfully");
    
    // Register zero-cross detector with auto frequency detection
    if (rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0) != RBDIMMER_OK) {
        Serial.println("ERROR: Failed to register zero-cross detector!");
        while (1) delay(1000);
    }
    Serial.println("Zero-cross detector registered");
    
    // Configure dimmer channel
    rbdimmer_config_t config = {
        .gpio_pin = DIMMER_PIN,
        .phase = PHASE_NUM,
        .initial_level = 0,  // Start with light off
        .curve_type = RBDIMMER_CURVE_RMS  // Best for incandescent bulbs
    };
    
    // Create the dimmer channel
    if (rbdimmer_create_channel(&config, &dimmer) != RBDIMMER_OK) {
        Serial.println("ERROR: Failed to create dimmer channel!");
        while (1) delay(1000);
    }
    Serial.println("Dimmer channel created");
    
    // Small delay to allow frequency detection
    delay(200);
    
    // Print detected frequency
    uint16_t freq = rbdimmer_get_frequency(PHASE_NUM);
    if (freq > 0) {
        Serial.printf("Detected mains frequency: %d Hz\n", freq);
    }
    
    Serial.println("\nStarting transition demonstration...");
    Serial.println("Watch the connected light for smooth transitions!\n");
}

/**
 * @brief Start a new transition based on current state
 * 
 * This function initiates the appropriate transition based on
 * the current position in the demonstration sequence
 */
void startNextTransition() {
    rbdimmer_err_t err;
    
    switch (currentState) {
        case TRANS_FADE_UP:
            Serial.println("Starting: Fade up to 100% (3 seconds)");
            transitionDuration = 3000;
            err = rbdimmer_set_level_transition(dimmer, 100, transitionDuration);
            if (err == RBDIMMER_OK) {
                transitionInProgress = true;
                transitionStartTime = millis();
                currentState = TRANS_FADE_DOWN;
            }
            break;
            
        case TRANS_FADE_DOWN:
            Serial.println("Starting: Fade down to 20% (2 seconds)");
            transitionDuration = 2000;
            err = rbdimmer_set_level_transition(dimmer, 20, transitionDuration);
            if (err == RBDIMMER_OK) {
                transitionInProgress = true;
                transitionStartTime = millis();
                currentState = TRANS_FADE_QUICK;
            }
            break;
            
        case TRANS_FADE_QUICK:
            Serial.println("Starting: Quick fade to 80% (0.5 seconds)");
            transitionDuration = 500;
            err = rbdimmer_set_level_transition(dimmer, 80, transitionDuration);
            if (err == RBDIMMER_OK) {
                transitionInProgress = true;
                transitionStartTime = millis();
                currentState = TRANS_FADE_OFF;
            }
            break;
            
        case TRANS_FADE_OFF:
            Serial.println("Starting: Fade to off (2 seconds)");
            transitionDuration = 2000;
            err = rbdimmer_set_level_transition(dimmer, 0, transitionDuration);
            if (err == RBDIMMER_OK) {
                transitionInProgress = true;
                transitionStartTime = millis();
                currentState = TRANS_FADE_UP;
            }
            break;
    }
    
    if (transitionInProgress) {
        Serial.println("Transition started, main loop continues...");
    }
}

/**
 * @brief Arduino main loop
 * 
 * Manages the transition sequence and provides real-time feedback
 * about the dimmer status. This demonstrates that the main program
 * continues running while transitions happen in the background.
 */
void loop() {
    static unsigned long lastStatusPrint = 0;
    static uint8_t lastPrintedLevel = 255;
    unsigned long currentTime = millis();
    
    // Check if we need to start a new transition
    if (!transitionInProgress) {
        // Wait 1 second between transitions
        static unsigned long waitStartTime = 0;
        if (waitStartTime == 0) {
            waitStartTime = currentTime;
        }
        
        if (currentTime - waitStartTime >= 1000) {
            waitStartTime = 0;
            startNextTransition();
        }
    } else {
        // Check if current transition should be complete
        if (currentTime - transitionStartTime >= transitionDuration + 100) {
            transitionInProgress = false;
            uint8_t level = rbdimmer_get_level(dimmer);
            Serial.printf("Transition complete! Now at %d%%\n\n", level);
        }
    }
    
    // Print current brightness during transitions (every 500ms)
    if (transitionInProgress && currentTime - lastStatusPrint >= 500) {
        lastStatusPrint = currentTime;
        uint8_t currentLevel = rbdimmer_get_level(dimmer);
        
        // Only print if level has changed significantly
        if (abs(currentLevel - lastPrintedLevel) >= 5) {
            Serial.printf("Current brightness: %d%%\n", currentLevel);
            lastPrintedLevel = currentLevel;
        }
    }
    
    // Demonstrate that main loop continues running
    static unsigned long lastHeartbeat = 0;
    if (currentTime - lastHeartbeat >= 10000) {
        lastHeartbeat = currentTime;
        Serial.println("[Main loop active - program continues during transitions]");
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}

/**
 * @example BasicTransition.ino
 * 
 * This example showcases the smooth transition capabilities of the
 * rbdimmerESP32 library. Key concepts demonstrated:
 * 
 * 1. **Non-blocking transitions**: The main program continues running
 *    while brightness changes happen in the background
 * 
 * 2. **Variable speeds**: Different transition durations from 0.5 to 3 seconds
 * 
 * 3. **FreeRTOS integration**: Transitions use FreeRTOS tasks for
 *    smooth, concurrent operation
 * 
 * 4. **Automatic handling**: The library manages all timing and step
 *    calculations internally
 * 
 * This functionality is perfect for:
 * - Mood lighting applications
 * - Theater and stage lighting
 * - Sunrise/sunset simulation
 * - Energy-saving gradual dimming
 * - User-friendly lighting interfaces
 * 
 * @note The smoothness of transitions depends on:
 *       - The transition duration (longer = smoother)
 *       - The load type (incandescent bulbs show smoothest transitions)
 *       - The curve type (RMS curve provides linear power change)
 */