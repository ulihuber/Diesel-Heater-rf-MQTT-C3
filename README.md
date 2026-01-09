Chinese Diesel Heater to MQTT Gateway
-------------------------------------

Based on the DieselHeaterRF Library  (by Jarno Kyttälä). My changes in this library include override of GPIO settings, heater-error handling and clear-text decode for state and error values. 

Does MQTT auto-discovery in Home-Assistant and creates a device with the available entities and icons.

The actual code is written for an  ESP32 and configured for the cheap ESP32C3 Supermini with CC1101 RF module.
For other boards GPIOs must be changed. 
GPIOs (MOSI, MISO, SCK, SS, GDO02) are defined globally in platformio.ini to overwrite defaults in DieselHeaterRF.h.

Things to adapt in Diesel-Heater-MQTT.h:
- LED-GPIO
- LED polarity 
- Timezone
- SSIDs (multiple SSIDs can be entered, the best signal strenght will be used)
- WiFi Password(s) 
- MQTT Server: Address, User, Password
- Update-Intervals
<img width="762" height="972" alt="grafik" src="https://github.com/user-attachments/assets/79f54f35-7659-4c10-ad69-14a2cdcf539a" />
