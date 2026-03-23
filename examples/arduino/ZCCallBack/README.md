# Zero-Cross Callback Advanced Example

This advanced example demonstrates professional interrupt handling and real-time synchronization techniques using the rbdimmerESP32 library's callback system, showcasing how to properly integrate with FreeRTOS for complex AC mains synchronized applications.

## Overview

The ZCCallBack example represents the most sophisticated use of the rbdimmerESP32 library, demonstrating how to tap into the zero-cross detection system for advanced synchronization needs. This example goes beyond simple dimming control to show how you can build complex systems that need precise timing synchronized with AC mains frequency.

Understanding zero-cross callbacks opens up a world of possibilities for creating professional-grade systems. Whether you're building industrial control systems, creating flicker-free environments for video production, or developing advanced lighting effects, this example provides the foundation for reliable, real-time synchronization with AC mains.

## What This Example Teaches

This example introduces several critical concepts for embedded systems development. First and foremost, it demonstrates proper interrupt service routine (ISR) handling in the ESP32 environment. ISRs are special functions that run when hardware events occur, and they have strict requirements - they must be fast, cannot use certain functions, and must not block. This example shows the correct way to handle these constraints.

The example also showcases the proper integration pattern between interrupts and FreeRTOS, the real-time operating system running on ESP32. When you need to do complex processing based on an interrupt event, you cannot do it directly in the ISR. Instead, this example demonstrates the standard pattern of using FreeRTOS queues to safely pass data from the ISR to a regular task where complex processing can occur.

Additionally, the example provides practical frequency measurement and stability monitoring. By analyzing the timing between zero-cross events, the system can detect power quality issues, measure exact mains frequency, and monitor for anomalies that might affect sensitive equipment.

## Understanding Zero-Cross Detection

Zero-cross detection is fundamental to AC power control. In alternating current systems, the voltage continuously changes in a sine wave pattern, crossing through zero volts twice per cycle. These zero-crossing points are critical for several reasons.

From a safety perspective, TRIACs and other semiconductor switches can only be reliably triggered when the voltage is low, making the zero-cross point ideal for initiating conduction. From a control perspective, the zero-cross provides a consistent timing reference that allows precise control of power delivery through phase-angle control. From a synchronization perspective, the zero-cross events occur at exactly twice the mains frequency, providing a stable clock reference for timing-critical applications.

## Hardware Requirements

- **ESP32 Board**: Any ESP32 variant with available GPIO pins
- **AC Dimmer Module**: RBDimmerESP32 module with zero-cross detection output
- **AC Load**: Incandescent bulb (60-100W) for dimming demonstration
- **Visual Indicator**: Built-in LED on GPIO 2 (standard on most ESP32 boards)
- **Safety Equipment**: Proper isolation and protection for AC mains connection

## Wiring Connections

| ESP32 Pin | Dimmer Module | Description |
|-----------|---------------|-------------|
| GPIO 18   | ZC (Zero Cross) | Zero-crossing detection signal input |
| GPIO 19   | PWM/CTRL | Dimmer control signal output |
| GPIO 2    | Built-in LED | Visual zero-cross indicator |
| 3.3V      | VCC | Module power supply |
| GND       | GND | Common ground reference |

⚠️ **Critical Safety Note**: The zero-cross signal must be properly isolated from AC mains voltage. The RBDimmerESP32 modules include this isolation, but always verify before connecting!

## Expected Serial Output

```
=== RBDimmerESP32 Zero-Cross Callback Example ===
LED initialized for zero-cross visualization
FreeRTOS queue created
Processing task created
Dimmer library initialized
Zero-cross detector registered
Callback registered successfully
Dimmer channel created at 60% brightness

Setup complete! Monitoring zero-cross events...
Watch the LED - it should flash at mains frequency

[Task] Zero-cross processing task started
[Task] Processing event #100
[Task] Timestamp: 1000 ms
[Task] Period: 10.00 ms
[Task] Instantaneous frequency: 50.00 Hz

========== Zero-Cross Statistics ==========
Library measured frequency: 50 Hz
Zero-crosses per second: 100
Total zero-crosses: 523

Period Statistics:
  Average: 10.00 ms (50.00 Hz)
  Minimum: 9.98 ms
  Maximum: 10.02 ms
  Stability: ±0.20%

Dimmer Status:
  Brightness: 60%
  Active: Yes

System Health:
  Queue spaces used: 0/10
  Task stack high water mark: 3200 bytes
  Free heap: 275432 bytes
==========================================
```

## The Architecture Explained

This example implements a sophisticated multi-layer architecture that separates time-critical operations from complex processing. Understanding this architecture is crucial for building reliable real-time systems.

At the hardware level, the zero-cross detector generates an interrupt on every zero-crossing of the AC waveform. This happens 100 times per second for 50Hz mains or 120 times per second for 60Hz mains. Each interrupt must be handled quickly to avoid missing subsequent events.

The interrupt service routine (ISR) layer handles these hardware interrupts. The `zeroCrossCallback` function runs in this context with several restrictions. It cannot call most ESP32 or Arduino functions, cannot perform lengthy calculations, and must complete as quickly as possible. In this example, the ISR performs only essential tasks: capturing timestamps, updating counters, and sending event data to a queue.

The FreeRTOS queue serves as a safe communication channel between the ISR and normal program tasks. Queues are thread-safe and interrupt-safe, making them perfect for this application. The ISR can quickly add events to the queue and return, while a separate task processes these events at its own pace.

The processing task layer is where complex operations occur. The `zeroCrossProcessingTask` runs as a normal FreeRTOS task, free from the restrictions of interrupt context. It can perform calculations, call any functions, communicate over networks, or update displays. This separation ensures that complex processing never interferes with the time-critical interrupt handling.

## Understanding ISR Safety

Writing safe interrupt service routines requires understanding several critical concepts. The ESP32 documentation specifies which functions are ISR-safe, and using unsafe functions can cause system crashes or unpredictable behavior.

Memory allocation is never allowed in ISRs, meaning no dynamic memory operations like malloc or new. Blocking operations are forbidden, including delay functions, mutex operations, or waiting for events. Many standard library functions are unsafe in interrupt context, particularly those involving floating-point math or string manipulation.

To ensure ISR code runs quickly, the IRAM_ATTR attribute places the function in internal RAM rather than slower flash memory. This is crucial for maintaining consistent timing. The example demonstrates all these safety practices in the `zeroCrossCallback` function.

## FreeRTOS Integration Patterns

The example showcases the standard pattern for deferring work from ISRs to tasks using FreeRTOS queues. This pattern is fundamental to many embedded systems and appears in various forms throughout professional embedded development.

The queue is created with a specific size (10 events in this example) and element size (sizeof(ZeroCrossEvent_t)). This pre-allocation ensures no dynamic memory operations occur during runtime. The ISR uses `xQueueSendFromISR` to add events, which includes special handling for potentially waking higher-priority tasks. The processing task uses `xQueueReceive` with portMAX_DELAY to block until events arrive, ensuring efficient CPU usage.

This pattern allows the system to handle brief bursts of events without loss while maintaining responsive operation. If the queue fills up, it indicates the processing task cannot keep up with events, which the example monitors and reports.

## Practical Applications

Understanding zero-cross callbacks enables numerous advanced applications. In power quality monitoring, you can detect frequency variations, voltage sags, or other anomalies by analyzing zero-cross timing patterns. For video production, synchronizing lighting changes to mains frequency eliminates visible flicker in recordings. Industrial control systems use zero-cross synchronization to coordinate multiple devices, ensuring they operate in phase with each other.

The callback system also enables advanced lighting effects. Strobe effects can be precisely timed to mains frequency multiples. Music visualization can synchronize to the mains frequency to avoid beat frequency interference. Multi-room lighting systems can maintain perfect synchronization across all zones.

## Performance Considerations

The example includes comprehensive performance monitoring to help you understand system behavior. The statistics show interrupt frequency, timing stability, queue usage, and task stack usage. These metrics help identify potential issues before they become problems.

Timing jitter measurement reveals power quality issues or system load problems. Queue depth monitoring ensures the processing task keeps up with events. Stack usage tracking prevents stack overflow issues. Memory monitoring helps identify memory leaks or fragmentation.

## Troubleshooting Guide

Common issues and their solutions include the LED not flashing (check zero-cross wiring, verify the GPIO pin is correct, and ensure the zero-cross signal is reaching the ESP32), irregular LED flashing indicating timing problems (check for electrical noise, verify proper grounding, and look for other interrupts interfering), queue overflow warnings suggesting processing is too slow (reduce processing complexity, increase task priority, or increase queue size), and system crashes or resets often caused by ISR safety violations (review callback code for unsafe operations, check for stack overflow, and verify all ISR functions have IRAM_ATTR).

## Extending the Example

This example provides a foundation for building more complex systems. Consider adding network reporting of frequency data for remote monitoring, implementing phase-locked loops for precise frequency tracking, creating multi-channel synchronization for complex effects, or building power quality analysis with harmonic detection.

## Safety Considerations

Working with AC mains synchronization requires extra safety attention. Always use properly isolated interfaces between AC mains and your microcontroller. Never attempt to measure AC voltage directly with microcontroller pins. Ensure all connections are properly insulated and enclosed. Consider adding optical isolation for critical applications. Test with low-voltage AC sources when possible during development.

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

## Advanced Topics to Explore

After mastering zero-cross callbacks, consider exploring phase-locked loop implementation for ultra-precise frequency tracking, building distributed systems with synchronized operations across multiple ESP32 devices, implementing custom protocol handlers triggered by zero-cross timing, creating advanced diagnostic tools for power quality analysis, or developing fail-safe systems that detect and respond to power anomalies.

---

*This example is part of the rbdimmerESP32 library - Professional AC dimmer control for ESP32*