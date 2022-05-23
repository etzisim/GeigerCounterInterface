#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <string.h>
#include "connect_mqtt.h"

String ProjectHostname = "GeigerCounterInterface";
#define debug true

WiFiManager wifiManager;
#define GeigerCounterGPIO D2

const int BufferSize = 600;
unsigned int CountsBuffer[BufferSize];
unsigned int CountsBufferIndex = 0;
static unsigned long intervall = 60000 / BufferSize;
unsigned long changeBufferIndexMillis = 0;
int countsMinute = 0;
bool enough_counts = false;

unsigned int SendIntervallMillis = 0;
unsigned int SendIntervall = 5000;
bool print = false;
const float multiplly_factor = 0.00458333333333333; //my calculation for J321 Geigertube

IRAM_ATTR void CountUP()
{
  CountsBuffer[CountsBufferIndex]++;
  print = true;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("");
  WiFi.hostname(ProjectHostname + "_" + String(ESP.getChipId(), HEX));
  wifiManager.setTimeout(60);

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
}

void loop()
{
  loop_mqtt();
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
        String Status_topic = "home/" + ProjectHostname + "/esp_" + String(ESP.getChipId(), HEX) + "/status";
        String Data_topic = "home/" + ProjectHostname + "/esp_" + String(ESP.getChipId(), HEX) + "/cpm";
        String Status_message = "{\"esp_id\":\"" + String(ESP.getChipId(), HEX) + "\",\"IP\":\"" + WiFi.localIP().toString() + "\"}";
        String Data_message = "{\"CPM\":\"" + String(countsMinute) + "\",\"mSv_h\":\"" + String(mSv_h) + "\"}";

        send_mqtt(Status_topic, Status_message, 0);
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
