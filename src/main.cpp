//**************************************************************************
// Chinese Diesel Heater to MQTT Gateway
//
// Does auto-discovery in Home-Assistant
// Uses  DieselHeaterRF Library  (by Jarno Kyttälä)
// Based on ESP32 and configured for the cheap ESP32C3 Supermini
// For other boards GPIOs must be changed. 
// GPIOs (MOSI, MISO, SCK, SS, GDO02) are defined globally in platformio.ini to overwrite defaults in DieselHeaterRF.h 
// Things to adapt in Diesel-Heater-MQTT.h:
// - GPIO  LED
// - LED polarity 
// - Timezone
// - SSIDs (multiple SSIDs can be listed, best RSSI will be taken.)
// - WiFi Password
// - MQTT Server: Address, User, Password
// - Update-Intervals
//
//**************************************************************************

#include "Diesel-Heater-MQTT.h"


WebServer server(80);
DieselHeaterRF heater;
heater_state_t state;

//**************************************************************************
// find best wireless network from list nets[]
void getBestWifi() 
{
  int bestRSSI = -999;

  WiFi.disconnect();
  //WiFi.persistent(true);
  WiFi.mode(WIFI_STA);

  Serial.printf("Network Scan\n");
  int i;
  for (i = 0; i < sizeof(nets) / sizeof(Networks); i++) 
    {
    int loops = 10;
    Serial.printf("SSID: %s --> ", nets[i].ssid);
    WiFi.begin(nets[i].ssid, nets[i].password);
    while ((WiFi.status() != WL_CONNECTED) && (loops-- > 0)) delay(500);
    if (WiFi.status() != WL_CONNECTED) 
    {
      Serial.printf("Not available!\n");
    } 
    else 
    {
      Serial.printf("RSSI: %d\n", WiFi.RSSI());
      if (WiFi.RSSI() > bestRSSI)  // keep index for SSID with better RSSI
      {
        bestRSSI = WiFi.RSSI();
        bestNet = i;
      }
    }
    WiFi.setAutoReconnect(false); 
    WiFi.disconnect();
    digitalWrite(LED_PIN, !LED_ON);  // blink as indication for network search
    delay(200);                      //
    digitalWrite(LED_PIN, LED_ON);
  }
  Serial.printf("Connecting to %s, ", nets[bestNet].ssid);
  //WiFi.setHostname("sensor");
  WiFi.begin(nets[bestNet].ssid, nets[bestNet].password);
  int loops = 10;
  while ((WiFi.status() != WL_CONNECTED) && (loops-- > 0)) delay(500);

  Serial.printf("Connected SSID: %s\n", nets[bestNet].ssid);
  Serial.printf("RSSI: %d\n", WiFi.RSSI());
  Serial.println(WiFi.localIP());
}


//****************************************************************
// MQTT routines

void MQTTreconnect() 
{
  // Init MQTT
  int attempts = 0;
  while (!MQTTclient.connected() && (attempts++ < 3))  // Loop if we're not reconnected
  {
    Serial.println("Attempting MQTT connection...");
    if (MQTTclient.connect(sensorname, mqtt_user, mqtt_password)) 
    {
      Serial.println("connected");
    } 
    else 
    {
      //Serial.printf("MQTT connect failed, rc= %s\n", MQTTclient.state());
      Serial.println("MQTT try again in 5 seconds\n");
      if (WiFi.status() != WL_CONNECTED) getBestWifi() ;
      delay(2000);  // Wait 5 seconds before retrying
    }
  }
  if (!MQTTclient.connected())
  {
    Serial.printf("No MQTT server\n");
  }
}


void MQTTpubRetained(String entity, String strValue) 
{
  char topic[80];

  strcpy(topic, "Heater/");
  Serial.printf("Topic: %s --> %s\n", topic, strValue.c_str());

  if (!MQTTclient.publish(topic, strValue.c_str(), true)) 
  {
    Serial.printf("MQTT publish retained - Fehler!\n");
  }
}

void sendMQTTDiscoveryMsg(String topic, String topic_type,  String unit, String entityClass, String iconString, String yamlTemplate) 
{
  String discoveryTopic = String("homeassistant/") + topic_type + "/Heater/" + topic + "/config";
  JsonDocument doc;
  doc["name"] = topic;  //String(initData.sensorName) + "_" + topic;
  doc["unique_id"] = String("Heater_") + topic;
  doc["force_update"] = true;
  doc["enabled_by_default"] = true;
  if (iconString.length() > 0)   doc["icon"] = iconString;
  if (topic_type == "sensor")
    {
    doc["state_topic"] = String("Heater/state");
    if (unit != "") doc["unit_of_measurement"] = unit;
    if (entityClass != "") doc["device_class"] = entityClass;
    doc["value_template"] = yamlTemplate;
    }
  else if (topic_type == "switch")
    { 
    doc["state_topic"] = String("Heater/switch/state");
    doc["command_topic"] = String("Heater/switch/set");
    doc["payload_on"] = String("ON");
    doc["payload_off"] = String("OFF");
    }

  JsonObject device = doc["device"].to<JsonObject>();
  doc["enabled_by_default"] = true;
  device["manufacturer"] = "UHU";
  device["model"] = version;
  device["name"] = String("Heater");
  device["identifiers"][0] = String("Heater");  //WiFi.macAddress();

  char output[512];
  doc.shrinkToFit();  // optional
  int n = serializeJson(doc, output);

  MQTTreconnect();
  if (!MQTTclient.publish(discoveryTopic.c_str(), (const uint8_t*)output, n, true)) 
     Serial.printf("MQTT publish discovery - Fehler!\n");
  delay(200);
}

void MQTTsendDiscover() 
  {
  sendMQTTDiscoveryMsg("Time",         "sensor", "",   "",            "mdi:clock-outline", "{{ value_json.time }}");
  sendMQTTDiscoveryMsg("State",        "sensor", "",   "",            "mdi:state-machine", "{{ value_json.state }}");
  sendMQTTDiscoveryMsg("Power",        "sensor", "",   "",            "mdi:gas-burner",    "{{ value_json.power }}");
  sendMQTTDiscoveryMsg("Error",        "sensor", "",   "",            "mdi:alert-circle",  "{{ value_json.error }}");
  sendMQTTDiscoveryMsg("Voltage",      "sensor", "V",  "voltage",     "mdi:battery-medium","{{ value_json.voltage}}");
  sendMQTTDiscoveryMsg("Ambient",      "sensor", "°C", "temperature", "",                  "{{ value_json.ambient}}");
  sendMQTTDiscoveryMsg("Case",         "sensor", "°C", "temperature", "",                  "{{ value_json.case}}");
  sendMQTTDiscoveryMsg("Setpoint",     "sensor", "°C", "temperature", "",                  "{{ value_json.setpoint}}");
  sendMQTTDiscoveryMsg("Pumpfrequency","sensor", "Hz", "frequency",   "",                  "{{ value_json.pumpfrequency }}");
  sendMQTTDiscoveryMsg("Mode",         "sensor", "",   "",            "",                  "{{ value_json.mode }}");
  sendMQTTDiscoveryMsg("IP",           "sensor", "",   "",            "mdi:wifi",          "{{ value_json.IP }}");
  sendMQTTDiscoveryMsg("SSID",         "sensor", "",   "",            "mdi:wifi",          "{{ value_json.SSID }}");
  sendMQTTDiscoveryMsg("RSSI_WIFI",    "sensor", "dB", "",            "mdi:wifi",          "{{ value_json.RSSI_WIFI}}");
  sendMQTTDiscoveryMsg("RSSI_433",     "sensor", "dB", "",            "mdi:antenna",       "{{ value_json.RSSI_433 }}");
  sendMQTTDiscoveryMsg("Heating",      "switch",  "",  "",            "mdi:radiator",      "{{ value_json.heating }}");
  }

void MQTTsendBlock() {
  char timeString[64];
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) 
    {
    strftime(timeString, sizeof(timeString), "%Y-%m-%dT%H:%M:%S", &timeinfo); //   "%d.%m.%Y %H:%M"
    }

  MQTTreconnect();
 
  JsonDocument doc;
  doc["time"] = timeString; //"01.01.2000"; //epoch_to_string(time(nullptr));
  doc["state"] = String(stateList[state.state]);
  doc["power"] = String(state.power == 0 ? "Off" : "On" );
  doc["voltage"] = String(state.voltage);
  doc["error"] = String(errorList[state.error]);
  doc["ambient"] = String(state.ambientTemp);
  doc["case"] = String(state.caseTemp);
  doc["setpoint"] = String(state.setpoint);
  doc["pumpfrequency"] = String(state.pumpFreq);
  doc["mode"] = String(state.autoMode == 0x32? "Manual" : "Auto" );
  doc["IP"] = WiFi.localIP().toString();
  doc["SSID"] = String(nets[bestNet].ssid);
  doc["RSSI_WIFI"] = String(WiFi.RSSI());
  doc["RSSI_433"] = String(state.rssi);
  doc["version"] =    version;
 
  char buffer[500];
  int n = serializeJson(doc, buffer);
  //Serial.printf("JSON buffersize data: %d\n", n);
  if (!MQTTclient.publish(String("Heater/state").c_str(), (const uint8_t*)buffer, n, true))
    Serial.printf("MQTT publish data - Fehler!\n");
  delay(200);
}

//**************************************************************************
//Setup

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nHeater - U.Huber 2026\n");
  Serial.println("Build: " __TIME__ "  " __DATE__);
  Serial.println(__FILE__);
  char *p1;
  for (char *p = __FILE__; *p != 0; p++)
    if (*p == '\\') p1 = p + 1;
  version = String(p1) + " " + String(__DATE__) + String("  ") + String(__TIME__);
  pinMode(LED_PIN, GPIO_MODE_INPUT_OUTPUT);  // for easy inverting
  digitalWrite(LED_PIN, LED_ON);

  getBestWifi();

  getNTPtime(); 

  Serial.println("Init MQTT");   
  MQTTclient.setServer(mqtt_server, 1883);
  MQTTclient.setBufferSize(512);  // discovery packet is bigger than 256
  MQTTsendDiscover();
  MQTTclient.setCallback(callback);
  MQTTclient.subscribe("Heater/switch/set");
  MQTTclient.publish("Heater/switch/state", "OFF", true);
  

  Serial.println("Init Webserver");
  server.on("/", handleRootPath);
  server.on("/answer", handleAnswer);
  server.begin();

  Serial.println("Init OTA");
  ArduinoOTA.setPassword("uhu");
  ArduinoOTA.setHostname(DNS_NAME);
  ArduinoOTA.begin();

  heater.begin();
#ifdef HEATER_ADDR
  heater.setAddress(HEATER_ADDR);
#else
  Serial.println("Started pairing, press and hold the pairing button on the heater's remote or LCD-Panel...");
  heaterAddr = heater.findAddress(60000UL);
  if (heaterAddr) 
    {
     Serial.print("Got address: ");
     Serial.println(heaterAddr, HEX);
     heater.setAddress(heaterAddr);
     } 
  else 
  {
     Serial.println("Failed to find a heater -  program stopped");   
     while(1) {}
  }
#endif

Serial.println("Startup complete");
}

//**************************************************************************
// Main loop - handle periodic updates 

int loopCount = 0;
void loop()
  {
  MQTTclient.loop();
  server.handleClient();
  ArduinoOTA.handle();
  

  if (loopCount++ % int(HEATER_POLL_INTERVAL / LOOP_INTERVAL) == 1) 
    {
  
    ArduinoOTA.handle();
    heater.sendCommand(HEATER_CMD_WAKEUP);

    if (heater.getState(&state)) 
      {
      Serial.printf("Power: %d, %s, %s, Voltage: %f, Ambient: %d, Case: %d, Setpoint: %d, PumpFreq: %f, Auto: %d, RSSI: %d\n", 
                     state.power, stateList[state.state], errorList[state.error], state.voltage, state.ambientTemp, state.caseTemp, state.setpoint, state.pumpFreq, state.autoMode, state.rssi); 
      }
    else 
      {
      Serial.println("Failed to get the state");
     }
    if (WiFi.status() == WL_CONNECTED) MQTTsendBlock();
    else Serial.printf("No WLAN, MQTT skipped\n"); 

  }
  delay(LOOP_INTERVAL);
}


//**************************************************************************
// callback - Wird aufgerufen, wenn HA den Schalter betätigt

void callback(char* topic, byte* payload, unsigned int length) 
  {
  String msg = "";
  Serial.println("Callback");

  for (int i = 0; i < length; i++) 
    {
    msg += (char)payload[i];
    }
  if (msg == "ON") 
    {
    digitalWrite(LED_PIN, LED_ON); 
    if (state.state == HEATER_STATE_OFF)
      {
      Serial.println("Turning heater ON");
      heater.sendCommand(HEATER_CMD_POWER);
      MQTTclient.publish("Heater/switch/state", "ON", true);
      }
    else
      {
      Serial.println("Heater already ON");
      }
    
    } 
  else if (msg == "OFF") 
    {
    digitalWrite(LED_PIN, !LED_ON); 
    if (!heatingOn && (state.state  <= HEATER_STATE_RUNNING))    //!= HEATER_STATE_OFF)) // do not re-trigger switch off
      {
      Serial.println("Turning heater OFF");
      heater.sendCommand(HEATER_CMD_POWER);
      MQTTclient.publish("Heater/switch/state", "OFF", true);
      }
    else
      {
      Serial.println("Heater already OFF");   
      }
    }

  }


  

//*****************************************************************
// Get local time
void getNTPtime()
  {
  // NTP mit Zeitzonen-String konfigurieren
  configTzTime(timeZone, ntpServer);
  
  // Auf Zeitabruf warten
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Fehler beim Abruf der Zeit!");
    return;
  }
  
  Serial.println(&timeinfo, "Aktuelle Zeit: %A, %B %d %Y %H:%M:%S");
  Serial.print("Zeitzone: ");
  Serial.println(timeZone);
  }

  
//*********************************************************
boolean activeWeb = false;  // Flag for staying in loop while in http config
void handleRootPath() 
{
  activeWeb = true;
  String sResponse;
  sResponse = F("<html>\n");
  sResponse += F("<head runat='UHU'>\n");
  sResponse += F("<meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>\n");
  sResponse += F("<title>Diesel Heater</title>\n");
  sResponse += F("<font color='#000000'><body bgcolor='#d0d0f0'>");
  sResponse += F("<meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=yes'>");
  sResponse += F("<h1>Setup</h1>");
  sResponse += F("<h3>");
  sResponse += version + "<BR>";
  sResponse += WiFi.localIP().toString();
  sResponse += F(" --- ");
  sResponse += String(sensorname);
  sResponse += F("<BR><BR>");

  // sResponse += F("<form action='/answer' method='POST'>");
  // sResponse += F("<BR>Neuer Sensorname: ");
  // sResponse += F("<input type='text' name='Name' value='' size='18'><BR>");
  // sResponse += F("<BR>Neue Sleeptime [s]: ");
  // sResponse += F("<input type='number' name='Sleeptime' value='");
  // sResponse += uint64_t(SLEEPTIME) / 1000000;
  // sResponse += F("' size='3'><BR><BR>");
  // sResponse += F("<input type='submit' name='SUBMIT' value='senden'>");
  // sResponse += F("<input type='submit' name='EXIT' value='abbrechen'>");
  // sResponse += F("</form></html>");
  server.setContentLength(sResponse.length());
  server.send(200, "text/html", sResponse);
}

void handleAnswer() 
{
  if (server.hasArg("SUBMIT"))  // send button pressed  sensorname
  {
    
    // if (strlen(server.arg("Name").c_str()) > 1) strcpy(sensorname, server.arg("Name").c_str());
    // Serial.printf("\nNeuer Name: %s\n", sensorname);
    // if (strlen(server.arg("Sleeptime").c_str()) > 1)
      // initData.sleepTime = (server.arg("Sleeptime")).toInt();
    // else
      // initData.sleepTime = SLEEPTIME;
    // Serial.printf("Neue Sleeptime: %d [s]\n", initData.sleepTime);
  }
  activeWeb = false;
}