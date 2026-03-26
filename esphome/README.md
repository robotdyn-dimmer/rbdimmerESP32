# RBDimmer ESPHome Component

ESPHome external component for **rbdimmerESP32** — professional AC dimmer control library for ESP32 with TRIAC phase-cut dimming and zero-cross detection.

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

## Entity Types

| Entity | Platform | Description |
|--------|----------|-------------|
| Hub | `rbdimmer:` | Library init + zero-cross registration |
| Light | `platform: rbdimmer` | Dimmer channel (brightness control) |
| Sensor | `platform: rbdimmer` | AC frequency, level, firing delay |
| Select | `platform: rbdimmer` | Dimming curve (LINEAR / RMS / LOG) |

## Documentation

| Language | Component Docs | Lambda Reference |
|----------|---------------|-----------------|
| English | [docs/en/](docs/en/README.md) | [docs/en/04_lambda_reference.md](docs/en/04_lambda_reference.md) |
| Russian | [docs/ru/](docs/ru/README.md) | [docs/ru/04_lambda_reference.md](docs/ru/04_lambda_reference.md) |

## Requirements

- ESPHome 2024.x or later
- ESP-IDF framework (not Arduino)
- rbdimmerESP32 library v2.0.0+
- RBDimmer hardware module

## License

MIT License. See [LICENSE](../LICENSE) for details.

## Links

- [RBDimmer Website](https://rbdimmer.com)
- [rbdimmerESP32 Library](https://github.com/robotdyn-dimmer/rbdimmerESP32)
- [RBDimmer Community Forum](https://www.rbdimmer.com/forum)
