Chinese Diesel Heater to MQTT Gateway
-------------------------------------

Based on the DieselHeaterRF Library  (by Jarno Kyttälä). My canges in this library include heater-error handling, clear-text decode for state and error values and override of GPIO settings. 

Does MQTT auto-discovery in Home-Assistant
Based on ESP32 and configured for the cheap ESP32C3 Supermini with CC1101 RF module.
For other boards GPIOs must be changed. 
GPIOs (MOSI, MISO, SCK, SS, GDO02) are defined globally in platformio.ini to overwrite defaults in DieselHeaterRF.h 
Things to adapt in Diesel-Heater-MQTT.h:
- LED-GPIO
- LED polarity 
- Timezone
- SSIDs (multiple SSIDs can be entered, the best signal strenght will be used)
- WiFi Password(s) 
- MQTT Server: Address, User, Password
- Update-Intervals
