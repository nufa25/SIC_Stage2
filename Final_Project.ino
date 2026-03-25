#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <DHT.h>
#include <PubSubClient.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define RED_PIN   25
#define GREEN_PIN 26
#define BLUE_PIN  27
#define BUZZER    14

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Wifi";
const char* password = "25mei2005oke";

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* topic_mqtt = "404NotFound/Unmul/sic";

const char* city = "Samarinda";
const char* apiKey = "95be204bd0ec0a72aefbb5d0f298c9d2";

WiFiClient espClient;
PubSubClient client(espClient);

// Koneksi WiFi
void setup_wifi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Callback
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message received from topic: ");
  Serial.println(topic);
  Serial.print("Message: ");

  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)message[i];
  }
  Serial.println(msg);

  // Respon ke pesan
  if (msg == "LED_RED") {
    digitalWrite(RED_PIN, LOW);    
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, HIGH);
  } else if (msg == "LED_GREEN") {
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, HIGH);
  } else if (msg == "LED_BLUE") {
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, LOW);
  } else if (msg == "BUZZER_ON") {
    tone(BUZZER, 1000);
  } else if (msg == "BUZZER_OFF") {
    noTone(BUZZER);
  }
}

// Koneksi MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "ESP32_" + WiFi.macAddress();
    clientId.replace(":", "");
    if (client.connect(clientId.c_str())) {
      Serial.println("Connect to broker.hivemq.com!");
      client.subscribe(topic_mqtt);
      Serial.print("Subscribe to the topic: ");
      Serial.println(topic_mqtt);
    } else {
      Serial.print("MQTT connection failed, rc=");
      Serial.print(client.state());
      Serial.println("Try Again...");
      delay(3000);
    }
  }
}

// --- Ambil suhu luar via API ---
float getOutdoorTemp() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=Samarinda&appid=95be204bd0ec0a72aefbb5d0f298c9d2&units=metric";
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      float temp = doc["main"]["temp"];
      http.end();
      return temp;
    } else {
      Serial.println("Failed to fetch API data!");
      http.end();
      return NAN;
    }
  }
  return NAN;
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Matikan semua LED (karena anode)
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);
  noTone(BUZZER);

  Wire.begin(23, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed to initialize!");
    while (true);
  }

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Tampilkan status awal
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Welcome to Monitoring System!");
  display.display();
  delay(1500);
}

// Loop utama
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float tempIncubator = dht.readTemperature();
  float tempOutdoor = getOutdoorTemp();

  if (isnan(tempIncubator) || isnan(tempOutdoor)) {
    Serial.println("Error reading sensor/API!");
    delay(2000);
    return;
  }

  Serial.printf("Incubator Temperature: %.2f°C | Outdoor Temperature: %.2f°C\n", tempIncubator, tempOutdoor);

  // --- OLED Display ---
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Egg Temperature Monitoring");

  display.setCursor(0, 15);
  display.print("Incubator: ");
  display.print(tempIncubator);
  display.println(" C");

  display.setCursor(0, 30);
  display.print("Outdoor: ");
  display.print(tempOutdoor);
  display.println(" C");

  // --- Logika LED dan buzzer ---
  if (tempIncubator < 36.0) {
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, HIGH);
    tone(BUZZER, 1000, 500);
    display.setCursor(0, 50);
    display.println("Status: To Cold!");
  } 
  else if (tempIncubator > 36.0 && tempIncubator < 37.0) {
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, LOW);
    noTone(BUZZER);
    display.setCursor(0, 50);
    display.println("Status: Suboptimal (a bit cooler)");
  } 
  else if (tempIncubator >= 37.0 && tempIncubator <= 38.0) {
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, LOW);
    noTone(BUZZER);
    display.setCursor(0, 50);
    display.println("Status: Ideal");
  } 
  else if (tempIncubator > 38.0 && tempIncubator < 39.0) {
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, LOW);
    noTone(BUZZER);
    display.setCursor(0, 50);
    display.println("Status: Suboptimal (a bit warmer)");
  } 
  else{
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);
    tone(BUZZER, 1000, 500);
    display.setCursor(0, 50);
    display.println("Status: To Warm!");
  }

  display.display();

  // Data ke broker MQTT
  char payload[150];
  snprintf(payload, sizeof(payload),
           "Incubator Temperature: %.2f°C | Outdoor Temperature: %.2f°C | Status: %s",
           tempIncubator, tempOutdoor,
           (tempIncubator < 36.0) ? "To Cold" :
           (tempIncubator >= 36.0 && tempIncubator < 37.0) ? "a bit cooler" :
           (tempIncubator >= 37.0 && tempIncubator <= 38.0) ? "Ideal" : 
           (tempIncubator >= 36.0 && tempIncubator < 37.0) ? "a bit cooler" : "To Warm");

  client.publish(topic_mqtt, payload);
  Serial.println("Data sent to MQTT broker!");
  delay(5000);
}
