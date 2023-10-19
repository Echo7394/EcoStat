#include <Wire.h>
#include <DHT.h>
#include <Adafruit_SSD1306.h>
#include <esp_system.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin (if used)
#define SCREEN_ADDRESS 0x3C // I2C address for the OLED

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHT_PIN 8 
DHT dht(DHT_PIN, DHT22);

#define BUTTON_PIN0 2 // Button to raise tempSet
#define BUTTON_PIN1 3 // Button to lower tempSet

#define RELAY_PIN 10
#define RELAY_PIN1 20

const char* ssid = "WiFiSSIDHere";
const char* password = "WiFiPassHere";
IPAddress localIP(192, 168, 0, 201);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);

int tempSet = 60; // Initial target temperature in degrees Fahrenheit

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 1000;

float celsiusToFahrenheit(float celsius) { //Converts Celsius to Fahrenheit
  return (celsius * 9.0 / 5.0) + 32.0;
}

void displayTempSet(int tempSet) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.print(F("Target: "));
  display.print(tempSet); // Display one decimal place
  display.display();
}

void setup() {
 // Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT); // Set the relay pin as an output
  pinMode(RELAY_PIN1, OUTPUT); // Same with relay pin 1
  digitalWrite(RELAY_PIN, LOW); // Ensure the relay is initially off
  digitalWrite(RELAY_PIN1, LOW);
  pinMode(BUTTON_PIN0, INPUT_PULLUP); // Gotta use le pullup for buttons because reasons
  pinMode(BUTTON_PIN1, INPUT_PULLUP); // Makin buttons inputs to make it cooler!

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(3000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize the OLED screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // Clear the display
  display.clearDisplay();
  display.display(); // Display the cleared screen

  dht.begin();

  // Serve a webpage to control temperature
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    float temperatureC = dht.readTemperature();
    float temperatureF = celsiusToFahrenheit(temperatureC);

 String html = "<html><body style='text-align: center;font-size: 24px;background-color: #000;color: #00FFD3;'>";
  html += "<head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "</head>";
  html += "<h1 style='color: #00FFD3;text-shadow: -3px 3px 5px #5C5C5C;'>EcoNet Control</h1>";
  html += "<p>Current Temperature: <strong id='currentTemp'>" + String(temperatureF, 1) + " F</strong></p>";
  html += "<p>Target Temperature: <strong><span id='tempSet'>" + String(tempSet) + "</span> F</strong></p>";
  html += "<p><button onclick='increaseTemp()'>Temp +</button></p>";
  html += "<p><button onclick='decreaseTemp()'>Temp -</button></p>";
  html += "<p><button onclick='fanOn()'>Fan On</button></p>";
  html += "<p><button onclick='fanOff()'>Fan Off</button></p>";
  html += "<style>";
  html += "button { background-color: #222; color: #fff; border: 2px solid #444; border-radius: 20px; font-size: 24px; width: 50vw; }";
  html += "</style>";
  html += "<script>";
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
  html += "function fanOn() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/fanOn', true);";
  html += "  xhr.send();";
  html += "}";
  html += "function fanOff() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/fanOff', true);";
  html += "  xhr.send();";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  request->send(200, "text/html", html);

  });

  server.on("/increase", HTTP_GET, [](AsyncWebServerRequest *request) {
    tempSet += 1.0; // Increase the target temperature by 1 degree
    request->send(200, "text/plain", "Increased target temperature by 1 degree.");
  });

  server.on("/decrease", HTTP_GET, [](AsyncWebServerRequest *request) {
    tempSet -= 1.0; // Decrease the target temperature by 1 degree
    request->send(200, "text/plain", "Decreased target temperature by 1 degree.");
  });

  server.on("/fanOn", HTTP_GET, [](AsyncWebServerRequest *request) {
  digitalWrite(RELAY_PIN1, HIGH); // Turn on the fan relay
  request->send(200, "text/plain", "Fan is turned on.");
});

  server.on("/fanOff", HTTP_GET, [](AsyncWebServerRequest *request) {
  digitalWrite(RELAY_PIN1, LOW); // Turn off the fan relay
  request->send(200, "text/plain", "Fan is turned off.");
});
  server.begin();

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN0), button0Pressed, FALLING); // Interrupt to detect button press
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN1), button1Pressed, FALLING);
}

void button0Pressed() {
  if (millis() - lastDebounceTime > debounceDelay) {
    tempSet += 1; // Increase the target temperature by 1 degree
    lastDebounceTime = millis();
  }
}

void button1Pressed() {
  if (millis() - lastDebounceTime > debounceDelay) {
    tempSet -= 1; // Increase the target temperature by 1 degree
    lastDebounceTime = millis();
  }
}

void loop() {
  static float previousTempSet = tempSet; // Store the previous value of tempSet
  static unsigned long fanStartTime = 0; // Variable to store the time when the fan relay was turned on
  int tempC = temperatureRead(); // read device temperature

  // Read temperature and humidity
  float temperatureC = dht.readTemperature();
  float temperatureF = celsiusToFahrenheit(temperatureC);
  float humidity = dht.readHumidity();

  int rssi = WiFi.RSSI();
  Serial.print("Wi-Fi Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");

  // Check if any reads failed
  if (!isnan(temperatureC) && !isnan(humidity)) {
    // Display the temperature in Fahrenheit on the OLED screen
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(F("Temp: "));
    display.print(temperatureF, 1); // Display one decimal place
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
    display.display();

    unsigned long lastFurnaceOffTime = 0; // Store the last time the furnace was turned off
    unsigned long furnaceDelay = 120000;    // Delay before turning the furnace back on (60,000 ms = 60 seconds)

    // Control the relay based on the target temperature (tempSet)
    if (temperatureF < (tempSet - 2)) {
      // Check if the furnace has been off for the delay period
      if (millis() - lastFurnaceOffTime >= furnaceDelay) {
        digitalWrite(RELAY_PIN, HIGH); // Turn on controlling relay
      }
    } else {
      if (temperatureF >= tempSet) {
        digitalWrite(RELAY_PIN, LOW); // Turn off controlling relay
        lastFurnaceOffTime = millis();  // Update the time when the furnace was turned off
      }

      // Check if tempSet has changed
      if (temperatureF < (tempSet - 2) || tempSet != previousTempSet) {
        // Print target temperature to the serial monitor
        Serial.print(F("Target Temperature: "));
        Serial.print(tempSet, 1);
        Serial.println(F(" °F"));

        // Print temperature and humidity to the serial monitor
        Serial.print(F("Temp: "));
        Serial.print(temperatureF, 1);
        Serial.println(F(" °F"));
        Serial.print(F("Humidity: "));
        Serial.print(humidity);
        Serial.println(F(" %"));

        // Store the current value of tempSet as the previous value
        previousTempSet = tempSet;

        // Display the new tempSet on the OLED screen for 5 seconds
        displayTempSet(tempSet);
      }
    }
  } else {
    Serial.println(F("Sum Ting Wong"));
  }

  delay(2000); // Wait for 2 seconds before the next reading
  unsigned long displayStartTime = millis(); // Get the current time
}
