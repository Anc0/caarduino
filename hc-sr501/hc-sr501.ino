/*
  Typical pin layout used:
  ----------------------------------
              HC           Node
              SR501        MCU
  Signal      Pin          Pin
  ----------------------------------
  S           OUT          D2
  3.3V        VCC          3.3V
  GND         GND          GND
*/

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config.h"

// Wifi and mqtt client classes
WiFiClient espClient;
PubSubClient client(espClient);

char msg_buff [16];
int prev_val, curr_val;

void setup() {
  Serial.println("Starting...");
  // Initialize serial communication
  Serial.begin(115200);
  /*// Connect to wifi network
  connect_wifi();
  // Initialize mqtt client
  client.setServer(MQTT_IP, 1883);*/
  Serial.println("Starting...");
  // #### INSERT SENSOR INIT ###
  // Initialize indicator led
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  // Initialize reading pin
  pinMode(D1, INPUT);
  // Initialize hall value
  prev_val = LOW;
}

void loop() {
  // #### INSERT SENSOR READINGS ####
  curr_val = digitalRead(D1);
  delay(50);
  curr_val = digitalRead(D1);
  /*if(curr_val != prev_val) {
    send_value(curr_val);
    prev_val = curr_val;
    }*/

  if (curr_val == LOW) {
    digitalWrite(BUILTIN_LED, HIGH);
    Serial.println("LOW");
  } else {
    digitalWrite(BUILTIN_LED, LOW);
    Serial.println("HIGH");
  }
  delay(1000);
}


// ################## Sensor helper functions ##################
void send_value(int value) {
  // Connect to the mqtt broker if disconnected
  if (!client.connected()) {
    connect_mqtt();
  }
  client.loop();

  // Publish the measurements
  if (value == LOW) {
    client.publish(MQTT_TOPIC, "0");
  } else {
    client.publish(MQTT_TOPIC, "1");
  }
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
