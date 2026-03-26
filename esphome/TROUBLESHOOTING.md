# Troubleshooting

This guide covers the most common issues encountered when using the rbdimmer ESPHome component, along with diagnostic steps and solutions.

---

## Quick Diagnostic Checklist

Before diving into specific issues, run through this checklist:

- [ ] Framework is `esp-idf`, not `arduino`
- [ ] `external_components` path points to the folder containing the `rbdimmer/` directory
- [ ] ZC pin is connected to the dimmer module's Z-C output (not the DIM output)
- [ ] DIM pin(s) are connected to the dimmer module's DIM input(s)
- [ ] Dimmer module has power (VCC and GND connected to ESP32's 3.3V and GND)
- [ ] AC mains is connected to the dimmer module's AC side
- [ ] Load is connected between the dimmer's AC-OUT and neutral
- [ ] No GPIO conflicts (ZC pin and DIM pin are not used by other components)
- [ ] Logger level is set to `DEBUG` during initial testing

---

## Issue: Light Flickers or Brightness is Unstable

### Symptoms

- The connected bulb or load flickers at certain brightness levels.
- Brightness jumps when the dimmer slider is moved slowly.
- Flickering is worse at low brightness levels (below 20%).

### Causes and Solutions

**1. ZC wiring problem — most common cause**

The zero-cross signal is being triggered by noise from TRIAC switching rather than the actual AC zero-crossing.

- Check that the ZC wire from the dimmer module to the ESP32 is short (under 30 cm) and not routed alongside the AC power wires.
- The rbdimmerESP32 v2.0.0 library includes a hardware noise gate (`ZC_DEBOUNCE_US = 3000 µs`) that filters spurious re-triggers. If you have a very old version of the library, update to v2.0.0.

**2. Shared ground loop**

The dimmer module's isolated GND is not the same as AC neutral. If you have connected them together, you have introduced a noise path.

- The GND connection on the dimmer module's low-voltage side should connect only to ESP32 GND. Do not connect it to AC neutral.

**3. Wrong curve for the load**

At 50% brightness with the `rms` curve, an LED bulb may flicker because the power waveform does not match the LED driver's requirements.

- Try switching to `logarithmic` for dimmable LED loads.
- Try `linear` for resistive loads or motors.
- Use the curve select entity to test different curves without reflashing.

**4. Non-dimmable LED load**

Non-dimmable LED bulbs have switching power supply drivers that do not tolerate phase-cut control. They will flicker regardless of settings.

- Replace with a bulb explicitly rated "dimmable".

**5. Flickering below 3%**

Levels below 3% are treated as OFF by the library. If your slider is in the 0–3% range, the light will appear off or flicker at the threshold.

- This is expected behavior. The effective dimming range starts at 3%.

---

## Issue: No Dimming Response — Light Does Not Respond

### Symptoms

- The light entity appears in Home Assistant but turning it on or changing brightness has no effect.
- The connected load stays fully off.

### Diagnostic Steps

**Step 1: Check the logs**

Enable debug logging and look for errors from the hub and light components:

```yaml
logger:
  level: DEBUG
```

After flashing, watch the boot logs in the ESPHome console or Home Assistant. Look for lines like:

```
[rbdimmer.hub] Zero-cross registered: pin=23, phase=0, freq=0
[rbdimmer.light] Channel created: pin=25, phase=0, curve=1
```

If you see `Failed to initialize` or `Failed to create channel`, the library initialization failed. Check the error code logged alongside.

**Step 2: Verify pin numbers**

The most common mistake is entering the wrong GPIO number. In ESPHome, `GPIO25` means GPIO number 25, which is pin 25 on an ESP32. Verify that the physical jumper wire from your dimmer module's DIM output actually goes to GPIO25 on your board.

**Step 3: Check the ZC signal**

If the ZC detector is not receiving an AC signal, the library initializes but the TRIAC output never fires (there are no zero-cross events to synchronize to).

- Add an AC frequency sensor to your config and watch its value in Home Assistant.
- If it reads 0 Hz after more than 2 seconds of boot, the ZC pin is not receiving a signal.

**Step 4: Swap pins and test**

Try reassigning the DIM pin to a known-working GPIO. GPIO2 on ESP32 is a safe test pin (it has an onboard LED on most DevKit boards, so you can also test it visually with a multimeter).

---

## Issue: AC Frequency Sensor Shows 0 Hz

### Symptoms

- The `ac_frequency` sensor reports `0` in Home Assistant.
- The dimmer seems to work but frequency shows 0.

### Explanation

This is almost always normal during the first ~0.5 seconds after power-on. The library measures frequency by counting 50 zero-cross pulses. At 50 Hz, this takes approximately 0.5 seconds. At 60 Hz, it takes approximately 0.4 seconds.

If the sensor is still showing 0 Hz after 5 seconds:

1. Check that AC mains is applied to the dimmer module's AC input side.
2. Check that the ZC output from the dimmer module is wired to the correct ESP32 GPIO.
3. Verify the `zero_cross_pin` in your YAML matches the physical wiring.
4. Check the logs for: `Zero-cross registered: pin=XX, phase=0, freq=0` — if this line is missing, the hub failed to initialize.

> ⚠️ The frequency sensor requires AC power to be present. If the dimmer module has no AC input, the ZC output will produce no pulses and the frequency will remain 0.

---

## Issue: Compilation Errors

### Error: `esp-idf framework required`

This component requires the ESP-IDF framework. Your `esp32:` block must be:

```yaml
esp32:
  board: esp32dev
  framework:
    type: esp-idf
```

If you see errors like `rbdimmerESP32.h: No such file or directory` or CMake errors referencing missing IDF components, the most likely cause is using `type: arduino` or not having a `framework:` block at all.

### Error: `component rbdimmer not found`

Check that your `external_components` block points to the correct directory. The `path` must be the **parent** directory of the `rbdimmer/` folder.

```
components/          <-- this is the path you specify
    rbdimmer/
        __init__.py
        light.py
        ...
```

Correct config:

```yaml
external_components:
  - source:
      type: local
      path: components   # correct: parent of rbdimmer/
    components: [rbdimmer]
```

Common mistake:

```yaml
external_components:
  - source:
      type: local
      path: components/rbdimmer  # wrong: this is the component itself, not its parent
```

### Error: `has_at_least_one_key`

```
Invalid configuration: At least one of phases, zero_cross_pin must be present
```

Your `rbdimmer:` block has neither `zero_cross_pin` nor `phases`. Add one:

```yaml
rbdimmer:
  zero_cross_pin: GPIO23  # add this
```

### Error: `'ESP_TIMER_ISR' undeclared`

```
error: 'ESP_TIMER_ISR' undeclared (first use in this function)
```

The library creates TRIAC timers with `ESP_TIMER_ISR` dispatch mode for sub-millisecond firing precision. This mode requires a Kconfig option that is not enabled by default.

Fix: add `sdkconfig_options` to your `esp32:` framework block:

```yaml
esp32:
  board: esp32dev
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD: "y"
```

This option must be present or the build will always fail with the above error.

### Error: `rbdimmer_id` not found

```
Component rbdimmer not found with id 'dimmer_hub'
```

The `id:` on your hub and the `rbdimmer_id:` on your light do not match. Either correct the spelling or remove `rbdimmer_id:` from the light (if you only have one hub, it is found automatically):

```yaml
rbdimmer:
  id: dimmer_hub          # <-- must match

light:
  - platform: rbdimmer
    rbdimmer_id: dimmer_hub  # <-- must match
```

---

## Issue: Home Assistant Shows Light Entity as Unavailable

### Symptoms

- The light entity appears in Home Assistant but shows as `unavailable`.
- No state is ever published.

### Solutions

1. Check that the ESP32 is connected to Wi-Fi and the API connection is active. Look for `API connected` in the device page or check the binary_sensor.status entity.
2. Check that the API encryption key in your YAML matches the key used when the device was added to Home Assistant. If you changed the key, remove and re-add the device in Home Assistant.
3. Check the boot log for hub initialization failures. If the hub fails (`mark_failed()`), no entities will publish state.

---

## Issue: Curve Select Resets After Reboot

### Explanation

This is expected behavior. The `select: platform: rbdimmer` entity changes the curve at runtime but does not write it to persistent storage. After a reboot, the curve returns to the value configured in the light's `curve:` option in YAML.

### Solution

To permanently change the default curve, update the `curve:` value in your YAML and reflash:

```yaml
light:
  - platform: rbdimmer
    name: "My Light"
    pin: GPIO25
    curve: logarithmic  # change this value and reflash
```

---

## Diagnostic Logging Setup

To capture detailed startup and runtime logs from the component:

```yaml
logger:
  level: DEBUG
  logs:
    rbdimmer.hub: DEBUG
    rbdimmer.light: DEBUG
    rbdimmer.sensor: DEBUG
    rbdimmer.select: DEBUG
```

This filters verbose debug output to only the rbdimmer components, keeping other ESPHome output at the default level.

Expected log output at boot (ESP-IDF framework, DEBUG level):

```
[D][rbdimmer.hub:047]: Zero-cross registered: pin=23, phase=0, freq=0
[I][rbdimmer.hub:050]: RBDimmer hub initialized with 1 phase(s)
[I][rbdimmer.light:046]: Channel created: pin=25, phase=0, curve=1
```

If the hub fails, you will see:

```
[E][rbdimmer.hub:035]: Failed to initialize rbdimmer library: 1
```

The number at the end is the `rbdimmer_err_t` error code:

| Code | Meaning |
|---|---|
| 0 | OK |
| 1 | Invalid argument |
| 2 | No memory |
| 3 | Not found |
| 4 | Already exists |
| 5 | Timer creation failed |
| 6 | GPIO configuration failed |

---

## Frequently Asked Questions

**Q: Can I use the Arduino framework?**

No. This component uses ESP-IDF-specific APIs that are not available in the ESPHome Arduino layer. Use `framework: type: esp-idf`.

**Q: Can I use multiple zero-cross detectors on one ESP32?**

Yes. Use the `phases:` list syntax in the hub and assign each phase a unique phase index (0–3) and a unique GPIO pin.

**Q: Can I have more than 8 channels?**

The library limit is 8 simultaneous channels (4 on ESP32-C3/C6). This is a compile-time constant in the library (`RBDIMMER_MAX_CHANNELS`). For ESP-IDF builds it can be increased via Kconfig, but this is not exposed through the ESPHome component configuration.

**Q: Why does the dimmer not respond immediately after I set the brightness?**

The TRIAC fires on the next zero-crossing event after `rbdimmer_set_level()` is called. At 50 Hz, zero-crossings happen every 10 ms, so the maximum latency from command to hardware response is approximately 10–20 ms. This is imperceptible in normal use.

**Q: The firing delay sensor shows a very large value (e.g., 9900 µs) at low brightness. Is this correct?**

Yes. At 1–3% brightness, the TRIAC fires very late in the half-cycle (close to the next zero-crossing). A firing delay of 9000–9900 µs on a 50 Hz supply (10,000 µs half-cycle) is correct for very low brightness settings. At 50% brightness with the `rms` curve, expect approximately 3300–3500 µs.

**Q: Does the component work with ESPHome installed via Home Assistant Add-on?**

Yes. The `external_components` block with `type: local` is supported in the ESPHome Add-on. Place the `components/` directory in a location accessible to the Add-on (the same folder as your YAML configuration files, or a subdirectory of it).

**Q: Do I need to specify `frequency: 50` or `frequency: 60`?**

No. The default `frequency: 0` (auto-detect) works reliably for all standard mains voltages. Only override this if your mains supply has unusual noise that interferes with frequency detection.

**Q: The light transitions feel jerky. How do I make them smoother?**

Transitions are handled by ESPHome's light component, which interpolates brightness and calls `write_state()` at a regular interval during the transition. If transitions feel jerky, try increasing the `default_transition_length` or using a longer `transition:` value in your automations:

```yaml
light:
  - platform: rbdimmer
    name: "My Light"
    pin: GPIO25
    default_transition_length: 2s  # increase for smoother feel
```

**Q: Can I use this component on an ESP8266?**

No. The rbdimmerESP32 library targets ESP32 family chips only. ESP8266 is not supported.

---

## Getting Help

If this guide does not resolve your issue:

1. Capture the full boot log with `logger: level: DEBUG`.
2. Note the exact error messages from the `rbdimmer.hub` and `rbdimmer.light` log tags.
3. Post on the forum at https://forum.rbdimmer.com with your YAML (redact secrets), your board type, and the log output.
4. For library bugs (not ESPHome component bugs), open an issue at https://github.com/robotdyn-dimmer/rbdimmerESP32/issues.
