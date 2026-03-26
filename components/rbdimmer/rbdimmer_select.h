#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/select/select.h"
#include "rbdimmer_light.h"

extern "C" {
#include <rbdimmerESP32.h>
}

namespace esphome {
namespace rbdimmer {

static const char *const TAG_SELECT = "rbdimmer.select";

class RBDimmerCurveSelect : public select::Select, public Component {
 public:
  void set_light(RBDimmerLight *light) { this->light_ = light; }

  void setup() override {
    if (this->light_ == nullptr || this->light_->get_channel() == nullptr) {
      ESP_LOGW(TAG_SELECT, "Light not ready, defaulting to RMS");
      this->publish_state("RMS");
      return;
    }

    rbdimmer_curve_t curve = rbdimmer_get_curve(this->light_->get_channel());
    switch (curve) {
      case RBDIMMER_CURVE_LINEAR:
        this->publish_state("LINEAR");
        break;
      case RBDIMMER_CURVE_RMS:
        this->publish_state("RMS");
        break;
      case RBDIMMER_CURVE_LOGARITHMIC:
        this->publish_state("LOG");
        break;
      default:
        this->publish_state("RMS");
        break;
    }
  }

  void dump_config() override {
    ESP_LOGCONFIG(TAG_SELECT, "RBDimmer Curve Select: %s", this->get_name().c_str());
  }

  float get_setup_priority() const override { return setup_priority::DATA - 1.0f; }

 protected:
  void control(const std::string &value) override {
    if (this->light_ == nullptr)
      return;

    rbdimmer_curve_t curve;
    if (value == "LINEAR") {
      curve = RBDIMMER_CURVE_LINEAR;
    } else if (value == "RMS") {
      curve = RBDIMMER_CURVE_RMS;
    } else if (value == "LOG") {
      curve = RBDIMMER_CURVE_LOGARITHMIC;
    } else {
      ESP_LOGW(TAG_SELECT, "Unknown curve value: %s", value.c_str());
      return;
    }

    this->light_->set_curve_runtime(curve);
    this->publish_state(value);
    ESP_LOGD(TAG_SELECT, "Curve changed to: %s", value.c_str());
  }

  RBDimmerLight *light_{nullptr};
};

}  // namespace rbdimmer
}  // namespace esphome
