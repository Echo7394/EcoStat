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

#define DHT_PIN 8    // Settings the data pin for the DHT22
DHT dht(DHT_PIN, DHT22);

#define BUTTON_PIN0 3 // Button to lower tempSet
#define BUTTON_PIN1 2 // Button to raise tempSet

#define RELAY_PIN 10 // Establishing the trigger pin for the "Heat" relay
#define RELAY_PIN1 20 // "Fan" Relay trigger pin

const char* ssid = "http_username";				 // Change to desired Wi-Fi SSID
const char* password = "http_password"; // Change to desired Wi-Fi Password
IPAddress localIP(192, 168, 0, 201);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80); // Web server to port 80 TCP

int tempSet = 60; // Initial target temperature in degrees Fahrenheit
const char* http_username = ""; // Change to your desired username
const char* http_password = "13371337"; // Change to your desired password

unsigned long lastDebounceTime = 0; // Momentary switches like to be bouncy
unsigned long debounceDelay = 1000; // These variables will take care of that later

bool heatingOn = false; // heating status, starts as false since RELAY_PIN typically starts LOW
bool fanisOn = false; // same as above but for.... the fan relay

float celsiusToFahrenheit(float celsius) { //Converts Celsius to Fahrenheit
  return (celsius * 9.0 / 5.0) + 32.0;
}

void displayTempSet(int tempSet) { // This function gets flashed briefly on the OLED
  display.clearDisplay();          // when the target temp "tempSet" is changed
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.print(F("Target: "));
  display.print(tempSet);
  display.display();
}

void setup() {
 // Serial.begin(115200);      // uncomment this if you need serial output for debugging
  pinMode(RELAY_PIN, OUTPUT); 
  pinMode(RELAY_PIN1, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Ensure the relay is initially off
  digitalWrite(RELAY_PIN1, LOW);
  pinMode(BUTTON_PIN0, INPUT_PULLUP); // Gotta use a pullup for buttons because otherwise
  pinMode(BUTTON_PIN1, INPUT_PULLUP); // Erwins Cats are going to get involved

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

  display.clearDisplay(); // This just ensures that we start with a clear display
  display.display();      // on the SSD1306 screen

  dht.begin();

  // Serve a webpage to control features remotely
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (!request->authenticate(http_username, http_password)) {
    return request->requestAuthentication();
  }
    float temperatureC = dht.readTemperature();
    float temperatureF = celsiusToFahrenheit(temperatureC);
    String heatingStatus = heatingOn ? "On" : "Off"; // Determine the state of heatingOn and assign it to heating Status with a variable string
    String fanStatus = fanisOn ? "On" : "Off";       // Same thing but you know... the fan

    String html = "<html><body style='text-align: center;font-size: 24px;background-color: #000;color: #00FFD3;'>";
      html += "<head>";
      html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
      html += "</head>";
      html += "<h1 style='color: #00FFD3;text-shadow: -3px 3px 5px #5C5C5C;'>EcoNet Control</h1>";
      html += "<p>Current Temperature: <strong id='currentTemp'>" + String(temperatureF, 1) + " F</strong></p>";
      html += "<p>Target Temperature: <strong><span id='tempSet'>" + String(tempSet) + "</span> F</strong></p>";
      html += "<p>Heating: <strong id='heatingStatus'>" + heatingStatus + "</strong></p>"; // Display heating status
      html += "<p>Fan: <strong id='fanStatus'>" + fanStatus + "</strong></p>"; // Display fan status
      html += "<p><button onclick='increaseTemp()'>Temp +</button></p>";
      html += "<p><button onclick='decreaseTemp()'>Temp -</button></p>";
      html += "<p><button onclick='fanOn()'>Fan On</button></p>";
      html += "<p><button onclick='fanOff()'>Fan Off</button></p>";
      html += "<style>";
      html += "button { background-color: #222; color: #fff; border: 2px solid #444; border-radius: 20px; font-size: 24px; width: 50vw; }";
      html += "</style>";
      html += "<script>";                                             // XML is foreign to me so ill be honest 
      html += "function increaseTemp() {";                            // ChatGPT helped me with this part
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

  server.on("/increase", HTTP_GET, [](AsyncWebServerRequest *request) { //comments comments so many comments....
	  if (!request->authenticate(http_username, http_password)) {
    return request->requestAuthentication();
  }
    tempSet += 1.0; // Increase the target temperature by 1 degree
    request->send(200, "text/plain", "Increased target temperature by 1 degree.");
  });

  server.on("/decrease", HTTP_GET, [](AsyncWebServerRequest *request) {
	  if (!request->authenticate(http_username, http_password)) {
    return request->requestAuthentication();
  }
    tempSet -= 1.0; // Decrease the target temperature by 1 degree
    request->send(200, "text/plain", "Decreased target temperature by 1 degree.");
  });

  server.on("/fanOn", HTTP_GET, [](AsyncWebServerRequest *request) {
	  if (!request->authenticate(http_username, http_password)) {
    return request->requestAuthentication();
  }
  digitalWrite(RELAY_PIN1, HIGH); // Turn on the fan relay
  fanisOn = true;
  request->send(200, "text/plain", "Fan is turned on.");
});

  server.on("/fanOff", HTTP_GET, [](AsyncWebServerRequest *request) {
	  if (!request->authenticate(http_username, http_password)) {
    return request->requestAuthentication();
  }
  digitalWrite(RELAY_PIN1, LOW); // Turn off the fan relay
  fanisOn = false;
  request->send(200, "text/plain", "Fan is turned off.");
});
  server.begin();

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN0), button0Pressed, FALLING); // Interrupt to detect button press
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN1), button1Pressed, FALLING);
}

void button0Pressed() {
  if (millis() - lastDebounceTime > debounceDelay) { // Fancy debounce stuff to make sure a button press = just 1 press
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
  static unsigned long fanStartTime = 0; // Variable to store the time when the fan relay was turned on, not currently used since i dont see a need for it yet.
  int tempC = temperatureRead(); // read ESP32C3 device temperature for display later

  // Read temperature and humidity
  float temperatureC = dht.readTemperature();
  float temperatureF = celsiusToFahrenheit(temperatureC); // converts C to F and assigns it to temperatureF because 'Merica
  float humidity = dht.readHumidity();

  int rssi = WiFi.RSSI(); 								 // Comments are exhausting.... I think you can figure this one out.
  Serial.print("Wi-Fi Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");

  // Check if any reads failed
  if (!isnan(temperatureC) && !isnan(humidity)) {  // Display the temperature in Fahrenheit on the OLED screen
    display.clearDisplay(); // Clear the OLED first in case anything was left behind
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
    display.display(); // Show that sweet sweet information on the OLED

    unsigned long lastFurnaceOffTime = 0; // Store the last time the furnace was turned off
    unsigned long furnaceDelay = 120000;    // Delay before turning the furnace back on (60,000 ms = 60 seconds)

    // Control the relay based on the target temperature (tempSet)
    if (temperatureF < (tempSet - 2)) {
      // Check if the furnace has been off for the delay period
      if (millis() - lastFurnaceOffTime >= furnaceDelay) {
        digitalWrite(RELAY_PIN, HIGH); // Turn on heat controlling relay
        heatingOn = true; // Setting the boolean heatingOn to true so other parts of the code know that RELAY_PIN is HIGH
		  				  // is there an easier way to do this?? Absolutely, but i refuse
      }
    } else {
      if (temperatureF >= tempSet) {
        digitalWrite(RELAY_PIN, LOW); // Turn off heat controlling relay
        lastFurnaceOffTime = millis();  // Update the time when the furnace was turned off
        heatingOn = false; // Setting the boolean heatingOn to false so other parts of the code know that RELAY_PIN is LOW
      }

      // Check if tempSet has changed
      if (temperatureF < (tempSet - 2) || tempSet != previousTempSet) { // Math is torture
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
}