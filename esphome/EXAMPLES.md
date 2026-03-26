# YAML Examples

All examples in this document are complete, self-contained YAML configurations. Copy the one closest to your setup, replace the GPIO numbers, secrets, and entity names, then flash.

---

## Example 1: Minimal

The smallest functional configuration. One dimmer channel, no sensors, no logging beyond errors.

```yaml
esphome:
  name: dimmer-minimal
  friendly_name: "Dimmer"

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD: "y"

api:
  encryption:
    key: !secret api_encryption_key

ota:
  - platform: esphome
    password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

external_components:
  - source:
      type: local
      path: components
    components: [rbdimmer]

rbdimmer:
  zero_cross_pin: GPIO23

light:
  - platform: rbdimmer
    name: "Dimmer"
    pin: GPIO25
```

**What you get:** A single dimmable light in Home Assistant. The curve defaults to `rms`, transitions default to 1 second.

---

## Example 2: Standard — One Channel with Sensors and Curve Select

The recommended starting point for most installations. Adds AC frequency monitoring, a brightness level readout, and a runtime curve selector.

```yaml
esphome:
  name: dimmer-standard
  friendly_name: "Living Room Dimmer"

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD: "y"

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
  ap:
    ssid: "Dimmer Fallback"
    password: !secret ap_password

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
    name: "Living Room Light"
    id: dimmer_ch1
    rbdimmer_id: dimmer_hub
    pin: GPIO25
    phase: 0
    curve: rms
    default_transition_length: 1s

sensor:
  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 60s
    ac_frequency:
      name: "AC Mains Frequency"
      phase: 0
    level:
      name: "Dimmer Level"
      light_id: dimmer_ch1
    firing_delay:
      name: "Firing Delay (diag)"
      light_id: dimmer_ch1

select:
  - platform: rbdimmer
    curve:
      name: "Dimmer Curve"
      light_id: dimmer_ch1
```

**What you get:** One dimmable light, plus three sensors (frequency in Hz, current brightness in %, firing delay in µs), plus a curve dropdown visible in the Home Assistant device page.

---

## Example 3: Multi-Channel — Two Channels on the Same Phase

Two independent TRIAC channels sharing one zero-cross detector. Common for dual-channel dimmer modules (e.g., 8A 2-channel).

```yaml
esphome:
  name: dimmer-dual
  friendly_name: "Dual Dimmer"

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD: "y"

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

external_components:
  - source:
      type: local
      path: components
    components: [rbdimmer]

# Single phase, two channels
rbdimmer:
  id: dimmer_hub
  zero_cross_pin: GPIO23
  frequency: 0

light:
  - platform: rbdimmer
    name: "Channel 1 - Living Room"
    id: dimmer_ch1
    rbdimmer_id: dimmer_hub
    pin: GPIO25
    phase: 0
    curve: rms
    default_transition_length: 1s

  - platform: rbdimmer
    name: "Channel 2 - Hallway"
    id: dimmer_ch2
    rbdimmer_id: dimmer_hub
    pin: GPIO26
    phase: 0           # Same phase as channel 1
    curve: rms
    default_transition_length: 1s

sensor:
  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 60s
    ac_frequency:
      name: "AC Frequency"
      phase: 0

select:
  - platform: rbdimmer
    curve:
      name: "Ch1 Curve"
      light_id: dimmer_ch1

  - platform: rbdimmer
    curve:
      name: "Ch2 Curve"
      light_id: dimmer_ch2
```

**What you get:** Two independent dimmable lights on the same zero-cross signal. Both can be controlled independently in Home Assistant. The AC frequency sensor reports once per minute.

> 💡 Both channels use `phase: 0` because they both respond to the same zero-crossing signal. The library's two-pass ISR ensures both channels are armed at exactly the same time on each zero-crossing event, so there is no synchronization drift between them.

---

## Example 4: Three-Phase — Three Independent Phases

Three phases with one channel each. Each phase has its own zero-cross detector and independent timing.

```yaml
esphome:
  name: dimmer-3phase
  friendly_name: "Three-Phase Dimmer"

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD: "y"

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

external_components:
  - source:
      type: local
      path: components
    components: [rbdimmer]

# Three phases, each with its own ZC pin
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

# One channel per phase
light:
  - platform: rbdimmer
    name: "Phase A - Workshop"
    id: dimmer_ch_a
    rbdimmer_id: dimmer_hub
    pin: GPIO25
    phase: 0
    curve: linear
    default_transition_length: 500ms

  - platform: rbdimmer
    name: "Phase B - Storage"
    id: dimmer_ch_b
    rbdimmer_id: dimmer_hub
    pin: GPIO26
    phase: 1
    curve: linear
    default_transition_length: 500ms

  - platform: rbdimmer
    name: "Phase C - Office"
    id: dimmer_ch_c
    rbdimmer_id: dimmer_hub
    pin: GPIO27
    phase: 2
    curve: rms
    default_transition_length: 1s

# Monitor all three phase frequencies
sensor:
  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 60s
    ac_frequency:
      name: "Phase A Frequency"
      phase: 0

  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 60s
    ac_frequency:
      name: "Phase B Frequency"
      phase: 1

  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 60s
    ac_frequency:
      name: "Phase C Frequency"
      phase: 2
```

**What you get:** Three independent lights, each phase-locked to its own AC phase. Three separate frequency sensors, one per phase. Each can be controlled and automated independently.

> 💡 Each sensor block here only has `ac_frequency` defined, so each creates exactly one frequency sensor. You can combine them into a single sensor block if preferred, but note that a single `sensor: platform: rbdimmer` block currently supports only one `ac_frequency` sub-sensor (one phase per block). Use multiple blocks for multiple phases.

---

## Example 5: Home Assistant Automations

This example demonstrates common Home Assistant automation patterns using the dimmer as the target entity. The ESPHome YAML is the same as Example 2; only the Home Assistant automations are shown.

### Motion-Triggered Dimming

Turn on the light at 60% when motion is detected, turn off after 5 minutes of no motion.

```yaml
# Home Assistant automation (configuration.yaml or automation editor)
automation:
  - alias: "Hallway Motion Dimming"
    trigger:
      - platform: state
        entity_id: binary_sensor.hallway_motion
        to: "on"
    action:
      - service: light.turn_on
        target:
          entity_id: light.hallway_dimmer
        data:
          brightness_pct: 60
          transition: 2

  - alias: "Hallway Motion Off"
    trigger:
      - platform: state
        entity_id: binary_sensor.hallway_motion
        to: "off"
        for:
          minutes: 5
    action:
      - service: light.turn_off
        target:
          entity_id: light.hallway_dimmer
        data:
          transition: 3
```

### Night Mode — Automatic Dimming at Sunset

Dim the living room light to 30% one hour after sunset.

```yaml
automation:
  - alias: "Evening Dim"
    trigger:
      - platform: sun
        event: sunset
        offset: "+01:00:00"
    action:
      - service: light.turn_on
        target:
          entity_id: light.living_room_light
        data:
          brightness_pct: 30
          transition: 10

  - alias: "Morning Full Brightness"
    trigger:
      - platform: sun
        event: sunrise
        offset: "+00:30:00"
    action:
      - service: light.turn_on
        target:
          entity_id: light.living_room_light
        data:
          brightness_pct: 100
          transition: 15
```

### Curve Auto-Select Based on Time of Day

Switch to the logarithmic curve in the evening for softer, more eye-friendly dimming.

```yaml
automation:
  - alias: "Evening LED Curve"
    trigger:
      - platform: time
        at: "19:00:00"
    action:
      - service: select.select_option
        target:
          entity_id: select.dimmer_curve
        data:
          option: LOG

  - alias: "Morning RMS Curve"
    trigger:
      - platform: time
        at: "07:00:00"
    action:
      - service: select.select_option
        target:
          entity_id: select.dimmer_curve
        data:
          option: RMS
```

### Brightness Controlled by a Numeric Input

Expose a `input_number` helper in Home Assistant and use it to set dimmer brightness from a slider in the dashboard.

```yaml
# In Home Assistant helpers
input_number:
  living_room_brightness:
    name: Living Room Brightness
    min: 0
    max: 100
    step: 5
    unit_of_measurement: "%"

# Automation
automation:
  - alias: "Sync Dimmer with Slider"
    trigger:
      - platform: state
        entity_id: input_number.living_room_brightness
    action:
      - service: light.turn_on
        target:
          entity_id: light.living_room_light
        data:
          brightness_pct: "{{ states('input_number.living_room_brightness') | int }}"
          transition: 1
```

---

## Example 6: Production Configuration

A complete production-ready YAML with Wi-Fi fallback AP, OTA, web server, comprehensive logging, and full sensor/select coverage. Suitable for a permanent installation.

```yaml
esphome:
  name: living-room-dimmer
  friendly_name: "Living Room Dimmer"
  comment: "rbdimmerESP32 v2.0.0 — 2-channel single phase"

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD: "y"

# Logging — use WARN in production; switch to DEBUG when diagnosing
logger:
  level: WARN
  # level: DEBUG  # uncomment to see detailed dimmer timing logs

# Home Assistant API
api:
  encryption:
    key: !secret api_encryption_key

# Over-the-air updates
ota:
  - platform: esphome
    password: !secret ota_password

# Wi-Fi with fallback hotspot
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  fast_connect: true
  ap:
    ssid: "LR-Dimmer Fallback"
    password: !secret ap_password

# Captive portal for fallback AP
captive_portal:

# Local web interface (optional — disable to save memory)
web_server:
  port: 80

# External component
external_components:
  - source:
      type: local
      path: components
    components: [rbdimmer]

# Hub
rbdimmer:
  id: dimmer_hub
  zero_cross_pin: GPIO23
  frequency: 0

# Channels
light:
  - platform: rbdimmer
    name: "Main Light"
    id: dimmer_ch1
    rbdimmer_id: dimmer_hub
    pin: GPIO25
    phase: 0
    curve: rms
    gamma_correct: 1.0
    default_transition_length: 1s

  - platform: rbdimmer
    name: "Accent Light"
    id: dimmer_ch2
    rbdimmer_id: dimmer_hub
    pin: GPIO26
    phase: 0
    curve: logarithmic
    gamma_correct: 1.0
    default_transition_length: 2s

# Sensors
sensor:
  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 60s
    ac_frequency:
      name: "AC Frequency"
      phase: 0
    level:
      name: "Main Light Level"
      light_id: dimmer_ch1

  - platform: wifi_signal
    name: "Wi-Fi Signal"
    update_interval: 60s

# Curve selects
select:
  - platform: rbdimmer
    curve:
      name: "Main Light Curve"
      light_id: dimmer_ch1

  - platform: rbdimmer
    curve:
      name: "Accent Light Curve"
      light_id: dimmer_ch2

# System uptime
sensor:
  - platform: uptime
    name: "Uptime"
    update_interval: 60s

# Restart button (visible in Home Assistant device page)
button:
  - platform: restart
    name: "Restart"

# System status binary sensor
binary_sensor:
  - platform: status
    name: "API Connected"
```

**What you get:** A fully featured two-channel dimmer with Home Assistant integration, OTA updates, a local web interface, Wi-Fi fallback, AC frequency monitoring, brightness level sensor for channel 1, curve selects for both channels, uptime tracking, a restart button, and API connection status.

> 💡 In production, keep `logger: level: WARN`. The `DEBUG` level generates a log line on every zero-crossing event (100–120 times per second), which adds measurable CPU load and can slow down the web interface.

---

## Example 7: Four-Channel Single Phase

Four independent TRIAC channels on one zero-cross detector. Tested hardware configuration:
- ZC detector → GPIO23
- Channel 0 → GPIO25
- Channel 1 → GPIO26
- Channel 2 → GPIO27
- Channel 3 → GPIO33

```yaml
esphome:
  name: dimmer-4ch
  friendly_name: "4-Channel Dimmer"

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD: "y"

logger:
  level: DEBUG

api:
  encryption:
    key: !secret api_encryption_key

ota:
  - platform: esphome
    password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "Dimmer Fallback"
    password: !secret ap_password

web_server:
  port: 80

external_components:
  - source:
      type: local
      path: components
    components: [rbdimmer]

# Single zero-cross for all 4 channels
rbdimmer:
  id: dimmer_hub
  zero_cross_pin: GPIO23
  frequency: 0

# Four channels — all on phase 0
light:
  - platform: rbdimmer
    name: "Dimmer Channel 0"
    id: dimmer_ch0
    rbdimmer_id: dimmer_hub
    pin: GPIO25
    phase: 0
    curve: rms
    gamma_correct: 1.0
    default_transition_length: 1s

  - platform: rbdimmer
    name: "Dimmer Channel 1"
    id: dimmer_ch1
    rbdimmer_id: dimmer_hub
    pin: GPIO26
    phase: 0
    curve: rms
    gamma_correct: 1.0
    default_transition_length: 1s

  - platform: rbdimmer
    name: "Dimmer Channel 2"
    id: dimmer_ch2
    rbdimmer_id: dimmer_hub
    pin: GPIO27
    phase: 0
    curve: rms
    gamma_correct: 1.0
    default_transition_length: 1s

  - platform: rbdimmer
    name: "Dimmer Channel 3"
    id: dimmer_ch3
    rbdimmer_id: dimmer_hub
    pin: GPIO33
    phase: 0
    curve: rms
    gamma_correct: 1.0
    default_transition_length: 1s

# AC frequency + per-channel level and firing delay
sensor:
  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 60s
    ac_frequency:
      name: "AC Frequency"
      phase: 0
    level:
      name: "Channel 0 Level"
      light_id: dimmer_ch0
    firing_delay:
      name: "Channel 0 Firing Delay"
      light_id: dimmer_ch0

  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 60s
    level:
      name: "Channel 1 Level"
      light_id: dimmer_ch1
    firing_delay:
      name: "Channel 1 Firing Delay"
      light_id: dimmer_ch1

  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 60s
    level:
      name: "Channel 2 Level"
      light_id: dimmer_ch2
    firing_delay:
      name: "Channel 2 Firing Delay"
      light_id: dimmer_ch2

  - platform: rbdimmer
    rbdimmer_id: dimmer_hub
    update_interval: 60s
    level:
      name: "Channel 3 Level"
      light_id: dimmer_ch3
    firing_delay:
      name: "Channel 3 Firing Delay"
      light_id: dimmer_ch3

  - platform: wifi_signal
    name: "WiFi Signal"
    update_interval: 60s

# Curve select per channel
select:
  - platform: rbdimmer
    curve:
      name: "Channel 0 Curve"
      light_id: dimmer_ch0

  - platform: rbdimmer
    curve:
      name: "Channel 1 Curve"
      light_id: dimmer_ch1

  - platform: rbdimmer
    curve:
      name: "Channel 2 Curve"
      light_id: dimmer_ch2

  - platform: rbdimmer
    curve:
      name: "Channel 3 Curve"
      light_id: dimmer_ch3
```

**What you get:** Four independent dimmable lights in Home Assistant. AC frequency sensor, level and firing delay sensors per channel, curve select per channel. Tested and verified on ESP32-D0WD rev1.0 with ESP-IDF 5.5.2.

> 💡 All four channels share the same zero-cross signal (`phase: 0`). The library's ISR arms all four channels simultaneously at each zero-crossing, so there is no phase drift between channels.

> 💡 The web server at `http://<device-ip>/` shows brightness sliders in **0–255** range — this is ESPHome's standard light protocol. The library internally receives 0–100 (converted with `roundf(brightness * 100)`). Full-scale slider position 255 maps exactly to library level 100.
