# ESP-IDF Multi-Channel Dimmer Control Example

This advanced example demonstrates how to control multiple independent AC dimmer channels using the rbdimmerESP32 library in the ESP-IDF environment, showcasing professional patterns for building scalable lighting control systems.

## Overview

Multi-channel dimmer control represents a significant step up in complexity from single-channel systems, introducing concepts essential for professional lighting applications. This ESP-IDF implementation demonstrates how to efficiently manage multiple dimmers using shared resources while maintaining independent control of each channel. The example showcases the architectural patterns that make it possible to build everything from simple two-zone home lighting to complex theatrical lighting systems with dozens of channels.

The power of this approach lies in its efficiency and scalability. By sharing a single zero-cross detector among all channels on the same AC phase, we reduce hardware complexity and ensure perfect synchronization. Each channel maintains its own control parameters, including brightness level and curve type, allowing you to optimize control for different load types. This architectural approach scales gracefully - the same patterns work whether you're controlling two channels or twenty.

## Understanding Multi-Channel Architecture

The architecture of a multi-channel dimmer system requires careful consideration of several factors. At the hardware level, all dimmers on the same AC phase can share a single zero-cross detector. This is possible because the zero-crossing occurs at the same instant for all loads on the same phase. The rbdimmerESP32 library leverages this fact to minimize resource usage while maintaining precise control.

From a software perspective, the system uses an array-based architecture that makes channel management straightforward. Each channel is represented by a structure containing its configuration, current state, and handle to the library's internal resources. This approach provides several advantages. First, it makes the code easily extensible - adding more channels is as simple as expanding the array. Second, it allows for efficient iteration over all channels for operations like status reporting or synchronized control. Third, it maintains clean separation between channel-specific and system-wide operations.

The interrupt handling in multi-channel systems deserves special attention. When a zero-crossing occurs, the single interrupt handler must quickly schedule timer events for all active channels. The library handles this efficiently by pre-calculating delay values and using hardware timers to ensure accurate triggering regardless of the number of channels. This approach maintains microsecond-precision timing even with many channels active simultaneously.

## Load-Specific Optimization

One of the key features demonstrated in this example is the use of different brightness curves for different load types. This optimization is crucial for achieving professional-quality lighting control. Incandescent bulbs and LED lights have fundamentally different electrical characteristics that affect how they respond to phase-angle dimming.

Incandescent bulbs are purely resistive loads. Their brightness is directly related to the RMS (Root Mean Square) power delivered to the filament. The RMS curve in the library compensates for the non-linear relationship between phase angle and RMS power, ensuring that a 50% setting delivers approximately 50% of the perceived brightness. This creates smooth, predictable dimming that users expect from traditional lighting.

LED loads present a different challenge. Most dimmable LED drivers include electronic circuits that don't respond linearly to phase-angle control. The logarithmic curve compensates for this non-linearity, but more importantly, it matches the human eye's logarithmic response to brightness. This double compensation results in dimming that feels natural and smooth to users, avoiding the common problem of LEDs that seem to jump from dim to bright with little control in between.

## Hardware Requirements

Building a multi-channel system requires careful attention to hardware selection and configuration:

- **ESP32 Board**: Any ESP32 variant with sufficient GPIO pins for your channel count
- **Development Environment**: ESP-IDF v4.0 or higher properly configured
- **AC Dimmer Modules**: One RBDimmer module per channel (all must be rated for your AC voltage)
- **Zero-Cross Detection**: Can be shared among all modules on the same phase
- **AC Loads**: Mix of load types to demonstrate curve optimization:
  - Incandescent bulbs (40-100W) for smooth, classic dimming
  - Dimmable LED bulbs or drivers for modern, efficient lighting
- **Power Supply**: Ensure adequate current for all ESP32 and dimmer modules
- **Safety Equipment**: Proper enclosure and isolation for AC components

## Wiring Connections

The wiring for multi-channel systems requires careful attention to signal routing and power distribution:

| Signal Type | ESP32 Pin | Dimmer Connection | Notes |
|------------|-----------|-------------------|--------|
| Zero-Cross | GPIO 18 | All ZC pins | Connect all ZC outputs together |
| Channel 1 | GPIO 19 | Dimmer 1 PWM | Incandescent load |
| Channel 2 | GPIO 21 | Dimmer 2 PWM | LED load |
| Power | 3.3V | All VCC pins | Ensure adequate current capacity |
| Ground | GND | All GND pins | Star ground configuration recommended |

⚠️ **Critical Safety Notes**:
- All dimmers must be on the same AC phase for shared zero-cross detection
- Maintain proper isolation between AC mains and control circuits
- Use appropriate wire gauge for the total load current
- Consider thermal management for multiple high-power loads

## Building and Configuration

The ESP-IDF build system provides powerful configuration options for customizing the multi-channel system:

```bash
# Configure your ESP-IDF environment
. ~/esp/esp-idf/export.sh

# Navigate to the example
cd examples/esp_idf/multi_dimmer

# Configure the project
idf.py menuconfig
```

In menuconfig, you can adjust several parameters that affect multi-channel operation. Under the RBDimmer configuration menu, you can set the maximum number of channels supported by the library. This affects memory allocation, so set it to match your largest expected configuration. You can also adjust timer parameters, though the defaults work well for most applications.

```bash
# Build the project
idf.py build

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

## Expected Console Output

The multi-channel system provides detailed logging to help understand its operation:

```
I (325) DIMMER_EXAMPLE: === RBDimmer Multi-Channel ESP-IDF Example ===
I (335) RBDIMMER: RBDimmer library initialized
I (345) DIMMER_EXAMPLE: Registering shared zero-cross detector...
I (355) RBDIMMER: Zero-cross detector registered on pin 18 for phase 0
I (365) DIMMER_EXAMPLE: Creating channel 1 (Main Light)...
I (375) RBDIMMER: Dimmer channel created on pin 19, phase 0
I (385) DIMMER_EXAMPLE: Channel 1 created: Incandescent on pin 19 (RMS curve)
I (395) DIMMER_EXAMPLE: Creating channel 2 (Accent LED)...
I (405) RBDIMMER: Dimmer channel created on pin 21, phase 0
I (415) DIMMER_EXAMPLE: Channel 2 created: Dimmable LED on pin 21 (Logarithmic curve)
I (425) DIMMER_EXAMPLE: Multi-channel system initialized successfully
I (935) DIMMER_EXAMPLE: Detected frequency: 50 Hz

========== Multi-Channel System Status ==========
System Info:
  Mains frequency: 50 Hz
  Active channels: 2
  Free heap: 287436 bytes

Channel Status:
  Channel 1 - Main Light:
    Load type: Incandescent
    GPIO pin: 19
    Current level: 50%
    Active: Yes
    Curve: RMS
    Delay: 5000 us
  Channel 2 - Accent LED:
    Load type: Dimmable LED
    GPIO pin: 21
    Current level: 50%
    Active: Yes
    Curve: Logarithmic
    Delay: 4750 us
```

## Demonstration Patterns Explained

The example includes several demonstration patterns that showcase different aspects of multi-channel control. Understanding these patterns helps you design your own lighting effects and control strategies.

The **Alternating Pattern** demonstrates complementary control, where one channel brightens as another dims. This maintains relatively constant total light output while creating visual interest. It's useful for accent lighting that draws attention without dramatically changing overall illumination levels.

The **Synchronized Control** pattern shows how multiple channels can work in unison. This is essential for applications where uniform lighting changes are needed, such as dimming an entire room or creating breathing effects. The synchronized approach ensures all channels change together without phase differences that could create visible flicker.

The **Cross-Fade Transitions** demonstrate smooth, simultaneous transitions on multiple channels. This pattern is crucial for scene changes in theatrical or architectural lighting. By transitioning multiple channels with different durations or directions, you can create complex lighting choreography that would be impossible with single-channel control.

The **Scene Presets** pattern shows how to implement pre-defined lighting configurations. This is perhaps the most practical pattern for real-world applications. Users can define scenes like "Dinner," "Movie," or "Reading," each with specific brightness levels for each channel. Smooth transitions between scenes create a professional feel that users appreciate.

## Performance and Scalability

Multi-channel systems must carefully manage resources to maintain performance. The ESP32's dual-core architecture can be leveraged effectively, with dimmer control running on one core and user interface or network communication on the other. The example demonstrates efficient patterns that scale well.

Memory usage scales linearly with channel count. Each channel requires approximately 100-120 bytes of RAM for the library's internal structures, plus any application-specific data you add. For an 8-channel system, total memory usage remains under 1KB, leaving plenty of room for other application features.

Timer resources are the primary limiting factor. Each channel requires two hardware timers - one for the turn-on delay and one for the pulse duration. The ESP32 has sufficient timers for 8-10 channels, depending on what other peripherals you're using. The library manages these efficiently, but be aware of potential conflicts with other timer-based features.

CPU usage remains low even with many channels. The zero-cross interrupt handler executes in under 10 microseconds, even with 8 channels active. The smooth transition tasks use minimal CPU time, allowing complex effects without impacting other system operations.

## Integration Strategies

The modular structure of this example makes it easy to integrate with other ESP-IDF components. Here are some common integration patterns that extend the multi-channel dimmer into complete systems.

For WiFi control, you can add the ESP-IDF WiFi component and create a web server or MQTT client. The non-blocking nature of the dimmer library ensures network operations don't affect dimming quality. You might create RESTful endpoints for channel control or subscribe to MQTT topics for each channel.

Bluetooth integration allows for smartphone app control. The ESP32's Bluetooth LE support is particularly useful for creating custom mobile apps. You can expose each channel as a BLE characteristic, allowing fine-grained control and status monitoring from any BLE-capable device.

For sensor integration, the multi-channel architecture makes it easy to create responsive lighting. Motion sensors can trigger specific channels, while ambient light sensors can adjust overall brightness. The example's scene system provides a framework for sensor-triggered scene changes.

## Troubleshooting Multi-Channel Systems

Multi-channel systems introduce additional complexity that can lead to specific issues. Here are common problems and their solutions.

If channels appear to interfere with each other, verify that all dimmers are on the same AC phase. Phase differences will cause timing conflicts. Also check for ground loops - use a star grounding configuration where all grounds connect at a single point.

Flickering across all channels usually indicates power supply issues. Ensure your 3.3V supply can provide enough current for all dimmer modules. Each module typically draws 20-30mA, so a 4-channel system needs at least 120mA plus ESP32 requirements.

If smooth transitions seem jerky, you may be hitting FreeRTOS task limits. Check the task stack size and priority settings. The default 2048-byte stack should be sufficient, but complex applications might need more.

## Advanced Applications

Once you've mastered the basic multi-channel patterns, consider these advanced applications that showcase the system's full potential.

**Dynamic Scene Generation**: Instead of fixed scenes, calculate channel levels based on time of day, occupancy, or other factors. This creates lighting that adapts to usage patterns without manual intervention.

**Music Synchronization**: Use the ESP32's ADC to sample audio and create lighting effects synchronized to music. Different channels can respond to different frequency bands, creating rich visual displays.

**Distributed Systems**: Multiple ESP32 boards can coordinate over WiFi or ESP-NOW to create building-wide lighting control. The zero-cross synchronization ensures all boards maintain phase alignment.

**Energy Monitoring**: Add current sensing to track power consumption per channel. This data can optimize usage patterns and provide insights into energy savings from LED conversion.

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

After mastering multi-channel control, explore the callback example to understand advanced interrupt handling and synchronization. Consider building a complete lighting control system with web interface, scheduled scenes, and sensor integration. The patterns demonstrated here provide the foundation for professional-grade lighting control applications.

---

*This example is part of the rbdimmerESP32 library - Professional AC dimmer control for ESP32*