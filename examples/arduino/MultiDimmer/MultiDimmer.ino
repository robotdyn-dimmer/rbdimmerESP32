/**
 * @file MultiDimmer.ino
 * @brief Multiple Dimmer Channels Control Example for ESP32
 * 
 * This advanced example demonstrates how to control multiple independent dimmer
 * channels using the rbdimmerESP32 library. It showcases the library's ability
 * to manage several dimmers simultaneously, each with its own settings and
 * behavior, perfect for multi-zone lighting applications.
 * 
 * @section Features Key Features Demonstrated
 * - Independent control of multiple dimmer channels
 * - Different curve types for different load types
 * - Simultaneous transitions on multiple channels
 * - Cross-fade effects between channels
 * - Synchronous and asynchronous operation modes
 * 
 * @section Hardware Hardware Requirements
 * - ESP32 development board (any variant)
 * - 2 or more RBDimmer AC dimmer modules
 * - Multiple AC loads (mix of incandescent and dimmable LED)
 * - Single zero-cross detector (shared between channels)
 * - Proper isolation between AC mains and ESP32
 * 
 * @section Wiring Wiring Connections
 * Zero-Cross (shared for all dimmers):
 * - GPIO 18 -> All dimmers' ZC outputs (connected together)
 * 
 * Dimmer 1 (Incandescent):
 * - GPIO 19 -> Dimmer 1 PWM input
 * 
 * Dimmer 2 (LED):
 * - GPIO 21 -> Dimmer 2 PWM input
 * 
 * Power (all dimmers):
 * - 3.3V -> All dimmers' VCC
 * - GND -> All dimmers' GND
 * 
 * @section Expected Expected Behavior
 * The example creates various lighting effects using two channels:
 * 1. Alternating brightness (one dim while other bright)
 * 2. Synchronized pulsing (both channels together)
 * 3. Chase effect (sequential brightening)
 * 4. Cross-fade (smooth transition of scene)
 * 5. Random independent changes
 * 
 * @section SerialOutput Serial Monitor Output Example
 * @code
 * === RBDimmer Multi-Channel Example ===
 * Library initialized successfully
 * Zero-cross detector registered for phase 0
 * Channel 1 created (Incandescent, RMS curve)
 * Channel 2 created (LED, Logarithmic curve)
 * Starting multi-channel demonstration...
 * 
 * Effect 1: Alternating brightness
 * - Channel 1: 80%, Channel 2: 20%
 * - Channel 1: 20%, Channel 2: 80%
 * 
 * Effect 2: Synchronized pulsing
 * - Both channels: 0% -> 100% -> 0%
 * 
 * Effect 3: Chase effect
 * - Channel 1 leading, Channel 2 following
 * @endcode
 * 
 * @section Applications Real-World Applications
 * This multi-channel approach is perfect for:
 * - Room lighting with multiple zones
 * - Accent lighting systems
 * - Stage and theater lighting
 * - Retail display lighting
 * - Architectural lighting control
 * - Mood lighting with color temperature control
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

/** @name Hardware Configuration
 *  @brief Pin assignments and system constants
 *  @{
 */
#define ZERO_CROSS_PIN 18    ///< Shared zero-cross detector pin
#define DIMMER_PIN_1   19    ///< First dimmer control pin
#define DIMMER_PIN_2   21    ///< Second dimmer control pin
#define PHASE_NUM      0     ///< Single phase system

// Optional: Define more channels if available
// #define DIMMER_PIN_3   22
// #define DIMMER_PIN_4   23
/** @} */

/** @name Channel Configuration
 *  @brief Number of dimmer channels in this example
 *  @{
 */
#define NUM_CHANNELS   2     ///< Total number of dimmer channels
/** @} */

/**
 * @brief Dimmer channel handles array
 * 
 * Array to store pointers to all dimmer channels for easy iteration
 */
rbdimmer_channel_t* channels[NUM_CHANNELS] = {NULL, NULL};

/**
 * @brief Channel names for debugging
 * 
 * Human-readable names for each channel to improve debug output
 */
const char* channelNames[NUM_CHANNELS] = {"Room Light", "Accent LED"};

/**
 * @brief Channel configurations
 * 
 * Structure to hold the configuration for each channel, including
 * load type and preferred curve
 */
struct {
    uint8_t pin;                    ///< GPIO pin number
    rbdimmer_curve_t curve;         ///< Brightness curve type
    const char* loadType;           ///< Load type description
} channelConfigs[NUM_CHANNELS] = {
    {DIMMER_PIN_1, RBDIMMER_CURVE_RMS, "Incandescent"},
    {DIMMER_PIN_2, RBDIMMER_CURVE_LOGARITHMIC, "LED"}
};

/**
 * @brief Current effect being demonstrated
 */
enum EffectType {
    EFFECT_ALTERNATE,    ///< Alternating brightness between channels
    EFFECT_SYNC_PULSE,   ///< Synchronized pulsing
    EFFECT_CHASE,        ///< Sequential chase effect
    EFFECT_CROSSFADE,    ///< Smooth cross-fade between channels
    EFFECT_RANDOM        ///< Random independent changes
};

EffectType currentEffect = EFFECT_ALTERNATE;
unsigned long effectStartTime = 0;
const unsigned long effectDuration = 10000; // 10 seconds per effect

/**
 * @brief Initialize all dimmer channels
 * 
 * Creates and configures each dimmer channel with appropriate
 * settings for its load type
 * 
 * @return true if all channels initialized successfully
 */
bool initializeChannels() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        Serial.printf("Creating channel %d (%s)...\n", i + 1, channelNames[i]);
        
        // Configure channel
        rbdimmer_config_t config = {
            .gpio_pin = channelConfigs[i].pin,
            .phase = PHASE_NUM,
            .initial_level = 0,
            .curve_type = channelConfigs[i].curve
        };
        
        // Create channel
        rbdimmer_err_t err = rbdimmer_create_channel(&config, &channels[i]);
        if (err != RBDIMMER_OK) {
            Serial.printf("ERROR: Failed to create channel %d (error: %d)\n", i + 1, err);
            return false;
        }
        
        Serial.printf("Channel %d created (%s, %s curve)\n", 
                     i + 1, 
                     channelConfigs[i].loadType,
                     channelConfigs[i].curve == RBDIMMER_CURVE_RMS ? "RMS" : "Logarithmic");
    }
    
    return true;
}

/**
 * @brief Arduino setup function
 * 
 * Initializes the multi-channel dimmer system
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        delay(10);
    }
    
    Serial.println("\n=== RBDimmer Multi-Channel Example ===");
    
    // Initialize the dimmer library
    if (rbdimmer_init() != RBDIMMER_OK) {
        Serial.println("ERROR: Failed to initialize dimmer library!");
        while (1) delay(1000);
    }
    Serial.println("Library initialized successfully");
    
    // Register single zero-cross detector for all channels
    Serial.printf("Registering zero-cross detector on pin %d...\n", ZERO_CROSS_PIN);
    if (rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0) != RBDIMMER_OK) {
        Serial.println("ERROR: Failed to register zero-cross detector!");
        while (1) delay(1000);
    }
    Serial.printf("Zero-cross detector registered for phase %d\n", PHASE_NUM);
    
    // Initialize all channels
    if (!initializeChannels()) {
        Serial.println("ERROR: Channel initialization failed!");
        while (1) delay(1000);
    }
    
    // Wait for frequency detection
    delay(500);
    uint16_t freq = rbdimmer_get_frequency(PHASE_NUM);
    if (freq > 0) {
        Serial.printf("Detected mains frequency: %d Hz\n", freq);
    }
    
    Serial.println("\nStarting multi-channel demonstration...");
    Serial.println("Watch how multiple lights can be controlled independently!\n");
    
    effectStartTime = millis();
}

/**
 * @brief Execute alternating brightness effect
 * 
 * Makes channels alternate between bright and dim settings,
 * creating a complementary lighting effect
 */
void effectAlternating() {
    static unsigned long lastSwitch = 0;
    static bool state = false;
    
    if (millis() - lastSwitch >= 2000) { // Switch every 2 seconds
        lastSwitch = millis();
        state = !state;
        
        if (state) {
            Serial.println("Alternating: Channel 1 bright, Channel 2 dim");
            rbdimmer_set_level_transition(channels[0], 80, 1000);
            rbdimmer_set_level_transition(channels[1], 20, 1000);
        } else {
            Serial.println("Alternating: Channel 1 dim, Channel 2 bright");
            rbdimmer_set_level_transition(channels[0], 20, 1000);
            rbdimmer_set_level_transition(channels[1], 80, 1000);
        }
    }
}

/**
 * @brief Execute synchronized pulsing effect
 * 
 * All channels pulse together in synchronization,
 * creating a breathing effect
 */
void effectSyncPulse() {
    static unsigned long pulseStart = 0;
    static bool increasing = true;
    
    if (pulseStart == 0) {
        pulseStart = millis();
        Serial.println("Synchronized pulse: All channels breathing together");
    }
    
    // Create sine wave pattern
    float phase = (millis() - pulseStart) / 3000.0 * PI; // 3 second period
    uint8_t level = (uint8_t)(50 + 50 * sin(phase));
    
    // Apply to all channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
        rbdimmer_set_level(channels[i], level);
    }
}

/**
 * @brief Execute chase effect
 * 
 * Channels light up in sequence, creating a moving light effect
 */
void effectChase() {
    static unsigned long stepTime = 0;
    static int currentChannel = 0;
    
    if (millis() - stepTime >= 500) { // Change every 500ms
        stepTime = millis();
        
        // Turn off previous channel
        int prevChannel = (currentChannel - 1 + NUM_CHANNELS) % NUM_CHANNELS;
        rbdimmer_set_level_transition(channels[prevChannel], 10, 300);
        
        // Turn on current channel
        rbdimmer_set_level_transition(channels[currentChannel], 90, 300);
        
        Serial.printf("Chase: Channel %d active\n", currentChannel + 1);
        
        // Move to next channel
        currentChannel = (currentChannel + 1) % NUM_CHANNELS;
    }
}

/**
 * @brief Execute cross-fade effect
 * 
 * Smooth transition between different lighting scenes
 */
void effectCrossFade() {
    static unsigned long fadeStart = 0;
    static uint8_t scene = 0;
    
    if (fadeStart == 0 || millis() - fadeStart >= 3000) {
        fadeStart = millis();
        scene = (scene + 1) % 3;
        
        switch (scene) {
            case 0: // Warm scene
                Serial.println("Cross-fade: Warm lighting scene");
                rbdimmer_set_level_transition(channels[0], 100, 2000);
                rbdimmer_set_level_transition(channels[1], 30, 2000);
                break;
                
            case 1: // Cool scene
                Serial.println("Cross-fade: Cool lighting scene");
                rbdimmer_set_level_transition(channels[0], 30, 2000);
                rbdimmer_set_level_transition(channels[1], 100, 2000);
                break;
                
            case 2: // Balanced scene
                Serial.println("Cross-fade: Balanced lighting scene");
                rbdimmer_set_level_transition(channels[0], 60, 2000);
                rbdimmer_set_level_transition(channels[1], 60, 2000);
                break;
        }
    }
}

/**
 * @brief Execute random changes effect
 * 
 * Each channel changes independently to random levels
 */
void effectRandom() {
    static unsigned long lastChange = 0;
    
    if (millis() - lastChange >= 2000) { // Change every 2 seconds
        lastChange = millis();
        
        Serial.println("Random: Independent channel changes");
        
        for (int i = 0; i < NUM_CHANNELS; i++) {
            uint8_t randomLevel = random(20, 90);
            uint16_t randomDuration = random(500, 2000);
            
            rbdimmer_set_level_transition(channels[i], randomLevel, randomDuration);
            Serial.printf("  Channel %d -> %d%% in %dms\n", 
                         i + 1, randomLevel, randomDuration);
        }
    }
}

/**
 * @brief Print status of all channels
 */
void printChannelStatus() {
    Serial.println("\n--- Channel Status ---");
    for (int i = 0; i < NUM_CHANNELS; i++) {
        uint8_t level = rbdimmer_get_level(channels[i]);
        bool active = rbdimmer_is_active(channels[i]);
        
        Serial.printf("%s: %d%% %s\n", 
                     channelNames[i], 
                     level,
                     active ? "" : "(inactive)");
    }
    Serial.println("--------------------\n");
}

/**
 * @brief Arduino main loop
 * 
 * Cycles through different multi-channel effects to demonstrate
 * the various ways multiple dimmers can work together
 */
void loop() {
    unsigned long currentTime = millis();
    
    // Switch effects every effectDuration milliseconds
    if (currentTime - effectStartTime >= effectDuration) {
        effectStartTime = currentTime;
        currentEffect = (EffectType)((currentEffect + 1) % 5);
        
        Serial.printf("\n=== Switching to effect %d ===\n", currentEffect + 1);
        
        // Reset all channels to known state
        for (int i = 0; i < NUM_CHANNELS; i++) {
            rbdimmer_set_level(channels[i], 0);
        }
        delay(500);
    }
    
    // Execute current effect
    switch (currentEffect) {
        case EFFECT_ALTERNATE:
            effectAlternating();
            break;
            
        case EFFECT_SYNC_PULSE:
            effectSyncPulse();
            break;
            
        case EFFECT_CHASE:
            effectChase();
            break;
            
        case EFFECT_CROSSFADE:
            effectCrossFade();
            break;
            
        case EFFECT_RANDOM:
            effectRandom();
            break;
    }
    
    // Print status periodically
    static unsigned long lastStatusPrint = 0;
    if (currentTime - lastStatusPrint >= 5000) {
        lastStatusPrint = currentTime;
        printChannelStatus();
    }
    
    // Small delay for system stability
    delay(10);
}

/**
 * @example MultiDimmer.ino
 * 
 * This advanced example demonstrates the power of multi-channel dimmer
 * control with the rbdimmerESP32 library. Key concepts shown:
 * 
 * 1. **Single Zero-Cross, Multiple Outputs**: All dimmers share one
 *    zero-cross detector, simplifying wiring and synchronization
 * 
 * 2. **Independent Control**: Each channel operates independently
 *    with its own brightness level and curve type
 * 
 * 3. **Load-Specific Curves**: Different curve types optimize
 *    performance for different load types (incandescent vs LED)
 * 
 * 4. **Complex Effects**: Demonstrates how multiple channels can
 *    create sophisticated lighting effects
 * 
 * 5. **Scalability**: The array-based approach makes it easy to
 *    add more channels (up to RBDIMMER_MAX_CHANNELS)
 * 
 * This example forms the foundation for:
 * - Home automation lighting systems
 * - Professional stage lighting control
 * - Architectural lighting installations
 * - Energy-efficient zone-based lighting
 * 
 * @note When using multiple channels, ensure your power supply
 *       can handle the total load of all dimmers combined.
 */