#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>

#define LED_BUILTIN 2
#define SOUND_SPEED 0.034

#define TRIG_PIN 23
#define ECHO_PIN 22
#define WATER_PUMP_PIN 21
#define TANK_HEIGHT_CM 130
#define LEVEL_WARNING_CM 20

AsyncWebServer server(8081);
AsyncWebSocket ws("/ws");
JSONVar sensorValues;

long duration;
float distanceCm;
float waterLevelPercent = 0.00;

String message = "";
// const char *ssid = "Xiaomi_7D23";
// const char *password = "1234567890";
const char *ssid = "INFINITUM01B6_2.4";
const char *password = "Tp6Cy6Us1r"; 
const char *hostname = "ESP32Server";

const int WATER_LEVEL_SENSOR_PIN = 23;
const int INTERVAL = 1000; 

void TurnOnWaterPump()
{
  digitalWrite(WATER_PUMP_PIN, LOW);
  sensorValues["WaterPumpState"] = "ON";
}

void TurnOffWaterPump()
{
  digitalWrite(WATER_PUMP_PIN, HIGH);
  sensorValues["WaterPumpState"] = "OFF";
}

void GetWaterLevel() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    distanceCm = duration / 58.0;

    waterLevelPercent = distanceCm * 100 / TANK_HEIGHT_CM;

    Serial.printf("Water distance: %f \n",  distanceCm);
    Serial.printf("Level percent: %.f \n", waterLevelPercent);

    sensorValues["WaterDistance"] = String(distanceCm);
    sensorValues["LevelPercent"] =  String(waterLevelPercent);
}


String GetSensorValues()
{
  GetWaterLevel();
  String jsonString = JSON.stringify(sensorValues);
  Serial.println(jsonString);
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

  Serial.printf("\nConnected to %s \n", ssid);
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
    
    if(message.indexOf("turnOn") >= 0)
    {
      TurnOnWaterPump();
      notifyClients(GetSensorValues());
    }

    if(message.indexOf("turnOff") >= 0)
    {
      TurnOffWaterPump();
      notifyClients(GetSensorValues());
    }

    if (message.indexOf("WD") >= 0)
    {
      GetWaterLevel();
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
  pinMode(ECHO_PIN, INPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);

  digitalWrite(WATER_PUMP_PIN, HIGH);
  digitalWrite(ECHO_PIN, LOW);

  initFS();
  initWiFi();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.serveStatic("/", SPIFFS, "/");
 
  initWebSocket();
  server.begin();
}

void loop()
{
  ws.cleanupClients();
}