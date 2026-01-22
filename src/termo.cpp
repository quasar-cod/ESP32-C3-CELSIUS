#include <Arduino.h>
#include <string>
#include <unordered_map>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include "time.h"
#include "miotime.h"
#include <U8g2lib.h>
#include <Wire.h>
#define SDA_PIN 5
#define SCL_PIN 6
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);   // EastRising 0.42" OLED
#define MY_NTP_SERVER "it.pool.ntp.org"
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"
#define ADDR  "celsius" 

// --- CONFIGURATION ---
const int ledPin = 8; // ESP32-C3 Built-in LED
String BOARD = "2";
int scanTime = 10; // Scan duration in seconds (Scan will restart automatically)

// const char* ssid = "TIM-39751438";// soggiorno
// const char* ssid = "TIM-39751438_TENDA";//tavernetta
const char* ssid = "TIM-39751438_EXT";
const char* password = "EFuPktKzk6utU2y5a5SEkUUQ";
const char* site = "http://myhomesmart.altervista.org/celsius/updatedata.php";

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
String lastTime;
int connecting_process_timed_out;
int Ndevices=0;

//************************************************************************************** */
void connect(){
  int r;
  for (r=1;r<10;r++){
    connecting_process_timed_out = 35;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid,password);
    Serial.println("***********************************************");
    Serial.println("Connecting to ");
    Serial.println(ssid);
    Serial.println("***********************************************");
    while (WiFi.status() != WL_CONNECTED & (connecting_process_timed_out > 0)){
      Serial.print(".");//3 flash 1.7 secondi
      digitalWrite(ledPin,LOW);
      delay(100);
      digitalWrite(ledPin,HIGH);
      delay(200);
      digitalWrite(ledPin,LOW);
      delay(100);
      digitalWrite(ledPin,HIGH);
      delay(200);
      digitalWrite(ledPin,LOW);
      delay(100);
      digitalWrite(ledPin,HIGH);
      delay(1000);
      connecting_process_timed_out--;
    }
    Serial.println("\n***********************************************");
    Serial.print("Successfully connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    String MAC = WiFi.macAddress();
    Serial.print("MAC Address: ");
    Serial.println(MAC);
    Serial.println("-------------");
    Serial.println("Abilito dns");
    if (MDNS.begin(ADDR)){
      Serial.println("Abilitato");
      break;
    }
    delay(r*60000);//ad ogni tentativo aumento il ritardo di un minuto
  }
  if (r==10) ESP.restart();
}
//************************************************************************************** */
void tmz(){
  // --> Here is the IMPORTANT ONE LINER needed in your sketch!
  // configTime(MY_TZ, MY_NTP_SERVER); //sulle esp8266 basta questa sola riga e le define
  configTime(0,0, MY_NTP_SERVER); //sulle ESP32 occorre separare in tre righe 
  setenv("TZ","CET-1CEST,M3.5.0/02,M10.5.0/03" ,1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
  Serial.println("***********************************************");
  Serial.println("NTP TZ DST - wait 1 minute");
  for (int i=0;i<44;i++){
    Serial.print(".");//2 flash 1.4 secondi
    digitalWrite(ledPin,LOW);
    delay(100);
    digitalWrite(ledPin,HIGH);
    delay(200);
    digitalWrite(ledPin,LOW);
    delay(100);
    digitalWrite(ledPin,HIGH);
    delay(1000);
  }
}
//************************************************************************************** */
void updatedata(bool isHeating) {
    if (WiFi.status() != WL_CONNECTED) return;
    Ndevices++;
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
    Serial.println("\nInitialized serial communications");
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
    connect();
    tmz();
    Serial.println("\nStarting Bluetooth...");
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyCallbacks(), false);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    lastTime=timeHM();
}

// void loop() {
//     struct tm timeinfo;
//     char tStr[10];
//     getLocalTime(&timeinfo);
//     strftime(tStr, sizeof(tStr), "%H:%M:%S", &timeinfo);
//     Serial.printf("\n--- SCAN START [%s] ---\n", tStr);
//     digitalWrite(ledPin, HIGH); // Turn LED ON during scan
//     NimBLEScanResults results = pBLEScan->start(10, false); // 10 second active scan
//     int found = 0;
//     for (int i = 0; i < results.getCount(); i++) {
//         if (MAC_NAMES.count(results.getDevice(i).getAddress().toString())) found++;
//     }
//     digitalWrite(ledPin, LOW); // Turn LED OFF after scan
//     Serial.printf("--- SCAN END. Devices: %d/%d ---\n", found, MAC_NAMES.size());
//     pBLEScan->clearResults(); 
//     delay(50000); // Wait 50s (Total cycle = 60s)
// }

void loop() {
  if (!pBLEScan->isScanning() && (timeHM() != lastTime)) {
    lastTime=timeHM();
    Serial.println("Scheduled: starting scan at: " + lastTime);
    Ndevices = 0;
    // clear seen devices for this new scan so duplicates from previous scans are allowed
    pBLEScan->clearResults(); 
    digitalWrite(ledPin,LOW);
    pBLEScan->start(scanTime, false);
    digitalWrite(ledPin,HIGH);
    currentSensor = "COUNT";
    currentBatt = 0;
    currentRSSI = "0";
    currentTemp=Ndevices;
    Serial.printf("Scan complete: %u devices found\n", Ndevices);
    if(WiFi.waitForConnectResult() == WL_CONNECTED)updatedata(false); 
    else connect();
  }
  u8g2.clearBuffer();
  u8g2.setCursor(0, 20);
  u8g2.print(timeHMS());
  u8g2.setCursor(0, 40);
  u8g2.print(Ndevices);
  u8g2.sendBuffer();
}