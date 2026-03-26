# Lambda & C++ Reference

This chapter documents direct access to the rbdimmerESP32 C library from ESPHome lambda
functions. It is intended for advanced users who need capabilities beyond what the standard
ESPHome entities provide — such as library-native transitions, runtime diagnostics, custom
automations driven by AC frequency, or fine-grained channel control.

---

## 4.1 Lambdas in ESPHome

ESPHome lambdas are inline C++ code blocks embedded in YAML. They are compiled directly into
the firmware, giving you full access to the ESP-IDF/Arduino runtime and to any external
components loaded into the build.

A lambda block looks like:

```yaml
on_boot:
  then:
    - lambda: |-
        ESP_LOGI("app", "Boot lambda running");
```

Inside a lambda you can call any C or C++ function that is linked into the firmware, access
ESPHome component instances with `id(component_id)`, and read/write global variables.

---

## 4.2 Accessing the Channel Handle

The rbdimmer component exposes the underlying library channel handle through the
`get_channel()` method on `RBDimmerLight`. This is the entry point for all direct library
calls.

**Type chain:**

```
id(my_light)                  →  RBDimmerLight*       (ESPHome component)
id(my_light)->get_channel()   →  rbdimmer_channel_t*  (opaque C handle)
```

`rbdimmer_channel_t*` is the handle accepted by every function in `rbdimmerESP32.h`. It is
allocated by the component during `setup()` and remains valid for the entire lifetime of the
firmware.

**Minimum YAML to have a light accessible by lambda:**

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [rbdimmer]

rbdimmer:
  id: dimmer_hub
  zero_cross_pin: GPIO23
  frequency: 0

light:
  - platform: rbdimmer
    name: "Living Room"
    id: my_light
    rbdimmer_id: dimmer_hub
    pin: GPIO25
    phase: 0
    curve: rms
    gamma_correct: 1.0        # disable ESPHome gamma when using library curves
```

> **Note on `gamma_correct: 1.0`:** ESPHome applies a gamma correction curve to all
> monochromatic lights by default (gamma = 2.8). When you use the library's own
> `RBDIMMER_CURVE_RMS` or `RBDIMMER_CURVE_LOGARITHMIC`, the curve compensation is already
> done inside the library. Setting `gamma_correct: 1.0` disables ESPHome's correction and
> prevents double-correction. Always set this when using library curves.

To get the channel handle inside any lambda:

```cpp
rbdimmer_channel_t *ch = id(my_light)->get_channel();
if (ch == nullptr) {
  return;  // component not yet initialized — guard always
}
```

---

## 4.3 Complete API Reference

All functions are declared in `rbdimmerESP32.h` with `extern "C"` linkage. They can be
called from any lambda without additional includes, because the component header already
includes the library header.

### 4.3.1 Level Control

| Function | Parameters | Returns | Description |
|---|---|---|---|
| `rbdimmer_set_level` | `channel`, `level_percent` (uint8_t 0-100) | `rbdimmer_err_t` | Set brightness immediately. Takes effect on the next zero-crossing (~10 ms max latency). Thread-safe. |
| `rbdimmer_set_level_transition` | `channel`, `level_percent` (uint8_t), `transition_ms` (uint32_t) | `rbdimmer_err_t` | Smooth fade to target level. Spawns a FreeRTOS task; returns immediately. Cancels any in-progress transition on the same channel. Minimum meaningful duration: 50 ms. |
| `rbdimmer_get_level` | `channel` | `uint8_t` | Current brightness 0-100. Returns 0 if channel is NULL. |

**Level clamping:** The library clamps values internally. Levels 0-2% result in the channel
being fully OFF. Level 100% is clamped to 99% internally. The effective dimming range is
3%-99%.

### 4.3.2 Curve Configuration

| Function | Parameters | Returns | Description |
|---|---|---|---|
| `rbdimmer_set_curve` | `channel`, `curve_type` (`rbdimmer_curve_t`) | `rbdimmer_err_t` | Change the brightness curve. Takes effect on the next zero-crossing. Thread-safe. |
| `rbdimmer_get_curve` | `channel` | `rbdimmer_curve_t` | Current curve type. Returns `RBDIMMER_CURVE_LINEAR` if channel is NULL. |

**Curve constants:**

| Constant | Value | Best for |
|---|---|---|
| `RBDIMMER_CURVE_LINEAR` | 0 | Motors, heaters, resistive loads |
| `RBDIMMER_CURVE_RMS` | 1 | Incandescent bulbs (constant perceived power) |
| `RBDIMMER_CURVE_LOGARITHMIC` | 2 | Dimmable LED bulbs (matches human perception) |

### 4.3.3 Channel Enable/Disable

| Function | Parameters | Returns | Description |
|---|---|---|---|
| `rbdimmer_set_active` | `channel`, `active` (bool) | `rbdimmer_err_t` | Enable or disable a channel. When disabled: GPIO is set LOW immediately, no timer activity. Configuration is preserved. |
| `rbdimmer_is_active` | `channel` | `bool` | True if channel is active. Returns false if channel is NULL. |

**`set_level(0)` vs `set_active(false)`:** Setting level to 0 keeps the channel active in
the manager (minimal ISR overhead per zero-crossing). `set_active(false)` removes the
channel from ISR processing entirely (zero overhead). Use `set_active(false)` when the
channel must be fully disabled for an extended period.

### 4.3.4 Diagnostics

| Function | Parameters | Returns | Description |
|---|---|---|---|
| `rbdimmer_get_frequency` | `phase` (uint8_t 0-3) | `uint16_t` | Measured mains frequency in Hz. Returns 0 during the initial measurement period (~0.5 s after boot). |
| `rbdimmer_get_delay` | `channel` | `uint32_t` | Current TRIAC firing delay in microseconds, measured from the zero-crossing edge. Returns 0 when the channel is OFF (level < 3%). Minimum non-zero value: 100 µs. |

### 4.3.5 Zero-Cross Callback

| Function | Parameters | Returns | Description |
|---|---|---|---|
| `rbdimmer_set_callback` | `phase` (uint8_t), `callback` (function pointer), `user_data` (void*) | `rbdimmer_err_t` | Register a callback invoked on every valid zero-crossing edge. Pass NULL as callback to deregister. |

**Callback signature:**

```cpp
void my_callback(void *user_data);
```

> **WARNING — ISR context:** The callback executes in GPIO interrupt context (ISR). You must
> observe all ISR constraints: no heap allocation, no blocking calls, no `delay()`, no
> `Serial.print()`, no FreeRTOS blocking APIs. Use `xQueueSendFromISR()` or atomic flags to
> communicate with tasks. Violations will cause a hard fault or watchdog reset.
>
> In v2.0.0 the callback fires only for zero-crossings that pass the noise gate
> (RBDIMMER_ZC_DEBOUNCE_US = 3000 µs). Spurious edges from electrical noise do not invoke
> the callback.

### 4.3.6 Utility

| Function | Parameters | Returns | Description |
|---|---|---|---|
| `rbdimmer_update_all` | none | `rbdimmer_err_t` | Force-recalculate firing delays for all active channels. Normally not needed — channels update automatically on every zero-crossing. |

---

## 4.4 Lambda Examples

### Example 1 — Set Level Directly

**What it does:** Sets a channel to a fixed brightness from a button press or automation.
Use this when you want precise, immediate control without involving the ESPHome light state
machine.

```yaml
button:
  - platform: template
    name: "Set 75%"
    on_press:
      - lambda: |-
          rbdimmer_channel_t *ch = id(my_light)->get_channel();
          if (ch != nullptr) {
            rbdimmer_set_level(ch, 75);
          }
```

---

### Example 2 — Smooth Transition (Library-Native)

**What it does:** Fades the light to a target level using the library's own FreeRTOS
transition task, bypassing ESPHome's transition mechanism entirely. Use this when you need
a transition triggered from a lambda where you cannot easily pass an ESPHome transition
duration, or when you want the library's fixed 20 ms step granularity.

**When to prefer ESPHome transitions instead:** For normal on/off actions triggered from
Home Assistant, prefer `default_transition_length` on the light entity — ESPHome's
mechanism produces smoother integration with HA sliders. See Section 4.5 for a comparison.

```yaml
button:
  - platform: template
    name: "Fade to 0% over 3 s"
    on_press:
      - lambda: |-
          rbdimmer_channel_t *ch = id(my_light)->get_channel();
          if (ch != nullptr) {
            // Third parameter: transition duration in milliseconds
            rbdimmer_set_level_transition(ch, 0, 3000);
          }
```

```yaml
button:
  - platform: template
    name: "Fade to 100% over 5 s"
    on_press:
      - lambda: |-
          rbdimmer_channel_t *ch = id(my_light)->get_channel();
          if (ch != nullptr) {
            rbdimmer_set_level_transition(ch, 100, 5000);
          }
```

---

### Example 3 — Read Current Level as Template Sensor

**What it does:** Exposes the library's internal brightness level (0-100) as an ESPHome
sensor. Useful for diagnostics, dashboards, or automations that need to read actual dimmer
state.

```yaml
sensor:
  - platform: template
    name: "Dimmer Level (raw)"
    id: dimmer_level_sensor
    unit_of_measurement: "%"
    accuracy_decimals: 0
    update_interval: 5s
    lambda: |-
      rbdimmer_channel_t *ch = id(my_light)->get_channel();
      if (ch == nullptr) {
        return 0.0f;
      }
      return static_cast<float>(rbdimmer_get_level(ch));
```

---

### Example 4 — Read AC Frequency as Template Sensor

**What it does:** Publishes the measured mains frequency. Returns `NaN` (not a number, shown
as "unavailable" in Home Assistant) during the ~0.5 s startup measurement period when the
library returns 0.

```yaml
sensor:
  - platform: template
    name: "Mains Frequency"
    id: ac_freq_sensor
    unit_of_measurement: "Hz"
    accuracy_decimals: 0
    update_interval: 60s
    lambda: |-
      uint16_t freq = rbdimmer_get_frequency(0);  // phase 0
      if (freq == 0) {
        return {};  // publish "unavailable" until measured
      }
      return static_cast<float>(freq);
```

> **Note:** `return {};` in a template sensor lambda publishes no value (the sensor stays at
> its last valid state or "unavailable"). Frequency is stable once detected — a 60-second
> update interval is appropriate.

---

### Example 5 — Read Firing Delay

**What it does:** Exposes the TRIAC firing delay in microseconds. This is an advanced
diagnostic value representing the time from zero-crossing to gate pulse. It changes with
brightness level and curve type. Useful for verifying that the library is computing delays
correctly for a given load type.

```yaml
sensor:
  - platform: template
    name: "TRIAC Firing Delay"
    id: firing_delay_sensor
    unit_of_measurement: "µs"
    accuracy_decimals: 0
    update_interval: 10s
    lambda: |-
      rbdimmer_channel_t *ch = id(my_light)->get_channel();
      if (ch == nullptr) {
        return 0.0f;
      }
      return static_cast<float>(rbdimmer_get_delay(ch));
```

At 50% brightness with `RBDIMMER_CURVE_RMS` on 50 Hz mains, a typical delay is around
5000 µs (half of the 10000 µs half-cycle). At level 0 or below LEVEL_MIN (3%), the delay
returns 0.

---

### Example 6 — Change Curve at Runtime

**What it does:** Lets the user switch brightness curve from the UI — for example to toggle
between incandescent and LED profiles without reflashing. The select entity built into the
component (`select: curve:`) is the preferred way to do this, but you can also do it from a
lambda if the logic is conditional.

```yaml
select:
  - platform: template
    name: "Dimmer Curve"
    id: curve_select
    options:
      - "LINEAR"
      - "RMS"
      - "LOG"
    initial_option: "RMS"
    optimistic: true
    on_value:
      - lambda: |-
          rbdimmer_channel_t *ch = id(my_light)->get_channel();
          if (ch == nullptr) return;

          rbdimmer_curve_t curve;
          if (x == "LINEAR") {
            curve = RBDIMMER_CURVE_LINEAR;
          } else if (x == "RMS") {
            curve = RBDIMMER_CURVE_RMS;
          } else {
            curve = RBDIMMER_CURVE_LOGARITHMIC;
          }
          rbdimmer_set_curve(ch, curve);
```

---

### Example 7 — Toggle Channel Active/Inactive (Template Switch)

**What it does:** Creates a switch that fully disables the dimmer channel hardware (no ISR
activity, GPIO held LOW) without going through the ESPHome light entity. Use this when you
want a hard hardware disable that persists regardless of the stored brightness level — for
example, a safety cutoff or a holiday mode.

```yaml
switch:
  - platform: template
    name: "Dimmer Hardware Enable"
    id: dimmer_hw_enable
    optimistic: true
    restore_mode: RESTORE_DEFAULT_ON
    turn_on_action:
      - lambda: |-
          rbdimmer_channel_t *ch = id(my_light)->get_channel();
          if (ch != nullptr) {
            rbdimmer_set_active(ch, true);
          }
    turn_off_action:
      - lambda: |-
          rbdimmer_channel_t *ch = id(my_light)->get_channel();
          if (ch != nullptr) {
            rbdimmer_set_active(ch, false);
          }
```

---

### Example 8 — Force Update All Channels

**What it does:** Forces the library to recalculate and re-apply firing delays for every
active channel. Normally this happens automatically on every zero-crossing. You might call
this after a sequence of bulk configuration changes (e.g., changing multiple curves in a
loop) to ensure all channels reflect the new settings before the next ZC edge.

```yaml
button:
  - platform: template
    name: "Force Dimmer Update"
    on_press:
      - lambda: |-
          rbdimmer_update_all();
```

---

### Example 9 — Conditional Dimming: Switch Curve Based on Brightness

**What it does:** Dynamically selects the brightness curve based on the current light level.
At high brightness (above 50%) the RMS curve is used for accurate power delivery; below 50%
the logarithmic curve is used for perceptually smooth low-light dimming. This is a practical
technique for loads that behave differently at low levels (e.g., some dimmable LED drivers).

```yaml
light:
  - platform: rbdimmer
    name: "Smart Dimmer"
    id: smart_light
    rbdimmer_id: dimmer_hub
    pin: GPIO25
    phase: 0
    curve: rms
    gamma_correct: 1.0
    on_state:
      - lambda: |-
          rbdimmer_channel_t *ch = id(smart_light)->get_channel();
          if (ch == nullptr) return;

          uint8_t level = rbdimmer_get_level(ch);

          if (level > 50) {
            rbdimmer_set_curve(ch, RBDIMMER_CURVE_RMS);
          } else if (level > 0) {
            rbdimmer_set_curve(ch, RBDIMMER_CURVE_LOGARITHMIC);
          }
          // level == 0: leave curve unchanged (doesn't matter when off)
```

---

### Example 10 — Night Mode Automation (Time-Based Dimming)

**What it does:** Sets up a time-based automation that reduces brightness to a night-mode
level at a configured time, and restores full brightness in the morning. Uses a library
transition for the initial fade-in, then hands control back to ESPHome's normal mechanisms.

This example requires the `time` component to be configured (e.g., Home Assistant time
source or SNTP).

```yaml
time:
  - platform: homeassistant
    id: ha_time

light:
  - platform: rbdimmer
    name: "Hallway Light"
    id: hallway_light
    rbdimmer_id: dimmer_hub
    pin: GPIO25
    phase: 0
    curve: logarithmic
    gamma_correct: 1.0
    default_transition_length: 2s

interval:
  - interval: 60s
    then:
      - lambda: |-
          auto now = id(ha_time).now();
          if (!now.is_valid()) return;

          rbdimmer_channel_t *ch = id(hallway_light)->get_channel();
          if (ch == nullptr) return;

          int hour = now.hour;

          if (hour >= 22 || hour < 6) {
            // Night mode: fade to 10% over 30 seconds
            rbdimmer_set_level_transition(ch, 10, 30000);
          } else if (hour == 6) {
            // Morning: fade to 80% over 60 seconds
            rbdimmer_set_level_transition(ch, 80, 60000);
          }
          // Other hours: ESPHome controls brightness normally via HA
```

> **Note:** The `interval` lambda runs every 60 seconds, so transitions will be re-triggered
> periodically during the night/morning hours. The library handles this gracefully —
> `rbdimmer_set_level_transition()` cancels any in-progress transition before starting a new
> one. If you want the transition to fire only once at the boundary hour, track state with a
> `global` variable.

---

## 4.5 ESPHome Transitions vs Library Transitions

Both ESPHome and the rbdimmerESP32 library provide smooth brightness transitions. They work
differently and serve different purposes.

| Aspect | ESPHome transition | Library transition (`rbdimmer_set_level_transition`) |
|---|---|---|
| **Trigger** | `transition_length` parameter on a light call | Direct function call in lambda |
| **Mechanism** | ESPHome loop calls `write_state()` repeatedly with interpolated brightness | FreeRTOS task created inside the library |
| **Step rate** | Every ESPHome loop tick (~10-20 ms) | Fixed 20 ms step interval |
| **Integration** | Full HA slider integration, transition shows in UI | Opaque to ESPHome; light state in HA may not update during transition |
| **Cancelable** | By any new light command from HA | Automatically canceled by the next `rbdimmer_set_level_transition()` call |
| **Concurrent transitions** | One per light entity | One per channel (new call cancels current) |
| **Recommended for** | All normal use (HA, automations, scenes) | Lambda automations, night mode, custom time-based fades where HA feedback is not needed |

**Recommendation:** For the majority of use cases, rely on ESPHome transitions
(`default_transition_length` or `transition_length` in service calls) and call only
`rbdimmer_set_level()` (immediate) from lambdas. Reserve `rbdimmer_set_level_transition()`
for scenarios where the transition should complete asynchronously without ESPHome
involvement.

---

## 4.6 Thread Safety

The library is designed to be called from any FreeRTOS task context. The following table
summarizes what is safe to call from different contexts.

| Function | Task context (lambda, automation) | ISR context (ZC callback) |
|---|---|---|
| `rbdimmer_set_level` | Safe | Safe (writes volatile fields) |
| `rbdimmer_set_level_transition` | Safe | **Not safe** — spawns a FreeRTOS task |
| `rbdimmer_get_level` | Safe | Safe (read-only) |
| `rbdimmer_set_curve` | Safe | Safe (writes volatile fields) |
| `rbdimmer_get_curve` | Safe | Safe (read-only) |
| `rbdimmer_set_active` | Safe | Safe (writes volatile field) |
| `rbdimmer_is_active` | Safe | Safe (read-only) |
| `rbdimmer_get_frequency` | Safe | Safe (read-only) |
| `rbdimmer_get_delay` | Safe | Safe (read-only) |
| `rbdimmer_set_callback` | Safe | **Not safe** — modifies phase table |
| `rbdimmer_update_all` | Safe | **Not safe** — iterates mutable state |
| `rbdimmer_create_channel` | Safe | **Not safe** — calls `malloc` |
| `rbdimmer_delete_channel` | Safe | **Not safe** — calls `free` |
| `id(my_light)->get_channel()` | Safe | Safe (pointer read only) |

All ESPHome lambda functions execute in task context, so all functions in this document's
examples are safe to use. The ISR column applies only if you register a custom zero-cross
callback with `rbdimmer_set_callback()`.

> **ISR callback rules (summary):** Keep callback execution under 10 µs. Never allocate
> memory. Never call FreeRTOS blocking APIs (`xQueueSend` blocking form, `vTaskDelay`,
> `xSemaphoreTake` without `0` timeout). Use the `FromISR` variants of FreeRTOS primitives
> (`xQueueSendFromISR`, `xSemaphoreGiveFromISR`). Use `portYIELD_FROM_ISR()` if your ISR
> woke a higher-priority task.

---

## 4.7 Common Patterns and Pitfalls

**Always guard the channel pointer:**

```cpp
rbdimmer_channel_t *ch = id(my_light)->get_channel();
if (ch == nullptr) return;  // or return 0.0f in sensor lambdas
```

The channel pointer is NULL before `setup()` completes. In practice this can only happen
if the component failed to initialize (wrong GPIO, no AC power at boot), but the guard
makes the code robust.

**Do not call `rbdimmer_init()` or `rbdimmer_register_zero_cross()` from lambdas:**

These are called by the hub component during `setup()`. Calling them again from a lambda
will return `RBDIMMER_ERR_ALREADY_EXIST` at best and corrupt internal state at worst.

**Do not mix library transitions with ESPHome transitions on the same channel:**

If a library transition is running and ESPHome calls `write_state()` (e.g., from a slider
move in HA), `rbdimmer_set_level()` will be called each tick, which will not cancel the
running library transition. The two will fight. If you use library transitions, avoid
ESPHome `transition_length > 0` for the same light entity during that period, or call
`rbdimmer_set_level()` (no transition) from `write_state()`.

**`gamma_correct: 1.0` is required when using library curves:**

ESPHome applies `gamma = 2.8` by default to convert the linear HA brightness slider to a
perceptual scale. The rbdimmer library curves (`RMS`, `LOGARITHMIC`) already incorporate
their own transfer functions. If you leave ESPHome gamma enabled (the default), the
brightness transformation is applied twice and the dimmer will behave non-linearly in an
unexpected way, particularly at low levels. Set `gamma_correct: 1.0` on every rbdimmer
light entity.

**Frequency returns 0 for ~0.5 s after boot:**

`rbdimmer_get_frequency(0)` returns 0 until the library has measured 50 zero-cross pulses.
In template sensor lambdas, use `return {};` (empty optional) to avoid publishing 0 Hz to
Home Assistant during this period.
