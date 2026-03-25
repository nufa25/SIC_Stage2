#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
 
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
 
const char* ssid = "A15";
const char* password = "Delima28";
const char* city = "Samarinda";
const char* apiKey = "95be204bd0ec0a72aefbb5d0f298c9d2";
 
void setup() {
 Serial.begin(115200);
 dht.begin();
 
 pinMode(RED_PIN, OUTPUT);
 pinMode(GREEN_PIN, OUTPUT);
 pinMode(BLUE_PIN, OUTPUT);
 pinMode(BUZZER, OUTPUT);
 
 Wire.begin(23, 22);
 
 if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Gagal menampilkan layar!");
    while(true);
 }
 
 digitalWrite(RED_PIN, HIGH);
 digitalWrite(GREEN_PIN, HIGH);
 digitalWrite(BLUE_PIN, HIGH);
 
 Serial.print("Menghubungkan");
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
 }
 Serial.println("\Terhubung!");
}
 
//API
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
     Serial.println("Gagal ambil data suhu luar!");
     http.end();
     return NAN;
   }
 }
 return NAN;
}
 
void loop() {
 // float suhuInkubator = dht.readTemperature();
 float suhuInkubator = 37.0;
 float suhuLuar = getOutdoorTemp();
 
 if (isnan(suhuInkubator) || isnan(suhuLuar)) {
   Serial.println("Error: Tidak dapat membaca sensor/API!");
   delay(2000);
   return;
 }
 
 Serial.print("Suhu Inkubator: "); Serial.println(suhuInkubator);
 Serial.print("Suhu Luar: "); Serial.println(suhuLuar);
 
 //OLED
 display.clearDisplay();
 display.setTextSize(1);
 display.setTextColor(SSD1306_WHITE);
 display.setCursor(0,0);
 display.println("Monitoring Suhu Telur");
 
 display.setTextSize(1);
 display.setCursor(0,15);
 display.print("Inkubator: ");
 display.print(suhuInkubator);
 display.println(" C");
 
 display.setCursor(0,27);
 display.print("Luar: ");
 display.print(suhuLuar);
 display.println(" C");
 
 //LED & buzzer
 if (suhuInkubator < 36.0) {
   digitalWrite(RED_PIN, LOW);
   digitalWrite(GREEN_PIN, LOW);
   digitalWrite(BLUE_PIN, HIGH);
   tone(BUZZER, 1000, 500);
   display.setCursor(0, 39);
   display.println("Status: Non-optimal");
   display.setCursor(0, 51);
   display.println("Suhu Terlalu Rendah!");
 } 
 else if (suhuInkubator >= 36.0 && suhuInkubator <= 36.9) {
   digitalWrite(RED_PIN, HIGH);
   digitalWrite(GREEN_PIN, HIGH);
   digitalWrite(BLUE_PIN, LOW);
   noTone(BUZZER);
   display.setCursor(0, 39);
   display.println("Status: Suboptimal");
   display.setCursor(0, 51);
   display.println("Suhu Cukup Rendah");
 } 
 else if (suhuInkubator > 38.0 && suhuInkubator <= 39.0) {
   digitalWrite(RED_PIN, HIGH);
   digitalWrite(GREEN_PIN, HIGH);
   digitalWrite(BLUE_PIN, LOW);
   noTone(BUZZER);
   display.setCursor(0, 39);
   display.println("Status: Suboptimal");
   display.setCursor(0, 51);
   display.println("Suhu Cukup Tinggi");
 } 
 else if (suhuInkubator >= 39.0) {
   digitalWrite(RED_PIN, HIGH);
   digitalWrite(GREEN_PIN, LOW);
   digitalWrite(BLUE_PIN, LOW);
   tone(BUZZER, 1000, 500);
   display.setCursor(0, 39);
   display.println("Status: Non-optimal");
   display.setCursor(0, 51);
   display.println("Suhu Terlalu Tinggi!");
 }
 else {
   digitalWrite(RED_PIN, LOW);
   digitalWrite(GREEN_PIN, HIGH);
   digitalWrite(BLUE_PIN, LOW);
   noTone(BUZZER);
   display.setCursor(0, 39);
   display.println("Status: Optimal (Suhu Ideal)");
   display.setCursor(0, 51);
   display.println("Suhu Ideal");
 }
 
 display.display();
 delay(5000);
}
