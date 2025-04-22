#include <Arduino.h>
#include "rbdimmerESP32.h"

#define ZERO_CROSS_PIN 18
#define DIMMER_PIN 19
#define PHASE_NUM 0

rbdimmer_channel_t* dimmer = NULL;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize the library
  rbdimmer_init();
  
  // Register zero-cross detector
  rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);
  
  // Create dimmer channel with RMS curve (best for incandescent bulbs)
  rbdimmer_config_t config = {
    .gpio_pin = DIMMER_PIN,
    .phase = PHASE_NUM,
    .initial_level = 50,  // Start at 50% brightness
    .curve_type = RBDIMMER_CURVE_RMS
  };
  
  rbdimmer_create_channel(&config, &dimmer);
  Serial.println("Dimmer initialized at 50% brightness");
}

void loop() {
  // Nothing needed in the loop - dimmer maintains its state
  delay(1000);
}
