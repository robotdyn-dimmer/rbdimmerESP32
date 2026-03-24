
# Hardware Connection
> [!NOTE]
> **Library GitHub:** [https://github.com/robotdyn-dimmer/rbdimmerESP32](https://github.com/robotdyn-dimmer/rbdimmerESP32)


> [!NOTE]
> Updated for rbdimmerESP32 v2.0.0. Minimum ESP-IDF: 5.3.

After choosing the right dimmer, you're ready to begin assembly and wiring.

A typical dimmer module includes two parts:

- **Zero-cross detection module** -- Detects the moment the AC waveform crosses zero
- **TRIAC module** -- Controls load power during each AC half-cycle

> [!NOTE]
> For more details on how TRIACs work, see the [TRIAC Operation Guide](https://www.rbdimmer.com/blog/diy-insights-1/ac-dimmer-based-on-zero-cross-detector-and-triac-operating-principles-and-applications-5).

---

## System Overview

### Block Diagram

```
+------------------------------------------------------------------+
|                        COMPLETE SYSTEM                            |
+------------------+------------------------+----------------------+
|   ESP32          |    Dimmer Module       |      AC Load         |
|   Controller     |    (Isolated)          |                      |
|                  |                        |                      |
|  +-----------+   |  +-----------------+   |  +---------------+   |
|  |    CPU    |   |  |  Zero-Cross     |   |  |               |   |
|  |           |   |  |  Detection      |   |  |   Light Bulb  |   |
|  |  ZC Pin <-----+--+    Circuit      |   |  |      or       |   |
|  | DIM Pin ------+-->  TRIAC Driver   +---+--+   Motor       |   |
|  |  GND   -------+--+    Circuit      |   |  |      or       |   |
|  |  3.3V  -------+--+                 |   |  |   Heater      |   |
|  +-----------+   |  |  OPTICAL        |   |  |               |   |
|                  |  |  ISOLATION      |   |  +---------------+   |
|   LOW VOLTAGE    |  |                 |   |    HIGH VOLTAGE      |
|   (3.3V DC)      |  | <-- BARRIER --> |   |    (110/220V AC)     |
+------------------+------------------------+----------------------+
```

### System Components

1. **ESP32 Microcontroller**: Provides timing control and user interface
2. **Isolated Dimmer Module**: Contains zero-crossing detection and TRIAC switching with optical isolation (the critical safety barrier between low and high voltage)
3. **AC Load**: The device being controlled (lights, motors, heaters)

### v2.0.0 Timing Architecture

- **ZC noise gate**: `ZC_DEBOUNCE_US` eliminates false re-triggers caused by TRIAC commutation spikes on the zero-cross line. The ISR ignores any ZC edge that arrives within the debounce window after the previous valid edge.
- **Two-pass ISR for multi-channel sync**: The first pass latches the zero-cross timestamp for all channels simultaneously; the second pass programs individual TRIAC fire timers. This keeps channels phase-locked even under heavy CPU load.

---

## Dimmer Module Selection

### Essential Features Checklist

**MUST HAVE features:**

- Optical isolation between AC and DC sides (>=2.5 kV)
- Built-in zero-cross detection circuit
- 3.3V logic compatibility (ESP32 logic levels)
- CE/UL certification for your region
- Proper heat sinking / adequate TRIAC cooling
- Screw terminals for secure AC connections

**AVOID these features:**

- Non-isolated designs
- Modules requiring 5V logic (not ESP32-safe without level shifter)
- Uncertified or homemade modules
- Modules without zero-cross detection
- Inadequate current ratings for your load

### Current Rating Selection

**Load current calculation:**

```text
Current (A) = Power (W) / Voltage (V)

Examples:
  100W bulb @ 110V = 0.91A
  100W bulb @ 220V = 0.45A
  500W heater @ 110V = 4.55A
```

> [!IMPORTANT]
> **2x guideline:** Choose a dimmer module rated for at least **2x the calculated load current**. This provides margin for inrush current (cold-filament incandescent bulbs draw 10-15x steady-state current at switch-on) and sustained thermal headroom.
>
> Example: 100W load at 220V = 0.45A --> use a module rated >= 1A (a 2A or 4A module is preferred).

---

## Wiring Diagram

The connection diagram is the same for all dimmer modules. For multi-channel dimmers (2-4 channels), or those with thermal control and fan output, refer to detailed schematics showing additional power connections and pinouts.

### Available Connection Diagrams

- Dimmer 4A connection
<img src="https://www.rbdimmer.com/web/image/4968-5c605a2e/dimpinout4A.png" alt="Wiring diagram for 4A dimmer" style="max-width:100%;height:auto;">
- Dimmer 8A connection
<img src="https://www.rbdimmer.com/web/image/2066-5a067a48/dimpinout8A.png" alt="Wiring diagram for 8A dimmer" style="max-width:100%;height:auto;">
- Dimmer 16/24A connection
<img src="https://www.rbdimmer.com/web/image/2212-e519e4d3/dimpinout16-24A.png" alt="Wiring diagram for 16/24A dimmer" style="max-width:100%;height:auto;">
- Dimmer Module 16/24A with Temperature-Controlled Active Cooling
<img src="https://www.rbdimmer.com/web/image/2334-cd0320ab/Dimmer16-24A-CS-pinout.png" alt="Wiring diagram for dimmer 16/24A with Temperature-Controlled Active Cooling" style="max-width:100%;height:auto;">
- Dimmer Module 16/24A with Current sensor, temperature control
<img src="https://www.rbdimmer.com/web/image/2401-46148b5d/Dimmer16-24A-CS-pinout.png" alt="Wiring diagram for dimmer 16/24A with Current sensor, Temperature-Controlled Active Cooling" style="max-width:100%;height:auto;">
- Dimmer 40A connection
<img src="https://www.rbdimmer.com/web/image/2136-95d9368a/dimpinout40A.png" alt="Wiring diagram for 40A dimmer" style="max-width:100%;height:auto;">
- AC Dimmer module 40A with Current sensor
<img src="https://www.rbdimmer.com/web/image/2106-acda7f4f/dimpinout40A-CS.png" alt="Wiring diagram for 40A dimmer with Current sensor and Temperature-Controlled Active Cooling" style="max-width:100%;height:auto;">
- Dimmer 8A 2 channels connection
<img src="https://www.rbdimmer.com/web/image/2185-034f4b3f/dimpinout8A2L.png" alt="Wiring diagram for 8A 2 channels dimmer" style="max-width:100%;height:auto;">
- Dimmer 10A 4 channels connection
<img src="https://www.rbdimmer.com/web/image/2120-dca73414/dimpinout10A4L.png" alt="Wiring diagram for 10A 4 channels dimmer" style="max-width:100%;height:auto;">

---

## Connecting to AC Power and Load

### Power Wiring

The dimmer is connected in series with the load. The **live AC L-IN** wire from the AC source connects to the dimmer input. The dimmer output **AC L-OUT** connects to the load. The **neutral wire AC-N** connects directly to both the zero-cross detector and the load.

### Power Wires and Wire Gauge Selection

The phase wire **AC L-IN**, which carries power through the dimmer to the load **AC L-OUT**, must be sized for the maximum RMS current.

### Wire Gauge Calculation

Use these formulas to determine the minimum cross-section:

- **Copper wire:** `S (mm2) = I (A) / 8`
- **Aluminum wire:** `S (mm2) = I (A) / 5`

These formulas are based on safe current density and heat dissipation (Joule-Lenz law):

```text
P = I^2 x R, where R = rho x L / S
```

**Where:**

- `P` = heat (W)
- `I` = current (A)
- `R` = resistance (Ohm)
- `rho` = material resistivity (Ohm*mm2/m)
- `L` = wire length (m)
- `S` = wire cross-section (mm2)

### Wire Gauge Table

If you're unfamiliar with these formulas, refer to the table below:

| Dimmer Rating | Copper Wire Min. Cross-Section | Aluminum Wire Min. Cross-Section |
|---------------|--------------------------------|----------------------------------|
| 4A | 0.5 mm2 | 0.8 mm2 |
| 8A | 1.0 mm2 | 1.6 mm2 |
| 10A | 1.5 mm2 | 2.0 mm2 |
| 16A | 2.5 mm2 | 4.0 mm2 |
| 24A | 3.0 mm2 | 5.0 mm2 |
| 40A | 5.0 mm2 | 8.0 mm2 |

> [!NOTE]
> This applies to single-core wires. For stranded wires, multiply the area by 1.2.

### Recommended Wire Type

**Copper wire** is strongly recommended for most dimmer-based projects due to:

- Better conductivity (rho = 0.0175 Ohm*mm2/m)
- Flexibility and long lifespan
- Oxidation resistance

**Aluminum wire** may be used in some cases but:

- Has higher resistivity (rho = 0.028 Ohm*mm2/m)
- Requires special connectors
- Not suitable for flexible connections

> [!TIP]
> If your home uses copper wires, continue using copper in your project.

### Neutral Wire (AC-N) for Zero-Cross

The neutral wire connected to the zero-cross detector carries very little current (typically 5-10 mA). It does not power the load, so it can be much thinner.

**Recommended size:** 0.25-0.5 mm2 or AWG22, standard for signal wires.

---

## Circuit Protection

### Choosing a Fuse

Every high-voltage circuit must include a fuse:

- Prevents damage from shorts or wiring errors
- Protects against water/dust-related faults
- Prevents overload damage to the dimmer

### Fuse Rating Calculation

Use the following formula:

```text
I(fuse) = I(load) x 1.25
```

Do not exceed the dimmer's rated current.

> [!TIP]
> **Example:** 1000W load at 220V = 4.5A
>
> `4.5A x 1.25 = 5.6A` --> Choose a 6A fuse.

> [!IMPORTANT]
> **2x current rating rule also applies here.** If your dimmer module is rated at 8A but your load only draws 4A, size the fuse for the load (5A), not for the module. The fuse protects the wiring and the load -- the dimmer's own rating is the upper ceiling.

### Fuse vs Circuit Breaker

| Fuses | Circuit Breakers |
|-------|------------------|
| Inexpensive | More expensive |
| Fast-acting and reliable | Resettable |
| Must be replaced after tripping | Convenient |

> [!TIP]
> **Recommendation:** Use fuses for DIY projects.
>
> If using breakers, choose a quality brand for fast response.

### Fuse Placement

Place the fuse **before the dimmer** on the **AC L-IN** wire.

For added safety, a second fuse may be added after the dimmer **AC L-OUT** if your load is sensitive or prone to shorts.

---

## General Wiring Recommendations

When connecting a load, always ensure that all electrical connections are securely insulated. Use terminal blocks or dedicated wire connectors. Never leave exposed wire ends, especially when working with high-voltage circuits.

### Insulation and Grounding

- Always place the dimmer and all electrical connections inside an **insulated enclosure** that prevents accidental contact
- If your device has a metal enclosure, it must be connected to **protective earth (ground)**
- Use insulation materials rated for at least **400V** to ensure proper safety margins

### Low-Voltage Side Wiring (ESP32 to Dimmer)

- **Wire type**: Standard jumper wires or 22-24 AWG stranded
- **Length**: Keep connections short (< 30 cm recommended)
- **Shielding**: Not required for short runs
- **Separation**: Keep away from AC wiring

### Jumper Wire Recommendations

- Avoid routing jumper wires near or across AC lines
- Do not touch jumper wires during operation, as your body can introduce electrical noise or distort signals

---

## Safety Guidelines

### High-Voltage Warning

> [!CAUTION]
> **WARNING!** Working with 110-220V AC mains voltage is potentially fatal.
>
> Always follow these essential safety rules:
>
> - **Never work on a device while it is connected to the power supply**
> - Always make sure the device is unplugged before beginning any work
> - Use tools with insulated handles
> - Do not touch bare wires or live contacts
> - Never operate or assemble the dimmer in humid or dusty environments
> - Never bypass safety isolations
> - Never use homemade AC switching circuits
> - Use only certified, isolated dimmer modules
>
> If your device must be used outdoors or in harsh conditions, use an enclosure rated at **IP67 or higher** to ensure protection from moisture and dust.

### Safety Checklists

#### Pre-Power Checklist

- [ ] Power is OFF at breaker / mains unplugged
- [ ] Circuits verified dead with non-contact voltage tester
- [ ] All AC connections are secure (screw terminals tightened)
- [ ] No exposed conductors anywhere
- [ ] Dimmer module ratings match or exceed load requirements
- [ ] All components inspected for physical damage
- [ ] Fuse installed and correctly rated
- [ ] ESP32 ground connected to dimmer module ground
- [ ] Lock-out / tag-out procedures followed (if applicable)

#### First Power-Up Checklist

- [ ] Start with dimmer level set to minimum (or 0%)
- [ ] Have shutdown procedure ready (know where the breaker / kill switch is)
- [ ] Power ESP32 first (low voltage only) and confirm zero-cross detection before applying AC
- [ ] Apply AC power and verify frequency stabilizes to 50 or 60 Hz
- [ ] Slowly increase dimmer level; watch for smoke, sparks, unusual noise
- [ ] Monitor dimmer module temperature by touch (power off first!) or IR thermometer
- [ ] Verify load responds linearly to level changes

#### Operational Safety

- **Weekly**: Visual check for damage or overheating signs
- **Monthly**: Check connection tightness (power off first)
- **Annually**: Professional inspection for commercial installations
- **Immediate shutdown required** if you notice: burning smell, visible sparks or arcing, unusual noises, excessive heat, or flickering/erratic operation

---

## Connecting to a Microcontroller

### ESP32 GPIO Pin Recommendations

#### Best Pins for Zero-Cross Detection (ZC)

- **Recommended**: GPIO 2, 4, 5, 12, 13, 14, 15, 25, 26, 27, 32, 33, 34, 35, 36, 39
- **Avoid**: GPIO 0, 1, 3 (boot strapping and serial TX/RX)
- **Avoid**: GPIO 6-11 (connected to internal flash memory -- do not use)

GPIO 34-39 are input-only, which is fine for ZC detection.

#### Best Pins for TRIAC Control (DIM)

- **Recommended**: GPIO 2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
- **Avoid**: GPIO 0, 1, 3 (boot/serial), GPIO 6-11 (flash), GPIO 34-39 (input-only)

> [!TIP]
> For multi-channel setups, prefer pins with lower numbers for DIM outputs. Keep ZC and DIM pins on the same GPIO bank when possible to reduce ISR latency.

### ESP32 Development Boards

| Board | Pros | Cons | Best For |
|-------|------|------|----------|
| **ESP32 DevKit V1** | Widely available, good docs | Basic features only | General projects |
| **ESP32-WROOM-32** | Stable, well-tested | Older design | Production systems |
| **ESP32-S3-DevKitC** | USB native, more memory | Higher cost | Advanced projects |
| **ESP32-C3-DevKitM** | RISC-V, lower power | Limited GPIO | Battery projects |
| **Wemos D1 Mini ESP32** | Compact, breadboard friendly | Limited GPIO access | Space-constrained |

### Connecting the Zero-Cross (Z-C) Output to an Interrupt Pin

The zero-cross detection **Z-C** output must be connected to a microcontroller pin that supports external interrupts. This allows the microcontroller to detect the exact moment the AC signal crosses zero and respond immediately.

- **Arduino (ATmega):** Use digital pins 2 or 3 (e.g., on Uno or Nano boards)
- **ESP8266:** Most GPIO pins support interrupts -- nearly any can be used
- **ESP32:** Any GPIO pin can be used for interrupts (see recommendations above)

### Connecting the Dimmer Pins (DIM)

The Dim control pin **DIM** can be connected to any available GPIO on the microcontroller.

Refer to the table below for recommended GPIO pins for various microcontroller families.

| Board | Pin Zero Cross (Z-C) | Pin Dim (DIM) |
|-------|----------------------|---------------|
| Leonardo | D7 (NOT CHANGEABLE) | D0-D6, D8-D13 |
| Mega | D2 (NOT CHANGEABLE) | D0-D1, D3-D70 |
| UNO / NANO | D2 (NOT CHANGEABLE) | D0-D1, D3-D20 |
| ESP8266 | D1(GPIO5), D5(GPIO14), D7(GPIO13), D2(GPIO4), D6(GPIO12), D8(GPIO15) | D0(GPIO16), D2(GPIO4), D6(GPIO12), D8(GPIO15), D1(GPIO5), D5(GPIO14), D7(GPIO13) |
| ESP32 | See GPIO recommendations above | See GPIO recommendations above |
| Arduino M0 / Arduino Zero | D7 (NOT CHANGEABLE) | D0-D6, D8-D13 |
| Arduino Due | D0-D53 | D0-D53 |
| STM32, Blue Pill (STM32F1) | PA0-PA15, PB0-PB15, PC13-PC15 | PA0-PA15, PB0-PB15, PC13-PC15 |

### VCC Power Requirements

The logic power supply **VCC** for the dimmer must match the logic level of your microcontroller -- not the main power supply used in your project.

> [!IMPORTANT]
> **Important:** Do not connect **VCC** to 12V or any higher voltage, even if your system uses such voltages. This can damage both the dimmer and your microcontroller.

| Microcontroller | Recommended VCC |
|-----------------|-----------------|
| ATmega (e.g., Uno, Nano, Mega) | 5V or 3.3V (depending on your project's logic level) |
| ESP8266 | 3.3V |
| ESP32 | 3.3V (or 1.8V in low-voltage designs) |
| STM32 | 3.3V |

#### ESP32 Power Budget

- **Voltage**: 3.3V regulated
- **Current**: 240 mA typical, 500 mA peak (Wi-Fi TX bursts)
- **Power**: ~1.2W continuous

> [!NOTE]
> Most dimmer modules are self-powered from AC mains for their high-voltage side. The VCC/GND connections only power the optocoupler LED and logic-level output. Typical draw is < 50 mA on the 3.3V rail.

---

## Dimmer Versions with Cooling Fan

For dimmers that include a fan, the fan is powered by **DC 5V**.

> [!NOTE]
> Fan power is independent of the AC load and the dimmer's high-voltage section.

## Dimmer Versions with Temperature Control

### Temperature Sensor Pin (TEMP)

If your dimmer includes a built-in temperature sensor, connect its **TEMP** output to an analog input (ADC pin) on your microcontroller.

### Fan Control Pin (FAN)

The fan control input can be connected to any GPIO on your microcontroller.

Temperature-monitored dimmers can track the temperature of their power stage and automatically prevent overheating or hardware failure.

The official software library for this dimmer model includes:

- Dynamic fan speed control based on real-time temperature
- Critical temperature alerts

---

## Load Compatibility

### Compatible Load Types

**Resistive loads** (incandescent bulbs, heaters, hot plates):
- Constant power factor, no inrush current issues
- Linear response to dimming -- best compatibility
- Recommended curve: `RBDIMMER_CURVE_RMS`

**LED loads** (dimmable LED bulbs):
- Must be rated "dimmable" -- non-dimmable LEDs will not work correctly
- May require logarithmic curve for perceptually smooth dimming
- Recommended curve: `RBDIMMER_CURVE_LOGARITHMIC`
- Test compatibility before deploying

> [!IMPORTANT]
> Use "dimmable"-rated LED bulbs only. Non-dimmable LEDs with built-in switch-mode drivers will flicker or fail.

**Motor loads** (fans, small motors):
- Inductive loads with high starting current
- May require soft-start (gradual ramp)
- Recommended curve: `RBDIMMER_CURVE_LINEAR`
- Use appropriate TRIAC rating for inrush

### Incompatible Loads

- **Electronic ballasts** (fluorescent lights) -- may cause interference or damage
- **Switch-mode power supplies** (computer PSUs, most modern electronics) -- phase control causes problems
- **Large unprotected motors and transformers** (except dimmer-rated types)

---

## Testing and Validation

### Test 1: Zero-Cross Detection

```cpp
// Test code for zero-cross detection
void test_zero_cross() {
    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 0);

    // Monitor frequency detection
    for (int i = 0; i < 30; i++) {
        delay(1000);
        uint16_t freq = rbdimmer_get_frequency(0);
        Serial.printf("Detected frequency: %d Hz\n", freq);
    }
}
```

**Expected results:**
- Frequency should stabilize to 50 or 60 Hz within 10-20 seconds
- Should not fluctuate more than +/-1 Hz
- If `rbdimmer_get_frequency()` returns 0, check ZC wiring

### Test 2: Gate Control (no AC load)

```cpp
void test_gate_control() {
    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 50);

    rbdimmer_config_t config = {4, 0, 0, RBDIMMER_CURVE_LINEAR};
    rbdimmer_channel_t* channel;
    rbdimmer_create_channel(&config, &channel);

    // Test different levels
    for (int level = 0; level <= 100; level += 10) {
        rbdimmer_set_level(channel, level);
        Serial.printf("Level: %d%%, Delay: %d us\n",
                      level, rbdimmer_get_delay(channel));
        delay(2000);
    }
}
```

### Test 3: Load Response (AC connected)

> [!CAUTION]
> Ensure all safety procedures are followed before applying AC power.

```cpp
void test_load_response() {
    rbdimmer_init();
    rbdimmer_register_zero_cross(2, 0, 0);

    rbdimmer_config_t config = {4, 0, 10, RBDIMMER_CURVE_RMS};
    rbdimmer_channel_t* channel;
    rbdimmer_create_channel(&config, &channel);

    Serial.println("Starting load test - 10% to 90%");
    delay(2000);

    // Gradual increase to avoid inrush
    for (int level = 10; level <= 90; level += 10) {
        rbdimmer_set_level_transition(channel, level, 1000);
        delay(2000);
        Serial.printf("Level: %d%%\n", level);
    }

    // Return to safe level
    rbdimmer_set_level_transition(channel, 10, 1000);
}
```

### Performance Validation

- **Timing accuracy**: Use an oscilloscope to verify zero-cross to gate-trigger timing is consistent across half-cycles
- **Temperature**: Monitor dimmer module temperature under sustained load; must remain within safe limits
- **Interference**: Check for RF interference with other devices at various power levels

---

## Troubleshooting

### No Zero-Cross Detection

**Symptoms:** `rbdimmer_get_frequency()` returns 0, no dimming response.

```cpp
void diagnose_zero_cross() {
    pinMode(2, INPUT);
    while (true) {
        int state = digitalRead(2);
        Serial.printf("ZC Pin State: %d\n", state);
        delay(100);
    }
}
```

**Solutions:**
1. Check wiring connections (ZC output to correct ESP32 GPIO)
2. Verify dimmer module has AC power (neutral connected)
3. Test ZC output with multimeter on DC side only
4. Replace dimmer module if faulty

### TRIAC Not Switching

**Symptoms:** Load does not respond to level changes, stuck on or off.

**Solutions:**
1. Verify DIM pin wiring and GPIO number in code
2. Check gate control pin with an LED indicator
3. Test with a different GPIO pin
4. Confirm dimmer module is rated for your load current

### Erratic Dimming / Flickering

**Possible causes:** Power supply noise, electrical interference, load incompatibility, poor connections.

**Solutions:**
1. Add power supply filtering / decoupling capacitors
2. Check all connection tightness
3. Test with a known-good resistive load (incandescent bulb)
4. Add ferrite cores on signal cables
5. In v2.0.0: verify `ZC_DEBOUNCE_US` is set appropriately -- too low allows TRIAC spike re-triggers; too high may clip legitimate ZC edges

---

> [!TIP]
> Your dimmer-based project is now wired up -- great job!
>
> Next, let's move on to writing your code and integrating the library or software components.

