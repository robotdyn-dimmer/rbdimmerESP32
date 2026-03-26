# ESPHome rbdimmer Component

An ESPHome external component that wraps the [rbdimmerESP32](https://github.com/robotdyn-dimmer/rbdimmerESP32) library (v2.0.0) and exposes AC TRIAC phase-cut dimming as native ESPHome entities — lights, sensors, and selects — with full Home Assistant integration.

## Features

- **Direct GPIO control** — no I2C or external controllers needed
- **Up to 8 channels** on up to 4 phases (including 3-phase systems)
- **Auto frequency detection** — 50/60 Hz
- **Three brightness curves** — Linear, RMS, Logarithmic
- **Smooth transitions** — via ESPHome or native library fading
- **Full Home Assistant integration** — light slider, sensors, curve selection
- **ESP-IDF framework** — ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6

## Quick Start

```yaml
esphome:
  name: my-dimmer

esp32:
  board: esp32dev
  framework:
    type: esp-idf

external_components:
  - source: github://robotdyn-dimmer/rbdimmerESP32@main
    components: [rbdimmer]
    refresh: 1d

rbdimmer:
  zero_cross_pin: GPIO23

light:
  - platform: rbdimmer
    name: "My Light"
    pin: GPIO25
    curve: rms
```

---

## What This Component Does

The `rbdimmer` component provides a bridge between the rbdimmerESP32 C library and the ESPHome ecosystem. Instead of writing raw C++ lambdas, you describe your dimmer hardware in YAML and get:

- **Light entities** — controllable from Home Assistant, with brightness sliders, transitions, and automations
- **Sensor entities** — AC mains frequency, current brightness level, and TRIAC firing delay
- **Select entities** — runtime switching between dimming curves (LINEAR / RMS / LOG)

The component initializes the underlying library, registers zero-cross detectors, and creates TRIAC channels automatically during ESP32 boot.

---

## Two Integration Methods

### Method 1: External Component (Recommended)

Use the pre-built ESPHome component. All configuration is done in YAML. No C++ required.

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [rbdimmer]

rbdimmer:
  zero_cross_pin: GPIO23

light:
  - platform: rbdimmer
    name: "Living Room"
    pin: GPIO25
```

### Method 2: Lambda (Advanced)

Use the rbdimmerESP32 library directly via ESPHome lambdas. Gives full control over the C API at the cost of more verbose YAML and no automatic entity management.

```yaml
esphome:
  on_boot:
    priority: 800
    then:
      - lambda: |-
          rbdimmer_init();
          rbdimmer_register_zero_cross(23, 0, 0);
          rbdimmer_config_t cfg = {.gpio_pin=25, .phase=0, .initial_level=0,
                                   .curve_type=RBDIMMER_CURVE_RMS};
          rbdimmer_create_channel(&cfg, &id(my_channel));
```

---

## Comparison Table

| Feature | External Component | Lambda |
|---|---|---|
| YAML-only configuration | Yes | No |
| Home Assistant light entity | Yes | Manual |
| Brightness transitions | Automatic | Manual |
| AC frequency sensor | Built-in | Manual |
| Curve select in HA | Built-in | Manual |
| Setup effort | Low | High |
| Flexibility | Standard | Full API access |
| Recommended for | Most users | Advanced/custom use cases |

---

## Quick Comparison: Supported Entities

| Platform | YAML key | What it creates |
|---|---|---|
| Hub | `rbdimmer:` | Initializes library, registers ZC pins |
| Light | `platform: rbdimmer` | Dimmable brightness-only light |
| Sensor | `platform: rbdimmer` | AC frequency, level %, firing delay |
| Select | `platform: rbdimmer` | Dropdown to change dimming curve |

---

## Entity Types

| Entity | Platform | Description |
|--------|----------|-------------|
| Hub | `rbdimmer:` | Library init + zero-cross registration |
| Light | `platform: rbdimmer` | Dimmer channel (brightness control) |
| Sensor | `platform: rbdimmer` | AC frequency, level, firing delay |
| Select | `platform: rbdimmer` | Dimming curve (LINEAR / RMS / LOG) |

---

## Documentation Index

| Document | Contents |
|---|---|
| [Install & Requirements](https://www.rbdimmer.com/docs/esphome-rbdimmer-install) | Requirements, hardware wiring, basic YAML |
| [Entities Reference](https://www.rbdimmer.com/docs/esphome-rbdimmer-entities) | All configuration options with tables |
| [YAML Examples](https://www.rbdimmer.com/docs/esphome-rbdimmer-examples) | Complete copy-paste YAML examples |
| [Lambda Reference](https://www.rbdimmer.com/docs/esphome-rbdimmer-lambda) | Direct C API access from lambdas |
| [Troubleshooting](https://www.rbdimmer.com/docs/esphome-rbdimmer-troubleshooting) | Common issues and diagnostic steps |

---

## Requirements Summary

- **ESPHome** 2024.x or later
- **Framework** `esp-idf` (required — Arduino framework is not supported by this component)
- **Hardware** ESP32, ESP32-S2, ESP32-S3, ESP32-C3, or ESP32-C6
- **Dimmer module** — RBDimmer or compatible optically-isolated TRIAC dimmer with zero-cross output

> ⚠️ This component requires `framework: esp-idf`. It will not compile with the Arduino framework. See [Install & Requirements](https://www.rbdimmer.com/docs/esphome-rbdimmer-install) for the correct `esp32:` block.

---

## Repository

- **Library source**: https://github.com/robotdyn-dimmer/rbdimmerESP32
- **Library version**: v2.0.0
- **Hardware catalog**: https://www.rbdimmer.com/dimmers-pricing
- **Support forum**: https://www.rbdimmer.com/forum
