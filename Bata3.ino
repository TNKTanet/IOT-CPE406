#include "DHT.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>

#define DHTPIN 5          // ขาเชื่อมต่อกับ DHT22
#define DHTTYPE DHT22     // ประเภทเซ็นเซอร์ DHT

DHT dht(DHTPIN, DHTTYPE); // กำหนดอ็อบเจ็กต์เซ็นเซอร์ DHT

// กำหนด SSID และรหัสผ่านของ Wi-Fi
const char* ssid = "A55 ของ Tanet";       
const char* password = "88888888";  

// กำหนด URL สำหรับอัปเดตข้อมูลอุณหภูมิและความชื้น
const char* serverUrlTemp = "http://192.168.248.22:3001/updateTemperature"; 
const char* serverUrlHum = "http://192.168.248.22:3001/updateHumidity";

// กำหนดตัวแปรสำหรับสร้าง HTTP Server
AsyncWebServer server(80);

// ตัวแปรที่ใช้ตรวจสอบการเชื่อมต่อ
bool websiteConnected = false;

// ฟังก์ชันส่งข้อมูลไปยังเซิร์ฟเวอร์
void sendSensorData() {
  // อ่านค่าอุณหภูมิและความชื้นจากเซ็นเซอร์
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // ตรวจสอบการอ่านค่าจากเซ็นเซอร์
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // ส่งข้อมูลอุณหภูมิไปยังเซิร์ฟเวอร์
  HTTPClient http;
  http.begin(serverUrlTemp);
  http.addHeader("Content-Type", "application/json");
  String tempPayload = "{\"temperature\":" + String(temperature) + "}";  
  int tempResponseCode = http.POST(tempPayload);
  
  // ส่งข้อมูลความชื้นไปยังเซิร์ฟเวอร์
  http.begin(serverUrlHum);
  http.addHeader("Content-Type", "application/json");
  String humPayload = "{\"humidity\":" + String(humidity) + "}";  
  int humResponseCode = http.POST(humPayload);
  http.end();

  // ตรวจสอบการเชื่อมต่อกับเว็บไซต์
  if (tempResponseCode == 200 && humResponseCode == 200 && !websiteConnected) {
    Serial.println("Website connection successful");
    websiteConnected = true;  // แสดงข้อความแค่ครั้งเดียว
  } else if (tempResponseCode != 200 || humResponseCode != 200) {
    // หากไม่สามารถเชื่อมต่อได้
    if (!websiteConnected) {
      Serial.println("Website connection failed");
      websiteConnected = true;  // แสดงข้อความแค่ครั้งเดียว
    }
  }

  // แสดงข้อมูลอุณหภูมิและความชื้นทุกครั้ง
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C | Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
}

// หน้าเว็บแสดงข้อมูลอุณหภูมิและความชื้น
String htmlPage() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  // ตรวจสอบการอ่านค่าจากเซ็นเซอร์
  if (isnan(temperature) || isnan(humidity)) {
    return "Failed to read from DHT sensor!";
  }

  String page = "<html><body><h1>Sensor Data</h1>";
  page += "<p>Temperature: " + String(temperature) + " °C</p>";
  page += "<p>Humidity: " + String(humidity) + " %</p>";
  page += "</body></html>";
  
  return page;
}

void setup() {
  Serial.begin(115200);
  dht.begin(); // เริ่มต้นเซ็นเซอร์ DHT

  // เชื่อมต่อ Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // ตั้งค่าเซิร์ฟเวอร์ HTTP
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String page = htmlPage();
    request->send(200, "text/html", page);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // ส่งข้อมูลทุก 2 วินาที
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 2000) {
    lastSend = millis();
    sendSensorData();
  }
}
