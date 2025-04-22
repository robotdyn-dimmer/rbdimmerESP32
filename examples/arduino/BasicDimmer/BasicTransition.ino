#include <Arduino.h>
#include "rbdimmerESP32.h"

#define ZERO_CROSS_PIN 18
#define DIMMER_PIN 19
#define PHASE_NUM 0

rbdimmer_channel_t* dimmer = NULL;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize dimmer
  rbdimmer_init();
  rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);
  
  rbdimmer_config_t config = {
    .gpio_pin = DIMMER_PIN,
    .phase = PHASE_NUM,
    .initial_level = 0,  // Start with light off
    .curve_type = rbDIMMER_CURVE_RMS
  };
  
  rbdimmer_create_channel(&config, &dimmer);
  Serial.println("Dimmer initialized");
}

void loop() {
  // Fade up to 100% over 3 seconds
  Serial.println("Fading up...");
  rbdimmer_set_level_transition(dimmer, 100, 3000);
  delay(4000);  // Wait for transition + 1 second
  
  // Fade down to 10% over 3 seconds
  Serial.println("Fading down...");
  rbdimmer_set_level_transition(dimmer, 10, 3000);
  delay(4000);  // Wait for transition + 1 second
}
