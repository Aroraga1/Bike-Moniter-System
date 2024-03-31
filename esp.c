#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <DHT.h>
#include <WiFi.h>
#include <ThingSpeak.h>

#define OLED_RESET    -1
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
TinyGPSPlus gps;
HardwareSerial GPS_Serial(1);
DHT dht(4, DHT11);

const int LDR_PIN = 5;
const int BUTTON_PIN = 8;
const int LED_PIN = 9;

int dataType = 0; // 0: DHT11, 1: LDR, 2: NEO-6M
bool dataChanged = false;

char ssid[] = "DESKTOP-BVD8MO5";
char password[] = "0987654321";
unsigned long channelID =  2250273; // Replace with your ThingSpeak channel ID
const char *writeAPIKey = "B3BMP9QU8C2Q82K5";

WiFiClient client;

void setup() {
  Serial.begin(115200);
  
  GPS_Serial.begin(9600, SERIAL_8N1, 18, 17);
  pinMode(LDR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  Wire.begin(6, 7);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Initialize Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  int ldrValue = analogRead(LDR_PIN);
  int buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW) {
    dataType = (dataType + 1) % 3;
    dataChanged = true;
    digitalWrite(LED_PIN, HIGH); // Turn on LED
    delay(1000); // LED blink duration
    digitalWrite(LED_PIN, LOW); // Turn off LED
    delay(1000); // Debounce delay
  }

  while (GPS_Serial.available() > 0) {
    char c = GPS_Serial.read();
    if (gps.encode(c)) {
      dataChanged = true;
    }
  }

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (dataChanged) {
    display.clearDisplay();

    if (dataType == 0) { // DHT11
      display.setCursor(0, 0);
      display.print(F("Temperature: "));
      display.print(temperature);
      display.println("C");

      display.setCursor(0, 16);
      display.print(F("Humidity: "));
      display.print(humidity);
      display.println("%");
    } else if (dataType == 1) { // LDR
      display.setCursor(0, 0);
      display.print("Light Intensity: ");
      display.println(ldrValue);
    } else if (dataType == 2) { // NEO-6M
      display.setCursor(0, 0);
      display.print("Latitude: ");
      display.println(gps.location.lat(), 6);

      display.setCursor(0, 16);
      display.print("Longitude: ");
      display.println(gps.location.lng(), 6);

      display.setCursor(0, 32);
      display.print("Altitude: ");
      display.println(gps.altitude.meters());
    }

    display.display();
    dataChanged = false;

    // Upload data to ThingSpeak
    uploadToThingSpeak(temperature, humidity, ldrValue, gps.location.lat(), gps.location.lng(), gps.altitude.meters());

    delay(5000); // Delay for 5 seconds before reading and uploading data again
  }
}

void uploadToThingSpeak(float temperature, float humidity, int ldrValue, float gpsLat, float gpsLng, float gpsAltitude) {
  ThingSpeak.begin(client);
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, ldrValue);

  ThingSpeak.setField(4, gpsLat);
  ThingSpeak.setField(5, gpsLng);
  ThingSpeak.setField(6, gpsAltitude);
  
  int status = ThingSpeak.writeFields(channelID, writeAPIKey);
  if (status == 200) {
    Serial.println("Data sent to ThingSpeak successfully.");
  } else {
    Serial.println("Error sending data to ThingSpeak. HTTP error code: " + String(status));
  }
}