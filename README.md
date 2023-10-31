
  
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
<img src="https://raw.githubusercontent.com/Echo7394/EcoStat/main/img/EcoNet.png" width="200" />
<img src="https://github.com/Echo7394/EcoStat/blob/main/Case_Models/microstat.png" width="200" />
</p>

**Currently a work in progress. A/C control will be added soon, however since its fall, and cold outside, I havent made it that far yet.**


Detailed description of the goals of this project:

A thermostat control system using a Xiao ESP32C3, and various sensors and peripherals. Here's an overview of what the code does:

1. It includes various libraries for Wi-Fi management, sensor readings (DHT22), OLED display control, and web server functionality.
2. It defines pins and addresses for the OLED display, DHT22 sensor, buttons, and relays.
3. It sets up Wi-Fi using the ESPAsyncWiFiManager library, allowing users to configure the network credentials via a captive portal.
4. It initializes the OLED display, creates a web server on port 80, and defines routes for controlling the thermostat remotely.
5. It handles HTTP GET requests for increasing/decreasing the target temperature, turning the fan on/off, and changing the operating mode (heating, cooling, or off).
6. It defines functions for reading the temperature, changing the mode, and updating the OLED display with current status.
7. It sets up interrupt handlers for physical buttons to change the target temperature, mode, or fan status.
8. The main loop continuously reads temperature and humidity from the DHT22 sensor and updates the OLED display.
9. Depending on the operating mode (heating or cooling), it controls relays to turn on/off the heating or cooling system to maintain the desired temperature.

Overall, this code creates a thermostat system with remote control capabilities, allowing users to adjust the target temperature, mode, and fan status through a web interface. It also displays current temperature and humidity on an OLED screen. The system can operate in heating or cooling mode based on the target temperature, and it provides visual feedback on the OLED screen for the user. The physical buttons provide additional control and feedback for users without accessing the web interface.


