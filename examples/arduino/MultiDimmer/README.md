# Multiple Dimmer Channels Control Example

This advanced example demonstrates how to control multiple independent AC dimmer channels simultaneously using the rbdimmerESP32 library, showcasing the foundation for sophisticated lighting control systems.

## Overview

The MultiDimmer example represents a significant step up in complexity from single-channel control, demonstrating how the rbdimmerESP32 library can manage multiple dimmers efficiently using shared resources. This example shows how a single zero-cross detector can synchronize multiple dimmer channels, each controlling different loads with optimized settings.

The power of multi-channel control lies in its ability to create complex lighting scenes and effects that would be impossible with a single dimmer. By coordinating multiple channels, you can create layered lighting environments, dynamic effects, and energy-efficient zone control systems.

## What This Example Demonstrates

This example cycles through five different lighting effects, each showcasing a different aspect of multi-channel control. The effects demonstrate both synchronized and independent channel operation, showing the flexibility of the system.

The **Alternating Effect** creates a complementary lighting pattern where one channel brightens as the other dims, maintaining overall light levels while creating visual interest. The **Synchronized Pulse** effect demonstrates how multiple channels can work in perfect harmony, creating a unified breathing effect across all connected lights. The **Chase Effect** shows sequential control, with light appearing to move from one channel to the next. The **Cross-Fade Effect** smoothly transitions between different lighting scenes, demonstrating how multi-channel systems can create different moods or environments. Finally, the **Random Effect** shows completely independent operation, with each channel changing to random levels at random times.

## Hardware Requirements

- **ESP32 Board**: Any ESP32 variant with sufficient GPIO pins
- **AC Dimmer Modules**: 2 or more RBDimmer modules (example uses 2)
- **AC Loads**: 
  - Channel 1: Incandescent bulb (60-100W)
  - Channel 2: Dimmable LED bulb or LED strip with dimmable driver
- **Zero-Cross Detector**: Single detector shared by all channels
- **Power Supply**: Adequate power for ESP32 and all dimmer modules

## Wiring Connections

The key insight for multi-channel systems is that all dimmers can share a single zero-cross detector, simplifying wiring and ensuring perfect synchronization:

| ESP32 Pin | Connection | Description |
|-----------|------------|-------------|
| GPIO 18   | All ZC pins | Shared zero-cross detection (connect all ZC pins together) |
| GPIO 19   | Dimmer 1 PWM | Control signal for channel 1 |
| GPIO 21   | Dimmer 2 PWM | Control signal for channel 2 |
| 3.3V      | All VCC pins | Power for all dimmer modules |
| GND       | All GND pins | Common ground |

⚠️ **Important**: When connecting multiple dimmers to the same zero-cross signal, ensure they're all on the same AC phase!

## Expected Serial Output

```
=== RBDimmer Multi-Channel Example ===
Library initialized successfully
Registering zero-cross detector on pin 18...
Zero-cross detector registered for phase 0
Creating channel 1 (Room Light)...
Channel 1 created (Incandescent, RMS curve)
Creating channel 2 (Accent LED)...
Channel 2 created (LED, Logarithmic curve)
Detected mains frequency: 50 Hz

Starting multi-channel demonstration...

=== Switching to effect 1 ===
Alternating: Channel 1 bright, Channel 2 dim
Alternating: Channel 1 dim, Channel 2 bright

--- Channel Status ---
Room Light: 20%
Accent LED: 80%
--------------------

=== Switching to effect 2 ===
Synchronized pulse: All channels breathing together
```

## Understanding Multi-Channel Architecture

The rbdimmerESP32 library implements an efficient architecture for multi-channel control. All channels on the same AC phase share a single zero-cross detector, which triggers synchronized timing for all associated dimmers. This approach has several advantages.

Resource efficiency is maximized because multiple channels share the same interrupt handler and timing reference, reducing CPU overhead. Perfect synchronization is achieved since all channels receive the same zero-crossing reference, ensuring they operate in phase with each other. The simplified wiring reduces complexity and potential points of failure in the system. Scalability is built-in, allowing you to add channels up to the library's configured maximum without changing the fundamental architecture.

## Load-Specific Optimization

One of the key features demonstrated in this example is the use of different brightness curves for different load types. The example configures Channel 1 with an RMS curve for incandescent bulbs and Channel 2 with a logarithmic curve for LED loads.

Incandescent bulbs respond well to RMS-compensated curves because they are purely resistive loads. The RMS curve ensures that perceived brightness changes linearly with the control percentage. LED loads, on the other hand, often have non-linear response characteristics due to their drivers. The logarithmic curve compensates for this, providing more natural-feeling brightness control.

## Creating Custom Effects

The example includes five pre-programmed effects, but the real power comes from creating your own. Consider these possibilities:

**Scene-based lighting** can be created by defining preset levels for each channel and smoothly transitioning between them. For example, a "Movie" scene might dim the room lights to 20% while keeping accent lights at 5%, while a "Reading" scene brings room lights to 70% with accent lights at 40%.

**Time-based automation** can adjust lighting throughout the day. Morning routines might gradually increase all channels over 30 minutes to simulate sunrise. Evening routines could slowly dim lights to prepare for sleep.

**Reactive lighting** can respond to sensors or user input. Motion sensors could trigger specific channels, while ambient light sensors adjust indoor lighting to maintain consistent illumination regardless of outdoor conditions.

## Expanding the System

The example uses two channels, but the library supports up to RBDIMMER_MAX_CHANNELS (default 8). To add more channels, simply define additional pins and expand the arrays. The modular approach makes scaling straightforward.

When planning larger systems, consider grouping channels logically. Room-based grouping puts all lights in a room on sequential channels. Function-based grouping organizes by purpose (task lighting, accent lighting, etc.). Zone-based grouping divides large spaces into controllable areas.

## Performance Considerations

Multi-channel systems require careful attention to performance. The ESP32's dual-core architecture can be leveraged by running dimmer control on one core and user interface or network communication on the other. Timer resources are limited, so the library uses them efficiently, but be aware of conflicts with other timer-based libraries.

Memory usage scales with channel count. Each channel requires approximately 100 bytes of RAM plus timer resources. For systems with many channels, consider using an ESP32 variant with more RAM.

## Troubleshooting Multi-Channel Systems

Common issues and solutions include channels not responding independently (ensure each channel has a unique GPIO pin and check for wiring shorts), synchronization problems (verify all dimmers share the same zero-cross signal and are on the same AC phase), flickering or unstable operation (check power supply capacity and ensure ground connections are solid), and effects not working as expected (verify channel array initialization and check that all channels are active).

## Safety Considerations

Multi-channel systems multiply the importance of electrical safety. Always use proper isolation between AC mains and control circuits. Ensure your electrical installation can handle the total load of all channels. Consider adding fuses or circuit breakers for each channel. Use proper enclosures to prevent accidental contact with live connections.

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

After mastering multi-channel control, explore advanced applications like web-based control interfaces for remote channel management, integration with home automation systems, DMX512 protocol implementation for professional lighting, and music-reactive lighting systems using multiple channels.

---

*This example is part of the rbdimmerESP32 library - Professional AC dimmer control for ESP32*