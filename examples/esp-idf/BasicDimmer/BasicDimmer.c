/**
 * @file BasicDimmer.c
 * @brief Basic AC Dimmer Control Example for ESP-IDF Framework
 * 
 * This example demonstrates fundamental usage of the rbdimmerESP32 library
 * in the ESP-IDF environment. It shows how to initialize the library,
 * register a zero-cross detector, create a dimmer channel, and control
 * brightness levels using pure C code without Arduino abstractions.
 * 
 * @section ESPIDFInfo ESP-IDF Specific Information
 * This example uses ESP-IDF's native APIs including:
 * - FreeRTOS for task delays and scheduling
 * - ESP logging system for debug output
 * - Direct GPIO control through driver/gpio.h
 * - esp_timer for microsecond timing
 * 
 * @section BuildConfig Build Configuration
 * Requires ESP-IDF v4.0 or higher. Configure using:
 * @code
 * idf.py set-target esp32
 * idf.py menuconfig  # Optional: Configure rbdimmer settings
 * idf.py build
 * idf.py flash monitor
 * @endcode
 * 
 * @section Hardware Hardware Requirements
 * - ESP32 development board (any variant supported by ESP-IDF)
 * - RBDimmer AC dimmer module
 * - AC load (incandescent bulb 40-100W recommended)
 * - Proper isolation between AC mains and ESP32
 * 
 * @section Wiring Wiring Connections
 * - Zero-Cross Pin: GPIO 18 -> Dimmer ZC output
 * - Dimmer Pin: GPIO 19 -> Dimmer PWM input
 * - VCC: 3.3V (ESP32) -> Dimmer VCC
 * - GND: GND (ESP32) -> Dimmer GND
 * 
 * @section Expected Expected Console Output
 * @code
 * I (325) DIMMER_EXAMPLE: AC Dimmer Test
 * I (335) RBDIMMER: RBDimmer library initialized
 * I (345) RBDIMMER: Zero-cross detector registered on pin 18 for phase 0
 * I (355) RBDIMMER: Dimmer channel created on pin 19, phase 0
 * I (365) DIMMER_EXAMPLE: AC Dimmer initialized successfully
 * I (375) DIMMER_EXAMPLE: Setting brightness to 10%
 * I (2375) DIMMER_EXAMPLE: Setting brightness to 20%
 * I (4375) DIMMER_EXAMPLE: Setting brightness to 30%
 * ...
 * I (18375) DIMMER_EXAMPLE: Smooth transition to 0%
 * I (24375) DIMMER_EXAMPLE: Smooth transition to 100%
 * @endcode
 * 
 * @section Theory Theory of Operation in ESP-IDF
 * The ESP-IDF implementation provides direct hardware control without
 * Arduino abstractions. This results in:
 * - Lower latency interrupt handling
 * - More precise timing control
 * - Better integration with ESP-IDF components
 * - Full access to ESP32 hardware features
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
 *  @brief GPIO pins and system parameters
 *  @{
 */
#define ZERO_CROSS_PIN  18   ///< GPIO pin connected to zero-cross detector
#define DIMMER_PIN      19   ///< GPIO pin connected to dimmer control
#define PHASE_NUM       0    ///< Phase number (0 for single-phase systems)
/** @} */

/** @name Dimmer Parameters
 *  @brief Configuration values for dimmer operation
 *  @{
 */
#define INITIAL_LEVEL   50   ///< Initial brightness level (%)
#define MIN_LEVEL       10   ///< Minimum brightness for demo (%)
#define MAX_LEVEL       90   ///< Maximum brightness for demo (%)
#define STEP_SIZE       10   ///< Brightness step size (%)
#define STEP_DELAY_MS   2000 ///< Delay between brightness steps (ms)
#define TRANSITION_TIME_MS 5000 ///< Time for smooth transitions (ms)
/** @} */

/**
 * @brief Global dimmer channel handle
 * 
 * This pointer holds the reference to our dimmer channel after creation.
 * In ESP-IDF, we explicitly manage memory and resources, so this global
 * reference is necessary for the dimmer control throughout the program.
 */
rbdimmer_channel_t* dimmer_channel = NULL;

/**
 * @brief Initialize system components
 * 
 * Performs basic ESP32 initialization tasks that might be needed
 * before starting the dimmer system. This is where you would add
 * any project-specific initialization.
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t system_init(void)
{
    // Initialize NVS if needed (for WiFi, settings storage, etc.)
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);
    
    // Any other system initialization can go here
    ESP_LOGI(TAG, "System initialization complete");
    return ESP_OK;
}

/**
 * @brief Initialize and configure the dimmer system
 * 
 * This function encapsulates all dimmer initialization steps:
 * 1. Initialize the rbdimmer library
 * 2. Register the zero-cross detector
 * 3. Create and configure the dimmer channel
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t dimmer_system_init(void)
{
    rbdimmer_err_t err;
    
    // Step 1: Initialize the dimmer library
    ESP_LOGI(TAG, "Initializing dimmer library...");
    err = rbdimmer_init();
    if (err != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to initialize AC Dimmer library (error: %d)", err);
        return ESP_FAIL;
    }
    
    // Step 2: Register zero-cross detector
    ESP_LOGI(TAG, "Registering zero-cross detector...");
    err = rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);
    if (err != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to register zero-cross detector (error: %d)", err);
        ESP_LOGE(TAG, "Check pin %d is available and supports interrupts", ZERO_CROSS_PIN);
        return ESP_FAIL;
    }
    
    // Step 3: Configure dimmer channel
    ESP_LOGI(TAG, "Creating dimmer channel...");
    rbdimmer_config_t config_channel = {
        .gpio_pin = DIMMER_PIN,
        .phase = PHASE_NUM,
        .initial_level = INITIAL_LEVEL,
        .curve_type = RBDIMMER_CURVE_RMS  // RMS curve for incandescent bulbs
    };
    
    err = rbdimmer_create_channel(&config_channel, &dimmer_channel);
    if (err != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to create dimmer channel (error: %d)", err);
        ESP_LOGE(TAG, "Check pin %d is available", DIMMER_PIN);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "AC Dimmer initialized successfully");
    ESP_LOGI(TAG, "Initial brightness: %d%%", INITIAL_LEVEL);
    
    // Wait for frequency detection
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Report detected frequency
    uint16_t freq = rbdimmer_get_frequency(PHASE_NUM);
    if (freq > 0) {
        ESP_LOGI(TAG, "Detected mains frequency: %d Hz", freq);
    } else {
        ESP_LOGW(TAG, "Frequency detection in progress...");
    }
    
    return ESP_OK;
}

/**
 * @brief Demonstrate stepped brightness control
 * 
 * Shows how to change brightness in discrete steps.
 * This is useful for button-controlled interfaces or
 * menu-driven brightness selection.
 */
static void demonstrate_stepped_control(void)
{
    ESP_LOGI(TAG, "=== Stepped Brightness Control Demo ===");
    
    // Increase brightness from MIN to MAX
    for (int brightness = MIN_LEVEL; brightness <= MAX_LEVEL; brightness += STEP_SIZE) {
        ESP_LOGI(TAG, "Setting brightness to %d%%", brightness);
        
        rbdimmer_err_t err = rbdimmer_set_level(dimmer_channel, brightness);
        if (err != RBDIMMER_OK) {
            ESP_LOGE(TAG, "Failed to set brightness (error: %d)", err);
        }
        
        // Wait before next step
        vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
    }
    
    // Brief pause at maximum
    ESP_LOGI(TAG, "Holding at maximum brightness");
    vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
}

/**
 * @brief Demonstrate smooth transition control
 * 
 * Shows how to use the smooth transition feature for
 * professional-looking brightness changes. The transitions
 * run in the background using FreeRTOS tasks.
 */
static void demonstrate_smooth_transitions(void)
{
    ESP_LOGI(TAG, "=== Smooth Transition Demo ===");
    
    // Smooth fade to off
    ESP_LOGI(TAG, "Smooth transition to 0%% over %d seconds", TRANSITION_TIME_MS / 1000);
    rbdimmer_err_t err = rbdimmer_set_level_transition(dimmer_channel, 0, TRANSITION_TIME_MS);
    if (err != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to start transition (error: %d)", err);
    }
    
    // Wait for transition to complete plus a small margin
    vTaskDelay(pdMS_TO_TICKS(TRANSITION_TIME_MS + 1000));
    
    // Smooth fade to full brightness
    ESP_LOGI(TAG, "Smooth transition to 100%% over %d seconds", TRANSITION_TIME_MS / 1000);
    err = rbdimmer_set_level_transition(dimmer_channel, 100, TRANSITION_TIME_MS);
    if (err != RBDIMMER_OK) {
        ESP_LOGE(TAG, "Failed to start transition (error: %d)", err);
    }
    
    // Wait for transition to complete
    vTaskDelay(pdMS_TO_TICKS(TRANSITION_TIME_MS + 1000));
}

/**
 * @brief Print current system status
 * 
 * Displays comprehensive information about the dimmer system
 * including current settings, detected frequency, and memory usage.
 */
static void print_system_status(void)
{
    ESP_LOGI(TAG, "=== System Status ===");
    
    // Dimmer information
    uint8_t level = rbdimmer_get_level(dimmer_channel);
    bool active = rbdimmer_is_active(dimmer_channel);
    rbdimmer_curve_t curve = rbdimmer_get_curve(dimmer_channel);
    uint32_t delay = rbdimmer_get_delay(dimmer_channel);
    
    ESP_LOGI(TAG, "Dimmer Status:");
    ESP_LOGI(TAG, "  Current level: %d%%", level);
    ESP_LOGI(TAG, "  Active: %s", active ? "Yes" : "No");
    ESP_LOGI(TAG, "  Curve type: %s", 
             curve == RBDIMMER_CURVE_LINEAR ? "Linear" :
             curve == RBDIMMER_CURVE_RMS ? "RMS" :
             curve == RBDIMMER_CURVE_LOGARITHMIC ? "Logarithmic" : "Unknown");
    ESP_LOGI(TAG, "  Current delay: %lu us", delay);
    
    // Frequency information
    uint16_t freq = rbdimmer_get_frequency(PHASE_NUM);
    ESP_LOGI(TAG, "Mains frequency: %d Hz", freq);
    
    // System information
    ESP_LOGI(TAG, "System Info:");
    ESP_LOGI(TAG, "  Free heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "  Minimum free heap: %d bytes", esp_get_minimum_free_heap_size());
    
    ESP_LOGI(TAG, "====================");
}

/**
 * @brief Main application entry point
 * 
 * The app_main function is the entry point for ESP-IDF applications.
 * It initializes the system and runs the main control loop demonstrating
 * various dimmer control techniques.
 * 
 * @note This function runs as the main FreeRTOS task with configurable
 *       stack size and priority in sdkconfig
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== RBDimmer ESP-IDF Basic Example ===");
    ESP_LOGI(TAG, "Firmware version: 1.0.0");
    ESP_LOGI(TAG, "Compiled: " __DATE__ " " __TIME__);
    
    // Initialize system components
    esp_err_t ret = system_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "System initialization failed");
        return;
    }
    
    // Initialize dimmer system
    ret = dimmer_system_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Dimmer initialization failed");
        return;
    }
    
    // Print initial status
    print_system_status();
    
    // Main demonstration loop
    ESP_LOGI(TAG, "Starting dimmer demonstration loop");
    
    while (1) {
        // Demonstrate stepped brightness control
        demonstrate_stepped_control();
        
        // Demonstrate smooth transitions
        demonstrate_smooth_transitions();
        
        // Print status between cycles
        print_system_status();
        
        // Pause before repeating
        ESP_LOGI(TAG, "Cycle complete, pausing before repeat...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/**
 * @example BasicDimmer.c
 * 
 * This ESP-IDF example demonstrates fundamental AC dimmer control using
 * the rbdimmerESP32 library in a pure C environment. Key concepts:
 * 
 * 1. **ESP-IDF Integration**: Shows proper initialization and error
 *    handling following ESP-IDF conventions
 * 
 * 2. **Resource Management**: Demonstrates explicit resource allocation
 *    and error checking at each step
 * 
 * 3. **FreeRTOS Usage**: Uses vTaskDelay for timing, showing how the
 *    dimmer integrates with FreeRTOS scheduling
 * 
 * 4. **Logging System**: Utilizes ESP-IDF's logging system for
 *    structured debug output with different severity levels
 * 
 * 5. **Professional Structure**: Code is organized into logical functions
 *    for initialization, demonstration, and monitoring
 * 
 * This example provides the foundation for integrating AC dimmer control
 * into larger ESP-IDF projects, including IoT applications, home
 * automation systems, and industrial control solutions.
 */