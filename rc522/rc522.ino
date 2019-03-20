/*
 * ----------------------------------------------------------------------
 * Example program showing how to read new NUID from a PICC to serial.
 * ----------------------------------------------------------------------
 * https://circuits4you.com
 * 
 * RC522 Interfacing with NodeMCU
 * 
 * Typical pin layout used:
 * ----------------------------------
 *             MFRC522      Node     
 *             Reader/PCD   MCU      
 * Signal      Pin          Pin      
 * ----------------------------------
 * RST/Reset   RST          D1 (GPIO5)        
 * SPI SS      SDA(SS)      D2 (GPIO4)       
 * SPI MOSI    MOSI         D7 (GPIO13)
 * SPI MISO    MISO         D6 (GPIO12)
 * SPI SCK     SCK          D5 (GPIO14)
 * 3.3V        3.3V         3.3V
 * GND         GND          GND
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config.h"

constexpr uint8_t RST_PIN = 5;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 4;     // Configurable, see typical pin layout above
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];

// Wifi and mqtt client classes
WiFiClient espClient;
PubSubClient client(espClient);

char id_buff [16];
int new_reading = 0;

void setup() { 
  // Initialize serial communication
  Serial.begin(115200);
  // Initialize indicator led
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  // Connect to wifi network
  connect_wifi();
  // Initialize mqtt client
  client.setServer(MQTT_IP, 1883);

  // #### INSERT SENSOR INIT ###
  sensor_init();
}
 
void loop() {
  // Doesn't work without this
  rfid.PICC_IsNewCardPresent();
  // Check if card was read.
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  // Retrieve type and check if is compatible.
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println("Your tag is not of type MIFARE Classic.");
    return;
  }

  // Check if card was already read (only for the last valid card)
  if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    // Convert read id to char array
    printHex(rfid.uid.uidByte, rfid.uid.size);
    
    Serial.print("The NUID tag is: ");
    Serial.println(id_buff);
    new_reading = 1;
  } else {
    Serial.println("Card read previously.");
    new_reading = 2;
  }

  if(new_reading > 0) {
    // Connect to the mqtt broker if disconnected
    if (!client.connected()) {
      connect_mqtt();
    }
    client.loop();
    if (new_reading == 1) {
      // Activate the seance
      client.publish("cabackend/activate", id_buff);   
      // Turn on the led
      digitalWrite(BUILTIN_LED, LOW);   
      // Wait for user to remove the card to prevent 
      // immediate accidental deactivation of the seance.
      delay(5000);
    } else if (new_reading == 2) {
      // Complete the seance
      client.publish("cabackend/deactivate", id_buff);
      // Reset current seance id
      for (byte i = 0; i < 4; i++) {
        nuidPICC[i] = 0xFF;
      }
      // Turn off the led      
      digitalWrite(BUILTIN_LED, HIGH);
      // Wait for user to remove the card to prevent 
      // immediate accidental activation of a new seance.
      delay(5000);
    }
    new_reading = 0;
  }

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
  
//  delay(1000);
}


// ################## Sensor helper functions ##################
// Sensor initialization
void sensor_init() {
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println("This code scan the MIFARE Classsic NUID.");
}

// Convert buffer id to char array
void printHex(byte *buffer, byte bufferSize) {
  String id = "";
  
  for (byte i = 0; i < bufferSize; i++) {
      id = String(id + String(buffer[i], HEX));
  }
  id.toCharArray(id_buff, 8);
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
