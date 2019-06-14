/*
  Typical pin layout used:
  ----------------------------------
              HC           Node
              SR501        MCU
  Signal      Pin          Pin
  ----------------------------------
  S           OUT          D7
  3.3V        VCC          3.3V
  GND         GND          GND
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config.h"

int pirPin = D7; // Input for HC-S501
int pirValue; // Place to store read PIR Value
int prevValue = 0;

// Wifi and mqtt client classes
WiFiClient espClient;
PubSubClient client(espClient);

char msg_buff [16];

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  // Connect to wifi network
  connect_wifi();
  // Initialize mqtt client
  client.setServer(MQTT_IP, 1883);

  // #### INSERT SENSOR INIT ###
  pinMode(pirPin, INPUT);
}

void loop() {
  // #### INSERT SENSOR READINGS ####
  pirValue = digitalRead(pirPin);
  
  // Connect to the mqtt broker if disconnected
  if (!client.connected()) {
    connect_mqtt();
  }
  client.loop();
  Serial.println("READING");
  // If the value changed, send it
  if (pirValue != prevValue) {
    Serial.println("SENDING");
    Serial.println(pirValue);
    dtostrf(pirValue, 5, 2, msg_buff);
  
    // Publish the measurements
    client.publish("cabackend/test", msg_buff);  
    prevValue = pirValue;
  }
  delay(100);
}

// ################## Wifi connection helper ###################

void connect_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to network: ");
  Serial.print(SSID);

  WiFi.begin(SSID, PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ################## Mqtt connection helper ###################

void connect_mqtt() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "SensorPublisher-";
    WiFi.mode(WIFI_STA);
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      // client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
