#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <AsyncElegantOTA.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <Ticker.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LED_BUILTIN 2
#define SOUND_SPEED 0.034

#define TRIG_PIN 23
#define ECHO_PIN 22
#define PUMP_PIN 21

#define MAX_WATER_LEVEL 20 // in cm
#define PUMP_ON_TIME 900000 // 15 minutes in microseconds
#define PUMP_OFF_TIME 900000 // 15 minutes in microseconds
#define WATER_CHECK_INTERVAL 1000 // in milliseconds

TaskHandle_t waterLevelTaskHandle;
TaskHandle_t pumpOnTaskHandle;
TaskHandle_t pumpOffTaskHandle;

Ticker waterLevelTicker;

void getWaterLevel();
void pumpOffTask(void* parameter);
void pumpOnTask(void* parameter);

AsyncWebServer server(8081);
AsyncWebSocket ws("/ws");
JSONVar sensorValues;

long duration;
float distanceCm;
bool relayState;

String message = "";
const char *ssid = "Xiaomi_7D23";
const char *password = "1234567890";
/* const char *ssid = "INFINITUM01B6_2.4";
const char *password = "Tp6Cy6Us1r"; */
const char *hostname = "ESP32Server";

String GetSensorValues()
{
  sensorValues["WaterDistance"] = String(distanceCm);
  sensorValues["RelayState"] = relayState;

  String jsonString = JSON.stringify(sensorValues);
  Serial.println(sensorValues);

  return jsonString;
}

void initFS()
{
  if (!SPIFFS.begin())
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else
  {
    Serial.println("SPIFFS mounted successfully");
  }
}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.hostname(hostname);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }

  Serial.printf("\nConnected to %s \n", &ssid);
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.getHostname());
}

void notifyClients(String sliderValues)
{
  ws.textAll(sliderValues);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    message = (char *)data;

    if (message.indexOf("WD") >= 0)
    {
      notifyClients(GetSensorValues());
    }

    if (message.indexOf("RS") >= 0)
    {
      notifyClients(GetSensorValues());
    }

    if (strcmp((char *)data, "getValues") == 0)
    {
      notifyClients(GetSensorValues());
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup()
{
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(ECHO_PIN, LOW);

  /* initFS();
  initWiFi();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  AsyncElegantOTA.begin(&server, "admin", "Abraham456..");
  server.serveStatic("/", SPIFFS, "/");
 */
/* 
  initWebSocket();
  server.begin(); */
  waterLevelTicker.attach_ms(WATER_CHECK_INTERVAL, getWaterLevel);
}

void loop()
{
  Serial.printf("Distance: %f \n", distanceCm);
  ws.cleanupClients();
}

void pumpOnTask(void* parameter) {
  digitalWrite(PUMP_PIN, HIGH);
  xTaskCreate(pumpOffTask, "PumpOffTask", 2048, NULL, 1, &pumpOffTaskHandle);
  vTaskDelete(pumpOnTaskHandle);
}

void pumpOffTask(void* parameter) {
  digitalWrite(PUMP_PIN, LOW);
  xTaskCreate(pumpOnTask, "PumpOnTask", 2048, NULL, 1, &pumpOnTaskHandle);
  vTaskDelete(pumpOffTaskHandle);
}

void getWaterLevel() {
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