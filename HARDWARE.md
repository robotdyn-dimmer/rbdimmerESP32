# Hardware Setup and Safety Guide

## ⚠️ CRITICAL SAFETY WARNING

**AC mains voltage is LETHAL**. This library is designed for use with **pre-built, isolated dimmer modules only**. Never work with AC mains voltage directly without proper training, isolation, and safety equipment.

**Required Safety Measures:**
- ✅ Use only certified, isolated dimmer modules
- ✅ Work with qualified electrical personnel
- ✅ Use proper safety equipment (insulated tools, safety glasses)
- ✅ Test with low voltage first
- ✅ Install appropriate fuses and circuit breakers
- ✅ Ensure proper grounding
- ❌ Never work on live circuits
- ❌ Never bypass safety isolations
- ❌ Never use homemade AC switching circuits

## Table of Contents

1. [System Overview](#system-overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Dimmer Module Selection](#dimmer-module-selection)
4. [Wiring and Connections](#wiring-and-connections)
5. [Load Compatibility](#load-compatibility)
6. [Safety Procedures](#safety-procedures)
7. [Testing and Validation](#testing-and-validation)
8. [Troubleshooting](#troubleshooting)
9. [Advanced Configurations](#advanced-configurations)

## System Overview

### Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        COMPLETE SYSTEM                          │
├─────────────────┬───────────────────────┬─────────────────────┤
│   ESP32         │    Dimmer Module      │      AC Load        │
│   Controller    │    (Isolated)         │                     │
│                 │                       │                     │
│  ┌───────────┐  │  ┌─────────────────┐  │  ┌───────────────┐  │
│  │    CPU    │  │  │  Zero-Cross     │  │  │               │  │
│  │           │  │  │  Detection      │  │  │   Light Bulb  │  │
│  │  Pin 2 ◄──┼──┼──┤    Circuit      │  │  │      or       │  │
│  │  Pin 4 ────┼──┼──►  TRIAC Driver  ├──┼──┤   Motor       │  │
│  │  GND   ────┼──┼──┤    Circuit      │  │  │      or       │  │
│  │  3.3V  ────┼──┼──┤                 │  │  │   Heater      │  │
│  └───────────┘  │  │  OPTICAL        │  │  │               │  │
│                 │  │  ISOLATION      │  │  └───────────────┘  │
│   LOW VOLTAGE   │  │                 │  │    HIGH VOLTAGE     │
│   (3.3V DC)     │  │  ◄── BARRIER ──►│  │    (110/220V AC)    │
└─────────────────┴───────────────────────┴─────────────────────┘
```

### System Components

1. **ESP32 Microcontroller**: Provides timing control and user interface
2. **Isolated Dimmer Module**: Contains zero-crossing detection and TRIAC switching
3. **AC Load**: The device being controlled (lights, motors, heaters)
4. **Optical Isolation**: Critical safety barrier between low and high voltage

## Hardware Requirements

### ESP32 Development Boards

#### Recommended Boards

| Board | Pros | Cons | Best For |
|-------|------|------|----------|
| **ESP32 DevKit V1** | Widely available, good documentation | Basic features only | General projects |
| **ESP32-WROOM-32** | Stable, well-tested | Older design | Production systems |
| **ESP32-S3-DevKitC** | USB native, more memory | Higher cost | Advanced projects |
| **ESP32-C3-DevKitM** | RISC-V, lower power | Limited GPIO | Battery projects |
| **Wemos D1 Mini ESP32** | Compact, breadboard friendly | Limited GPIO access | Space-constrained |

#### GPIO Requirements

**Minimum Required:**
- 1 × Digital Input (Zero-crossing detection)
- 1 × Digital Output (TRIAC gate control)
- Power and ground connections

**Recommended for Multi-Channel:**
- 4 × Digital Inputs (Multi-phase zero-crossing)
- 8 × Digital Outputs (Multiple TRIAC channels)

#### GPIO Pin Recommendations

**Best Pins for Zero-Cross Detection:**
- GPIO 2, 4, 5, 12, 13, 14, 15, 25, 26, 27
- Avoid: GPIO 0, 1, 3 (boot/serial pins)
- Avoid: GPIO 6-11 (connected to flash memory)

**Best Pins for TRIAC Control:**
- Any available GPIO except boot and flash pins
- Prefer pins with lower numbers for better performance

### Power Supply Considerations

#### ESP32 Power Requirements
- **Voltage**: 3.3V (regulated)
- **Current**: 240mA typical, 500mA peak
- **Power**: ~1.2W continuous

#### Dimmer Module Power
- **Most modules**: Self-powered from AC mains
- **Some modules**: Require 3.3V or 5V auxiliary power
- **Current**: Usually < 50mA for control circuits

## Dimmer Module Selection

### Essential Features Checklist

#### ✅ **MUST HAVE** Features
- [ ] **Optical Isolation**: Between AC and DC sides (≥2.5kV)
- [ ] **Zero-Cross Detection**: Built-in ZC detection circuit
- [ ] **3.3V Logic**: Compatible with ESP32 logic levels
- [ ] **CE/UL Certification**: Safety certified for your region
- [ ] **Proper Heat Sinking**: Adequate TRIAC cooling
- [ ] **Screw Terminals**: Secure AC connections

#### ⚠️ **AVOID** These Features
- [ ] Non-isolated designs
- [ ] Modules requiring 5V logic
- [ ] Uncertified/homemade modules
- [ ] Modules without zero-cross detection
- [ ] Inadequate current ratings

### Recommended Modules

#### Professional Grade
```
Model: RobotDyn AC Light Dimmer
- Voltage: 110-220V AC
- Current: 1-4A versions available
- Isolation: 2.5kV optical
- Logic: 3.3V compatible
- ZC Detection: Built-in
- Certification: CE certified
```

#### Alternative Options
```
Generic Isolated TRIAC Modules:
- Look for: MOC3021/MOC3041 optoisolators
- TRIAC: BT136/BT138 series
- Heat sink: Adequate for current rating
- Terminals: Screw-down AC connections
```

### Current Rating Selection

**Load Current Calculation:**
```
Current (A) = Power (W) ÷ Voltage (V)

Examples:
- 100W bulb @ 110V = 0.91A
- 100W bulb @ 220V = 0.45A
- 500W heater @ 110V = 4.55A
```

**Module Rating Guidelines:**
- Choose module rated for **2× the load current**
- Example: 100W load → 2A minimum module rating
- Always include safety margin for inrush current

## Wiring and Connections

### Basic Single-Channel Setup

```
ESP32 Side (Low Voltage):
┌─────────────┐
│    ESP32    │
│      Pin 2  ├─── Zero-Cross Input
│      Pin 4  ├─── Gate Control Output  
│      GND    ├─── Ground
│      3.3V   ├─── Power (if needed)
└─────────────┘

Dimmer Module (Isolated):
┌─────────────────────┐
│   Dimmer Module     │
│                     │
│  ZC Out ────────────┤ (to ESP32 Pin 2)
│  Gate In ───────────┤ (to ESP32 Pin 4)
│  GND ───────────────┤ (to ESP32 GND)
│  VCC ───────────────┤ (to ESP32 3.3V, if needed)
│                     │
│ ╔══════════════════╗│ ← OPTICAL ISOLATION BARRIER
│ ║   AC In    L     ║├─── Live (Hot) from mains
│ ║   AC In    N     ║├─── Neutral  
│ ║   AC Out   L     ║├─── Live (Hot) to load
│ ╚══════════════════╝│
└─────────────────────┘
```


Multiple Dimmer Modules:
Each channel needs its own dimmer module
OR use a multi-channel isolated module
```

### Wire Selection and Routing

#### Low Voltage Side (ESP32)
- **Wire Type**: Standard jumper wires or 22-24 AWG stranded
- **Length**: Keep connections short (< 30cm recommended)
- **Shielding**: Not required for short runs
- **Separation**: Keep away from AC wiring

#### High Voltage Side (AC)
- **Wire Type**: Rated for voltage and current
- **Gauge**: According to load current (12-14 AWG typical)
- **Insulation**: Rated for AC voltage + safety margin
- **Conduit**: Use appropriate electrical conduit
- **Separation**: Maintain safe distances from low voltage

### Connection Procedures

#### Step 1: Low Voltage Connections First
1. **Power Off**: Ensure ESP32 is not powered
2. **Connect Ground**: ESP32 GND to module GND
3. **Connect Signals**: ZC and Gate control pins
4. **Connect Power**: 3.3V if required by module
5. **Double Check**: Verify all connections

#### Step 2: AC Connections (POWER OFF!)
⚠️ **CRITICAL**: AC mains power must be OFF during wiring

1. **Install Breaker**: Add appropriate circuit protection
2. **Connect Neutral**: AC neutral to module and load
3. **Connect Live**: AC live through module to load
4. **Ground Safety**: Connect safety ground if required
5. **Secure Connections**: All AC connections must be secure
6. **Insulate**: Cover all AC connections properly

#### Step 3: Testing Sequence
1. **Visual Inspection**: Check all connections
2. **Continuity Test**: Use multimeter (power off)
3. **Insulation Test**: Verify isolation (if equipment available)
4. **Low Voltage Test**: Power ESP32 only, test logic
5. **AC Test**: Apply AC power with appropriate safety measures

## Load Compatibility

### Compatible Load Types

#### Resistive Loads ✅
```
Examples: Incandescent bulbs, heaters, hot plates
Characteristics:
- Constant power factor
- No inrush current issues  
- Linear response to dimming
- Best compatibility

Recommended Settings:
- Curve: RBDIMMER_CURVE_RMS
- Works with all dimmer types
```

#### LED Loads ⚠️
```  
Examples: LED bulbs, LED strips
Characteristics:
- May have built-in drivers
- Some types are not dimmable
- May require specific curve

Recommended Settings:
- Curve: RBDIMMER_CURVE_LOGARITHMIC
- Test compatibility first
- Use "dimmable" rated LEDs only
```

#### Motor Loads ⚠️
```
Examples: Fans, small motors
Characteristics:
- Inductive loads
- High starting current
- May require soft-start

Recommended Settings:
- Curve: RBDIMMER_CURVE_LINEAR  
- Use appropriate TRIAC rating
- Consider motor-rated dimmers
```

### Incompatible Loads ❌

#### Electronic Ballasts
- Fluorescent lights with electronic ballasts
- May cause interference or damage
- Use relay switching instead

#### Switch-Mode Power Supplies  
- Computer power supplies
- Most modern electronics
- Phase control causes problems

#### Reactive Loads
- Large motors without proper protection
- Transformers (except dimmer-rated)
- Unfiltered inductive loads

### Load Testing Procedure

1. **Start Small**: Test with low-power resistive load first
2. **Monitor Current**: Check actual vs expected current draw
3. **Check Waveforms**: Use oscilloscope if available
4. **Temperature Check**: Monitor dimmer module temperature
5. **Long-term Test**: Run for extended periods
6. **Document Results**: Record compatibility and settings

## Safety Procedures

### Pre-Installation Safety

#### Electrical Safety Training
- [ ] Understand AC electrical hazards
- [ ] Know local electrical codes
- [ ] Have emergency procedures ready
- [ ] Use proper personal protective equipment

#### Test Equipment
- [ ] Digital multimeter
- [ ] Non-contact voltage tester  
- [ ] Circuit analyzer (if available)
- [ ] Insulation tester (for professional installations)

### Installation Safety Checklist

#### Before Starting
- [ ] Turn OFF power at breaker
- [ ] Test circuits with voltage tester
- [ ] Lock out/tag out procedures
- [ ] Verify dimmer module ratings
- [ ] Check all components for damage

#### During Installation  
- [ ] Work on de-energized circuits only
- [ ] Use insulated tools
- [ ] Make secure connections
- [ ] Follow manufacturer wiring diagrams
- [ ] Double-check all connections

#### Before Energizing
- [ ] Visual inspection of all connections
- [ ] Verify no exposed conductors
- [ ] Check load compatibility
- [ ] Set initial dimmer level to minimum
- [ ] Have shutdown procedure ready

### Operating Safety

#### Regular Inspections
- **Weekly**: Visual check for damage or overheating
- **Monthly**: Check connection tightness
- **Annually**: Professional inspection for commercial use

#### Warning Signs
- **Immediate shutdown required**:
  - Burning smell
  - Visible sparks or arcing
  - Unusual noises
  - Excessive heat
  - Flickering or erratic operation

## Testing and Validation

### Functional Testing

#### Test 1: Zero-Cross Detection
```cpp
// Test code for zero-cross detection
void test_zero_cross() {
    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 0);
    
    // Monitor frequency detection
    for(int i = 0; i < 30; i++) {
        delay(1000);
        uint16_t freq = rbdimmer_get_frequency(0);
        Serial.printf("Detected frequency: %d Hz\n", freq);
    }
}
```

**Expected Results:**
- Frequency should stabilize to 50 or 60 Hz within 10-20 seconds
- Should not fluctuate more than ±1 Hz

#### Test 2: Gate Control
```cpp
// Test TRIAC gate control (without AC load)
void test_gate_control() {
    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 50);
    
    rbdimmer_config_t config = {4, 0, 0, RBDIMMER_CURVE_LINEAR};
    rbdimmer_channel_t* channel;
    rbdimmer_create_channel(&config, &channel);
    
    // Test different levels
    for(int level = 0; level <= 100; level += 10) {
        rbdimmer_set_level(channel, level);
        Serial.printf("Level: %d%%, Delay: %d us\n", 
                      level, rbdimmer_get_delay(channel));
        delay(2000);
    }
}
```

#### Test 3: Load Response (AC Connected)
⚠️ **Safety First**: Ensure all safety procedures are followed

```cpp
// Test with actual AC load
void test_load_response() {
    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 0);
    
    rbdimmer_config_t config = {4, 0, 10, RBDIMMER_CURVE_RMS};
    rbdimmer_channel_t* channel;
    rbdimmer_create_channel(&config, &channel);
    
    Serial.println("Starting load test - 10% to 90%");
    delay(2000);
    
    // Gradual increase to avoid inrush
    for(int level = 10; level <= 90; level += 10) {
        rbdimmer_set_level_transition(channel, level, 1000);
        delay(2000);
        Serial.printf("Level: %d%%\n", level);
    }
    
    // Return to safe level
    rbdimmer_set_level_transition(channel, 10, 1000);
}
```

### Performance Validation

#### Timing Accuracy Test
- Use oscilloscope to measure timing accuracy
- Zero-cross to gate trigger should be consistent
- Verify timing matches calculated values

#### Temperature Testing
- Monitor dimmer module temperature under load
- Ensure temperature remains within safe limits
- Test with maximum expected load

#### Interference Testing
- Check for radio frequency interference
- Verify no interference with other devices
- Test different load types and power levels

## Troubleshooting

### Common Hardware Issues

#### Problem: No Zero-Cross Detection
**Symptoms:**
- `rbdimmer_get_frequency()` returns 0
- No dimming response

**Diagnostics:**
```cpp
// Check zero-cross signal
void diagnose_zero_cross() {
    pinMode(2, INPUT);
    
    while(true) {
        int state = digitalRead(2);
        Serial.printf("ZC Pin State: %d\n", state);
        delay(100);
    }
}
```

**Solutions:**
1. Check wiring connections
2. Verify dimmer module power
3. Test with multimeter (DC side only)
4. Replace dimmer module if faulty

#### Problem: TRIAC Not Switching
**Symptoms:**
- Load doesn't respond to level changes
- Constant on or off state

**Diagnostics:**
1. Check gate control pin with LED indicator
2. Verify dimmer module specifications  
3. Test with known working load

**Solutions:**
1. Verify gate control connections
2. Check for adequate gate current
3. Test different GPIO pin
4. Replace dimmer module

#### Problem: Erratic Dimming
**Symptoms:**
- Inconsistent brightness levels
- Flickering or jumping

**Possible Causes:**
1. Power supply noise
2. Electrical interference
3. Load incompatibility
4. Poor connections

**Solutions:**
1. Add power supply filtering
2. Check all connection tightness
3. Test with different load type
4. Add ferrite cores on cables

### Measurement and Analysis

#### Oscilloscope Analysis
If oscilloscope is available:

1. **Zero-Cross Signal**: Should be clean digital pulses
2. **Gate Control**: Should show precise timing relative to ZC
3. **Load Voltage**: Should show expected phase-cut waveform
4. **Load Current**: Should follow voltage waveform

#### Multimeter Measurements
Safe measurements (power off):

1. **Continuity**: All connections should have good continuity  
2. **Isolation**: >1MΩ between AC and DC sides
3. **Resistance**: Load resistance should match expectations

## Advanced Configurations

### Multi-Phase Systems

For three-phase AC systems:

```cpp
// Three-phase setup
void setup_three_phase() {
    rbdimmer_init();
    
    // Register three phases
    rbdimmer_register_zero_cross(2, 0, 0);  // Phase A
    rbdimmer_register_zero_cross(15, 1, 0); // Phase B  
    rbdimmer_register_zero_cross(4, 2, 0);  // Phase C
    
    // Create channels on each phase
    rbdimmer_config_t config_a = {5, 0, 0, RBDIMMER_CURVE_RMS};
    rbdimmer_config_t config_b = {18, 1, 0, RBDIMMER_CURVE_RMS};
    rbdimmer_config_t config_c = {19, 2, 0, RBDIMMER_CURVE_RMS};
    
    rbdimmer_channel_t* channel_a, *channel_b, *channel_c;
    rbdimmer_create_channel(&config_a, &channel_a);
    rbdimmer_create_channel(&config_b, &channel_b);
    rbdimmer_create_channel(&config_c, &channel_c);
}
```

### High-Power Applications

For loads >1kW:

1. **Use appropriate dimmer modules** (10A+ rating)
2. **Implement thermal monitoring**
3. **Add cooling systems** if needed
4. **Use professional installation** practices

### Remote Monitoring

```cpp
// Add current monitoring (if available)
void monitor_system() {
    // Read current sensor (if connected)
    float current = read_current_sensor();
    float power = current * 230.0; // Approximate
    
    Serial.printf("Load Current: %.2f A\n", current);
    Serial.printf("Load Power: %.1f W\n", power);
    Serial.printf("Dimmer Level: %d%%\n", rbdimmer_get_level(channel));
    Serial.printf("Frequency: %d Hz\n", rbdimmer_get_frequency(0));
}
```

---

## Support and Resources

### Technical Support
- **Email**: dev@rbdimmer.com
- **Forum**: [https://www.rbdimmer.com/forum](https://www.rbdimmer.com/forum)
- **Documentation**: [https://www.rbdimmer.com/knowledge/article/45](https://www.rbdimmer.com/knowledge/article/45)

### Hardware Sources
- **Dimmer Modules**: [https://www.rbdimmer.com/dimmers-pricing](https://www.rbdimmer.com/dimmers-pricing)
- **Project Examples**: [https://www.rbdimmer.com/blog/dimmers-projects-5](https://www.rbdimmer.com/blog/dimmers-projects-5)

### Legal and Safety
- Always follow local electrical codes
- Use qualified electricians for permanent installations
- Maintain proper documentation and certifications
- Regular safety inspections are recommended

---

**Remember: Safety is not optional. When in doubt, consult with qualified electrical professionals.**

*Hardware guide for rbdimmerESP32 v1.0.0*