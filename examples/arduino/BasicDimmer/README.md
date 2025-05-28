# Basic Dimmer Control Example

This example demonstrates the fundamental usage of the rbdimmerESP32 library for controlling a single AC dimmer channel with a fixed brightness level.

## Overview

The BasicDimmer example is the simplest starting point for understanding AC dimmer control with ESP32. It shows how to initialize the library, register a zero-cross detector, create a dimmer channel, and maintain a constant brightness level.

## What This Example Does

When you run this example, the dimmer will:
- Initialize and configure the rbdimmerESP32 library
- Set up a zero-cross detector on GPIO 18
- Create a dimmer channel on GPIO 19  
- Set the connected AC load to 50% brightness
- Maintain this brightness level continuously
- Print status information every 5 seconds

## Hardware Requirements

- **ESP32 Board**: Any ESP32 development board (ESP32, ESP32-S2, ESP32-S3, ESP32-C3)
- **AC Dimmer Module**: RBDimmer AC dimmer module
- **AC Load**: Incandescent bulb (40-100W recommended for testing)
- **Power Supply**: Appropriate AC mains connection with safety isolation

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
=== RBDimmer Basic Example ===
Initializing RBDimmer library...
Library initialized successfully
Registering zero-cross detector on pin 18...
Zero-cross detector registered
Creating dimmer channel...
Dimmer channel created successfully
Setting brightness to 50%...
Dimmer is now running at 50% brightness
Setup complete! Dimmer is active.
Detected mains frequency: 50 Hz

--- Dimmer Status ---
Brightness: 50%
Frequency: 50 Hz
Active: Yes
Uptime: 5 seconds
-------------------
```

## How It Works

This example demonstrates phase-angle control dimming:
1. The zero-cross detector identifies when the AC waveform crosses zero
2. The library calculates the appropriate delay for 50% brightness
3. After each zero-crossing, the TRIAC is triggered after this delay
4. The result is that only part of each AC half-cycle reaches the load
5. This creates the dimming effect while maintaining stable operation

## Customization

You can modify these constants in the code:
- `INITIAL_BRIGHTNESS`: Change the brightness level (0-100%)
- `ZERO_CROSS_PIN`: Use a different GPIO for zero-cross detection
- `DIMMER_PIN`: Use a different GPIO for dimmer control
- Curve type: Change from `RBDIMMER_CURVE_RMS` to `RBDIMMER_CURVE_LOGARITHMIC` for LED loads

## Platforms

This example is compatible with:
- **Arduino IDE** (1.5.0 or higher)
- **PlatformIO** (recommended)
- **ESP-IDF**

## Troubleshooting

If the dimmer doesn't work correctly:
1. Check all wiring connections
2. Verify the zero-cross signal is reaching the ESP32
3. Ensure the ESP32 has sufficient power supply
4. Check serial output for error messages
5. Try with a simple incandescent bulb first

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

Once you've mastered this basic example, try:
1. `BasicTransition` - Learn smooth brightness transitions
2. `MultiDimmer` - Control multiple dimmer channels
3. `ZCCallBack` - Advanced zero-cross event handling

---

*This example is part of the rbdimmerESP32 library - Professional AC dimmer control for ESP32*