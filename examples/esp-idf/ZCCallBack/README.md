# ESP-IDF Zero-Cross Callback Advanced Example

This sophisticated example demonstrates professional interrupt handling and real-time synchronization patterns using the rbdimmerESP32 library's callback system in a pure ESP-IDF environment, providing the foundation for building systems that require microsecond-precision synchronization with AC mains.

## Overview

The zero-cross callback example represents the pinnacle of real-time control using the rbdimmerESP32 library. It demonstrates how to properly handle hardware interrupts in the ESP32 environment while maintaining the strict timing requirements necessary for AC power control. This example goes beyond simple dimming to show how you can tap into the precise timing of AC mains for advanced applications ranging from power quality monitoring to synchronized multi-device control systems.

Understanding and implementing proper interrupt handling is crucial for professional embedded systems development. The ESP32, like most modern microcontrollers, has specific requirements and limitations for interrupt service routines (ISRs). Code running in interrupt context must be fast, cannot use most system functions, and must be placed in internal RAM for consistent timing. This example demonstrates all these concepts with production-quality code that maintains microsecond precision while performing complex analysis.

The three-layer architecture demonstrated here - hardware interrupts, minimal ISR processing, and deferred task handling - is a fundamental pattern in real-time embedded systems. By separating time-critical operations from complex processing, we achieve both the precision required for AC control and the flexibility needed for sophisticated applications. This pattern appears in everything from motor control to telecommunications systems, making it an essential skill for embedded developers.

## Understanding Interrupt Context in ESP-IDF

Working with interrupts in ESP-IDF requires understanding several critical concepts that differ from normal programming. When a hardware event occurs (in our case, the AC voltage crossing zero), the processor immediately suspends whatever it was doing and jumps to the interrupt handler. This happens with minimal delay - typically under 2 microseconds on the ESP32 when the handler is in IRAM.

However, this immediate response comes with strict limitations. The interrupt handler runs with interrupts disabled, meaning no other interrupts can occur while it's executing. This makes every microsecond count - spending too long in the ISR can cause other time-critical events to be missed. The ESP-IDF documentation recommends keeping ISR execution time under 10 microseconds for optimal system performance.

The IRAM_ATTR attribute is crucial for interrupt handlers. By default, ESP32 code runs from external flash memory, which requires cache access and can introduce variable latency. Interrupt handlers marked with IRAM_ATTR are placed in internal RAM, ensuring consistent, fast execution. This uses precious IRAM space (only 128KB available for user code), but it's essential for maintaining timing precision.

Inside the ISR, you're limited to a small subset of functions. Most ESP-IDF and FreeRTOS functions are not interrupt-safe and will cause crashes or undefined behavior if called from an ISR. The safe functions are specifically marked with "FromISR" suffixes, like xQueueSendFromISR. These functions are designed to work with interrupts disabled and include special handling for task scheduling.

## The Architecture Deep Dive

The example implements a sophisticated multi-layer architecture that separates concerns while maintaining real-time performance. At the hardware layer, the zero-cross detector in the dimmer module generates a pulse every time the AC voltage crosses zero. This happens 100 times per second for 50Hz mains or 120 times per second for 60Hz mains. The ESP32's GPIO peripheral detects the rising edge of this pulse and triggers an interrupt.

The interrupt service routine (zero_cross_callback) represents the second layer. Its job is minimal but critical: capture the exact time of the event, calculate the period since the last event, and pass this information to the processing layer. The ISR also toggles an LED for visual feedback - this direct GPIO manipulation is safe and fast in interrupt context. Importantly, the ISR never blocks or waits. If the communication queue is full, it simply increments an overflow counter and continues. This ensures the ISR always completes quickly, maintaining system timing integrity.

The FreeRTOS queue serves as the communication channel between layers. Queues are thread-safe and interrupt-safe when used with the proper FromISR functions. The queue is pre-allocated with a fixed size, eliminating any dynamic memory operations. This is crucial because memory allocation is never allowed in interrupt context. The queue size (20 events in this example) provides buffering for brief periods when the processing task might be delayed by other system activities.

The processing task (zero_cross_processing_task) represents the third layer where complex operations occur. Running as a normal FreeRTOS task, it's free from the restrictions of interrupt context. Here we can perform floating-point calculations, update statistics, log to the console, or even send data over a network. The task blocks on the queue, consuming zero CPU time when there are no events to process. This efficient design allows the system to handle other tasks while maintaining precise zero-cross tracking.

## Real-Time Frequency Analysis

One of the most powerful features demonstrated in this example is real-time frequency analysis. By measuring the time between zero-crossings, we can determine the exact frequency of the AC mains with high precision. This has numerous practical applications beyond simple dimmer control.

The frequency measurement works by capturing microsecond-precision timestamps at each zero-crossing. The period between events directly relates to the mains frequency: for 50Hz mains, zero-crossings occur every 10 milliseconds (100 times per second), while 60Hz mains produces zero-crossings every 8.33 milliseconds (120 times per second). By averaging multiple measurements, we can achieve frequency accuracy better than 0.01Hz.

The stability analysis goes beyond simple frequency measurement to characterize the quality of the AC power. By tracking minimum and maximum periods, we can calculate the percentage variation in frequency. High-quality power systems maintain frequency within ±0.1%, while poor quality or overloaded systems might show variations exceeding 1%. This information is valuable for sensitive equipment that requires stable power.

The example also demonstrates how to identify the mains standard (50Hz or 60Hz) automatically. By comparing the measured frequency to the nominal standards, the system can adapt its operation accordingly. This is particularly useful for products that must work worldwide without manual configuration.

## Performance Monitoring and Optimization

The example includes comprehensive performance monitoring to help you understand and optimize system behavior. These metrics are essential for building reliable production systems that must operate continuously without degradation.

Queue monitoring reveals whether the processing task is keeping up with interrupt events. The queue depth shows how many events are waiting to be processed. If this number grows continuously, it indicates the processing task is too slow or being blocked by other operations. The overflow counter tracks events lost due to a full queue - any non-zero value here indicates a system design issue that needs addressing.

Task stack monitoring helps optimize memory usage. FreeRTOS allocates a fixed stack for each task, and running out of stack space causes immediate system crashes. The stack high water mark shows the minimum free stack space observed, allowing you to reduce the allocation if there's excess or increase it if you're running close to the limit.

Timing precision monitoring through the stability percentage gives insight into both the quality of the AC power and the accuracy of your interrupt handling. Variations beyond ±0.5% might indicate electrical noise, grounding issues, or software problems causing interrupt latency.

## Hardware Requirements and Setup

Building this advanced example requires careful attention to hardware configuration:

- **ESP32 Development Board**: Any variant, but modules with good power supply filtering perform better
- **RBDimmer Module**: Must include zero-cross detection output
- **Test Load**: Incandescent bulb (60-100W) provides stable, visible dimming
- **Oscilloscope** (optional but recommended): For verifying timing precision
- **Logic Analyzer** (optional): For detailed interrupt timing analysis
- **Stable Power Supply**: Both for ESP32 and AC mains - unstable power affects measurements

## Wiring Connections

| ESP32 Pin | Connection | Function | Notes |
|-----------|------------|----------|-------|
| GPIO 18 | Dimmer ZC | Zero-cross input | Must support interrupts |
| GPIO 19 | Dimmer PWM | Control output | Any GPIO |
| GPIO 2 | Built-in LED | Visual indicator | Usually on-board |
| 3.3V | Dimmer VCC | Power supply | Stable 3.3V required |
| GND | Dimmer GND | Ground reference | Star ground recommended |

⚠️ **Critical Safety Notes**:
- The zero-cross signal must be properly isolated from AC mains
- Never connect AC voltage directly to ESP32 pins
- Verify isolation with a multimeter before powering the system
- Use optical isolation for maximum safety in production designs

## Building and Advanced Configuration

The ESP-IDF build system provides extensive configuration options for optimizing the callback system:

```bash
# Enter the ESP-IDF environment
. ~/esp/esp-idf/export.sh

# Navigate to example directory
cd examples/esp_idf/zc_callback

# Configure the project
idf.py menuconfig
```

In menuconfig, navigate to Component config → FreeRTOS to adjust critical parameters. The tick rate (default 100Hz) affects timing granularity. Higher rates provide better resolution but increase overhead. For precision applications, consider increasing to 1000Hz. The interrupt stack size affects how much stack space is available for ISRs. The default is usually sufficient, but complex ISRs might need more.

Under Component config → ESP32-specific, you can configure interrupt allocation. The interrupt watchdog timeout determines how long an ISR can run before triggering a system reset. For debugging, you might increase this, but production systems should use the default to catch runaway ISRs.

## Expected Console Output Explained

Understanding the console output helps verify correct operation and diagnose issues:

```
I (405) DIMMER_CALLBACK: [Task] Zero-cross processing task started
```
This confirms the FreeRTOS task started successfully and is ready to process events.

```
I (1405) DIMMER_CALLBACK: Detected frequency: 50 Hz
I (1405) DIMMER_CALLBACK: Zero-cross count: 100 (last second)
```
Shows the system correctly detecting 50Hz mains with exactly 100 zero-crossings per second.

```
I (1415) DIMMER_CALLBACK: Period stability: ±0.15%
```
Indicates excellent power quality with minimal frequency variation.

```
I (2405) DIMMER_CALLBACK: [Task] Processed event #200, period: 10.01 ms
```
Detailed event logging shows precise timing measurements.

## Advanced Applications

The callback system enables sophisticated applications beyond simple dimming:

**Power Quality Monitoring**: By analyzing frequency stability, you can detect grid problems, overload conditions, or failing generators. The data can be logged for long-term analysis or trigger alarms for critical deviations.

**Phase-Locked Systems**: Multiple devices can synchronize their operation to the AC mains phase. This is essential for avoiding beat frequencies in lighting systems or ensuring multiple motors run in sync.

**Harmonic Analysis**: With faster sampling and FFT processing in the task layer, you can analyze harmonic content of the AC waveform, useful for detecting problematic loads or power quality issues.

**Precision Timing**: The AC mains provides a long-term stable time reference. By counting zero-crossings, you can implement accurate clocks that don't drift over extended periods.

**Energy Monitoring**: Combined with current sensing, zero-cross timing enables accurate power factor measurement and energy consumption tracking.

## Integration Strategies

The callback system integrates well with other ESP-IDF components:

**WiFi Reporting**: The processing task can send frequency and stability data to cloud services for remote monitoring. The non-blocking architecture ensures network operations don't affect timing precision.

**SD Card Logging**: Long-term data logging to SD cards allows detailed power quality analysis. The task-based architecture prevents SD card operations from affecting interrupt timing.

**Display Systems**: Real-time frequency and stability can be shown on local displays. The regular statistics updates provide perfect timing for display refresh.

**Modbus Integration**: Industrial systems often use Modbus for communication. The callback data can be exposed via Modbus registers for integration with PLCs and SCADA systems.

## Troubleshooting Guide

When the LED doesn't flash at the expected rate, check the zero-cross signal with an oscilloscope. Verify you see clean pulses at twice the mains frequency. Check that the GPIO pin configuration matches your hardware. Some ESP32 pins have restrictions on interrupt capability.

If you see frequency measurement errors or instability, examine your grounding. Ground loops between the AC side and DC side can inject noise. Use star grounding with a single point connection. Check for nearby sources of electromagnetic interference. Motors, switching power supplies, and fluorescent lights can inject noise.

Queue overflow warnings indicate the processing task isn't keeping up. Reduce the complexity of processing in the task, increase the task priority, or increase the queue size. Check for other high-priority tasks that might be blocking execution.

System crashes or resets often indicate ISR violations. Review the callback function for any non-ISR-safe operations. Verify all ISR functions have IRAM_ATTR. Check stack usage - ISRs use the interrupt stack, which is limited.

## Performance Tuning

For maximum performance, place all interrupt-related code in IRAM. This includes the callback function and any functions it calls. Monitor IRAM usage with the size command to ensure you're not exceeding limits.

Minimize ISR execution time by deferring all non-critical operations. Even simple floating-point operations should be avoided in the ISR. Use integer arithmetic where possible.

Optimize queue sizes based on your system's worst-case latency. If other tasks might delay processing for 100ms, ensure your queue can buffer 10-12 events (for 50Hz systems).

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

## Conclusion

This example demonstrates professional-grade interrupt handling and real-time synchronization techniques essential for advanced embedded systems. The patterns shown here - IRAM-safe ISRs, FreeRTOS queue communication, and comprehensive monitoring - form the foundation for building reliable, precise control systems. Whether you're developing power quality monitors, synchronized lighting systems, or industrial control applications, these techniques ensure your system maintains microsecond precision while performing complex processing tasks.

---

*This example is part of the rbdimmerESP32 library - Professional AC dimmer control for ESP32*