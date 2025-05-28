# rbdimmerESP32 Library Examples

This directory contains comprehensive examples demonstrating the capabilities of the rbdimmerESP32 library across two development frameworks: Arduino and ESP-IDF. Each example is carefully crafted to teach specific concepts while building upon previous knowledge, creating a natural learning progression from basic control to advanced real-time systems.

## Overview

The rbdimmerESP32 library provides professional-grade AC dimmer control for ESP32 microcontrollers. These examples demonstrate how to harness the library's full potential, whether you're building a simple home lighting controller or a sophisticated industrial automation system. The examples are organized by framework and complexity level, allowing you to start with basic concepts and progress to advanced techniques at your own pace.

## Arduino Framework Examples

The Arduino examples provide an accessible entry point for developers familiar with the Arduino ecosystem. These examples emphasize ease of use while demonstrating professional programming practices. Each example includes detailed comments explaining not just what the code does, but why specific approaches were chosen.

### 1. BasicDimmer - Fundamental Dimmer Control

This example serves as your introduction to AC dimmer control with the rbdimmerESP32 library. It demonstrates the essential initialization sequence and shows how to set a fixed brightness level. The example focuses on core concepts including library initialization, zero-cross detection registration, dimmer channel creation, and basic brightness control. This is the perfect starting point for understanding how the library manages hardware resources to provide stable, flicker-free dimming.

**Key concepts demonstrated:**
- Library initialization and error handling
- Zero-cross detector configuration
- Single channel dimmer creation
- Setting and maintaining brightness levels
- Basic status monitoring

### 2. BasicTransition - Smooth Brightness Transitions

Building on the basic control concepts, this example introduces one of the library's most powerful features: non-blocking smooth transitions. Using FreeRTOS tasks behind the scenes, the library can gradually change brightness levels over specified time periods while your main program continues executing. This example shows how to create professional-looking fade effects that are essential for modern lighting applications.

**Key concepts demonstrated:**
- Non-blocking transition system architecture
- FreeRTOS task integration for smooth effects
- Multiple transition patterns and timing
- Concurrent operation with main program
- Creating engaging lighting effects

### 3. MultiDimmer - Multi-Channel Control Systems

This advanced example demonstrates how to control multiple independent dimmer channels simultaneously. It showcases the library's efficient resource sharing, where multiple channels use a single zero-cross detector while maintaining independent control. The example includes different optimization curves for different load types (incandescent vs LED), demonstrating how to achieve optimal performance with mixed lighting systems.

**Key concepts demonstrated:**
- Multi-channel architecture and management
- Shared zero-cross detection for efficiency
- Load-specific optimization curves
- Creating coordinated lighting effects
- Scene management and transitions
- Scalable system design patterns

### 4. ZCCallBack - Advanced Synchronization and Callbacks

The most sophisticated Arduino example demonstrates professional interrupt handling and real-time synchronization. It shows how to register callback functions that execute on every zero-crossing event, enabling precise synchronization with AC mains frequency. The example includes proper FreeRTOS queue usage for safe ISR-to-task communication and comprehensive frequency monitoring, making it ideal for applications requiring precise timing or power quality analysis.

**Key concepts demonstrated:**
- Zero-cross callback registration and handling
- ISR-safe programming patterns
- FreeRTOS queue communication from interrupts
- Real-time frequency measurement and monitoring
- System performance analysis
- Advanced debugging and diagnostics

## ESP-IDF Framework Examples

The ESP-IDF examples showcase the library's capabilities in a professional embedded development environment. These examples demonstrate lower-level control, better performance optimization, and direct hardware access while maintaining clean, maintainable code structure. The ESP-IDF examples are ideal for commercial products, industrial applications, or any project requiring maximum control and efficiency.

### 1. BasicDimmer.c - Professional Dimmer Control Foundation

This comprehensive example demonstrates fundamental dimmer control concepts in the ESP-IDF environment. Unlike the Arduino version, it shows explicit error handling at every step, proper use of ESP-IDF logging system, and structured code organization suitable for larger projects. The example emphasizes professional coding practices including modular function design, comprehensive error reporting, and system resource monitoring.

**Key concepts demonstrated:**
- ESP-IDF component initialization patterns
- Comprehensive error handling and recovery
- Structured logging for debugging
- FreeRTOS integration best practices
- System resource monitoring
- Professional code organization

### 2. MultiDimmer.c - Scalable Multi-Channel Architecture

This advanced example shows how to build efficient multi-channel dimmer systems in ESP-IDF. It demonstrates professional patterns for managing multiple channels including array-based channel management, efficient resource sharing, and optimized control algorithms. The example includes sophisticated features like scene management, synchronized effects, and comprehensive system monitoring, showing how to build commercial-grade lighting control systems.

**Key concepts demonstrated:**
- Efficient multi-channel resource management
- Array-based scalable architecture
- Advanced scene and effect programming
- Performance optimization techniques
- Comprehensive system status monitoring
- Production-ready error handling

### 3. ZCCallBack.c - Real-Time Interrupt Handling Masterclass

The most advanced ESP-IDF example provides a masterclass in professional interrupt handling and real-time system design. It demonstrates critical concepts including IRAM-safe interrupt handlers, microsecond-precision timing, and sophisticated frequency analysis. The example shows how to build systems that maintain real-time performance while performing complex analysis, making it ideal for power quality monitoring, synchronized control systems, or any application requiring precise AC mains synchronization.

**Key concepts demonstrated:**
- IRAM_ATTR usage for minimal interrupt latency
- Professional ISR design patterns
- Advanced FreeRTOS queue management
- Real-time frequency and stability analysis
- Comprehensive performance monitoring
- Production-grade system diagnostics

## Key Differences: Arduino vs ESP-IDF

Understanding when to use Arduino versus ESP-IDF examples helps you choose the right approach for your project. The Arduino examples prioritize ease of use and rapid development. They hide complex implementation details while providing powerful functionality through simple APIs. This makes them ideal for prototypes, hobby projects, or applications where development speed is more important than maximum optimization.

The ESP-IDF examples provide direct hardware control and maximum performance. They expose the full capabilities of the ESP32 hardware and allow fine-grained control over system resources. The explicit error handling and structured approach make them suitable for commercial products where reliability and performance are critical. The detailed logging and monitoring capabilities help with debugging complex systems and ensuring long-term stability.

Both frameworks use the same underlying rbdimmerESP32 library, ensuring consistent dimmer control quality. The choice between them depends on your project requirements, development timeline, and performance needs. Many developers start with Arduino examples for rapid prototyping, then move to ESP-IDF for production deployment.

## Learning Path Recommendations

For beginners, we recommend starting with the Arduino BasicDimmer example to understand fundamental concepts. Progress through BasicTransition to learn about smooth effects, then explore MultiDimmer for complex systems. The Arduino ZCCallBack example prepares you for advanced concepts needed in professional applications.

For experienced embedded developers, you might start directly with the ESP-IDF examples. However, reviewing the Arduino examples can still be valuable for understanding the library's capabilities in a simplified context. The progression from BasicDimmer.c through MultiDimmer.c to ZCCallBack.c builds the knowledge needed for creating sophisticated AC control systems.

Regardless of your experience level, each example includes comprehensive documentation explaining not just how to use the library, but why specific approaches are taken. This educational approach helps you understand the underlying principles, enabling you to create custom solutions for your specific needs.

## Building and Running Examples

Each example directory contains its own README with specific build instructions. For Arduino examples, you'll need the Arduino IDE or PlatformIO with ESP32 board support. For ESP-IDF examples, you'll need the ESP-IDF development framework version 4.0 or higher. All examples are tested with common ESP32 development boards and RBDimmer AC dimmer modules.

## Safety Considerations

Working with AC mains voltage requires proper safety precautions. Always ensure proper electrical isolation between AC mains and your microcontroller. Use only properly designed dimmer modules with built-in isolation. Never attempt to connect AC mains directly to microcontroller pins. Test your circuits with appropriate safety equipment and consider using low-voltage AC sources during development.

## Support and Resources

- **Dimmers Website**: https://rbdimmer.com
- **Library Repository**: https://github.com/robotdyn-dimmer/rbdimmerESP32
- **Dimmers Catalog**: https://www.rbdimmer.com/dimmers-pricing
- **Hardware Documentation**: https://www.rbdimmer.com/knowledge/article/45
- **Library Documentation**: https://www.rbdimmer.com/knowledge/article/59
- **Example Projects**: https://www.rbdimmer.com/blog/dimmers-projects-5
- **Support Forum**: https://www.rbdimmer.com/forum

## Contributing

We welcome contributions to improve these examples. Whether you're fixing bugs, improving documentation, or adding new examples, please feel free to submit pull requests. Make sure your code follows the existing style and includes comprehensive documentation explaining your additions.

## Author

- **Developer**: dev@rbdimmer.com
- **Version**: 1.0.0
- **License**: MIT

---

*These examples are part of the rbdimmerESP32 library - Professional AC dimmer control for ESP32*