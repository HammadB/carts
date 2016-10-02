/*
 * Huzzah Pins
 * RFID Reader
 * VCC  - 3.3 V
 * RST  - 2
 * GND  - GND
 * MISO - 12 
 * MOSI - 13 
 * SCK  - 14 
 * NSS  - 16
 * 
 * Hx711 Load Cell
 * E+   - Blue wire to load cell
 * E-   - White wire to load cell
 * A-   - Ground to load cell
 * A+   - 3.3V to load cell
 * 
 * GND  - Ground
 * DT   - 5
 * SCK  - 4
 * VCC  - 3.3V 
 * 
 * Piezoelectric
 * Connected to pin 15, 22 Ohm to ground
 * 
 * Patrick Zhang 10/1/16
 * 
 * RFID code adapted from Luke Woodberry
 * Scale code adapted from Nathan Seidle
 */

#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "HX711.h"

#define DOUT  5
#define CLK  4
#define RST_PIN  2
#define SS_PIN  16
#define PIEZO  15
#define ERROR_LED 0

HX711 scale(DOUT, CLK);
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
float calibration_factor = 190300;
int lockout = 1500; //1.5 seconds lock out on identical item

String weight_products[50];
int num_weight_products = 0;
char* previous_uid = "";
uint32_t last_scan = 0;

String device_id = "1234";
const char* ssid = "CalVisitor";
const char* host = "carts-c0d29.firebaseio.com";
WiFiClientSecure client;
const int httpPort = 443;
const char* fingerprint = "7A 54 06 9B DC 7A 25 B3 86 8D 66 53 48 2C 0B 96 42 C7 B3 0A";
bool contains_weighted(const JsonObject& obj , const char* key);
bool by_weight(String product_id);
void error_sound();
void success_sound();
void setup() {
  Serial.begin(9600);  // Initialize serial communications with the PC
  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card
  pinMode(ERROR_LED, OUTPUT);
  digitalWrite(ERROR_LED, HIGH);

  //connect to wifi
  WiFi.begin(ssid);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //connect to wifi
  Serial.println("");
  Serial.println("WiFi connected");

  //connect to firebase
  Serial.println("\nAttempting to connect to server...");
  if (client.connect(host,443)) {
    if (client.verify(fingerprint, host)) {
      Serial.println("connected to server");
    } else {
      Serial.println("ssl cert mismatch");
    }
  } else {
    Serial.println("Failed to connect to server");
  }

  //configure the scale
  scale.set_scale(calibration_factor);
  scale.tare();

  //determine which products are by weight
  client.println(String("GET ") + "/products.json HTTP/1.1");
  client.println("Host: " + String(host));
  client.println();
  int timeout_count = 50;
  while(!client.available() && timeout_count > 0 ) {
    timeout_count -= 1;
    delay(100);
  }

  if (timeout_count == 0) {
    Serial.println("Connection Timeout.");
    return;
  }

  char json[512];
  int index = 0;
  while(client.available()){
    String line = client.readStringUntil('\r');
    index = line.indexOf("{");
    if(index >= 0) {
      line.substring(index).toCharArray(json,512);
    }
  }
  
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& products = jsonBuffer.parseObject(json);
  for (const JsonPair& pair : products) {
    if (contains_weighted(pair.value.as<JsonObject>(),"by_weight")) {
      
      weight_products[num_weight_products] = String(pair.key);
      num_weight_products += 1;
    }
  }
  Serial.println("Ready to scan.");
}

void loop() {
  // Look for new cards, and select one if present
  if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
    delay(50);
    return;
  }
  // Now a card is selected. The UID and SAK is in mfrc522.uid.
  // Dump UID
  //Serial.print(F("Card UID:"));
  char card_uid [10];
  int index = 0;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    index += snprintf(card_uid+index,10-index,"%02X", mfrc522.uid.uidByte[i]);
  } 
  if (strcmp(card_uid, previous_uid) == 0 && millis() - last_scan < lockout) {
    last_scan = millis(); //extend 1.5 second lockout
  }
  else {
    last_scan = millis();
    previous_uid = card_uid;
    bool weighted = by_weight(String(card_uid));
    if (weighted) {
      if (scale.get_units() < 0.05 ) {
        //nothing on the scale for this
        error_sound();
        return;
      }
    }
    Serial.println("Scanned: " + String(card_uid));

    String url = "/carts/" + device_id + ".json";
    
    client.println(String("GET ") + url + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println();
    int timeout_count = 50;
    while(!client.available() && timeout_count > 0 ) {
      timeout_count -= 1;
      delay(100);
    }

    if (timeout_count == 0) {
      Serial.println("Connection Timeout.");
      error_sound();
      return;
    }

    char json[512];
    int index = 0;
    while(client.available()){
      String line = client.readStringUntil('\r');
      index = line.indexOf("{");
      if(index >= 0) {
        line.substring(index).toCharArray(json,512);
      }
    }
    
    StaticJsonBuffer<512> jsonBuffer;
    JsonObject& cart = jsonBuffer.parseObject(json);
    String patch_data;
    if(!weighted) {
      unsigned int count = cart.get<unsigned int>(card_uid);
      count += 1;
  
      patch_data = "{\"" + String(card_uid) + "\":" + String(count) + "}";
    } else {
      float weight = cart.get<float>(card_uid);
      weight += scale.get_units();

      patch_data = "{\"" + String(card_uid) + "\":" + String(weight) + "}";
    }
    client.println(String("PATCH ") + url + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("User-Agent: curl/7.47.0");
    client.println("Accept: */*");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(patch_data.length());
    client.println();
    client.println(patch_data);
    
    while(!client.available()) {
      delay(100);
    }
    
    while(client.available()) {
      String line = client.readStringUntil('\r');
    }
    Serial.println("Successfully scanned. Ready for next item.");
    success_sound();
  }
}

bool contains_weighted(const JsonObject& obj , const char* key) {
  for (const JsonPair& pair : obj) {
    if (strcmp(pair.key, key) == 0) {
      return true;
    }
  }
  return false;
}

bool by_weight(String product_id) {
  for (int i = 0; i < num_weight_products; i++) {
    if (weight_products[i].equals(product_id)) {
      return true; 
    }
  }
  return false;
}

void error_sound() {
  digitalWrite(ERROR_LED, LOW);
  
  int noteDuration = 1000 / 4;
  tone(PIEZO, 196, noteDuration);
  
  int pause = noteDuration * 1.30;
  delay(pause);
  // stop the tone playing:
  noTone(PIEZO);
  digitalWrite(ERROR_LED, HIGH);
}

void success_sound() {
  int noteDuration = 1000 / 8;
  tone(PIEZO, 330, noteDuration);
  delay(noteDuration*1.1);
  tone(PIEZO, 349, noteDuration);
  delay(noteDuration*1.1);
  noTone(PIEZO);
}

