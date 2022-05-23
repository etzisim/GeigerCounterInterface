#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <string.h>
#include "connect_mqtt.h"

#define GeigerCounterGPIO D2 // define GPIO to CAJOE board
#define debug false

String ProjectHostname = "GeigerCounterInterface";
WiFiManager wifiManager;
ADC_MODE(ADC_VCC);

const int BufferSize = 600;
unsigned int CountsBuffer[BufferSize];
unsigned int CountsBufferIndex = 0;
static unsigned long intervall = 60000 / BufferSize;
unsigned long changeBufferIndexMillis = 0;
int countsMinute = 0;
bool enough_counts = false;
static int timeout = 60;

unsigned int SendIntervallMillis = 0;
unsigned int SendIntervall = 5000;
bool print = false;
const float multiplly_factor = 0.00458333333333333; // my calculation for J321 Geigertube

IRAM_ATTR void CountUP()
{
  CountsBuffer[CountsBufferIndex]++;
  print = true;
}

void send_status_mqtt()
{
  String Status_topic = "home/" + ProjectHostname + "/esp_" + String(ESP.getChipId(), HEX) + "/status";
  String Status_message = "{\"esp_id\":\"" + String(ESP.getChipId(), HEX) + "\",\"IP\":\"" + WiFi.localIP().toString() + "\",\"uptime\":\"" + (millis() / 1000) + "\",\"send_intervall\":\"" + (SendIntervall / 1000) + "\",\"free_heap\":\"" + ESP.getFreeHeap() + "\",\"MHz\":\"" + ESP.getCpuFreqMHz() + "\",\"Vcc\":\"" + ESP.getVcc() + "\"}";

  send_mqtt(Status_topic, Status_message, 0);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("");

  const String prj_name = ProjectHostname + "_" + String(ESP.getChipId(), HEX);
  WiFi.hostname(prj_name.c_str());
  ArduinoOTA.setHostname(prj_name.c_str());

  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_FS
      type = "filesystem";
    } });

  wifiManager.setTimeout(timeout);
  wifiManager.setConfigPortalTimeout(timeout);
  wifiManager.setConnectTimeout(timeout);

  // Print Infos
  Serial.print(ProjectHostname);
  Serial.print(" on ESP ID: ");
  Serial.println(String(ESP.getChipId(), HEX));
  Serial.print("GeigerCounterGPIO is ");
  Serial.println(GeigerCounterGPIO);

  if (!wifiManager.autoConnect(ProjectHostname.c_str()))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    // reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }
  init_mqtt();
  attachInterrupt(digitalPinToInterrupt(GeigerCounterGPIO), CountUP, FALLING);
  changeBufferIndexMillis = millis();
  ArduinoOTA.begin();

  send_status_mqtt();
}

void loop()
{
  loop_mqtt();
  ArduinoOTA.handle();
  // put your main code here, to run repeatedly:
  if (millis() - changeBufferIndexMillis >= intervall)
  {
    changeBufferIndexMillis = changeBufferIndexMillis + intervall;

    CountsBufferIndex++;
    if (CountsBufferIndex >= BufferSize)
    {
      CountsBufferIndex = 0;
      enough_counts = true;
    }
    CountsBuffer[CountsBufferIndex] = 0;
  }

  if (millis() - SendIntervallMillis >= SendIntervall)
  {
    SendIntervallMillis = millis();

    if (print)
    {
      if (enough_counts)
      {
        countsMinute = 0;
        for (int i = 0; i < BufferSize; i++)
        {
          countsMinute = countsMinute + CountsBuffer[i];
        }
        float mSv_h = multiplly_factor * countsMinute;

        // send MQTT
        send_status_mqtt();
        String Data_topic = "home/" + ProjectHostname + "/esp_" + String(ESP.getChipId(), HEX) + "/cpm";
        String Data_message = "{\"CPM\":\"" + String(countsMinute) + "\",\"mSv_h\":\"" + String(mSv_h) + "\"}";

        send_mqtt(Data_topic, Data_message, 0);
        print = false;
      }

#if (debug)

      if (enough_counts)
      {
        Serial.print("Counts last Minute: ");
        Serial.println(countsMinute);
        print = false;
      }
      else
      {
        Serial.print("Buffer not full: ");
        Serial.print(CountsBufferIndex);
        Serial.print("/");
        Serial.println(BufferSize);
      }

#endif
    }
  }
}
