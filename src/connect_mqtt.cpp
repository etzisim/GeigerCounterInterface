#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

WiFiClient espClient;
PubSubClient client(espClient);

void send_mqtt(String, String, boolean);
String Topic_upgrade = "esp_" + String(ESP.getChipId(), HEX) + "/upgrade";
String Topic_upgrademode = "esp_" + String(ESP.getChipId(), HEX) + "/waitforupgrade";
void callback(char *topic, byte *payload, unsigned int length)
{

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }

  Serial.println();
  Serial.println("-----------------------");

  if (String(topic) == Topic_upgrade && (char)payload[0] == '1')
  {
    Serial.println("wait for upgrade!");
    ArduinoOTA.begin();
    ESP.wdtDisable();
    send_mqtt(Topic_upgrade, "0", true);
    send_mqtt(Topic_upgrademode, "1", false);
    unsigned long start_upgrade_wait_millis = millis();
    while (millis() - start_upgrade_wait_millis <= 300000)
    {
      ArduinoOTA.handle();
    }
    send_mqtt(Topic_upgrademode, "0", false);
    Serial.println("time for upgrade ended");
  }
}

void loop_mqtt()
{
  client.loop();
}

void set_upgrade_sub()
{
  // Once connected, publish an announcement...
  //client.publish("outTopic", "hello world");
  Serial.println("upgrade topic = " + Topic_upgrade);
  client.subscribe(Topic_upgrade.c_str());
}

void init_mqtt()
{
  client.setServer("192.168.1.2", 1883);
  client.setCallback(callback);
}

void connect_mqtt()
{
  client.setBufferSize(5000);
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      //ESP.deepSleep(3.5 * 3600 * 1000000);
      delay(5000);
    }
  }
}

void send_mqtt(String topic, String message, boolean rem)
{
  connect_mqtt();
  Serial.print("Size of message: ");
  Serial.println(message.length());

  client.publish(topic.c_str(), message.c_str(), rem);
}

void disconnect_mqtt()
{
  client.disconnect();
}