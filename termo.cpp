#include <Arduino.h>
#include <string>
#include <unordered_map>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"
#include "miotime.h"                   // for time() ctime()
//dati per display
#include <U8g2lib.h>
#include <Wire.h>
#define SDA_PIN 5
#define SCL_PIN 6
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);   // EastRising 0.42" OLED


// --- CONFIGURATION ---
const int ledPin = 8; // ESP32-C3 Built-in LED
String BOARD = "2";

const char* ssid = "TIM-39751438_EXT";
const char* password = "EFuPktKzk6utU2y5a5SEkUUQ";
const char* site = "http://myhomesmart.altervista.org/celsius/updatedata.php";

#define MY_NTP_SERVER "it.pool.ntp.org"
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"

static const std::unordered_map<std::string, std::string> MAC_NAMES = {
    {"b0:e9:fe:8e:9c:2a", "SWBT05"},
    {"b0:e9:fe:cb:04:8d", "SWBT06"}
};

// --- GLOBALS ---
float currentTemp = NAN;
int currentBatt = 0;
String currentSensor = "";
String currentRSSI = "";
NimBLEScan* pBLEScan;

void updatedata(bool isHeating) {
    if (WiFi.status() != WL_CONNECTED) return;
    HTTPClient http;
    WiFiClient client;
    
    String postData = "board=" + BOARD + 
                      "&sensorName=" + currentSensor + 
                      "&temperature=" + String(currentTemp, 1) + 
                      "&humidity=" + String(isHeating ? 1 : 0) + 
                      "&battery=" + String(currentBatt) + 
                      "&RSSI=" + currentRSSI +
                      "&time=" + timeHMS() +
                      "&date=" + dateYMD();

    Serial.println(postData);
    http.begin(client, site);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(postData);
    String payload = http.getString();  //--> Get the response payload
    Serial.print("httpCode : ");
    Serial.println(httpCode); //--> Print HTTP return code
    Serial.print("payload  : ");
    Serial.println(payload);  //--> Print request response payload
    Serial.println("-------------");
    http.end();
}

static void decodeTRV(uint8_t* payload, size_t pLen, std::string addr) {
    // Only decode long data-rich packets (Manufacturer Data)
    if (pLen >= 27) {
        currentTemp = (float)payload[16] - 128.0f;
        float targetTemp = (float)payload[17] - 128.0f;
        
        // Battery at Index 26 (0x64 = 100%)
        currentBatt = payload[26];
        if (currentBatt > 100) currentBatt = 100;

        bool isHeating = (targetTemp > currentTemp);
        currentSensor = MAC_NAMES.at(addr).c_str();

        Serial.printf("[%s] Amb: %.1f | Trg: %.1f | Bat: %d%% | Heat: %s\n", 
                      currentSensor.c_str(), currentTemp, targetTemp, currentBatt, isHeating ? "YES" : "NO");
        
        updatedata(isHeating);
    }
}

class MyCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) override {
        std::string addr = advertisedDevice->getAddress().toString();
        if (MAC_NAMES.find(addr) != MAC_NAMES.end()) {
            currentRSSI = String(advertisedDevice->getRSSI());
            decodeTRV(advertisedDevice->getPayload(), advertisedDevice->getPayloadLength(), addr);
        }
    }
};

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW); // LED off initially
//dati per display
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.setFont(u8g2_font_timR14_tf);
  u8g2.clearBuffer();
  u8g2.setCursor(0,15);
  u8g2.print("Starting");
  u8g2.setCursor(0,40);
  u8g2.print("wait 1min");
  u8g2.sendBuffer();

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    
    configTzTime(MY_TZ, MY_NTP_SERVER);
    Serial.println("\nTime Synced. Starting Bluetooth...");

    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyCallbacks(), false);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void loop() {
    struct tm timeinfo;
    char tStr[10];
    getLocalTime(&timeinfo);
    strftime(tStr, sizeof(tStr), "%H:%M:%S", &timeinfo);

    Serial.printf("\n--- SCAN START [%s] ---\n", tStr);
    digitalWrite(ledPin, HIGH); // Turn LED ON during scan

    NimBLEScanResults results = pBLEScan->start(10, false); // 10 second active scan
    
    int found = 0;
    for (int i = 0; i < results.getCount(); i++) {
        if (MAC_NAMES.count(results.getDevice(i).getAddress().toString())) found++;
    }

    digitalWrite(ledPin, LOW); // Turn LED OFF after scan
    Serial.printf("--- SCAN END. Devices: %d/%d ---\n", found, MAC_NAMES.size());

    pBLEScan->clearResults(); 
    delay(50000); // Wait 50s (Total cycle = 60s)
}