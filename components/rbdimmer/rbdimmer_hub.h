#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"

extern "C" {
#include <rbdimmerESP32.h>
}

#include <vector>

namespace esphome {
namespace rbdimmer {

static const char *const TAG_HUB = "rbdimmer.hub";

struct PhaseConfig {
  uint8_t phase;
  uint8_t pin;
  uint16_t frequency;
};

class RBDimmerHub : public Component {
 public:
  void add_phase(uint8_t phase, uint8_t pin, uint16_t frequency) {
    this->phases_.push_back({phase, pin, frequency});
  }

  void setup() override {
    rbdimmer_err_t err = rbdimmer_init();
    if (err != RBDIMMER_OK) {
      ESP_LOGE(TAG_HUB, "Failed to initialize rbdimmer library: %d", err);
      this->mark_failed();
      return;
    }

    for (const auto &phase : this->phases_) {
      err = rbdimmer_register_zero_cross(phase.pin, phase.phase, phase.frequency);
      if (err != RBDIMMER_OK) {
        ESP_LOGE(TAG_HUB, "Failed to register zero-cross on pin %d for phase %d: %d",
                 phase.pin, phase.phase, err);
        this->mark_failed();
        return;
      }
      ESP_LOGI(TAG_HUB, "Zero-cross registered: pin=%d, phase=%d, freq=%d",
               phase.pin, phase.phase, phase.frequency);
    }

    this->initialized_ = true;
    ESP_LOGI(TAG_HUB, "RBDimmer hub initialized with %d phase(s)", this->phases_.size());
  }

  void on_shutdown() override {
    if (this->initialized_) {
      rbdimmer_deinit();
      ESP_LOGI(TAG_HUB, "RBDimmer hub deinitialized");
    }
  }

  void dump_config() override {
    ESP_LOGCONFIG(TAG_HUB, "RBDimmer Hub:");
    for (const auto &phase : this->phases_) {
      ESP_LOGCONFIG(TAG_HUB, "  Phase %d: ZC pin=%d, freq=%d Hz",
                    phase.phase, phase.pin, phase.frequency);
    }
  }

  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  bool is_initialized() const { return this->initialized_; }

 protected:
  std::vector<PhaseConfig> phases_;
  bool initialized_{false};
};

}  // namespace rbdimmer
}  // namespace esphome
