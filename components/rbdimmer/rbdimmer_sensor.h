#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h"
#include "rbdimmer_hub.h"
#include "rbdimmer_light.h"

extern "C" {
#include <rbdimmerESP32.h>
}

namespace esphome {
namespace rbdimmer {

static const char *const TAG_SENSOR = "rbdimmer.sensor";

class RBDimmerSensor : public PollingComponent {
 public:
  void set_hub(RBDimmerHub *hub) { this->hub_ = hub; }

  void set_ac_frequency_sensor(sensor::Sensor *sens) { this->ac_frequency_sensor_ = sens; }
  void set_ac_frequency_phase(uint8_t phase) { this->ac_frequency_phase_ = phase; }

  void set_level_sensor(sensor::Sensor *sens) { this->level_sensor_ = sens; }
  void set_level_light(RBDimmerLight *light) { this->level_light_ = light; }

  void set_firing_delay_sensor(sensor::Sensor *sens) { this->firing_delay_sensor_ = sens; }
  void set_firing_delay_light(RBDimmerLight *light) { this->firing_delay_light_ = light; }

  void update() override {
    if (this->ac_frequency_sensor_ != nullptr) {
      uint16_t freq = rbdimmer_get_frequency(this->ac_frequency_phase_);
      if (freq > 0) {
        this->ac_frequency_sensor_->publish_state(static_cast<float>(freq));
      }
    }

    if (this->level_sensor_ != nullptr && this->level_light_ != nullptr) {
      rbdimmer_channel_t *ch = this->level_light_->get_channel();
      if (ch != nullptr) {
        this->level_sensor_->publish_state(static_cast<float>(rbdimmer_get_level(ch)));
      }
    }

    if (this->firing_delay_sensor_ != nullptr && this->firing_delay_light_ != nullptr) {
      rbdimmer_channel_t *ch = this->firing_delay_light_->get_channel();
      if (ch != nullptr) {
        this->firing_delay_sensor_->publish_state(static_cast<float>(rbdimmer_get_delay(ch)));
      }
    }
  }

  void dump_config() override {
    ESP_LOGCONFIG(TAG_SENSOR, "RBDimmer Sensors:");
    if (this->ac_frequency_sensor_)
      ESP_LOGCONFIG(TAG_SENSOR, "  AC Frequency (phase %d): %s",
                    this->ac_frequency_phase_, this->ac_frequency_sensor_->get_name().c_str());
    if (this->level_sensor_)
      ESP_LOGCONFIG(TAG_SENSOR, "  Level: %s", this->level_sensor_->get_name().c_str());
    if (this->firing_delay_sensor_)
      ESP_LOGCONFIG(TAG_SENSOR, "  Firing Delay: %s", this->firing_delay_sensor_->get_name().c_str());
  }

 protected:
  RBDimmerHub *hub_{nullptr};

  sensor::Sensor *ac_frequency_sensor_{nullptr};
  uint8_t ac_frequency_phase_{0};

  sensor::Sensor *level_sensor_{nullptr};
  RBDimmerLight *level_light_{nullptr};

  sensor::Sensor *firing_delay_sensor_{nullptr};
  RBDimmerLight *firing_delay_light_{nullptr};
};

}  // namespace rbdimmer
}  // namespace esphome
