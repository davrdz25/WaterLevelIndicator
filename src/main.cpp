#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <AsyncElegantOTA.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>

#define LED_BUILTIN 2
#define SOUND_SPEED 0.034

bool ledState;

hw_timer_t *My_timer = NULL;

void IRAM_ATTR onTimer(){
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

AsyncWebServer server(8081);
AsyncWebSocket ws("/ws");
JSONVar sensorValues;

long duration;
float distanceCm;
bool relayState;

String message = "";
const char* ssid = "Xiaomi_7D23";
const char* password = "1234567890"; 
/* const char *ssid = "INFINITUM01B6_2.4";
const char *password = "Tp6Cy6Us1r"; */
const char *hostname = "ESP32Server";

const int waitTime = 15;
const int relayPin = 21;
const int echoPin = 22;
const int triggPin = 23;

void GetDistance()
{
  digitalWrite(triggPin, LOW);
  delayMicroseconds(10);
  digitalWrite(triggPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * SOUND_SPEED / 2;
}

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

  My_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(My_timer, &onTimer, true);
  timerAlarmWrite(My_timer, 1000000, true);
  timerAlarmEnable(My_timer);
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
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(triggPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(relayPin, LOW);

  initFS();
  initWiFi();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  AsyncElegantOTA.begin(&server, "admin", "Abraham456..");
  server.serveStatic("/", SPIFFS, "/");

  initWebSocket();
  server.begin(); 
}

void loop()
{
  Serial.println(distanceCm);

  GetDistance();
  if (distanceCm < 20)
  {
      digitalWrite(relayPin, HIGH);
      relayState = false;
  }
  else 
  {
    digitalWrite(relayPin, LOW);
    relayState = true;
  }

  if(relayState)
  {
    My_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(My_timer, &onTimer, true);
    timerAlarmWrite(My_timer, 1000000*60, true);
    timerAlarmEnable(My_timer);
  }

  delay(1000);
  ws.cleanupClients();
}