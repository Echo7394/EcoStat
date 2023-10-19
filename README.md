
  
# EcoStat
Seeed Studio Xiao ESP32C3 Smart Thermostat, but simple, and intuitive. Includes internet access to all functions, but does not spy on you, bloat the mcu with unneccesary code or gather any kind of pointless telemetry whatsover.
Energy saving features are included while not requiring you to sacrifice normal functionality.

Parts required:

**-Seeed Studio Xiao ESP32C3 or other similarly sized ESP32C3 board**

**-Any 128x64 SSD1306 I2C OLED Screen**

**-Two 5V One Channel Relay Module, I used "HiLetgo 2pcs 5V One Channel Relay Module Relay Switch with OPTO Isolation High Low Level Trigger" from Amazon**
  
**-A DHT22 Sensor, I used a "Gowoops 2pcs DHT22/AM2302 Digital Humidity and Temperature Sensor Module" from Amazon**
  
**-Micro Momentary Button, I used a 4 pin type, you can use anything that fits the mounting hole (6mm or less).**
  
   I also used beads as the buttons themselves for the momentary switches just because the actual button pieces were not long enough, get creative.


The screen displays Temperature, Humidity, MCU Temp, and Wifi Signal Strength.
Buttons toggle temperature up & down by one degree at a time.
The "Heat" relay does not trigger until the target temperature is 2 degrees less than the current temperature to prevent excessive toggling and save energy / gas / LP.
The web page also provides the option to trigger the "Fan" relay seperately for air circulation without climate control.
<p float="left">
<img src="https://github.com/Echo7394/EcoStat/blob/main/img/20231013_210953.jpg" width="200" />
<img src="https://github.com/Echo7394/EcoStat/blob/main/img/20231014_162400.jpg" width="200" />
<img src="https://github.com/Echo7394/EcoStat/blob/main/img/Screenshot%20from%202023-10-15%2016-58-46.png" width="200" />
<img src="https://github.com/Echo7394/EcoStat/blob/main/Case_Models/microstat.png" width="200" />
</p>

**Currently a work in progress. A/C control will be added soon, however since its fall, and cold outside, I havent made it that far yet.**


