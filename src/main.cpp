#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <Ticker.h>

#define LED_BUILTIN 2
#define SOUND_SPEED 0.034

#define TRIG_PIN 23
#define ECHO_PIN 22
#define WATER_PUMP_PIN 21
#define TANK_HEIGHT_CM 150
#define LEVEL_WARNING_CM 30
#define TURN_ON_TIME_SECS 1200
#define SUSPENDED_TIME_SECS 900

AsyncWebServer server(8081);
AsyncWebSocket ws("/ws");
JSONVar sensorValues;

Ticker TickerGetWaterLevel;
Ticker TickerTurnOnWaterPump;
Ticker TickerTurnOffWaterPump;

bool autoEnabled;
bool FullTank = false;
int waterLevelPercent = 0;
int distanceCm = 0;
int WaterPumpState;
long duration = 0;

String message = "";
// const char *ssid = "Xiaomi_7D23";
// const char *password = "1234567890";

/* const char *ssid = "INFINITUM39C4_2.4";
const char *password = "44VYPcV77T"; */

const char *ssid = "INFINITUM01B6_2.4";
const char *password = "Tp6Cy6Us1r";

const char *hostname = "ESP32Server";

const int WATER_LEVEL_SENSOR_PIN = 23;
const int INTERVAL = 1000;

void TurnOnWaterPump();
void TurnOffWaterPump();

String GetSensorValues()
{
  String jsonString = JSON.stringify(sensorValues);

  sensorValues["autoEnabled"] = autoEnabled;
  sensorValues["WaterDistance"] = String(distanceCm);
  sensorValues["LevelPercent"] = String(waterLevelPercent);
  sensorValues["WaterPumpState"] = WaterPumpState;
  sensorValues["FullTank"] = FullTank;

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
    digitalWrite(LED_BUILTIN,LOW);
    Serial.print('.');
    delay(900);
  }

  Serial.printf("\nConnected to %s \n", ssid);
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.getHostname());

  digitalWrite(LED_BUILTIN, HIGH);

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

    if (message.indexOf("turnOnPump") >= 0)
      TurnOnWaterPump();

    if (message.indexOf("turnOffPump") >= 0)
    {

      if(TickerTurnOffWaterPump.active())
        TickerTurnOffWaterPump.detach();
      
      if(TickerTurnOffWaterPump.active())
        TickerTurnOffWaterPump.detach();
      
      if(autoEnabled)
        WaterPumpState = -1;
      else
        WaterPumpState = 0;

      TurnOffWaterPump();
    }

    if(message.indexOf("autoEnabled") >= 0)
    {
      autoEnabled = true;

      if(WaterPumpState == 0)
        TurnOnWaterPump();

    }

    if(message.indexOf("autoDisabled") >= 0)
    {
      autoEnabled = false;
      
      if(WaterPumpState == 1)
        TurnOffWaterPump();
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

void GetWaterLevel()
{

  if(distanceCm <= 30){
    autoEnabled = false;
    FullTank = true;
    TurnOffWaterPump();
  } else
    FullTank = false;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);

  distanceCm = duration / 58.0;
  waterLevelPercent = distanceCm * 100 / TANK_HEIGHT_CM;
}

void setup()
{
  Serial.begin(115200);
  TickerGetWaterLevel.attach(1,GetWaterLevel);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  pinMode(LED_BUILTIN,OUTPUT);


  digitalWrite(WATER_PUMP_PIN, HIGH);
  digitalWrite(ECHO_PIN, LOW);
  digitalWrite(LED_BUILTIN, LOW);

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

void TurnOnWaterPump()
{
  digitalWrite(WATER_PUMP_PIN, LOW);

  WaterPumpState = 1;

  if(autoEnabled)
  {
    TickerTurnOffWaterPump.once(TURN_ON_TIME_SECS, TurnOffWaterPump);
  }
  
  notifyClients(GetSensorValues());
}

void TurnOffWaterPump()
{
  WaterPumpState = 0;
  digitalWrite(WATER_PUMP_PIN, HIGH);

  if(autoEnabled)
  {
    WaterPumpState = -1;
    TickerTurnOnWaterPump.once(SUSPENDED_TIME_SECS, TurnOnWaterPump);
  }

  notifyClients(GetSensorValues());
}