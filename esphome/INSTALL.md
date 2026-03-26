# Install & Requirements

## Requirements

### ESPHome

ESPHome **2024.x or later** is required. The component uses `add_idf_component()`, which was introduced in ESPHome's ESP-IDF native build support and is stable from the 2024 release line onwards.

### Framework

> ⚠️ **ESP-IDF framework only.** The Arduino framework is not supported. The rbdimmerESP32 library uses ESP-IDF-specific APIs (`esp_timer`, `gpio_isr_service`, `ESP_INTR_FLAG_IRAM`) that are not available in the ESPHome Arduino layer.

Your `esp32:` block must specify `type: esp-idf`:

```yaml
esp32:
  board: esp32dev
  framework:
    type: esp-idf
```

The component has been tested with ESP-IDF 5.3.1 and 5.5.1. Any version in the 5.x range should work.

### Supported Chips

| Chip | Max Channels | Notes |
|---|---|---|
| ESP32 | 8 | All variants (WROOM-32, WROVER, etc.) |
| ESP32-S2 | 8 | Single-core |
| ESP32-S3 | 8 | Dual-core, USB native |
| ESP32-C3 | 4 | RISC-V, reduced channel limit |
| ESP32-C6 | 4 | Wi-Fi 6, reduced channel limit |

> 💡 The channel limit is a compile-time constant (`RBDIMMER_MAX_CHANNELS`) set in the library's Kconfig. It defaults to 8 for dual-core chips and 4 for single-core RISC-V chips.

---

## Hardware

### What You Need

1. **An ESP32 board** — any from the table above. A standard DevKit V1 works fine.
2. **An RBDimmer module** (or compatible) — a pre-built, optically isolated TRIAC dimmer module with a zero-cross detection output. The module handles the high-voltage AC side. Your ESP32 only ever sees 3.3V logic signals.
3. **A compatible AC load** — incandescent bulb, dimmable LED, resistive heater, or AC motor/fan (see load compatibility section below).
4. **Wiring** — standard 22-24 AWG jumper wires for the low-voltage connections.

> ⚠️ Never connect your ESP32 directly to AC mains. Always use a certified, optically isolated dimmer module. The optical barrier is the critical safety boundary between 3.3V logic and 110/220V AC.

### Load Compatibility

| Load Type | Compatible | Recommended Curve |
|---|---|---|
| Incandescent bulb | Yes | `rms` |
| Dimmable LED bulb | Yes (must be rated "dimmable") | `logarithmic` |
| Resistive heater | Yes | `linear` |
| AC motor / fan | Yes | `linear` |
| Non-dimmable LED | No | — |
| Electronic ballast | No | — |
| Switch-mode power supply | No | — |

---

## Wiring

### Block Diagram

```
ESP32 (3.3V)                  Dimmer Module (Isolated)         AC Load
+------------------+          +------------------------+       +----------+
|                  |          |                        |       |          |
|  ZC_PIN (input) <-----------+ Zero-Cross Output      |       |  Bulb /  |
|                  |          |                        |       |  Heater  |
|  DIM_PIN (output)-----------> TRIAC Gate Input       +-------+  / Motor |
|                  |          |                        |       |          |
|  GND            -----------+ GND (isolated side)    |       |          |
|  3.3V           -----------+ VCC (isolated side)    |       |          |
+------------------+          +------------------------+       +----------+
                                         |
                              AC LINE IN / NEUTRAL
                              (high voltage, isolated)
```

### Connection Table (Single Phase, Two Channels)

| ESP32 Pin | Dimmer Pin | Direction | Function |
|---|---|---|---|
| GPIO23 | Z-C | Input | Zero-cross detection signal |
| GPIO25 | DIM1 | Output | TRIAC gate for channel 1 |
| GPIO26 | DIM2 | Output | TRIAC gate for channel 2 |
| GND | GND | — | Common ground (isolated side) |
| 3.3V | VCC | — | Logic power for dimmer module |

### Connection Table (Single Phase, Four Channels)

| ESP32 Pin | Dimmer Pin | Direction | Function |
|---|---|---|---|
| GPIO23 | Z-C | Input | Zero-cross detection signal |
| GPIO25 | DIM1 | Output | TRIAC gate for channel 0 |
| GPIO26 | DIM2 | Output | TRIAC gate for channel 1 |
| GPIO27 | DIM3 | Output | TRIAC gate for channel 2 |
| GPIO33 | DIM4 | Output | TRIAC gate for channel 3 |
| GND | GND | — | Common ground (isolated side) |
| 3.3V | VCC | — | Logic power for dimmer module |

> 💡 GPIO27 and GPIO33 are output-capable on all ESP32 variants. GPIO33 is input-only in some datasheets but works as output in practice on ESP32-D0WD. Always verify with your specific module.

> 💡 For the zero-cross pin, prefer GPIO pins that are not used for boot strapping or serial output. On ESP32, GPIO23 and GPIO22 are reliable choices. Avoid GPIO 0, 1, 3, and 6-11.

> 💡 For the TRIAC output pins, avoid input-only pins (GPIO34-39 on ESP32). Any standard output-capable GPIO works.

### Multi-Phase Wiring Example

For a three-phase installation, each phase needs its own zero-cross signal:

| ESP32 Pin | Function | Phase |
|---|---|---|
| GPIO23 | ZC Phase A | 0 |
| GPIO22 | ZC Phase B | 1 |
| GPIO21 | ZC Phase C | 2 |
| GPIO25 | DIM Channel 1 | Phase A (0) |
| GPIO26 | DIM Channel 2 | Phase B (1) |
| GPIO27 | DIM Channel 3 | Phase C (2) |

---

## Basic YAML Configuration

The minimal working configuration for one dimmer channel on a single-phase circuit:

```yaml
esphome:
  name: my-dimmer
  friendly_name: "My Dimmer"

esp32:
  board: esp32dev
  framework:
    type: esp-idf

logger:
  level: INFO

api:
  encryption:
    key: !secret api_encryption_key

ota:
  - platform: esphome
    password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

# Load the rbdimmer external component
external_components:
  - source:
      type: local
      path: components    # path to the folder containing the rbdimmer/ directory
    components: [rbdimmer]

# Hub: initialize the library and register the zero-cross detector
rbdimmer:
  id: dimmer_hub
  zero_cross_pin: GPIO23    # ZC output from dimmer module
  frequency: 0              # 0 = auto-detect (recommended)

# Light entity: one TRIAC channel
light:
  - platform: rbdimmer
    name: "Living Room Light"
    rbdimmer_id: dimmer_hub
    pin: GPIO25             # TRIAC gate output
    phase: 0
    curve: rms
```

### Selecting the `path` for `external_components`

The `path` in `external_components` must point to the directory that **contains** the `rbdimmer/` component folder. If your ESPHome YAML is inside the `ESPHome/` directory and the component is at `ESPHome/components/rbdimmer/`, then use:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [rbdimmer]
```

If your YAML is at the project root and the component folder is at `ESPHome/components/rbdimmer/`, adjust accordingly:

```yaml
external_components:
  - source:
      type: local
      path: ESPHome/components
    components: [rbdimmer]
```

### Framework Block

The minimum required `esp32:` block for this component:

```yaml
esp32:
  board: esp32dev    # or: esp32s3box, esp32-c3-devkitm-1, etc.
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD: "y"
```

> ⚠️ `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD: "y"` is **required**. The library creates TRIAC timers with `ESP_TIMER_ISR` dispatch mode for precise sub-millisecond firing. Without this option the build fails with:
> ```
> error: 'ESP_TIMER_ISR' undeclared (first use in this function)
> ```

> ⚠️ Do not use `type: arduino`. The component will fail to compile.

---

## How It Works

When your ESP32 boots, the `rbdimmer` hub runs at hardware priority — before any light or sensor components. It:

1. Calls `rbdimmer_init()` to set up the library's internal state.
2. Calls `rbdimmer_register_zero_cross(pin, phase, frequency)` for each configured phase. This configures the GPIO as an interrupt input (rising edge) and begins auto-detecting the mains frequency.
3. Each light platform then calls `rbdimmer_create_channel()` to allocate a TRIAC output channel on its configured pin.

After boot, the library runs autonomously via ISR and ESP timers. Every zero-crossing event on the ZC pin triggers the ISR, which schedules TRIAC firing delays for all channels on that phase. Your ESP32's main loop is not involved in the timing — it only calls `rbdimmer_set_level()` when a new brightness value is requested.

### Frequency Auto-Detection

When `frequency: 0` is set (the default and recommended value), the library measures the mains frequency automatically by counting zero-cross pulses over 50 cycles (~0.5 seconds at 50 Hz). During this window:

- `rbdimmer_get_frequency()` returns 0 — this is normal and expected.
- Any channels created during this window will operate with a 50 Hz assumption until measurement completes.
- Once the frequency locks (50 or 60 Hz), all channels are automatically updated.

The AC frequency sensor will show 0 Hz for the first ~0.5 seconds after power-on. This is not an error.
