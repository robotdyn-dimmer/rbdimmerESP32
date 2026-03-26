# Entities Reference

This document describes every configuration option for each platform provided by the `rbdimmer` component. All YAML blocks are copy-paste ready.

---

## Hub (`rbdimmer:`)

The hub is the root component. It must be declared before any light, sensor, or select entities. It initializes the rbdimmerESP32 library and registers one or more zero-cross detectors.

### Single-Phase (Shorthand)

Use this form when you have a single-phase AC supply:

```yaml
rbdimmer:
  id: dimmer_hub
  zero_cross_pin: GPIO23
  frequency: 0
```

> 💡 `frequency: 0` means auto-detect. This is the recommended value for all installations.

### Multi-Phase (Explicit Form)

Use this form when you have two or more independent AC phases, each with its own zero-cross detector:

```yaml
rbdimmer:
  id: dimmer_hub
  phases:
    - phase: 0
      zero_cross_pin: GPIO23
      frequency: 0
    - phase: 1
      zero_cross_pin: GPIO22
      frequency: 0
    - phase: 2
      zero_cross_pin: GPIO21
      frequency: 0
```

> ⚠️ You must use either `zero_cross_pin` (shorthand) or `phases` (explicit). Using both at the same time is a configuration error.

### Hub Configuration Variables

| Variable | Type | Required | Default | Description |
|---|---|---|---|---|
| `id` | ID | No | auto-generated | Reference ID for this hub. Required if you use multiple hubs or reference it explicitly in other entities. |
| `zero_cross_pin` | GPIO pin | Yes (shorthand) | — | GPIO input pin connected to the dimmer module's Z-C output. Used when configuring a single phase. |
| `frequency` | integer | No | `0` | Mains frequency hint. `0` = auto-detect (recommended). `50` = force 50 Hz. `60` = force 60 Hz. Range: 0–65. |
| `phases` | list | Yes (multi-phase) | — | List of phase configurations. Each phase entry requires `phase`, `zero_cross_pin`, and optionally `frequency`. |
| `phases[].phase` | integer | Yes | — | Phase index. `0` to `3`. Each must be unique. |
| `phases[].zero_cross_pin` | GPIO pin | Yes | — | GPIO input pin for this phase's zero-cross signal. |
| `phases[].frequency` | integer | No | `0` | Same as top-level `frequency`, but per-phase. |

> 💡 The hub initializes at `setup_priority::HARDWARE` — the highest available priority. All light entities initialize at `HARDWARE - 1`, ensuring the hub is always ready before any channel is created.

---

## Light Platform (`platform: rbdimmer`)

Each light entity maps to one TRIAC output channel. The light appears in Home Assistant as a dimmable light with a brightness slider and supports transitions.

### Minimal Light

```yaml
light:
  - platform: rbdimmer
    name: "Living Room"
    pin: GPIO25
```

### Full Light Configuration

```yaml
light:
  - platform: rbdimmer
    name: "Living Room Light"
    id: dimmer_ch1
    rbdimmer_id: dimmer_hub
    pin: GPIO25
    phase: 0
    curve: rms
    gamma_correct: 1.0
    default_transition_length: 1s
```

### Light Configuration Variables

| Variable | Type | Required | Default | Description |
|---|---|---|---|---|
| `name` | string | Yes | — | Entity name in Home Assistant. |
| `id` | ID | No | auto-generated | Internal ID. Required if a sensor or select references this light. |
| `rbdimmer_id` | ID | No | auto (first hub) | ID of the `rbdimmer:` hub. Only required when multiple hubs are defined. |
| `pin` | GPIO pin | Yes | — | Output GPIO connected to the dimmer module's DIM input. Must be a standard output-capable pin. |
| `phase` | integer | No | `0` | Phase index this channel belongs to. Must match a phase registered in the hub. Range: 0–3. |
| `curve` | enum | No | `rms` | Brightness curve algorithm. See table below. |
| `gamma_correct` | float | No | `1.0` | ESPHome gamma correction applied before passing brightness to the library. `1.0` = no correction (recommended — the library's curves already handle perceptual correction). |
| `default_transition_length` | time | No | `1s` | Default fade duration when turning on or off from Home Assistant. |

### Curve Options

| Value | YAML key | Best for | Behavior |
|---|---|---|---|
| Linear | `linear` | Resistive heaters, motors, fans | Direct percentage-to-phase-angle mapping. Power output is proportional to the slider position but perceived brightness is not linear. |
| RMS | `rms` | Incandescent bulbs | Compensates for the RMS characteristics of sinusoidal AC. Power output is proportional to the square of the level. Slider feels perceptually smooth for filament loads. |
| Logarithmic | `logarithmic` | Dimmable LED bulbs | Applies a logarithmic transfer function matching human brightness perception. Small slider changes at the low end produce the same visual step as larger changes at the high end. |

> 💡 When in doubt, start with `rms`. It is the default and works acceptably for most load types. Switch to `logarithmic` for LED loads if you notice the bottom third of the slider range feels compressed.

### Effective Dimming Range

The library enforces a safe operating range internally:

| Level sent | Actual behavior |
|---|---|
| 0% | OFF — no TRIAC firing |
| 1–2% | Treated as OFF (below `LEVEL_MIN = 3%`) |
| 3–99% | Normal dimming |
| 100% | Clamped to 99% internally |

This clamping happens in the library and is invisible to ESPHome. The Home Assistant slider full range (0–100%) works correctly.

---

## Sensor Platform (`platform: rbdimmer`)

The sensor platform exposes diagnostic and monitoring values. All three sub-sensors are optional — include only what you need.

### All Sensors

```yaml
sensor:
  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 60s
    ac_frequency:
      name: "AC Mains Frequency"
      phase: 0
    level:
      name: "Channel 1 Brightness"
      light_id: dimmer_ch1
    firing_delay:
      name: "Channel 1 Firing Delay"
      light_id: dimmer_ch1
```

### AC Frequency Only

```yaml
sensor:
  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 60s
    ac_frequency:
      name: "AC Mains Frequency"
      phase: 0
```

### Level and Firing Delay Only

```yaml
sensor:
  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 10s
    level:
      name: "Dimmer Level"
      light_id: dimmer_ch1
    firing_delay:
      name: "Firing Delay"
      light_id: dimmer_ch1
```

### Sensor Platform Configuration Variables

| Variable | Type | Required | Default | Description |
|---|---|---|---|---|
| `rbdimmer_id` | ID | No | auto (first hub) | ID of the parent hub. Required when multiple hubs are defined. |
| `update_interval` | time | No | `60s` | How often to poll and publish sensor values. |
| `ac_frequency` | sub-sensor | No | — | If present, creates a sensor reporting the detected mains frequency in Hz. |
| `level` | sub-sensor | No | — | If present, creates a sensor reporting the current brightness of the referenced channel in %. |
| `firing_delay` | sub-sensor | No | — | If present, creates a sensor reporting the TRIAC firing delay in microseconds (diagnostic use). |

### AC Frequency Sub-Sensor Variables

| Variable | Type | Required | Default | Description |
|---|---|---|---|---|
| `name` | string | Yes | — | Sensor name in Home Assistant. |
| `phase` | integer | No | `0` | Which phase's frequency to report. Range: 0–3. |
| Any standard sensor option | — | No | — | `icon`, `entity_category`, `filters`, etc. |

The AC frequency sensor has:
- Unit: `Hz`
- Device class: `frequency`
- State class: `measurement`
- Default icon: `mdi:sine-wave`

> 💡 The frequency sensor will report 0 Hz for approximately the first 0.5 seconds after power-on while the library auto-detects. Once locked to 50 or 60 Hz, the value is stable. A 60-second `update_interval` is sufficient.

### Level Sub-Sensor Variables

| Variable | Type | Required | Default | Description |
|---|---|---|---|---|
| `name` | string | Yes | — | Sensor name in Home Assistant. |
| `light_id` | ID | Yes | — | ID of the `rbdimmer` light entity this sensor reads from. |
| Any standard sensor option | — | No | — | `icon`, `filters`, etc. |

The level sensor has:
- Unit: `%`
- State class: `measurement`
- Default icon: `mdi:brightness-percent`

### Firing Delay Sub-Sensor Variables

| Variable | Type | Required | Default | Description |
|---|---|---|---|---|
| `name` | string | Yes | — | Sensor name in Home Assistant. |
| `light_id` | ID | Yes | — | ID of the `rbdimmer` light entity this sensor reads from. |
| Any standard sensor option | — | No | — | `icon`, `filters`, etc. |

The firing delay sensor has:
- Unit: `us` (microseconds)
- Entity category: `diagnostic`
- Default icon: `mdi:timer-outline`

> 💡 The firing delay sensor is useful during setup to verify that the brightness curve is producing expected phase angles. At 50% brightness with the `rms` curve on a 50 Hz supply, the delay should be approximately 3300–3500 µs (roughly 1/3 of the 10,000 µs half-cycle). It is not needed for normal operation.

---

## Select Platform (`platform: rbdimmer`)

The select platform creates a dropdown in Home Assistant that lets you change a channel's dimming curve at runtime without recompiling firmware.

### Curve Select

```yaml
select:
  - platform: rbdimmer
    curve:
      name: "Channel 1 Curve"
      light_id: dimmer_ch1
```

### Multiple Selects (One per Channel)

```yaml
select:
  - platform: rbdimmer
    curve:
      name: "Living Room Curve"
      light_id: dimmer_ch1

  - platform: rbdimmer
    curve:
      name: "Bedroom Curve"
      light_id: dimmer_ch2
```

### Select Configuration Variables

| Variable | Type | Required | Default | Description |
|---|---|---|---|---|
| `curve` | sub-entity | No | — | If present, creates a select entity for this channel's curve. |
| `curve.name` | string | Yes | — | Entity name in Home Assistant. |
| `curve.light_id` | ID | Yes | — | ID of the `rbdimmer` light entity to control. |
| Any standard select option | — | No | — | `entity_category`, `icon`, etc. |

The curve select has:
- Options: `LINEAR`, `RMS`, `LOG`
- Entity category: `config`
- Default icon: `mdi:chart-bell-curve`

The select reads the channel's current curve on boot and publishes it as the initial state. Changes take effect on the next zero-crossing event (within ~20 ms).

> ⚠️ The curve changed via the select entity is **not persisted** to flash. After a reboot, the curve returns to the value set in the `curve:` option of the corresponding light entity. To permanently change the default, update the `curve:` value in your YAML and reflash.

---

## Entity Relationships

The diagram below shows how the entities reference each other:

```
rbdimmer: (hub)
    |
    +-- light: platform: rbdimmer  (id: dimmer_ch1)
    |       |
    |       +-- sensor: level          (light_id: dimmer_ch1)
    |       +-- sensor: firing_delay   (light_id: dimmer_ch1)
    |       +-- select: curve          (light_id: dimmer_ch1)
    |
    +-- light: platform: rbdimmer  (id: dimmer_ch2)
    |       |
    |       +-- select: curve          (light_id: dimmer_ch2)
    |
    +-- sensor: ac_frequency           (rbdimmer_id: dimmer_hub, phase: 0)
```

- The hub (`rbdimmer:`) must exist before any other entity.
- Each light references a hub via `rbdimmer_id`.
- Sensor `level` and `firing_delay` sub-sensors each reference a specific light via `light_id`.
- Select `curve` references a specific light via `light_id`.
- Sensor `ac_frequency` references the hub (via `rbdimmer_id`) and a phase index — not a specific light.
