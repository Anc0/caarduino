#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config.h"

// Wifi and mqtt client classes
WiFiClient espClient;
PubSubClient client(espClient);

// Sensor variables
const int analog_ip = A0; //Naming analog input pin
int inputVal  = 0;        //Variable to store analog input values

char msg_buff [16];

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  // Connect to wifi network
  connect_wifi();
  // Initialize mqtt client
  client.setServer(MQTT_IP, 1883);

}

void loop() {
  inputVal = analogRead (analog_ip); // Analog Values 0 to 1023
  
  // Connect to the mqtt broker if disconnected
  if (!client.connected()) {
    connect_mqtt();
  }
  client.loop();

  Serial.println(inputVal);
  dtostrf(inputVal, 5, 2, msg_buff);
  
  // Publish the measurements
  client.publish("cabackend/fsr_01", msg_buff);
  delay(3);
}

// ################## Sensor helper functions ##################
// #### INSERT SENSOR HELPER FUNCTIONS ####

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
