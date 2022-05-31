#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <string.h>
#include "avg_calc.h"
#include "connect_mqtt.h"
#include "make_json.h"

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

int calc_intervals(int n)
{
  return n * 60 * 1000 / SendIntervall;
}

avg_calc AVG_5m(calc_intervals(5));
avg_calc AVG_10m(calc_intervals(10));
avg_calc AVG_15m(calc_intervals(15));
avg_calc AVG_20m(calc_intervals(20));
avg_calc AVG_25m(calc_intervals(25));
avg_calc AVG_30m(calc_intervals(30));

IRAM_ATTR void CountUP() // interupt to count tics of Geiger CAJOE board
{
  CountsBuffer[CountsBufferIndex]++;
  print = true;
}

void send_status_mqtt()
{
  String Status_topic = "home/" + ProjectHostname + "/esp_" + String(ESP.getChipId(), HEX) + "/status";

  // generate json message
  genJson JsonStatus;
  JsonStatus.add("esp_id", String(ESP.getChipId(), HEX));
  JsonStatus.add("IP", WiFi.localIP().toString());
  JsonStatus.add("uptime", String(millis() / 1000));
  JsonStatus.add("send_intervall", String(SendIntervall / 1000));
  JsonStatus.add("free_heap", String(ESP.getFreeHeap()));
  JsonStatus.add("MHz", String(ESP.getCpuFreqMHz()));
  JsonStatus.add("Vcc", String(ESP.getVcc()));
  JsonStatus.add("BufferReady", String(enough_counts));
  JsonStatus.add("AVGBufferMinutes", String(AVG_30m.GetSize() * SendIntervall / 1000 / 60));

  send_mqtt(Status_topic, JsonStatus.getJson(), 0);
}

void wifiConnect()
{
  if (!(WiFi.status() == WL_CONNECTED))
  {
    if (!wifiManager.autoConnect(ProjectHostname.c_str()))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
    init_mqtt();
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("");
  Serial.println("");

  const String prj_name = ProjectHostname + "_" + String(ESP.getChipId(), HEX);
  WiFi.hostname(prj_name.c_str());
  WiFi.setHostname(prj_name.c_str());
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
#if (!debug)
  wifiManager.setDebugOutput(false);
  Serial.println("Debug disabled no more output");
#endif

  wifiConnect();
  attachInterrupt(digitalPinToInterrupt(GeigerCounterGPIO), CountUP, FALLING);
  changeBufferIndexMillis = millis();
  ArduinoOTA.begin();

  send_status_mqtt();
}

void loop()
{
  wifiConnect();
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

    if (enough_counts || debug)
    {
      countsMinute = 0;
      for (int i = 0; i < BufferSize; i++)
      {
        countsMinute = countsMinute + CountsBuffer[i];
      }
      float mSv_h = multiplly_factor * countsMinute;

      AVG_5m.addVal(mSv_h);
      AVG_10m.addVal(mSv_h);
      AVG_15m.addVal(mSv_h);
      AVG_20m.addVal(mSv_h);
      AVG_25m.addVal(mSv_h);
      AVG_30m.addVal(mSv_h);

      // send MQTT
      send_status_mqtt();

      String Data_topic = "home/" + ProjectHostname + "/esp_" + String(ESP.getChipId(), HEX) + "/cpm";

      // generate json message
      genJson JsonData;
      JsonData.add("CPM", String(countsMinute));
      JsonData.add("mSv_h", String(mSv_h));
      JsonData.add("AVG_mSv_h", String(AVG_30m.getAverage()));
      JsonData.add("AVG_5m_mSv_h", String(AVG_5m.getAverage()));
      JsonData.add("AVG_10m_mSv_h", String(AVG_10m.getAverage()));
      JsonData.add("AVG_15m_mSv_h", String(AVG_15m.getAverage()));
      JsonData.add("AVG_20m_mSv_h", String(AVG_20m.getAverage()));
      JsonData.add("AVG_25m_mSv_h", String(AVG_25m.getAverage()));
      JsonData.add("AVG_30m_mSv_h", String(AVG_30m.getAverage()));
      JsonData.add("sizeof_list", String(AVG_30m.GetSize()));

#if (debug)
      Serial.println(Data_topic);
      Serial.println(JsonData.getJson());
#endif

      send_mqtt(Data_topic, JsonData.getJson(), 0);

      print = false;
    }
    else
    {
      send_status_mqtt();
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
