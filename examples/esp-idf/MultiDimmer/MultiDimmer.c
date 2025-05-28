/**
 * @file MultiDimmer.c
 * @brief Multi-Channel AC Dimmer Control Example for ESP-IDF Framework
 * 
 * This advanced example demonstrates controlling multiple independent AC dimmer
 * channels using the rbdimmerESP32 library in the ESP-IDF environment. It
 * showcases how to efficiently manage several dimmers with different load types
 * using a single zero-cross detector, all implemented in pure C code.
 * 
 * @section Architecture System Architecture
 * The multi-channel system demonstrates several key architectural concepts:
 * - Shared zero-cross detection for all channels on the same phase
 * - Independent brightness control for each channel
 * - Different curve types optimized for different load types
 * - Efficient resource usage through shared interrupts
 * - Scalable design pattern for adding more channels
 * 
 * @section RealWorld Real-World Applications
 * This multi-channel approach is essential for:
 * - Multi-zone room lighting control
 * - Mixed lighting systems (incandescent + LED)
 * - Stage and theater lighting
 * - Greenhouse grow light control
 * - Industrial process lighting
 * - Architectural accent lighting
 * 
 * @section Hardware Hardware Requirements
 * - ESP32 development board
 * - 2 x RBDimmer AC dimmer modules (expandable to more)
 * - Mixed AC loads:
 *   - Channel 1: Incandescent bulb (60-100W)
 *   - Channel 2: Dimmable LED bulb or driver
 * - Single zero-cross detector (shared between modules)
 * - Proper AC mains isolation
 * 
 * @section Wiring Wiring Connections
 * Zero-Cross (shared):
 * - GPIO 18 -> Both dimmers' ZC outputs (connected together)
 * 
 * Channel 1 (Incandescent):
 * - GPIO 19 -> Dimmer 1 PWM input
 * 
 * Channel 2 (LED):
 * - GPIO 21 -> Dimmer 2 PWM input
 * 
 * Power (both modules):
 * - 3.3V -> VCC (all modules)
 * - GND -> GND (all modules)
 * 
 * @section Expected Expected Console Output
 * @code
 * I (325) DIMMER_EXAMPLE: === RBDimmer Multi-Channel ESP-IDF Example ===
 * I (335) RBDIMMER: RBDimmer library initialized
 * I (345) DIMMER_EXAMPLE: Registering shared zero-cross detector...
 * I (355) RBDIMMER: Zero-cross detector registered on pin 18 for phase 0
 * I (365) DIMMER_EXAMPLE: Creating channel 1 (Incandescent)...
 * I (375) RBDIMMER: Dimmer channel created on pin 19, phase 0
 * I (385) DIMMER_EXAMPLE: Creating channel 2 (LED)...
 * I (395) RBDIMMER: Dimmer channel created on pin 21, phase 0
 * I (405) DIMMER_EXAMPLE: Multi-channel system initialized successfully
 * I (415) DIMMER_EXAMPLE: Detected frequency: 50 Hz
 * I (425) DIMMER_EXAMPLE: Starting multi-channel demonstration...
 * I (435) DIMMER_EXAMPLE: Setting: Ch1=75%, Ch2=25%
 * I (2435) DIMMER_EXAMPLE: Setting: Ch1=25%, Ch2=75%
 * @endcode
 * 
 * @section Performance Performance Considerations
 * Multi-channel systems require careful attention to:
 * - Interrupt latency (all channels share zero-cross ISR)
 * - Timer resource allocation (2 timers per channel)
 * - Memory usage (scales linearly with channel count)
 * - CPU load (increases with transition complexity)
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "rbdimmerESP32.h"

/** @brief Tag for ESP-IDF logging system */
static const char *TAG = "DIMMER_EXAMPLE";

/** @name Hardware Configuration
 *  @brief GPIO pin assignments for multi-channel system
 *  @{
 */
#define ZERO_CROSS_PIN  18   ///< Shared zero-cross detector GPIO
#define DIMMER_PIN_1    19   ///< Channel 1 control GPIO (Incandescent)
#define DIMMER_PIN_2    21   ///< Channel 2 control GPIO (LED)
#define PHASE_NUM       0    ///< AC phase number (0 for single-phase)

// Expandable to more channels
// #define DIMMER_PIN_3    22
// #define DIMMER_PIN_4    23
// #define NUM_CHANNELS    4
/** @} */

/** @name System Configuration
 *  @brief Multi-channel system parameters
 *  @{
 */
#define NUM_CHANNELS    2    ///< Number of dimmer channels
#define DEMO_DELAY_MS   2000 ///< Delay between demonstration steps
#define STATUS_INTERVAL_MS 10000 ///< Status print interval
/** @} */

/**
 * @brief Channel information structure
 * 
 * Holds configuration and runtime information for each channel
 */
typedef struct {
    rbdimmer_channel_t* handle;    ///< Channel handle from library
    const char* name;              ///< Human-readable channel name
    const char* load_type;         ///< Type of connected load
    uint8_t gpio_pin;              ///< GPIO pin number
    rbdimmer_curve_t curve_type;   ///< Brightness curve type
    uint8_t current_level;         ///< Current brightness level
} channel_info_t;

/**
 * @brief Global channel array
 * 
 * Stores information about all configured channels for easy management
 */
static channel_info_t channels[NUM_CHANNELS] = {
    {
        .handle = NULL,
        .name = "Main Light",
        .load_type = "Incandescent",
        .gpio_pin = DIMMER_PIN_1,
        .curve_type = RBDIMMER_CURVE_RMS,
        .current_level = 50
    },
    {
        .handle = NULL,
        .name = "Accent LED",
        .load_type = "Dimmable LED",
        .gpio_pin = DIMMER_PIN_2,
        .curve_type = RBDIMMER_CURVE_LOGARITHMIC,
        .current_level = 50
    }
};

/**
 * @brief Initialize the multi-channel dimmer system
 * 
 * Performs complete initialization:
 * 1. Initialize rbdimmer library
 * 2. Register shared zero-cross detector
 * 3. Create all dimmer channels with appropriate settings
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t multi_dimmer_init(void)
{
    rbdimmer_err_t err;
    
    ESP_LOGI(TAG, "=== RBDimmer Multi-Channel ESP-IDF Example ===");
    
    // Step 1: Initialize the library
    err = rbdimmer_init();
    if (err != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to initialize dimmer library (error: %d)", err);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Dimmer library initialized");
    
    // Step 2: Register shared zero-cross detector
    ESP_LOGI(TAG, "Registering shared zero-cross detector...");
    err = rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);
    if (err != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to register zero-cross detector (error: %d)", err);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Zero-cross detector registered on pin %d", ZERO_CROSS_PIN);
    
    // Step 3: Create all channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
        ESP_LOGI(TAG, "Creating channel %d (%s)...", i + 1, channels[i].name);
        
        // Configure channel
        rbdimmer_config_t config = {
            .gpio_pin = channels[i].gpio_pin,
            .phase = PHASE_NUM,
            .initial_level = channels[i].current_level,
            .curve_type = channels[i].curve_type
        };
        
        // Create channel
        err = rbdimmer_create_channel(&config, &channels[i].handle);
        if (err != RBDIMMER_OK) {
            ESP_LOGE(TAG, "Failed to create channel %d (error: %d)", i + 1, err);
            return ESP_FAIL;
        }
        
        ESP_LOGI(TAG, "Channel %d created: %s on pin %d (%s curve)", 
                 i + 1, channels[i].load_type, channels[i].gpio_pin,
                 channels[i].curve_type == RBDIMMER_CURVE_RMS ? "RMS" : "Logarithmic");
    }
    
    ESP_LOGI(TAG, "Multi-channel system initialized successfully");
    
    // Wait for frequency detection
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Report detected frequency
    uint16_t freq = rbdimmer_get_frequency(PHASE_NUM);
    if (freq > 0) {
        ESP_LOGI(TAG, "Detected frequency: %d Hz", freq);
    }
    
    return ESP_OK;
}

/**
 * @brief Set brightness level for a specific channel
 * 
 * Updates channel brightness and logs the change
 * 
 * @param channel_index Channel index (0 to NUM_CHANNELS-1)
 * @param level Brightness level (0-100%)
 * @return ESP_OK on success
 */
static esp_err_t set_channel_level(int channel_index, uint8_t level)
{
    if (channel_index >= NUM_CHANNELS) {
        ESP_LOGE(TAG, "Invalid channel index: %d", channel_index);
        return ESP_FAIL;
    }
    
    rbdimmer_err_t err = rbdimmer_set_level(channels[channel_index].handle, level);
    if (err != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to set level for channel %d", channel_index + 1);
        return ESP_FAIL;
    }
    
    channels[channel_index].current_level = level;
    ESP_LOGI(TAG, "%s set to %d%%", channels[channel_index].name, level);
    
    return ESP_OK;
}

/**
 * @brief Demonstrate alternating brightness pattern
 * 
 * Creates a complementary lighting effect where channels
 * alternate between bright and dim settings
 */
static void demonstrate_alternating(void)
{
    ESP_LOGI(TAG, "\n=== Alternating Brightness Pattern ===");
    
    // First state: Ch1 bright, Ch2 dim
    ESP_LOGI(TAG, "Setting: Ch1=75%%, Ch2=25%%");
    set_channel_level(0, 75);
    set_channel_level(1, 25);
    vTaskDelay(pdMS_TO_TICKS(DEMO_DELAY_MS));
    
    // Second state: Ch1 dim, Ch2 bright
    ESP_LOGI(TAG, "Setting: Ch1=25%%, Ch2=75%%");
    set_channel_level(0, 25);
    set_channel_level(1, 75);
    vTaskDelay(pdMS_TO_TICKS(DEMO_DELAY_MS));
}

/**
 * @brief Demonstrate synchronized control
 * 
 * Shows how multiple channels can be controlled together
 * for unified lighting effects
 */
static void demonstrate_synchronized(void)
{
    ESP_LOGI(TAG, "\n=== Synchronized Control ===");
    
    // All channels to same level
    uint8_t levels[] = {0, 30, 60, 90, 60, 30, 0};
    
    for (int i = 0; i < sizeof(levels) / sizeof(levels[0]); i++) {
        ESP_LOGI(TAG, "All channels to %d%%", levels[i]);
        
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            set_channel_level(ch, levels[i]);
        }
        
        vTaskDelay(pdMS_TO_TICKS(DEMO_DELAY_MS / 2));
    }
}

/**
 * @brief Demonstrate smooth transitions on multiple channels
 * 
 * Shows how smooth transitions can be used with multiple
 * channels for professional lighting effects
 */
static void demonstrate_smooth_multi_transitions(void)
{
    ESP_LOGI(TAG, "\n=== Multi-Channel Smooth Transitions ===");
    
    // Opposite transitions
    ESP_LOGI(TAG, "Cross-fade: Ch1 100%%->0%%, Ch2 0%%->100%% (3 seconds)");
    rbdimmer_set_level_transition(channels[0].handle, 0, 3000);
    rbdimmer_set_level_transition(channels[1].handle, 100, 3000);
    vTaskDelay(pdMS_TO_TICKS(3500));
    
    // Update tracked levels
    channels[0].current_level = 0;
    channels[1].current_level = 100;
    
    // Reverse transitions
    ESP_LOGI(TAG, "Cross-fade: Ch1 0%%->100%%, Ch2 100%%->0%% (3 seconds)");
    rbdimmer_set_level_transition(channels[0].handle, 100, 3000);
    rbdimmer_set_level_transition(channels[1].handle, 0, 3000);
    vTaskDelay(pdMS_TO_TICKS(3500));
    
    // Update tracked levels
    channels[0].current_level = 100;
    channels[1].current_level = 0;
}

/**
 * @brief Demonstrate scene presets
 * 
 * Shows how to create and switch between predefined
 * lighting scenes using multiple channels
 */
static void demonstrate_scenes(void)
{
    ESP_LOGI(TAG, "\n=== Scene Presets ===");
    
    // Scene definitions
    typedef struct {
        const char* name;
        uint8_t levels[NUM_CHANNELS];
    } scene_t;
    
    scene_t scenes[] = {
        {"Bright Work", {90, 70}},
        {"Relaxed Evening", {30, 50}},
        {"Movie Mode", {10, 20}},
        {"Wake Up", {100, 100}}
    };
    
    // Cycle through scenes
    for (int i = 0; i < sizeof(scenes) / sizeof(scenes[0]); i++) {
        ESP_LOGI(TAG, "Activating scene: %s", scenes[i].name);
        
        // Use smooth transitions to change scenes
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            rbdimmer_set_level_transition(channels[ch].handle, 
                                          scenes[i].levels[ch], 
                                          1000);
            channels[ch].current_level = scenes[i].levels[ch];
        }
        
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/**
 * @brief Print comprehensive system status
 * 
 * Displays detailed information about all channels and
 * system resources for monitoring and debugging
 */
static void print_system_status(void)
{
    ESP_LOGI(TAG, "\n========== Multi-Channel System Status ==========");
    
    // System information
    uint16_t freq = rbdimmer_get_frequency(PHASE_NUM);
    ESP_LOGI(TAG, "System Info:");
    ESP_LOGI(TAG, "  Mains frequency: %d Hz", freq);
    ESP_LOGI(TAG, "  Active channels: %d", NUM_CHANNELS);
    ESP_LOGI(TAG, "  Free heap: %d bytes", esp_get_free_heap_size());
    
    // Channel status
    ESP_LOGI(TAG, "\nChannel Status:");
    for (int i = 0; i < NUM_CHANNELS; i++) {
        uint8_t level = rbdimmer_get_level(channels[i].handle);
        bool active = rbdimmer_is_active(channels[i].handle);
        uint32_t delay = rbdimmer_get_delay(channels[i].handle);
        
        ESP_LOGI(TAG, "  Channel %d - %s:", i + 1, channels[i].name);
        ESP_LOGI(TAG, "    Load type: %s", channels[i].load_type);
        ESP_LOGI(TAG, "    GPIO pin: %d", channels[i].gpio_pin);
        ESP_LOGI(TAG, "    Current level: %d%%", level);
        ESP_LOGI(TAG, "    Active: %s", active ? "Yes" : "No");
        ESP_LOGI(TAG, "    Curve: %s", 
                 channels[i].curve_type == RBDIMMER_CURVE_RMS ? "RMS" : "Logarithmic");
        ESP_LOGI(TAG, "    Delay: %lu us", delay);
    }
    
    ESP_LOGI(TAG, "================================================\n");
}

/**
 * @brief Main application entry point
 * 
 * Initializes the multi-channel dimmer system and runs
 * a continuous demonstration of various control patterns
 * and effects possible with multiple channels.
 */
void app_main(void)
{
    // Initialize multi-channel system
    esp_err_t ret = multi_dimmer_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Multi-channel initialization failed");
        return;
    }
    
    ESP_LOGI(TAG, "Starting multi-channel demonstration...");
    ESP_LOGI(TAG, "Watch how different loads respond to their optimized curves!");
    
    // Print initial status
    print_system_status();
    
    // Main demonstration loop
    uint32_t loop_count = 0;
    TickType_t last_status_time = xTaskGetTickCount();
    
    while (1) {
        ESP_LOGI(TAG, "\n===== Demonstration Loop %lu =====", ++loop_count);
        
        // Run different demonstrations
        demonstrate_alternating();
        demonstrate_synchronized();
        demonstrate_smooth_multi_transitions();
        demonstrate_scenes();
        
        // Print status periodically
        if ((xTaskGetTickCount() - last_status_time) * portTICK_PERIOD_MS >= STATUS_INTERVAL_MS) {
            print_system_status();
            last_status_time = xTaskGetTickCount();
        }
        
        // Pause before next loop
        ESP_LOGI(TAG, "Loop complete, pausing...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/**
 * @example MultiDimmer.c
 * 
 * This ESP-IDF example demonstrates professional multi-channel AC dimmer
 * control using the rbdimmerESP32 library. Key concepts illustrated:
 * 
 * 1. **Shared Resources**: Shows how multiple channels efficiently share
 *    a single zero-cross detector, reducing hardware requirements
 * 
 * 2. **Load Optimization**: Demonstrates using different curve types
 *    (RMS for incandescent, logarithmic for LED) for optimal control
 * 
 * 3. **Scalable Architecture**: The array-based channel management makes
 *    it easy to expand to more channels
 * 
 * 4. **Scene Management**: Shows how to create lighting scenes and
 *    smoothly transition between them
 * 
 * 5. **Professional Patterns**: Demonstrates error handling, logging,
 *    and monitoring suitable for production systems
 * 
 * This example is ideal for:
 * - Home automation systems with zone lighting
 * - Commercial lighting control
 * - Stage and theater lighting
 * - Industrial lighting applications
 * - IoT-enabled lighting systems
 * 
 * The modular structure makes it easy to integrate with other ESP-IDF
 * components like WiFi, Bluetooth, or cloud services for remote control.
 */