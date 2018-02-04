//
// Temperature and Humidity Sensors for ESP-12e and DHT-22 Sensor
//
// -- Output sent to MQTT server at //piano:1884
// -- MQTT streams: /feeds/temperature, /feeds/humidity
//  
// Guy Turcotte
// February 2016

// Parameters for the Arduino IDE:
//
//   - Board: NodeMCU 1.0 (ESP12E Module)
//   - Port: /dev/cu.SLAB_USBtoUART
//
//   - Librairies:
//
//     DHT22: Adafruit "DHT Sensor Library" v 1.2.3
//     MQTT:  PubSubClient version 2.6.0

// ---- Libraries ----

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#include  "local/local_params.h"

// ---- Development Mode ----

#define debugging 0

#if debugging
  #define init_debug() { Serial.begin(9600);  }
  #define debug(msg)   { Serial.print(msg);   }
  #define debugln(msg) { Serial.println(msg); }
#else
  #define init_debug()
  #define debug(msg)
  #define debugln(msg)
#endif

#define LED   16

// ---- DHT 22 sensor ----

#define SENSOR 1

#if SENSOR
  #define DHTPIN 5

  DHT dht(DHTPIN, DHT22);
#endif

// ---- MQTT Server Parameters ----

#define MQTT_ID          "tempSensor" " " __DATE__ " " __TIME__

#define TEMPERATURE_FEED "/feeds/temperature"
#define HUMIDITY_FEED    "/feeds/humidity"

// ---- Global State (you don't need to change this!) ----

WiFiClient   wifi_client;
PubSubClient mqtt(wifi_client);

// ==== setup() ====

void setup() 
{
  delay(200);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  init_debug();
  delay(4000);
}

// ==== loop() ====

void loop() 
{
  float humidity;
  float temperature;

  // Ensure that MQTT is still connected

  MQTT_connect();

  #if SENSOR
    
    // Grab the current state of the sensor
    
    humidity    = dht.readHumidity();
    temperature = dht.readTemperature();
  #else
    humidity    = 30.0;
    temperature = 21.0;  
  #endif


  if (isnan(humidity) || isnan(temperature)) {
    debugln(F("Failed to read from DHT sensor!"));
    blink(4);
    return;
  }

  debug(F("T:"));  debug  (temperature);  
  debug(F(" H:")); debugln(humidity);

  // Publish data

  MQTT_publish(TEMPERATURE_FEED, temperature);
  MQTT_publish(HUMIDITY_FEED,    humidity);

  // Repeat every 15 minutes

  for (int i = 0; i < 90; i++) {
    delay(10000UL);
    //mqtt.loop();
  }
}

// ---- blink() ----

void blink(int count)
{
  while (count--) {
    digitalWrite(LED, LOW);
    delay(500);
    digitalWrite(LED, HIGH);
    delay(500);
  }
}

// ---- WIFI_connect() ----

void WIFI_connect()
{
  if (WiFi.status() == WL_CONNECTED) return;
  
  // Connect to WiFi access point.
  
  debug  (F("Connecting to "));
  debugln(WLAN_SSID);

  debug  (F("MAC Address: "));
  debugln(WiFi.macAddress());

  do {
    int count = 20;

    WiFi.mode(WIFI_STA);
    WiFi.begin(WLAN_SSID, WLAN_PASSWD);
    delay(1);

    while ((count--) && (WiFi.status() != WL_CONNECTED)) {
      delay(1000);
      debug(F("."));
    }
    debugln();

    if (WiFi.status() != WL_CONNECTED) {
      //WiFi.mode(WIFI_OFF);
      delay(500);
    }
  } while (WiFi.status() != WL_CONNECTED);
  
  debugln(F("WiFi connected"));
  
  debug  (F("IP address: "));
  debugln(WiFi.localIP());  
}

// ---- MQTT_publish(feed, val) ----

void MQTT_publish(const char * feed, float val)
{
  char msg[20];
  
  dtostrf(val, 4, 1, msg);
  
  debugln(F("Publishing the following message"));
  debugln(msg);
  if (!mqtt.publish(feed, msg)) {
    debugln(F("Unable to publish message"));
  }
}

// ---- MQTT_connect() ----

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.

void MQTT_connect() 
{
  // Return if already connected.

  if (mqtt.loop()) return;
  
  WIFI_connect();
  
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  
  debug  (F("Trying to connect to MQTT Server with ID: "));
  debugln(MQTT_ID);
  
  while (!mqtt.connect(MQTT_ID)) { // connect will return 1 for connected

    switch (mqtt.state()) {
      case -4: debugln(F("Connection Timeout")); break;
      case -3: debugln(F("Connection Losted" )); break;
      case -2: debugln(F("Connect Failed"    )); break;
      case -1: debugln(F("Disconnected"      )); break;
      case  1: debugln(F("Wrong protocol"    )); break;
      case  2: debugln(F("ID rejected"       )); break;
      case  3: debugln(F("Server unavail"    )); break;
      case  4: debugln(F("Bad user/pass"     )); break;
      case  5: debugln(F("Not authed"        )); break;
      default: debugln(F("Unknown error"     )); break;
    }
    
    delay(5000);  // wait 5 seconds

    WIFI_connect();
  }

  debugln(F("Connected"));
}

