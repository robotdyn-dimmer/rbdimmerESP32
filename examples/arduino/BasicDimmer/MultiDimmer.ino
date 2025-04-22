#include <Arduino.h>
#include "rbdimmerESP32.h"

#define ZERO_CROSS_PIN 18
#define DIMMER_PIN_1 19
#define DIMMER_PIN_2 21
#define PHASE_NUM 0

rbdimmer_channel_t* dimmer1 = NULL;
rbdimmer_channel_t* dimmer2 = NULL;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize dimmer library
  rbdimmer_init();
  rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);
  
  // Create first dimmer channel (for incandescent bulb)
  rbdimmer_config_t config1 = {
    .gpio_pin = DIMMER_PIN_1,
    .phase = PHASE_NUM,
    .initial_level = 50,
    .curve_type = rbDIMMER_CURVE_RMS
  };
  rbdimmer_create_channel(&config1, &dimmer1);
  
  // Create second dimmer channel (for LED light)
  rbdimmer_config_t config2 = {
    .gpio_pin = DIMMER_PIN_2,
    .phase = PHASE_NUM,
    .initial_level = 50,
    .curve_type = rbDIMMER_CURVE_LOGARITHMIC  // Better for LEDs
  };
  rbdimmer_create_channel(&config2, &dimmer2);
  
  Serial.println("Two dimmer channels initialized");
}

void loop() {
  // Alternate level between channels
  Serial.println("Channel 1: 80%, Channel 2: 20%");
  rbdimmer_set_level(dimmer1, 80);
  rbdimmer_set_level(dimmer2, 20);
  delay(3000);
  
  Serial.println("Channel 1: 20%, Channel 2: 80%");
  rbdimmer_set_level(dimmer1, 20);
  rbdimmer_set_level(dimmer2, 80);
  delay(3000);
}
