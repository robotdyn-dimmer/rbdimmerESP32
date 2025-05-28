/**
 * @file rbdimmerESP32.h
 * @brief ESP32 AC Dimmer Library using hardware timers and interrupts
 * 
 * This library provides an easy-to-use interface for controlling AC dimmers
 * with ESP32 microcontrollers, utilizing hardware features like timers and 
 * hardware interrupts for precise and efficient dimming control.
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

 #ifndef RBDIMMER_H
 #define RBDIMMER_H
 
 #include <Arduino.h>
 #include <driver/gpio.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 // Constants
 #define RBDIMMER_MAX_PHASES 4                 // Maximum number of phases
 #define RBDIMMER_MAX_CHANNELS 8               // Maximum number of channels
 #define RBDIMMER_DEFAULT_PULSE_WIDTH_US 50    // Default pulse width in microseconds
 #define RBDIMMER_DEFAULT_FREQUENCY 0          // Default mains frequency (50 Hz)
 #define RBDIMMER_FREQUENCY_MIN 45             // Minimum allowed frequency
 #define RBDIMMER_FREQUENCY_MAX 65             // Maximum allowed frequency
 #define RBDIMMER_MEASURE_CYCLES 10            // Number of cycles for frequency measurement
 #define RBDIMMER_MIN_DELAY_US 50              // Minimum delay for safe triac operation
 
 // Enumerations
 typedef enum {
     RBDIMMER_CURVE_LINEAR,                    // Linear curve (no RMS consideration)
     RBDIMMER_CURVE_RMS,                       // RMS-compensated curve
     RBDIMMER_CURVE_LOGARITHMIC,               // Logarithmic curve (for LEDs)
     RBDIMMER_CURVE_CUSTOM                     // Custom curve
 } rbdimmer_curve_t;
 
 typedef enum {
     RBDIMMER_EDGE_FALLING,                    // Falling edge
     RBDIMMER_EDGE_RISING                      // Rising edge
 } rbdimmer_edge_t;
 
 typedef enum {
     RBDIMMER_OK = 0,                          // Operation completed successfully
     RBDIMMER_ERR_INVALID_ARG,                 // Invalid argument
     RBDIMMER_ERR_NO_MEMORY,                   // Memory allocation failed
     RBDIMMER_ERR_NOT_FOUND,                   // Object not found
     RBDIMMER_ERR_ALREADY_EXIST,               // Object already exists
     RBDIMMER_ERR_TIMER_FAILED,                // Timer initialization failed
     RBDIMMER_ERR_GPIO_FAILED                  // GPIO initialization failed
 } rbdimmer_err_t;
 
 // Timer state enum
 typedef enum {
     TIMER_STATE_IDLE,        // Waiting for zero-crossing
     TIMER_STATE_DELAY,       // Waiting for delay to finish
     TIMER_STATE_PULSE_ON,    // Pulse is active, waiting to turn off
 } timer_state_t;
 
 // Forward declarations for opaque types
 typedef struct rbdimmer_channel_s rbdimmer_channel_t;
 
 // Public configuration structure
 typedef struct {
     uint8_t gpio_pin;                 // Output signal pin
     uint8_t phase;                    // Phase number (for multi-phase systems)
     uint8_t initial_level;            // Initial level percentage (0-100)
     rbdimmer_curve_t curve_type;      // Level curve type
 } rbdimmer_config_t;
 
 /**
  * @brief Initialize the RBDimmer library
  * 
  * Must be called before any other function of the library.
  * 
  * @return RBDIMMER_OK if successful, otherwise an error code
  */
 rbdimmer_err_t rbdimmer_init(void);
 
 /**
  * @brief Register a zero-cross detector
  * 
  * @param pin GPIO pin connected to the zero-cross detector
  * @param phase Phase number (0-3) for multi-phase systems
  * @param frequency Initial mains frequency (Hz), typically 50 or 60
  * @return RBDIMMER_OK if successful, otherwise an error code
  */
 rbdimmer_err_t rbdimmer_register_zero_cross(uint8_t pin, uint8_t phase, uint16_t frequency);
 
 /**
  * @brief Create a dimmer channel
  * 
  * @param config Configuration structure with channel parameters
  * @param channel Pointer to store the created channel handle
  * @return RBDIMMER_OK if successful, otherwise an error code
  */
 rbdimmer_err_t rbdimmer_create_channel(rbdimmer_config_t* config, rbdimmer_channel_t** channel);
 
 /**
  * @brief Set channel level
  * 
  * @param channel Channel handle
  * @param level_percent Level percentage (0-100)
  * @return RBDIMMER_OK if successful, otherwise an error code
  */
 rbdimmer_err_t rbdimmer_set_level(rbdimmer_channel_t* channel, uint8_t level_percent);
 
 /**
  * @brief Set channel level with smooth transition
  * 
  * @param channel Channel handle
  * @param level_percent Target level percentage (0-100)
  * @param transition_ms Transition time in milliseconds
  * @return RBDIMMER_OK if successful, otherwise an error code
  */
 rbdimmer_err_t rbdimmer_set_level_transition(rbdimmer_channel_t* channel, uint8_t level_percent, uint32_t transition_ms);
 
 /**
  * @brief Set level curve type
  * 
  * @param channel Channel handle
  * @param curve_type Curve type (linear, RMS, logarithmic, or custom)
  * @return RBDIMMER_OK if successful, otherwise an error code
  */
 rbdimmer_err_t rbdimmer_set_curve(rbdimmer_channel_t* channel, rbdimmer_curve_t curve_type);
 
 /**
  * @brief Enable or disable a channel
  * 
  * @param channel Channel handle
  * @param active true to enable, false to disable
  * @return RBDIMMER_OK if successful, otherwise an error code
  */
 rbdimmer_err_t rbdimmer_set_active(rbdimmer_channel_t* channel, bool active);
 
 /**
  * @brief Get current channel level
  * 
  * @param channel Channel handle
  * @return Current level percentage (0-100)
  */
 uint8_t rbdimmer_get_level(rbdimmer_channel_t* channel);
 
 /**
  * @brief Get measured mains frequency for specified phase
  * 
  * @param phase Phase number
  * @return Frequency in Hz, or 0 if not measured yet
  */
 uint16_t rbdimmer_get_frequency(uint8_t phase);
 
 /**
  * @brief Set callback function for zero-cross events
  * 
  * @param phase Phase number
  * @param callback Callback function
  * @param user_data User data to pass to the callback
  * @return RBDIMMER_OK if successful, otherwise an error code
  */
 rbdimmer_err_t rbdimmer_set_callback(uint8_t phase, void (*callback)(void*), void* user_data);
 
 /**
  * @brief Force update of all channels
  * 
  * @return RBDIMMER_OK if successful, otherwise an error code
  */
 rbdimmer_err_t rbdimmer_update_all(void);
 
 /**
  * @brief Delete a dimmer channel and release resources
  * 
  * @param channel Channel handle
  * @return RBDIMMER_OK if successful, otherwise an error code
  */
 rbdimmer_err_t rbdimmer_delete_channel(rbdimmer_channel_t* channel);
 
 /**
  * @brief Deinitialize the RBDimmer library
  * 
  * @return RBDIMMER_OK if successful, otherwise an error code
  */
 rbdimmer_err_t rbdimmer_deinit(void);
 
 /**
  * @brief Check if a channel is active
  * 
  * @param channel Channel handle
  * @return true if active, false otherwise
  */
 bool rbdimmer_is_active(rbdimmer_channel_t* channel);
 
 /**
  * @brief Get the curve type of a channel
  * 
  * @param channel Channel handle
  * @return Current curve type
  */
 rbdimmer_curve_t rbdimmer_get_curve(rbdimmer_channel_t* channel);
 
 /**
  * @brief Get the current delay setting of a channel
  * 
  * @param channel Channel handle
  * @return Current delay in microseconds
  */
 uint32_t rbdimmer_get_delay(rbdimmer_channel_t* channel);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif // RBDIMMER_H