# ESP-IDF Basic Dimmer Control Example

This example demonstrates the fundamental usage of the rbdimmerESP32 library in the ESP-IDF environment, showing how to implement AC dimmer control using pure C code without Arduino abstractions.

## Overview

The ESP-IDF implementation of the rbdimmerESP32 library provides direct access to ESP32 hardware capabilities, offering lower latency and more precise control compared to Arduino-based implementations. This example serves as the foundation for understanding how to integrate professional AC dimmer control into ESP-IDF projects, whether you're building IoT devices, industrial control systems, or home automation solutions.

Working directly with ESP-IDF provides several advantages. You gain complete control over system resources and timing, can integrate seamlessly with other ESP-IDF components like WiFi, Bluetooth, or advanced peripherals, and achieve optimal performance through direct hardware access. The trade-off is slightly more complex code structure, but the benefits in terms of control and performance make it worthwhile for professional applications.

## What This Example Demonstrates

This ESP-IDF example showcases several key aspects of embedded systems programming. First, it demonstrates proper error handling at every step, following ESP-IDF conventions where each function returns an error code that must be checked. This defensive programming approach ensures robust operation in production environments.

The example also illustrates the proper use of FreeRTOS APIs, showing how the dimmer library integrates with the ESP32's real-time operating system. You'll see how to use task delays, understand task priorities, and work within the FreeRTOS scheduling model. The dimmer operations themselves run as background tasks, allowing your main application to continue processing other activities.

Additionally, the code demonstrates ESP-IDF's powerful logging system, showing how to use different log levels (error, warning, info, debug) to create informative output that aids in development and debugging. The structured logging approach makes it easy to filter messages by severity and component, essential for complex projects.

## ESP-IDF vs Arduino Implementation

Understanding the differences between ESP-IDF and Arduino implementations helps you choose the right approach for your project. In ESP-IDF, you work directly with the hardware abstraction layer, giving you finer control over timing and resources. The initialization process is more explicit - you manually initialize each component and check for errors, whereas Arduino often handles this behind the scenes.

Memory management is also more explicit in ESP-IDF. You have direct control over heap allocation and can monitor memory usage precisely. The example shows how to check free heap space, helping you optimize memory usage in resource-constrained applications. Task stack sizes are configurable, allowing you to minimize memory usage while ensuring stable operation.

The interrupt handling in ESP-IDF is more sophisticated, with direct access to interrupt priorities and the ability to place interrupt handlers in IRAM for minimal latency. This results in more precise dimmer control, especially important for applications requiring exact timing or multiple synchronized channels.

## Hardware Requirements

- **ESP32 Board**: Any ESP32 variant supported by ESP-IDF
- **Development Environment**: ESP-IDF v4.0 or higher
- **AC Dimmer Module**: RBDimmer AC dimmer module
- **AC Load**: Incandescent bulb (40-100W) for best visual results
- **USB Cable**: For programming and monitoring
- **Power Supply**: Stable 3.3V for ESP32

## Wiring Connections

| ESP32 Pin | Dimmer Module | Description |
|-----------|---------------|-------------|
| GPIO 18   | ZC (Zero Cross) | Zero-crossing detection signal |
| GPIO 19   | PWM/CTRL | Dimmer control signal |
| 3.3V      | VCC | Module power supply |
| GND       | GND | Common ground |

⚠️ **Safety Warning**: Always ensure proper electrical isolation between AC mains and your ESP32! The RBDimmer modules include built-in isolation, but always verify connections before applying power.

## Building and Flashing

Setting up and building an ESP-IDF project follows a standard workflow. First, ensure your ESP-IDF environment is properly configured:

```bash
# Set up ESP-IDF environment (adjust path as needed)
. ~/esp/esp-idf/export.sh

# Navigate to the example directory
cd examples/esp_idf/basic_dimmer

# Set target chip (esp32, esp32s2, esp32s3, esp32c3, etc.)
idf.py set-target esp32

# Configure project (optional - for advanced settings)
idf.py menuconfig

# Build the project
idf.py build

# Flash and monitor
idf.py flash monitor
```

The menuconfig step allows you to customize various parameters including dimmer-specific settings if the library provides Kconfig options. You can adjust maximum channels, default frequencies, and other parameters without modifying code.

## Expected Console Output

```
I (325) DIMMER_EXAMPLE: === RBDimmer ESP-IDF Basic Example ===
I (335) DIMMER_EXAMPLE: Firmware version: 1.0.0
I (345) DIMMER_EXAMPLE: Compiled: Dec 15 2024 10:30:45
I (355) DIMMER_EXAMPLE: System initialization complete
I (365) DIMMER_EXAMPLE: Initializing dimmer library...
I (375) RBDIMMER: RBDimmer library initialized
I (385) DIMMER_EXAMPLE: Registering zero-cross detector...
I (395) RBDIMMER: Zero-cross detector registered on pin 18 for phase 0
I (405) DIMMER_EXAMPLE: Creating dimmer channel...
I (415) RBDIMMER: Dimmer channel created on pin 19, phase 0
I (425) DIMMER_EXAMPLE: AC Dimmer initialized successfully
I (435) DIMMER_EXAMPLE: Initial brightness: 50%
I (935) DIMMER_EXAMPLE: Detected mains frequency: 50 Hz

=== System Status ===
I (945) DIMMER_EXAMPLE: Dimmer Status:
I (945) DIMMER_EXAMPLE:   Current level: 50%
I (955) DIMMER_EXAMPLE:   Active: Yes
I (955) DIMMER_EXAMPLE:   Curve type: RMS
I (965) DIMMER_EXAMPLE:   Current delay: 5000 us
I (965) DIMMER_EXAMPLE: Mains frequency: 50 Hz
I (975) DIMMER_EXAMPLE: System Info:
I (975) DIMMER_EXAMPLE:   Free heap: 298765 bytes
I (985) DIMMER_EXAMPLE:   Minimum free heap: 295432 bytes
```

## Code Structure Explained

The ESP-IDF example follows a modular structure that separates concerns and makes the code maintainable. The main function (`app_main`) serves as the entry point, but delegates specific tasks to dedicated functions. This approach makes the code easier to understand and modify.

The `system_init` function handles any system-wide initialization your project might need. While simple in this example, it's where you would initialize NVS (for storing settings), WiFi, or other system components. Keeping this separate from dimmer initialization maintains clean separation of concerns.

The `dimmer_system_init` function encapsulates all dimmer-related initialization. By checking each step's return value and providing detailed error messages, it makes troubleshooting much easier. This defensive programming style is essential for production-quality embedded systems.

The demonstration functions (`demonstrate_stepped_control` and `demonstrate_smooth_transitions`) show different usage patterns. Separating these makes it easy to adapt the code for your specific needs - you might use only stepped control for button interfaces, or only smooth transitions for automated systems.

## Memory and Performance Considerations

ESP-IDF provides excellent tools for monitoring system resources. The example shows how to check free heap memory, which is crucial for long-running applications. The rbdimmer library is designed to be memory-efficient, but it's important to monitor usage in your complete application.

Each dimmer channel requires approximately 100 bytes of RAM plus associated timer resources. The zero-cross interrupt handler is placed in IRAM for minimal latency, using about 200 bytes of IRAM. These are small amounts, but in complex projects with many components, every byte counts.

Performance-wise, the interrupt latency is typically under 2 microseconds when the handler is in IRAM. The dimmer timing accuracy is within 1% of the target value, more than sufficient for lighting applications. The FreeRTOS task switching overhead for smooth transitions is negligible, allowing hundreds of transitions per second if needed.

## Integration with Other Components

One of ESP-IDF's strengths is its component system. The rbdimmer library integrates seamlessly with other ESP-IDF components. For example, you could add WiFi control by including the WiFi component and creating a web server that adjusts dimmer levels. The non-blocking nature of the dimmer library ensures it won't interfere with network operations.

You might also integrate with the ESP-IDF console component to create a command-line interface for dimmer control during development. Or use the NVS (Non-Volatile Storage) component to save dimmer settings that persist across reboots. The modular structure of the example makes these additions straightforward.

## Troubleshooting

Common issues when working with ESP-IDF include build errors (ensure ESP-IDF is properly installed and environment variables are set), flash errors (check USB drivers and try reducing baud rate), and monitor connection issues (ensure no other program is using the serial port).

For dimmer-specific issues, the detailed logging helps identify problems quickly. If the zero-cross isn't detected, you'll see error messages indicating the specific failure point. The status printing function helps verify that all parameters are set correctly.

## Advanced Topics

Once you've mastered this basic example, consider exploring advanced topics like implementing multiple channels with different phases, creating custom brightness curves for specific load types, integrating with ESP-RainMaker for cloud control, or using the ULP coprocessor for ultra-low power operation while maintaining dimmer control.

## Resources

- **Dimmers Website**: https://rbdimmer.com
- **Library Repository**: https://github.com/robotdyn-dimmer/rbdimmerESP32
- **Dimmers Catalog**: https://www.rbdimmer.com/dimmers-pricing
- **Hardware Documentation**: https://www.rbdimmer.com/knowledge/article/45
- **Library Documentation**: https://www.rbdimmer.com/knowledge/article/59
- **Example Projects**: https://www.rbdimmer.com/blog/dimmers-projects-5
- **Support Forum**: https://www.rbdimmer.com/forum

## Author

- **Developer**: dev@rbdimmer.com
- **Version**: 1.0.0
- **License**: MIT

## Next Steps

After understanding this basic example, explore the multi-channel example to see how to control multiple dimmers simultaneously, or dive into the callback example to understand advanced interrupt handling and synchronization techniques. The ESP-IDF implementation provides the foundation for building sophisticated lighting control systems with professional-grade reliability.

---

*This example is part of the rbdimmerESP32 library - Professional AC dimmer control for ESP32*