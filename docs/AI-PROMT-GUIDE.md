# AI Code Generation Guide for rbdimmerESP32

## Overview

This guide provides ready-to-use prompts for AI assistants (ChatGPT, Claude, DeepSeek, etc.) to help you generate high-quality Arduino code using the rbdimmerESP32 library. These prompts include comprehensive context about the library, ensuring AI-generated code follows best practices and safety guidelines.

## Table of Contents

1. [Master Prompt Template](#master-prompt-template)
2. [Specialized Prompts](#specialized-prompts)
3. [Usage Examples](#usage-examples)
4. [Best Practices](#best-practices)
5. [Common Use Cases](#common-use-cases)

---

## Master Prompt Template

Copy and paste this comprehensive prompt into your AI assistant conversation:

### 🤖 **MASTER PROMPT FOR rbdimmerESP32**

```
You are an expert Arduino developer helping me create code using the rbdimmerESP32 library for ESP32 AC dimmer control. Please follow these guidelines strictly:

## LIBRARY OVERVIEW
The rbdimmerESP32 library is a professional AC dimmer control library for ESP32 microcontrollers that provides:
- Hardware timer integration with microsecond precision
- Multi-channel support (up to 8 channels)
- Multi-phase support (up to 4 phases)
- Multiple brightness curves (Linear, RMS, Logarithmic)
- Automatic frequency detection (50/60Hz)
- Smooth transitions using FreeRTOS tasks
- Zero-crossing interrupt callbacks

## SAFETY REQUIREMENTS - CRITICAL
⚠️ ALWAYS include these safety warnings in your code:
- Only use with pre-built, isolated dimmer modules
- Never work with AC mains voltage directly
- AC mains voltage is LETHAL - proper isolation required
- Follow local electrical codes
- Use appropriate fuses and circuit protection

## CORE API FUNCTIONS
```cpp
// Essential functions to use:
rbdimmer_err_t rbdimmer_init(void);
rbdimmer_err_t rbdimmer_register_zero_cross(uint8_t pin, uint8_t phase, uint16_t frequency);
rbdimmer_err_t rbdimmer_create_channel(rbdimmer_config_t* config, rbdimmer_channel_t** channel);
rbdimmer_err_t rbdimmer_set_level(rbdimmer_channel_t* channel, uint8_t level_percent);
rbdimmer_err_t rbdimmer_set_level_transition(rbdimmer_channel_t* channel, uint8_t level_percent, uint32_t transition_ms);
rbdimmer_err_t rbdimmer_set_curve(rbdimmer_channel_t* channel, rbdimmer_curve_t curve_type);
uint8_t rbdimmer_get_level(rbdimmer_channel_t* channel);
uint16_t rbdimmer_get_frequency(uint8_t phase);
```

## BRIGHTNESS CURVES
- RBDIMMER_CURVE_LINEAR: For motor control, direct phase angle
- RBDIMMER_CURVE_RMS: For incandescent bulbs, power-linear
- RBDIMMER_CURVE_LOGARITHMIC: For LEDs, perceptually linear

## TYPICAL SETUP PATTERN
```cpp
#include <rbdimmerESP32.h>

rbdimmer_channel_t* dimmer_channel;

void setup() {
    Serial.begin(115200);
    
    // Initialize library
    rbdimmer_init();
    
    // Register zero-cross detector (pin, phase, frequency)
    rbdimmer_register_zero_cross(2, 0, 0); // Auto-detect frequency
    
    // Configure channel
    rbdimmer_config_t config = {
        .gpio_pin = 4,              // Dimmer output pin
        .phase = 0,                 // Phase number
        .initial_level = 0,         // Starting level 0-100%
        .curve_type = RBDIMMER_CURVE_RMS
    };
    
    // Create channel
    rbdimmer_create_channel(&config, &dimmer_channel);
}
```

## ERROR HANDLING
Always check return values:
```cpp
rbdimmer_err_t err = rbdimmer_init();
if (err != RBDIMMER_OK) {
    Serial.printf("Error: %d\n", err);
    return;
}
```

## HARDWARE CONNECTIONS
- Pin 2: Zero-cross detector input (from dimmer module)
- Pin 4: Gate control output (to dimmer module)
- GND: Common ground
- 3.3V: Power (if dimmer module requires it)

## COMMON ISSUES TO AVOID
1. Don't call rbdimmer_init() multiple times
2. Always register zero-cross before creating channels
3. Use levels 0-100 only
4. For smooth transitions, use rbdimmer_set_level_transition()
5. Choose appropriate curve for load type

## DOCUMENTATION REFERENCES
- Main Documentation: https://github.com/robotdyn-dimmer/rbdimmerESP32/blob/main/README.md
- API Reference: https://github.com/robotdyn-dimmer/rbdimmerESP32/blob/main/docs/API.md
- Hardware Guide: https://github.com/robotdyn-dimmer/rbdimmerESP32/blob/main/docs/HARDWARE.md
- Troubleshooting: https://github.com/robotdyn-dimmer/rbdimmerESP32/blob/main/docs/TROUBLESHOOTING.md

## EXAMPLE REFERENCES
- Basic Usage: https://github.com/robotdyn-dimmer/rbdimmerESP32/blob/main/examples/arduino/BasicDimmer/BasicDimmer.ino
- Smooth Transitions: https://github.com/robotdyn-dimmer/rbdimmerESP32/blob/main/examples/arduino/BasicTransition/BasicTransition.ino
- Multiple Channels: https://github.com/robotdyn-dimmer/rbdimmerESP32/blob/main/examples/arduino/MultiDimmer/MultiDimmer.ino
- Advanced Callbacks: https://github.com/robotdyn-dimmer/rbdimmerESP32/blob/main/examples/arduino/ZCCallBack/ZCCallBack.ino

## YOUR REQUEST
Please generate Arduino code using the rbdimmerESP32 library that:
[Insert your specific requirements here]

Requirements:
- Include proper error handling
- Add safety warnings in comments
- Use appropriate brightness curve for the load type
- Follow the typical setup pattern
- Include helpful Serial.print() statements for debugging
- Ensure code is well-commented and production-ready
```

---

## Specialized Prompts

### 🎯 **Basic Dimmer Control Prompt**

```
Using the rbdimmerESP32 library context above, create a simple Arduino sketch that:
- Controls a single AC dimmer channel
- Implements basic on/off and brightness control
- Uses serial commands for user interaction
- Includes proper error handling and safety warnings
- Uses RMS curve for incandescent bulb control

Hardware setup: ESP32 with dimmer module on pins 2 (zero-cross) and 4 (gate control)
```

### 🌈 **Smooth Lighting Effects Prompt**

```
Using the rbdimmerESP32 library context above, create an Arduino sketch that:
- Implements smooth lighting effects (fade in/out, breathing, etc.)
- Uses rbdimmer_set_level_transition() for smooth changes
- Provides multiple effect modes selectable via serial input
- Includes timing controls for effect speed
- Uses logarithmic curve for LED compatibility

Create at least 3 different lighting effects with user-selectable timing.
```

### 🔄 **Multi-Channel Control Prompt**

```
Using the rbdimmerESP32 library context above, create an Arduino sketch that:
- Controls 3 independent dimmer channels
- Implements synchronized and individual control modes
- Uses different brightness curves for different load types
- Provides web interface or serial control
- Includes channel status monitoring and error handling

Hardware: 3 dimmer modules sharing zero-cross (pin 2), gate controls on pins 4, 5, 18
```

### 📱 **IoT Integration Prompt**

```
Using the rbdimmerESP32 library context above, create an Arduino sketch that:
- Integrates WiFi connectivity for remote control
- Implements MQTT or HTTP REST API for dimmer control
- Includes JSON configuration for multiple channels
- Provides status reporting and monitoring
- Implements OTA (Over-The-Air) update capability
- Uses appropriate curves based on load type configuration

Focus on professional IoT practices and error recovery.
```

### ⚡ **Advanced Callback System Prompt**

```
Using the rbdimmerESP32 library context above, create an Arduino sketch that:
- Uses zero-crossing callbacks for advanced synchronization
- Implements power monitoring and frequency analysis
- Creates synchronized effects across multiple channels
- Includes real-time performance monitoring
- Uses FreeRTOS tasks for complex operations
- Implements safety monitoring and automatic shutdown

Focus on professional-grade features and system reliability.
```

---

## Usage Examples

### Example 1: Basic Request
```
[Paste Master Prompt]

Please generate Arduino code using the rbdimmerESP32 library that:
- Controls a single lamp with smooth dimming
- Accepts serial commands: "on", "off", "level 50", "fade 75 3000"
- Shows current status every 5 seconds
- Uses RMS curve for incandescent bulb
```

### Example 2: Complex Request
```
[Paste Master Prompt]

Please generate Arduino code using the rbdimmerESP32 library that:
- Controls 4 independent ceiling lights
- Creates a "party mode" with random brightness changes
- Implements "sunrise simulation" effect over 30 minutes
- Provides WiFi web interface for control
- Saves settings to EEPROM
- Uses logarithmic curve for LED lights
```

### Example 3: Problem-Solving Request
```
[Paste Master Prompt]

I'm having issues with flickering at low brightness levels (0-10%). Please generate Arduino code that:
- Implements proper curve selection for LED loads
- Includes diagnostic functions to check zero-crossing detection
- Provides troubleshooting output via serial monitor
- Tests different minimum brightness levels
- Includes frequency monitoring and stability checks
```

---

## Best Practices

### ✅ **Do This**

1. **Always use the full Master Prompt** - provides complete context
2. **Be specific about hardware setup** - mention pins, load types, power ratings
3. **Request error handling** - AI should include proper error checking
4. **Ask for comments** - well-documented code is easier to understand
5. **Specify load type** - helps AI choose correct brightness curve
6. **Request safety warnings** - AI should include safety reminders
7. **Ask for debugging output** - Serial.print() statements help troubleshooting

### ❌ **Avoid This**

1. **Don't use partial context** - AI needs full library information
2. **Don't skip safety requirements** - always emphasize safety
3. **Don't ignore error handling** - AC dimming requires robust error checking
4. **Don't mix libraries** - stick to rbdimmerESP32 functions only
5. **Don't forget hardware specifications** - voltage, current, isolation requirements

### 💡 **Pro Tips**

1. **Iterate and refine**: Start with basic request, then add complexity
2. **Ask for alternatives**: "Show me 2 different approaches to..."
3. **Request explanations**: "Explain why you chose this brightness curve..."
4. **Include testing code**: "Add diagnostic functions to verify operation..."
5. **Ask for optimization**: "How can this code be made more efficient?"

---

## Common Use Cases

### 🏠 **Home Automation**
```
Create a smart home lighting controller that integrates with Home Assistant, 
supports multiple rooms, includes scheduling, and provides energy monitoring.
```

### 🎭 **Stage Lighting**
```
Design a DMX-compatible lighting controller for stage applications with 
precise timing, multiple channels, and synchronized effects.
```

### 🏭 **Industrial Control**
```
Develop a motor speed controller with safety interlocks, remote monitoring, 
and automatic fault detection for industrial applications.
```

### 🎓 **Educational Projects**
```
Create a learning platform for AC dimming concepts with interactive controls, 
waveform visualization, and educational explanations.
```

### 🔬 **Research Applications**
```
Build a precision power control system for laboratory equipment with 
data logging, precise calibration, and research-grade accuracy.
```

---

## Troubleshooting AI Responses

### Common AI Mistakes and How to Fix Them

#### ❌ **AI uses wrong function names**
**Fix**: Remind AI to use exact function names from the API reference

#### ❌ **AI forgets error handling**
**Fix**: Add "with comprehensive error handling" to your request

#### ❌ **AI mixes different dimmer libraries**
**Fix**: Emphasize "ONLY use rbdimmerESP32 library functions"

#### ❌ **AI ignores safety requirements**
**Fix**: Start request with "This is for AC mains voltage - emphasize safety"

#### ❌ **AI uses deprecated patterns**
**Fix**: Reference the specific example links in your prompt

### Follow-up Questions to Ask AI

1. "Can you add more detailed error checking?"
2. "How would this code handle power outages?"
3. "Can you optimize this for lower memory usage?"
4. "What safety features should be added?"
5. "How can I test this code safely?"

---

## Support and Resources

### Getting Help with AI-Generated Code

1. **Test thoroughly**: Always test AI-generated code with proper safety measures
2. **Validate against examples**: Compare with official library examples
3. **Check documentation**: Verify function usage against API documentation
4. **Ask for explanations**: If code seems unusual, ask AI to explain the approach
5. **Seek community help**: Use the [rbdimmer forum](https://www.rbdimmer.com/forum) for complex issues

### Reporting Issues

If AI consistently generates incorrect code:
1. Check if the prompt includes all necessary context
2. Try different AI assistants for comparison
3. Report persistent issues to the library maintainers
4. Share working prompts with the community

---

**Remember**: AI-generated code is a starting point. Always review, test, and validate code before using it with AC mains voltage. Safety is your responsibility.

*AI Prompts Guide for rbdimmerESP32 v1.0.0*