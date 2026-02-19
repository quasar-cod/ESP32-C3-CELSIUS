#include <Arduino.h>
#include <string>
#include <NimBLEDevice.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "time.h"
#include "miotime.h"

#define ADDR  "celsius" 
#define SDA_PIN 5
#define SCL_PIN 6
#define MY_NTP_SERVER "it.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   
#define SCAN_TIME_S 10
#define SCAN_INTERVAL_MS 60000UL
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);   // EastRising 0.42" OLED

int Ndevices=0;
String BOARD = "3";
int i,j;
const int ledPin = 8; //logica invertita
int pt = 1;
String sensorName = "";
float temperature = NAN;
float humidity = NAN;
int battery = -1;
String RSSI = "";

// const char* ssid = "TIM-24326654";// soggiorno
// const char* ssid = "TIM-24326654_TENDA";//tavernetta
const char* ssid = "TIM-24326654_EXT";// notte
const char* password = "T9ZDHXACUfdTUC33DcTCASsz";
const char* site = "http://myhomesmart.altervista.org/";
//const char* site = "http://hp-i3/tappa/";

int connecting_process_timed_out;

int scanTime = 10; // Scan duration in seconds (Scan will restart automatically)
NimBLEScan* pBLEScan;
String lastTime;
// Track devices seen during the current scan to avoid duplicate prints
static std::unordered_set<std::string> seenDevices;

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
    time_t now = time(nullptr);
  int ntpRetry = 0;
  // 1738713600 = Thursday, February 5, 2026 00:00:00
  while (now < 1738713600 && ntpRetry < 100) { 
    delay(1000); // Small delay to let the UDP packet arrive
    now = time(nullptr); 
    ntpRetry++;
    digitalWrite(ledPin,LOW);
    delay(100);
    digitalWrite(ledPin,HIGH);
    delay(200);
    digitalWrite(ledPin,LOW);
    delay(100);
    digitalWrite(ledPin,HIGH);
  }
  if (now < 1738713600) {
    ESP.restart();
  }
}
//************************************************************************************** */

void updatedata(bool isHeatingStatus = false, bool isTRV = false){
  HTTPClient http;
  WiFiClient client;
  int httpCode;
  String postData = "";
  
  // Decide what to send in the humidity field
  // If it's a TRV, we send 1 or 0. Otherwise, we send the sensor humidity.
  String humidityValue = isTRV ? String(isHeatingStatus ? 1 : 0) : String(humidity);

  Serial.println("Invio dati a updatedata.php");
  postData = "board=" + BOARD;
  postData += "&sensorName=" + sensorName;
  postData += "&temperature=" + String(temperature);
  postData += "&humidity=" + humidityValue; // This now handles both types
  postData += "&battery=" + String(battery);
  postData += "&RSSI=" + RSSI;
  postData += "&time=" + timeHMS();
  postData += "&date=" + dateYMD();

  Serial.print("postData: ");
  Serial.println(postData);

  http.begin(client, "http://myhomesmart.altervista.org/celsius/updatedata.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  httpCode = http.POST(postData);
  
  if(httpCode != 200) {
    // Retry logic with board suffix "B"
    postData = "board=" + BOARD + "B";
    postData += "&sensorName=" + sensorName;
    postData += "&temperature=" + String(temperature);
    postData += "&humidity=" + humidityValue;
    postData += "&battery=" + String(battery);
    postData += "&RSSI=" + RSSI;
    postData += "&time=" + timeHMS();
    postData += "&date=" + dateYMD();
    httpCode = http.POST(postData);
  }

  Serial.printf("httpCode: %d | Response: %s\n", httpCode, http.getString().c_str());
  Serial.println("-------------");
  http.end();
}

// Map MAC (lowercase, colon-separated) to human-friendly sensor name
static const std::unordered_map<std::string, std::string> MAC_NAMES = {
    {"f7:86:17:6f:ad:57", "SWBT01"},
    {"f1:39:38:e5:68:0a", "SWBT02"},
    {"c0:23:17:1f:65:4f", "SWBT03"},
    {"ca:c8:11:8d:e2:c6", "SWBT04"},
    {"b0:e9:fe:8e:9c:2a", "SWBT05"},
    {"b0:e9:fe:cb:04:8d", "SWBT06"}
  };

static bool decodeFD3D(const std::string &advLower, uint8_t* sdata, size_t sLen, uint8_t* mdata, size_t mLen, size_t payloadLen, uint8_t* fullPayload) {
    auto inRange = [](float v){ return v > -40.0f && v < 85.0f; };
    
    // --- TIPO 1: MINI (26) ---
    if (payloadLen == 26) {
        if (sLen >= 3) battery = sdata[2];
        if (mdata != nullptr && mLen >= 13) {
            int temp_int = (int)mdata[11] - 128;
            float temp_frac = ((int)mdata[10]) / 10.0f;
            temperature = (temp_int >= 0) ? (float)temp_int + temp_frac : (float)temp_int - temp_frac;
            humidity = (float)(mdata[12] & 0x7F);
            Serial.print("TIPO 1 MINI: ");
            Serial.println(sensorName);
            updatedata(); // Uses default params: isHeatingStatus=false, isTRV=false
            Ndevices++;
            return true;
        }
    } 
    // --- TIPO 2: DISPLAY (28) ---
    else if (payloadLen == 28) {
        if (sLen >= 3) battery = sdata[2];
        if (sLen >= 5) {
            int temp_int = ((int)sdata[4]) - 128;
            float temp_frac = ((float)sdata[3]) / 10.0f;
            temperature = (temp_int >= 0) ? (float)temp_int + temp_frac : (float)temp_int - temp_frac;
            if (sLen >= 6) humidity = (float)sdata[5];
            Serial.print("TIPO 2 DISPLAY: ");
            Serial.println(sensorName);
            updatedata(); // Uses default params
            Ndevices++;
            return true;
        }
    }
    // --- TIPO 3: TERMO (31) ---
    else if (payloadLen == 31) {
        if (fullPayload != nullptr) {
            temperature = (float)fullPayload[16] - 128.0f;
            float targetTemp = (float)fullPayload[17] - 128.0f;
            battery = fullPayload[26];
            if (battery > 100) battery = 100;
            bool isHeating = (targetTemp > temperature);
            Serial.print("TIPO 3 TERMO: ");
            Serial.println(sensorName);
            // PASS FLAGS: isHeatingStatus = isHeating, isTRV = true
            updatedata(isHeating, true); 
            Ndevices++;
            return true;
        }
    }
    Serial.printf("payloadLen: %d \n",payloadLen);
    return false;
}

class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
public:
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) override {
        if (!advertisedDevice) return;

        std::string advAddr = advertisedDevice->getAddress().toString();
        auto toLower = [](std::string s){ for (auto &c: s) c = tolower(c); return s; };
        std::string advLower = toLower(advAddr);

        if (MAC_NAMES.find(advLower) == MAC_NAMES.end()) return;
        if (seenDevices.find(advLower) != seenDevices.end()) return;
        seenDevices.insert(advLower);

        uint8_t* payload = advertisedDevice->getPayload();
        size_t payloadLen = advertisedDevice->getPayloadLength();
        if (!payload || payloadLen == 0) return;

        sensorName = MAC_NAMES.at(advLower).c_str();
        RSSI = advertisedDevice->getRSSI();

        uint8_t *sdata = nullptr, *mdata = nullptr;
        size_t sLen = 0, mLen = 0;
        size_t idx = 0;

        // One pass to find segments
        while (idx + 1 < payloadLen) {
            uint8_t len = payload[idx];
            if (len == 0 || idx + len >= payloadLen) break;
            uint8_t type = payload[idx + 1];

            if (type == 0x16) { // Service Data
                sdata = payload + idx + 4;
                sLen = len - 3;
            } else if (type == 0xFF) { // Manufacturer Data
                mdata = payload + idx + 2;
                mLen = len - 1;
            }
            idx += (size_t)len + 1;
        }

        // Pass everything to the unified decoder
        decodeFD3D(advLower, sdata, sLen, mdata, mLen, payloadLen, payload);
    }
};

AdvertisedDeviceCallbacks advCallbacks;
// ----------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("\nInitialized serial communications");
  pinMode(ledPin, OUTPUT);
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
  // Initialize the BLE stack
  NimBLEDevice::init("");
  pBLEScan = NimBLEDevice::getScan();
  // Assign the advertised device callback handler
  pBLEScan->setAdvertisedDeviceCallbacks(&advCallbacks, true);
  // Configure scan settings
  // Use active scan to request scan responses and improve discovery reliability
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(100);
  Serial.printf("\nSetting scan for %d seconds...\n", scanTime);
  lastTime=timeHM();
  Serial.printf("Time %s", lastTime);
}

void loop() {
  if (!pBLEScan->isScanning() && (timeHM() != lastTime)) {
    lastTime=timeHM();
    Serial.println("Scheduled: starting scan at: " + lastTime);
    Serial.println("§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§");
    // clear seen devices for this new scan so duplicates from previous scans are allowed
    Ndevices = 0;
    seenDevices.clear();
    digitalWrite(ledPin,LOW);
    pBLEScan->start(scanTime, false);
    digitalWrite(ledPin,HIGH);
    //preparo i dati per la funzione updatedata()
    sensorName = "COUNT";
    humidity = 0;
    battery = 0;
    RSSI = "0";
    temperature=Ndevices;
    Serial.printf("Scan complete: %u devices found\n", Ndevices);
    if(WiFi.waitForConnectResult() == WL_CONNECTED)updatedata(); 
    else connect();
  }
  u8g2.clearBuffer();
  u8g2.setCursor(0, 20);
  u8g2.print(timeHMS());
  u8g2.setCursor(0, 40);
  u8g2.print(Ndevices);
  u8g2.sendBuffer();
}