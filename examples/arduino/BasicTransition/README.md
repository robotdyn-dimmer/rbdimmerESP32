# Smooth Brightness Transitions Example

This example demonstrates how to create smooth, professional-looking brightness transitions using the rbdimmerESP32 library's built-in transition functionality.

## Overview

The BasicTransition example showcases one of the most powerful features of the rbdimmerESP32 library - the ability to create smooth brightness changes over time without blocking your main program. This is achieved using FreeRTOS tasks that handle the transition in the background while your code continues to run normally.

## What This Example Does

This example creates a continuous light show that demonstrates various transition effects. The connected light will cycle through a sequence of smooth brightness changes, showing different transition speeds and demonstrating how transitions can create professional lighting effects. The sequence includes fade-ins, fade-outs, and quick transitions, all happening automatically while the main program remains responsive.

The complete cycle consists of:
- Slow fade from darkness to full brightness (3 seconds)
- Gentle fade down to a dim level (2 seconds)  
- Quick jump to bright (0.5 seconds)
- Smooth fade to complete darkness (2 seconds)

## Hardware Requirements

- **ESP32 Board**: Any ESP32 development board (ESP32, ESP32-S2, ESP32-S3, ESP32-C3)
- **AC Dimmer Module**: RBDimmer AC dimmer module
- **AC Load**: Incandescent bulb (60-100W provides the smoothest visual effect)
- **Power Supply**: Proper AC mains connection with safety isolation

## Wiring Connections

| ESP32 Pin | Dimmer Module | Description |
|-----------|---------------|-------------|
| GPIO 18   | ZC (Zero Cross) | Zero-crossing detection signal |
| GPIO 19   | PWM/CTRL | Dimmer control signal |
| 3.3V      | VCC | Module power supply |
| GND       | GND | Common ground |

⚠️ **Safety Warning**: Always ensure proper electrical isolation between AC mains and your ESP32!

## Expected Serial Output

```
=== RBDimmer Smooth Transitions Example ===
Library initialized successfully
Zero-cross detector registered
Dimmer channel created
Detected mains frequency: 50 Hz

Starting transition demonstration...
Watch the connected light for smooth transitions!

Starting: Fade up to 100% (3 seconds)
Transition started, main loop continues...
Current brightness: 15%
Current brightness: 33%
Current brightness: 67%
Current brightness: 89%
Transition complete! Now at 100%

Starting: Fade down to 20% (2 seconds)
Transition started, main loop continues...
Current brightness: 75%
Current brightness: 47%
Current brightness: 25%
Transition complete! Now at 20%

[Main loop active - program continues during transitions]
```

## How Smooth Transitions Work

The transition system in rbdimmerESP32 uses sophisticated timing and task management to create smooth effects. When you request a transition, the library calculates the required brightness steps based on the duration and creates a background task that updates the brightness at regular intervals. This approach ensures that your main program never gets blocked, allowing you to handle user input, sensor readings, or network communications while the lights smoothly change.

The smoothness of the transition depends on several factors. The transition duration plays a major role - longer transitions appear smoother because the brightness changes are more gradual. The type of load also matters significantly. Incandescent bulbs provide the smoothest visual effect due to their thermal inertia, while LED loads may show more discrete steps unless they have good dimming drivers.

## Customization Options

You can modify the transition sequence to create your own lighting patterns. Try adjusting these parameters in the code:

The transition durations can be changed to create different effects. Very short transitions (under 500ms) create snappy, responsive changes suitable for effects lighting. Medium transitions (1-3 seconds) provide pleasant, noticeable changes good for general lighting control. Long transitions (5-10 seconds or more) create very subtle changes perfect for mood lighting or sunrise simulation.

You can also modify the brightness targets to create different patterns. Consider creating a breathing effect by transitioning between 20% and 80%, or a flash effect by quickly going to 100% and back. The possibilities are endless.

## Understanding Non-Blocking Operation

One of the key advantages demonstrated in this example is non-blocking operation. Traditional dimming code might use delays, which would freeze your entire program during transitions. The rbdimmerESP32 library instead uses FreeRTOS tasks, allowing your code to continue running. This means you can:
- Respond to button presses during transitions
- Update displays or indicators
- Handle network requests
- Read sensors and react to changes
- Control multiple dimmers independently

## Platforms

This example is compatible with:
- **Arduino IDE** (1.5.0 or higher with ESP32 board support)
- **PlatformIO** (recommended for best development experience)
- **ESP-IDF**

## Troubleshooting

If transitions don't appear smooth:
1. Ensure you're using an appropriate load (incandescent bulbs work best)
2. Check that your power supply is stable
3. Verify the zero-cross detection is working correctly
4. Try longer transition durations
5. For LED loads, ensure they're dimmable and try the logarithmic curve

If transitions don't start:
1. Check serial output for error messages
2. Verify sufficient free memory for FreeRTOS tasks
3. Ensure the dimmer is properly initialized
4. Check that previous transitions have completed

## Advanced Concepts

This example introduces several advanced concepts that you can explore further. The FreeRTOS task system allows for complex lighting scenarios where multiple transitions can happen simultaneously on different channels. The timing system ensures accurate transitions regardless of system load. The state machine approach demonstrated here can be extended to create complex lighting sequences for theatrical or architectural applications.

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

After mastering smooth transitions, explore these advanced examples:
1. `MultiDimmer` - Control multiple channels with independent transitions
2. `ZCCallBack` - Synchronize effects with AC mains frequency
3. Create custom transition curves for specific effects
4. Implement web control with smooth transitions
5. Build a sunrise alarm clock with gradual brightening

---

*This example is part of the rbdimmerESP32 library - Professional AC dimmer control for ESP32*