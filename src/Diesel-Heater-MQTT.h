#pragma once
/*    For ESP32C3 Supermini
 * 
 *    ESP32         CC1101
 *    -----         ------
 *    10  <-------> GDO2
 *    4   <-------> SCK
 *    3v3 <-------> VCC
 *    6   <-------> MOSI
 *    5   <-------> MISO
 *    7   <-------> CSn
 *    GND <-------> GND
 * 
 */


#include <arduino.h>
#include "DieselHeaterRF.h"
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson 
#include "time.h"


#define LED_PIN 8
#define LED_ON 1         // LED polarity. 1 is active-high. can be changed if LED is inverted

#define DNS_NAME "heater"  // used by ArduinoOTA (uses ESPmDNS internally)
#define HEATER_POLL_INTERVAL  10000
#define LOOP_INTERVAL         100
#define HEATER_ADDR 0x336F1BA6



// WiFi connection
typedef struct  // for table of available  wireless networks
{
  const char *ssid;
  const char *password;
} Networks;

Networks nets[] = { { "SSID1", "password1" },  // table of available SSIDs
                    { "SSID2", "password2" } };
int bestNet = 0;

// Zeitzone-String für Deutschland (automatisch Sommer/Winterzeit)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0; // UTC (wird durch Zeitzonen-String überschrieben)
const int daylightOffset_sec = 0; // Wird ignoriert wenn Zeitzonen-String verwendet wird
// Posix Zeitzonen-String für Deutschland (automatische DST-Umstellung)
const char* timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";

// MQTT
const char *sensorname = "Heater";
const char *mqtt_server = "homeassistant";        //"homeassistant.local";        // network address of MQTT server
const char *mqtt_user = "MQTT-user";         // MQTT Username here
const char *mqtt_password = "MQTT-password";  // MQTT Password here
WiFiClient espClient;
PubSubClient MQTTclient(espClient);

/* Globals */
String version;
uint32_t heaterAddr; // Heater address is a 32 bit unsigned int. Use the findAddress() to get your heater's address.
bool heatingOn = false;

void getBestWifi();
void MQTTreconnect();
void MQTTpubRetained(String entity, String strValue);
void MQTTsendDiscover();
void MQTTsendBlock();
void handleRootPath();
void handleAnswer();
void getNTPtime();
void callback(char* topic, byte* payload, unsigned int length) ;
