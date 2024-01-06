/**************************************************************************************
░█▀▀░█▀▀░█▀█░█▀▀░▀█▀░█▀█░▀█▀     													  
░█▀▀░█░░░█░█░▀▀█░░█░░█▀█░░█░                                                          
░▀▀▀░▀▀▀░▀▀▀░▀▀▀░░▀░░▀░▀░░▀░                                                          
GNU GENERAL PUBLIC LICENSE V3.0                                                       
Made by Echo7394                             
This is the most updated version of the code!!
**************************************************************************************/
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP_WiFiManager.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_SSD1306.h>
#include <esp_system.h>
#include <ESPAsyncWebServer.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1        // Reset pin (if used)
#define SCREEN_ADDRESS 0x3C  // I2C address for the OLED

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHT_PIN 20  // Sets the data pin for the DHT22
DHT dht(DHT_PIN, DHT22);

#define BUTTON_FAN 4   // Button to switch fan on and off manually
#define BUTTON_MODE 3  // Button to select "Heating / Cooling / Off"
#define BUTTON_PIN0 8  // Button to raise tempSet
#define BUTTON_PIN1 2  // Button to lower tempSet

#define buzzerPin 5   // trigger pin for piezo buzzer
#define RELAY_PIN 10   // trigger pin for "Heat" relay
#define RELAY_PIN0 5   // trigger pin for "Cooling" relay
#define RELAY_PIN1 21  // trigger pin for "Fan" relay

String ssid = "blank";      // Will store the Wi-Fi SSID from the captive portal
String password = "blank";  // Will store the Wi-Fi password

//IPAddress localIP(192, 168, 0, 201); //Uncomment to set a Static IP for EcoStat
//IPAddress gateway(192, 168, 0, 1);
//IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);  // Web server to port 80 TCP

int tempSet = 68;                        // Initial target temperature in degrees Fahrenheit
const char *http_username = "admin";          // Change to your desired username
const char *http_password = "13371337";  // Change to your desired password

unsigned long lastDebounceTime = 0;       // Momentary switches like to be bouncy
unsigned long debounceDelay = 300;       // These variables will take care of that later
unsigned long modeStatusLastUpdated = 0;  // will be used for timing of displayModeStatus

bool heatingOn = false;  // heating status, starts as false since RELAY_PIN typically starts LOW
bool coolingOn = false;  // cooling status
bool fanisOn = false;    // same as above but for.... the fan relay

bool wifiCredentialsEntered = false;

unsigned long lastDHTPollTime = 0;
const unsigned long dhtPollInterval = 3000; // var to poll the DHT22 every 3 seconds, for stability

float celsiusToFahrenheit(float celsius) {  //Converts Celsius to Fahrenheit
  return (celsius * 9.0 / 5.0) + 32.0;
}

int mode = 0;  // 0 for off, 1 for heating, 2 for cooling

void changeMode() {
  mode = (mode + 1) % 3;  // Cycle through modes: 0 -> 1 -> 2 -> 0
  // Turn off heating and cooling when switching modes
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(RELAY_PIN0, LOW);
}

void displayTempSet(int tempSet) {  // This function gets flashed briefly on the OLED
  display.clearDisplay();           // when the target temp "tempSet" is changed
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.print(F("Target: "));
  display.print(tempSet);
  display.display();
}

void displayFanStatus() {  // Gets flashed to the OLED whenever fanisOn changes
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  if (fanisOn == true) {
    display.print(F("Fan: On"));
  } else {
    display.print(F("Fan: Off"));
  }
  display.display();
}

int modeFan = 0;

void switchFan() {
  modeFan = (modeFan + 1) % 2; // Cycle fan state 0 -> 1 -> 0
  digitalWrite(RELAY_PIN1, modeFan == 1); // Turn on the fan relay when modeFan is 1
  fanisOn = (modeFan == 1);
}

void displayModeStatus(String modeStatus) {  // Gets flashed to the OLED whenever modeStatus changes
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(40, 10);
  display.print(F("Mode: "));
  display.setCursor(25,30);
  display.print(modeStatus);
  display.display();

  // Record the time when modeStatus is updated for use later
  modeStatusLastUpdated = millis();
}

void playTone(float frequency, int duration) {
  tone(buzzerPin, frequency);
  delay(duration);
  noTone(buzzerPin);
  delay(100);  // pause between tones
}

// setup function for BUTTON_MODE to increment the value of changeMode
void buttonmodePressed() {
  if (millis() - lastDebounceTime > debounceDelay) {
    changeMode();
    lastDebounceTime = millis();
  }
}

void button0Pressed() {
  if (millis() - lastDebounceTime > debounceDelay) {  // Fancy debounce stuff to make sure a button press = just 1 press
    tempSet += 1;                                     // Increase the target temperature by 1 degree
    lastDebounceTime = millis();
  }
}

void button1Pressed() {
  if (millis() - lastDebounceTime > debounceDelay) {
    tempSet -= 1;  // Increase the target temperature by 1 degree
    lastDebounceTime = millis();
  }
}

void buttonfanPressed() {
  if (millis() - lastDebounceTime > debounceDelay) {
  switchFan();
  lastDebounceTime = millis();
  }
}

void setup() {
//Serial.begin(115200);  // uncomment this if you need serial output for debugging
  delay(3000);           // Delay because i've heard setup can get skipped sometimes
                         // Do not know if thats true so this may not be neccessary

   playTone(700, 150);
   playTone(1000, 150);

  // Initialize the OLED screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  display.setTextWrap(true);
  display.clearDisplay();  // This just ensures that we start with a clear display
  display.display();       // on the SSD1306 screen

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RELAY_PIN0, OUTPUT);
  pinMode(RELAY_PIN1, OUTPUT);   // Set Relay trigger pins as outputs, because they are.
  digitalWrite(RELAY_PIN, LOW);  // Ensure the relay is initially off
  digitalWrite(RELAY_PIN0, LOW);
  digitalWrite(RELAY_PIN1, LOW);
  pinMode(BUTTON_PIN0, INPUT_PULLUP);  // Gotta use a pullup for buttons because otherwise
  pinMode(BUTTON_PIN1, INPUT_PULLUP);  // Erwins Cats are going to get involved
  pinMode(BUTTON_FAN, INPUT_PULLUP);

  // display a notification to SSD1306 Screen for user setup if WiFi not attached to SSID
  WiFi.mode(WIFI_STA);
  ESP_WiFiManager wifiManager("EcoStatSetup");
  Serial.println("Waiting for WiFi credentials...");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 0);
  display.print("Connect to WiFi:");
  display.setCursor(28, 15);
  display.print("EcoStatSetup");
  display.setCursor(17, 40);
  display.print("In Your Browser");
  display.setCursor(10, 55);
  display.print("Enter: 192.168.4.1");
  display.display();
  Serial.println(wifiManager.getStoredWiFiPass());
  wifiManager.autoConnect("EcoStatSetup");  // Start a captive portal for the user to enter WiFi Credentials
  ssid = wifiManager.getStoredWiFiSSID();
  password = wifiManager.getStoredWiFiPass();
  wifiManager.setConfigPortalTimeout(300);  // Setup portal timeout in seconds, in case user doesnt setup WiFi

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(40, 10);
    display.print(F("WiFi"));  // display to SSD1306
    display.setCursor(5, 30);
    display.print(F("Connecting"));
    display.display();
    delay(1000);
  }
  Serial.println("Connected to WiFi");
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(40, 10);
  display.print(F("WiFi"));  // display to SSD1306
  display.setCursor(10, 30);
  display.print(F("Connected"));
  display.display();
  delay(3000);

  ArduinoOTA.begin();
  ArduinoOTA.setHostname("ESP32C3");
  ArduinoOTA.setPassword("ESP32C3");
  dht.begin();

  // Serve a webpage to control features remotely,
  // also added password protection so that you can access it outside of your home LAN
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    float temperatureC = dht.readTemperature();
    float temperatureF = celsiusToFahrenheit(temperatureC);
    String heatingStatus = heatingOn ? "On" : "Off";  // Determine the state of heatingOn and assign it to heating Status with a variable string
    String fanStatus = fanisOn ? "On" : "Off";        // Same thing but you know... the fan
    String coolingStatus = coolingOn ? "On" : "Off";  // yep


    String html = "<html><body style='text-align: center;font-size: 24px;background-color: #000;color: #00FFD3;@import url('https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500;900&display=swap');'>";
    html += "<head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<meta http-equiv='refresh' content='8'>"; // Refresh the page every 8 sec to update values
    html += "</head>";
    html += "<h1 style='margin-top: -8px; color: #00FFD3;text-shadow: -3px 3px 5px #5C5C5C;'>EcoStat Control</h1>";
    html += "<p>Current Temperature: <strong id='currentTemp'>" + String(temperatureF, 1) + " F</strong></p>";
    html += "<p>Target Temperature: <strong><span id='tempSet'>" + String(tempSet) + "</span> F</strong></p>";
    html += "<p>Heating: <strong id='heatingStatus'>" + heatingStatus + "</strong></p>";  // Display heating status
    html += "<p>Cooling: <strong id='coolingStatus'>" + coolingStatus + "</strong></p>";  // Display cooling status
    html += "<p>Fan: <strong id='fanStatus'>" + fanStatus + "</strong></p>";              // Display fan status
    html += "<p>Mode: <strong id='modeStatus'>" + (String((mode == 0 ? "Off" : (mode == 1 ? "Heating" : "Cooling")))) + "</strong></p>";
    html += "<p><button onclick='increaseTemp()'>Temp +</button></p>";
    html += "<p><button onclick='decreaseTemp()'>Temp -</button></p>";
    html += "<p><button onclick='switchFan()'>Fan</button></p>";
    html += "<p><button onclick='changeMode()'>Mode</button></p>";
    html += "<p><button class='diff' onclick='restart()'>Restart Device</button></p>";
    html += "<p><button class='diff' onclick='reset()'>Reset WiFi</button></p>";
    html += "<style>";
    html += "button { background-color: #222; color: #fff; border: 2px solid #444; border-radius: 20px; font-size: 24px; width: 25vw; }";
    html += "button.diff { background-color: #222; color: #fff; border: 2px solid #444; border-radius: 20px; font-size: 10px; width: 13vw; }";
    html += "</style>";
    html += "<script>";                   // XML is hell
    html += "function increaseTemp() {";  
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/increase', true);";
    html += "  xhr.send();";
    html += "  xhr.onload = function () {";
    html += "    if (xhr.status === 200) {";
    html += "      var tempSetSpan = document.getElementById('tempSet');";
    html += "      tempSetSpan.innerText = parseInt(tempSetSpan.innerText) + 1;";
    html += "    }";
    html += "  };";
    html += "}";
    html += "function decreaseTemp() {";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/decrease', true);";
    html += "  xhr.send();";
    html += "  xhr.onload = function () {";
    html += "    if (xhr.status === 200) {";
    html += "      var tempSetSpan = document.getElementById('tempSet');";
    html += "      tempSetSpan.innerText = parseInt(tempSetSpan.innerText) - 1;";
    html += "    }";
    html += "  };";
    html += "}";
    html += "function switchFan() {";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/fanSwitch', true);";
    html += "  xhr.send();";
    html += "  xhr.onload = function () {";
    html += "    if (xhr.status === 200) {";
    html += "      var fanStatusVal = document.getElementById('fanStatus');";
    html += "      fanStatusVal.innerText = (fanStatusVal.innerText === 'On') ? 'Off' : 'On';";
    html += "    }";
    html += "  };";
    html += "}";
    html += "function changeMode() {";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/changeMode', true);";
    html += "  xhr.send();";
    html += "  xhr.onload = function () {";
    html += "    if (xhr.status === 200) {";
    html += "      var modeStatus = document.getElementById('modeStatus');";
    html += "      modeStatus.innerText = '" + (String((mode == 0 ? "Heating" : (mode == 1 ? "Cooling" : "Off")))) + "';";
    html += "    }";
    html += "  };";
    html += "}";
    html += "function restart() {";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/restart', true);";
    html += "  xhr.send();";
    html += "  xhr.onload = function () {";
    html += "    if (xhr.status === 200) {";
    html += "      alert('Device is restarting.');";
    html += "    } else {";
    html += "      alert('Failed to restart the device.');";
    html += "    }";
    html += "  };";
    html += "}";
    html += "function reset() {";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  xhr.open('GET', '/reset', true);";
    html += "  xhr.send();";
    html += "  xhr.onload = function () {";
    html += "    if (xhr.status === 200) {";
    html += "      alert('Device is clearing WiFi Settings');";
    html += "    } else {";
    html += "      alert('Failed to reset.');";
    html += "    }";
    html += "  };";
    html += "}";
    html += "</script>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/increase", HTTP_GET, [](AsyncWebServerRequest *request) {  //comments comments so many comments....
    if (!request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    tempSet += 1.0;  // Increase the target temperature by 1 degree
    request->send(200, "text/plain", "Increased target temperature by 1 degree.");
  });

  server.on("/decrease", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    tempSet -= 1.0;  // Decrease the target temperature by 1 degree
    request->send(200, "text/plain", "Decreased target temperature by 1 degree.");
  });
  server.on("/fanSwitch", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (!request->authenticate(http_username, http_password)) {
    return request->requestAuthentication();
  }
  switchFan();
  request->send(200, "text/plain", "Fan state switched.");
  });
  server.on("/changeMode", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    changeMode();
    request->send(200, "text/plain", "Mode switched.");
    displayModeStatus(String((mode == 0 ? "Off" : (mode == 1 ? "Heating" : "Cooling"))));  // Update and display the new mode
  });
  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    ESP.restart();
  });
  server.on("/reset", HTTP_GET, [&wifiManager](AsyncWebServerRequest *request) {
    if (!request->authenticate(http_username, http_password)) {
      return request->requestAuthentication();
    }
    wifiManager.resetSettings();
    ESP.restart();
  });

  server.begin();

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN0), button0Pressed, FALLING);  // Interrupt to detect button press
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN1), button1Pressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_FAN), buttonfanPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_MODE), buttonmodePressed, FALLING);
}

void loop() {
  ArduinoOTA.handle();

  static bool previousFanState = fanisOn;
  static float previousTempSet = tempSet;  // Store the value of tempSet
  static unsigned long fanStartTime = 0;   // Variable to store the time when the fan relay was turned on, not currently used since i dont see a need for it yet.
  int tempC = temperatureRead();           // read ESP32C3 device temperature for display later
  

  // Read temperature and humidity and only execute this block every 3 seconds for DHT22 stability
  if (millis() - lastDHTPollTime > dhtPollInterval) {
  float temperatureC = dht.readTemperature();
  float temperatureF = celsiusToFahrenheit(temperatureC);  // converts C to F and assigns it to temperatureF because 'Merica
  float humidity = dht.readHumidity();
  lastDHTPollTime = millis();


  int rssi = WiFi.RSSI();  // Comments are exhausting.... I think you can figure this one out.
  Serial.print("Wi-Fi Signal Strength (RSSI): ");
  Serial.print(rssi);  // Not sure why i didnt just output the function itself though
  Serial.println(" dBm");

  // Check if any reads failed
  if (!isnan(temperatureC) && !isnan(humidity)) {  // Display the temperature in Fahrenheit on the OLED screen
    display.clearDisplay();                        // Clear the OLED first in case anything was left behind
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(F("Temp: "));
    display.print(temperatureF, 1);  // Display one decimal place
    display.setCursor(0, 30);
    display.print(F("H: "));
    display.print(humidity);
    display.print(F(" %"));
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("CPU: "));
    display.print(tempC);
    display.setCursor(56, 56);
    display.print(F("RSSI: "));
    display.print(rssi);
    display.print(F("dBm"));
    display.display();  // Show that sweet sweet information on the OLED

    unsigned long lastFurnaceOffTime = 0;  // Store the last time the furnace was turned off
    unsigned long furnaceDelay = 120000;   // Delay before turning the furnace back on (120,000 ms = 120 seconds)

    // Check if the mode status was updated
    if (millis() - modeStatusLastUpdated < 2000) {
      // Display the updated mode status
      displayModeStatus(String((mode == 0 ? "Off" : (mode == 1 ? "Heating" : "Cooling"))));  // Update and display the new mode
      display.display();
    }

    if (mode == 1) {                       // Heating mode
      if (temperatureF < (tempSet - 2)) {  // Turns on heating if the current sensor temp is 2 deg less than the target temp
        if (millis() - lastFurnaceOffTime >= furnaceDelay) {
          digitalWrite(RELAY_PIN, HIGH);
          heatingOn = true;
        }
      } else {
        if (temperatureF >= tempSet) {
          digitalWrite(RELAY_PIN, LOW);
          lastFurnaceOffTime = millis();
          heatingOn = false;
        }
      }
    } else if (mode == 2) {                // Cooling mode
      if (temperatureF > (tempSet + 3)) {  // Turns on cooling if the current sensor temp is 3 degrees more than the target temp
        digitalWrite(RELAY_PIN0, HIGH);
        coolingOn = true;
      } else {
        if (temperatureF <= tempSet) {
          digitalWrite(RELAY_PIN0, LOW);
          coolingOn = false;
        }
      }
    } else if (mode == 0) {
      digitalWrite(RELAY_PIN0, LOW);
      coolingOn = false;
      digitalWrite(RELAY_PIN, LOW);
      heatingOn = false;
    }
  } else {
    //Errror handling if DHT22 Sensor fails or any other logic fails
    display.clearDisplay();  // Clear the OLED first in case anything was left behind
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(F("Error: Failure"));
    display.display();
    Serial.println(F("Sum Ting Wong"));
    playTone(1000, 10000);
  }
  } // End of delayed DHT22 polling block
  if (tempSet != previousTempSet) {  
          Serial.print(F("Target Temperature: "));
          Serial.print(tempSet, 1);
          Serial.println(F(" °F"));

          displayTempSet(tempSet);
          previousTempSet = tempSet;
        }

  if (fanisOn != previousFanState) {
    displayFanStatus();
    previousFanState = fanisOn;
  }
  delay(500);
}