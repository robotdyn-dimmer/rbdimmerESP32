/**
 * @file BasicDimmer.ino
 * @brief Basic AC Dimmer Control Example for ESP32
 * 
 * This example demonstrates the fundamental usage of the rbdimmerESP32 library
 * for controlling a single AC dimmer channel. It shows how to initialize the
 * library, register a zero-cross detector, create a dimmer channel, and set
 * a fixed brightness level.
 * 
 * @section Hardware Hardware Requirements
 * - ESP32 development board (any variant)
 * - RBDimmer AC dimmer module
 * - AC load (incandescent bulb recommended for testing)
 * - Proper isolation between AC mains and ESP32
 * 
 * @section Wiring Wiring Connections
 * - Zero-Cross Pin: GPIO 18 -> Dimmer ZC output
 * - Dimmer Pin: GPIO 19 -> Dimmer PWM input
 * - VCC: 3.3V (ESP32) -> Dimmer VCC
 * - GND: GND (ESP32) -> Dimmer GND
 * 
 * @section Expected Expected Output
 * When running this example, you should observe:
 * 1. Serial output showing initialization status
 * 2. The connected AC load operating at 50% brightness
 * 3. Stable, flicker-free dimming
 * 4. Serial messages confirming successful operation
 * 
 * Serial Monitor Output:
 * @code
 * === RBDimmer Basic Example ===
 * Initializing RBDimmer library...
 * Library initialized successfully
 * Registering zero-cross detector on pin 18...
 * Zero-cross detector registered
 * Creating dimmer channel...
 * Dimmer channel created successfully
 * Setting brightness to 50%...
 * Dimmer is now running at 50% brightness
 * Setup complete! Dimmer is active.
 * @endcode
 * 
 * @section Theory Theory of Operation
 * AC dimming works by controlling when the TRIAC turns on during each
 * half-cycle of the AC waveform. By delaying the turn-on point, we
 * control the amount of power delivered to the load:
 * - 0% brightness = TRIAC never turns on
 * - 50% brightness = TRIAC turns on halfway through each half-cycle
 * - 100% brightness = TRIAC turns on immediately after zero-cross
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

/** @name Pin Definitions
 *  @brief GPIO pins used for dimmer control
 *  @{
 */
#define ZERO_CROSS_PIN 18  ///< GPIO pin connected to zero-cross detector output
#define DIMMER_PIN 19      ///< GPIO pin connected to dimmer control input
#define PHASE_NUM 0        ///< Phase number (0 for single-phase systems)
/** @} */

/** @name Configuration Constants
 *  @brief Dimmer configuration parameters
 *  @{
 */
#define INITIAL_BRIGHTNESS 50  ///< Initial brightness level in percent (0-100)
#define MAINS_FREQUENCY 0      ///< Mains frequency: 0=auto-detect, 50=50Hz, 60=60Hz
/** @} */

/**
 * @brief Global dimmer channel handle
 * 
 * This pointer holds the reference to our dimmer channel after creation.
 * It's used to control the dimmer throughout the program lifetime.
 */
rbdimmer_channel_t* dimmer = NULL;

/**
 * @brief Arduino setup function
 * 
 * This function runs once at startup and initializes all components:
 * 1. Serial communication for debugging
 * 2. RBDimmer library initialization
 * 3. Zero-cross detector registration
 * 4. Dimmer channel creation and configuration
 * 5. Setting initial brightness level
 */
void setup() {
    // Initialize serial communication for debug output
    Serial.begin(115200);
    
    // Wait for serial port to connect (needed for native USB boards)
    while (!Serial && millis() < 3000) {
        ; // Wait up to 3 seconds for serial connection
    }
    
    // Print welcome message
    Serial.println("\n=== RBDimmer Basic Example ===");
    Serial.println("Initializing RBDimmer library...");
    
    // Step 1: Initialize the RBDimmer library
    rbdimmer_err_t err = rbdimmer_init();
    if (err != RBDIMMER_OK) {
        Serial.printf("ERROR: Failed to initialize library (error code: %d)\n", err);
        Serial.println("Check your ESP32 board and ensure enough memory is available");
        while (1) {
            delay(1000); // Halt execution on error
        }
    }
    Serial.println("Library initialized successfully");
    
    // Step 2: Register zero-cross detector
    Serial.printf("Registering zero-cross detector on pin %d...\n", ZERO_CROSS_PIN);
    err = rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, MAINS_FREQUENCY);
    if (err != RBDIMMER_OK) {
        Serial.printf("ERROR: Failed to register zero-cross detector (error code: %d)\n", err);
        Serial.println("Check your wiring and ensure the pin supports interrupts");
        while (1) {
            delay(1000); // Halt execution on error
        }
    }
    Serial.println("Zero-cross detector registered");
    
    // Step 3: Configure and create dimmer channel
    Serial.println("Creating dimmer channel...");
    
    /**
     * @brief Dimmer channel configuration
     * 
     * This structure defines all parameters for the dimmer channel:
     * - gpio_pin: Output pin that controls the TRIAC
     * - phase: Which AC phase this dimmer is connected to
     * - initial_level: Starting brightness (we'll override this)
     * - curve_type: RMS curve is best for incandescent bulbs
     */
    rbdimmer_config_t config = {
        .gpio_pin = DIMMER_PIN,
        .phase = PHASE_NUM,
        .initial_level = 0,  // Start with light off
        .curve_type = RBDIMMER_CURVE_RMS  // RMS curve for incandescent bulbs
    };
    
    err = rbdimmer_create_channel(&config, &dimmer);
    if (err != RBDIMMER_OK) {
        Serial.printf("ERROR: Failed to create dimmer channel (error code: %d)\n", err);
        Serial.println("Check your pin configuration and available timers");
        while (1) {
            delay(1000); // Halt execution on error
        }
    }
    Serial.println("Dimmer channel created successfully");
    
    // Step 4: Set brightness level
    Serial.printf("Setting brightness to %d%%...\n", INITIAL_BRIGHTNESS);
    err = rbdimmer_set_level(dimmer, INITIAL_BRIGHTNESS);
    if (err != RBDIMMER_OK) {
        Serial.printf("WARNING: Failed to set brightness (error code: %d)\n", err);
    } else {
        Serial.printf("Dimmer is now running at %d%% brightness\n", INITIAL_BRIGHTNESS);
    }
    
    // Print status information
    Serial.println("\nSetup complete! Dimmer is active.");
    Serial.println("The connected load should now be at 50% brightness");
    
    // Wait a moment for frequency detection
    delay(500);
    
    // Print detected frequency
    uint16_t frequency = rbdimmer_get_frequency(PHASE_NUM);
    if (frequency > 0) {
        Serial.printf("Detected mains frequency: %d Hz\n", frequency);
    } else {
        Serial.println("Frequency detection in progress...");
    }
}

/**
 * @brief Arduino main loop function
 * 
 * In this basic example, the loop function simply maintains the set
 * brightness level. The dimmer operates autonomously using hardware
 * timers and interrupts, so no continuous updates are needed.
 * 
 * The loop prints status information every 5 seconds to show
 * that the system is running.
 */
void loop() {
    // Static variable to track last print time
    static unsigned long lastPrintTime = 0;
    
    // Print status every 5 seconds
    if (millis() - lastPrintTime >= 5000) {
        lastPrintTime = millis();
        
        // Get current dimmer status
        uint8_t currentLevel = rbdimmer_get_level(dimmer);
        uint16_t frequency = rbdimmer_get_frequency(PHASE_NUM);
        bool isActive = rbdimmer_is_active(dimmer);
        
        // Print status information
        Serial.println("\n--- Dimmer Status ---");
        Serial.printf("Brightness: %d%%\n", currentLevel);
        Serial.printf("Frequency: %d Hz\n", frequency);
        Serial.printf("Active: %s\n", isActive ? "Yes" : "No");
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
        Serial.println("-------------------");
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}

/**
 * @example BasicDimmer.ino
 * 
 * This example demonstrates basic AC dimmer control with the rbdimmerESP32
 * library. It's the simplest starting point for understanding how to:
 * 
 * 1. Initialize the library
 * 2. Register a zero-cross detector  
 * 3. Create a dimmer channel
 * 4. Set a fixed brightness level
 * 
 * The dimmer will maintain the set brightness level indefinitely,
 * demonstrating the autonomous operation of the library using
 * hardware resources.
 * 
 * @note This example uses RMS curve type, which is optimal for
 *       resistive loads like incandescent bulbs. For LED loads,
 *       consider using RBDIMMER_CURVE_LOGARITHMIC instead.
 */