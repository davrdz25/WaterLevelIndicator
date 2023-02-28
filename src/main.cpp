#include <Arduino.h>
#include <Ticker.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// define pins
#define TRIG_PIN 4
#define ECHO_PIN 5
#define PUMP_PIN 13

// define constants
#define MAX_WATER_LEVEL 20 // in cm
#define PUMP_ON_TIME 900000 // 15 minutes in microseconds
#define PUMP_OFF_TIME 900000 // 15 minutes in microseconds
#define WATER_CHECK_INTERVAL 1000 // in milliseconds

// define task handles
TaskHandle_t waterLevelTaskHandle;
TaskHandle_t pumpOnTaskHandle;
TaskHandle_t pumpOffTaskHandle;

// define ticker for water level check
Ticker waterLevelTicker;

// function to get water level
void getWaterLevel(void* parameter) {
  while (true) {
    // measure distance using HC-SR04
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH);
    // convert duration to distance in cm
    float distance = duration / 58.0;
    // check if water level is below threshold
    if (distance < MAX_WATER_LEVEL) {
      // stop water level checking
      waterLevelTicker.detach();
      // create task to turn on the pump
      xTaskCreate(pumpOnTask, "PumpOnTask", 2048, NULL, 1, &pumpOnTaskHandle);
      // delete this task
      vTaskDelete(waterLevelTaskHandle);
    }
    // wait for next water level check
    vTaskDelay(pdMS_TO_TICKS(WATER_CHECK_INTERVAL));
  }
}

// function to turn on the water pump
void pumpOnTask(void* parameter) {
  // turn on the pump
  digitalWrite(PUMP_PIN, HIGH);
  // create task to turn off the pump after the pump-on time
  xTaskCreate(pumpOffTask, "PumpOffTask", 2048, NULL, 1, &pumpOffTaskHandle);
  // delete this task
  vTaskDelete(pumpOnTaskHandle);
}

// function to turn off the water pump
void pumpOffTask(void* parameter) {
  // turn off the pump
  digitalWrite(PUMP_PIN, LOW);
  // create task to turn on the pump after the pump-off time
  xTaskCreate(pumpOnTask, "PumpOnTask", 2048, NULL, 1, &pumpOnTaskHandle);
  // delete this task
  vTaskDelete(pumpOffTaskHandle);
}

void setup() {
  // initialize serial communication
  Serial.begin(115200);
  // set pin modes
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  // start water level checking using ticker
  waterLevelTicker.attach_ms(1000, getWaterLevel);
}

void loop() {
  // do nothing
}