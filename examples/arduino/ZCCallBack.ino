#include <Arduino.h>
#include "rbdimmerESP32.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define ZERO_CROSS_PIN 18
#define DIMMER_PIN 19
#define LED_PIN 2  // Onboard LED for zero-cross visualization
#define PHASE_NUM 0

rbdimmer_channel_t* dimmer = NULL;
uint32_t zeroCount = 0;

// FreeRTOS components
QueueHandle_t zeroCrossQueue;
TaskHandle_t zeroCrossTaskHandle;

// Simple message type for our queue
typedef struct {
  uint32_t timestamp;
} ZeroCrossEvent_t;

// Callback function for zero-cross events (runs in ISR context)
void zeroCrossCallback(void* arg) {
  // Create event
  ZeroCrossEvent_t event;
  event.timestamp = millis();
  
  // Send to queue from ISR
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(zeroCrossQueue, &event, &xHigherPriorityTaskWoken);
  
  // If a higher priority task was woken, request context switch
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

// Task to process zero-cross events
void zeroCrossProcessingTask(void* parameter) {
  ZeroCrossEvent_t event;
  
  // Task loop - will run forever
  for (;;) {
    // Wait for an item from the queue
    if (xQueueReceive(zeroCrossQueue, &event, portMAX_DELAY)) {
      // Process the event (now we're in task context, not ISR)
      
      // Toggle LED to visualize zero-crossing
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      
      // Count zero-crossing events
      zeroCount++;
      
      // Additional processing can be done here safely
      // This doesn't affect the zero-cross interrupt timing
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Setup LED for visual zero-cross indication
  pinMode(LED_PIN, OUTPUT);
  
  // Create the queue to send events from ISR to task
  zeroCrossQueue = xQueueCreate(10, sizeof(ZeroCrossEvent_t));
  if (zeroCrossQueue == NULL) {
    Serial.println("Error creating the queue");
    while (1); // Stop execution on error
  }
  
  // Create the task to process zero-cross events
  BaseType_t xReturned = xTaskCreate(
    zeroCrossProcessingTask,  // Task function
    "ZeroCrossTask",          // Task name
    2048,                     // Stack size (bytes)
    NULL,                     // No parameters needed
    5,                        // Medium priority
    &zeroCrossTaskHandle      // Task handle
  );
  
  if (xReturned != pdPASS) {
    Serial.println("Error creating the task");
    while (1); // Stop execution on error
  }
  
  // Initialize dimmer
  rbdimmer_init();
  rbdimmer_register_zero_cross(ZERO_CROSS_PIN, PHASE_NUM, 0);
  
  // Register zero-cross callback
  rbdimmer_set_callback(PHASE_NUM, zeroCrossCallback, NULL);
  
  // Create dimmer channel
  rbdimmer_config_t config = {
    .gpio_pin = DIMMER_PIN,
    .phase = PHASE_NUM,
    .initial_level = 60,
    .curve_type = rbDIMMER_CURVE_RMS
  };
  
  rbdimmer_create_channel(&config, &dimmer);
  Serial.println("Dimmer with zero-cross callback and task processing initialized");
}

void loop() {
  // Print zero-cross statistics every second
  static unsigned long lastPrint = 0;
  
  if (millis() - lastPrint >= 1000) {
    uint16_t frequency = rbdimmer_get_frequency(PHASE_NUM);
    
    Serial.printf("Zero-cross count: %u, Detected frequency: %u Hz\n", 
                  zeroCount, frequency);
    
    lastPrint = millis();
  }
  
  delay(10);
}
