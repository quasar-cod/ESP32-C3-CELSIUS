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
String BOARD = "2";
int i,j;
const int ledPin = 8; //logica invertita
int pt = 1;
String sensorName = "";
float temperature = NAN;
float humidity = NAN;
int battery = -1;
String RSSI = "";

// const char* ssid = "TIM-39751438";// soggiorno
// const char* ssid = "TIM-39751438_TENDA";//tavernetta
const char* ssid = "TIM-39751438_EXT";// notte
const char* password = "EFuPktKzk6utU2y5a5SEkUUQ";
const char* site = "http://myhomesmart.altervista.org/";
//const char* site = "http://hp-i3/tappa/";

int connecting_process_timed_out;

// Use the keys of MAC_NAMES as the implicit whitelist (lowercase, colon-separated MACs)

// Map MAC (lowercase, colon-separated) to human-friendly sensor name
static const std::unordered_map<std::string, std::string> MAC_NAMES = {
    {"f7:86:17:6f:ad:57", "SWBT01"},
    {"f1:39:38:e5:68:0a", "SWBT02"},
    {"c0:23:17:1f:65:4f", "SWBT03"},
    {"ca:c8:11:8d:e2:c6", "SWBT04"}
};

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
void updatedata(){
  HTTPClient http;
  WiFiClient client;
  int httpCode;     //--> Variables for HTTP return code.
  String postData = ""; //--> Variables sent for HTTP POST request data.
  String payload = "";  //--> Variable for receiving response from HTTP POST.

  Serial.println("updatedata.php");
  postData = "board=" + BOARD;
  postData += "&sensorName=" + sensorName;
  postData += "&temperature=" + String(temperature);
  postData += "&humidity=" + String(humidity);
  postData += "&battery=" + String(battery);
  postData += "&RSSI=" + RSSI;
  postData += "&time=" + timeHMS();
  postData += "&date=" + dateYMD();
  Serial.println(postData);  //--> Print request response payload
  payload = "";
// http.begin(client,"http://hp-i3/celsius/updatedata.php");  //--> Specify request destination
  http.begin(client,"http://myhomesmart.altervista.org/celsius/updatedata.php");  //--> Specify request destination
  //NB va usato http e non https
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");  //--> Specify content-type header   
  httpCode = http.POST(postData); //--> Send the request
  if(httpCode != 200) {
    Serial.println("REupdatedata.php");
    postData = "board=" + BOARD+"B";
    postData += "&sensorName=" + sensorName;
    postData += "&temperature=" + String(temperature);
    postData += "&humidity=" + String(humidity);
    postData += "&battery=" + String(battery);
    postData += "&RSSI=" + RSSI;
    postData += "&time=" + timeHMS();
    postData += "&date=" + dateYMD();
    http.POST(postData);
  }
  payload = http.getString();  //--> Get the response payload
  Serial.print("httpCode : ");
  Serial.println(httpCode); //--> Print HTTP return code
  Serial.print("payload  : ");
  Serial.println(payload);  //--> Print request response payload
  Serial.println("-------------");
  http.end();  //Close connection
}

// ----------------------------------------------------------------------
// 1. DATA CALLBACKS (AdvertisedDeviceCallbacks):
//    Handles processing data when a device is found.
// ----------------------------------------------------------------------

// Decode SwitchBot FD3D service data. Returns true if decoded and printed.
static bool decodeFD3D(const std::string &advLower, uint8_t* sdata, size_t sLen, uint8_t* mdata=nullptr, size_t mLen=0, bool useAlt=false) {
    if (sLen >= 3) battery = sdata[2];
    auto inRange = [](float v){ return v > -40.0f && v < 85.0f; };
    if (useAlt) {
        // Use manufacturer-data mapping observed in captures (PayloadLen==26):
        // mdata layout: [0..1]=company id, [2..7]=MAC, [8]=battery, [9]=?, [10]=temp_frac, [11]=temp_int+128, [12]=humidity|flags
        if (mdata != nullptr && mLen >= 13) {
            // int8_t raw_b = (int8_t)mdata[8];
            // int m_battery = raw_b < 0 ? -raw_b : (int)raw_b; // some captures encode as signed byte (0xC5 == -59)
            // if (m_battery < 0) m_battery = 0;
            // if (m_battery > 100) m_battery = 100;
            int temp_int = (int)mdata[11] - 128;
            float temp_frac = ((int)mdata[10]) / 10.0f;
            float t = (temp_int >= 0) ? (float)temp_int + temp_frac : (float)temp_int - temp_frac;
            float h = (float)(mdata[12] & 0x7F);
            if (inRange(t)) temperature = t;
            humidity = (h >= 0.0f && h <= 100.0f) ? h : NAN;
            Serial.printf("  Battery: %d%%\n", battery);
            // Serial.printf("  m_Battery: %d%%\n", m_battery);
            Serial.printf("  Temperature: %.1f°C\n", temperature);
            Serial.printf("  Humidity: %.0f%%\n", humidity);
            Serial.println("TIPO 1 MINI");
            Serial.println("------------------------------------");
            updatedata();
            Ndevices++;
            return true;
        }
        // Fallback to previous candidates if manufacturer-data not present
        Serial.println("  Manufacturer-data not present or too short; falling back to candidates");
        if (sLen >= 2) {
            uint16_t u16_le = (uint16_t)sdata[0] | ((uint16_t)sdata[1] << 8);
            float cand_le_div10 = (float)u16_le / 10.0f;
            if (inRange(cand_le_div10)) temperature = cand_le_div10;
        }
        Serial.printf("  Battery (raw): %d\n", (sLen >= 3) ? (int)sdata[2] : -1);
        if (!isnan(temperature)) Serial.printf("  Chosen Temperature: %.2f°C\n", temperature);
        else Serial.println("  Chosen Temperature: <unknown>");
        Serial.println("------------------------------------");
        return true;
    }
    if (sLen >= 5) {
        int temp_int = ((int)sdata[4]) - 128;
        float temp_frac = ((float)sdata[3]) / 10.0f;
        if (temp_int >= 0) temperature = (float)temp_int + temp_frac;
        else temperature = (float)temp_int - temp_frac;
        if (!inRange(temperature)) {
            int16_t t_be = (int16_t)((sdata[3] << 8) | sdata[4]);
            float t_be_f = (float)t_be / 10.0f;
            int8_t t_i8 = (int8_t)sdata[3];
            float t_i8_f = (float)t_i8;
            if (inRange(t_be_f)) temperature = t_be_f;
            else if (inRange(t_i8_f)) temperature = t_i8_f;
        }
    } else if (sLen >= 4) {
        int8_t t_i8 = (int8_t)sdata[3];
        temperature = (float)t_i8;
    }
    if (sLen >= 6) humidity = (float)sdata[5];
    Serial.printf("  Battery: %d%%\n", (battery >= 0) ? battery : -1);
    if (!isnan(temperature)) Serial.printf("  Temperature: %.1f°C\n", temperature);
    else Serial.println("  Temperature: <unknown>");
    if (!isnan(humidity)) Serial.printf("  Humidity: %.1f%%\n", humidity);
    else Serial.println("  Humidity: <unknown>");
    Serial.println("TIPO 2 DISPLAY");
    Serial.println("------------------------------------");
    updatedata();
    Ndevices++;
    return true;
}

class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
public:
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) override {
        if (advertisedDevice == nullptr) return;
        // 1. Filter by MAC Address (case-insensitive string compare)
        std::string advAddr = advertisedDevice->getAddress().toString();
        auto toLower = [](std::string s){ for (auto &c: s) c = tolower(c); return s; };
        std::string advLower = toLower(advAddr);
        // Only process MACs present in MAC_NAMES (acts as the whitelist)
        if (MAC_NAMES.find(advLower) == MAC_NAMES.end()) return;
        // Deduplicate within a single scan: skip if we've already processed this MAC
        if (seenDevices.find(advLower) != seenDevices.end()) return;
        seenDevices.insert(advLower);
        // Debug: print summary for every (allowed) discovered device
        size_t payloadLenDbg = advertisedDevice->getPayloadLength();
        std::string nameDbg = advertisedDevice->getName();
        uint8_t* payloadDbg = advertisedDevice->getPayload();
        Serial.println("Found device!!!!!!");
        if (payloadDbg != nullptr && payloadLenDbg > 0) {
            Serial.print("  Payload: ");
            for (size_t i = 0; i < payloadLenDbg && i < 32; ++i) Serial.printf("%02X ", payloadDbg[i]);
            Serial.println();
        } else {
            Serial.println("  Payload: <none>");
            return;
        }
        RSSI = advertisedDevice->getRSSI();
        // Prefer configured friendly name for this MAC, fall back to advertised name
        if (MAC_NAMES.find(advLower) != MAC_NAMES.end()) sensorName = MAC_NAMES.at(advLower).c_str();
        else sensorName = advertisedDevice->getName().c_str();
        Serial.printf("  PayloadLen: %u\n",(unsigned)payloadLenDbg);
        Serial.printf("  Name: %s\n", sensorName.c_str());
        Serial.printf("  MAC: %s\n", advertisedDevice->getAddress().toString().c_str());
        Serial.printf("  RSSI: %d dBm\n", advertisedDevice->getRSSI());
        uint8_t* payload = advertisedDevice->getPayload();
        size_t payloadLen = advertisedDevice->getPayloadLength();
        // Parse AD structures in the payload to find Service Data (type 0x16)
        // looking for UUID 0xFD3D (little-endian bytes: 0x3D 0xFD).
        size_t idx = 0;
        bool decoded = false;
        while (idx + 1 < payloadLen) {
            uint8_t len = payload[idx];
            if (len == 0) break;
            if (idx + len >= payloadLen) break;
            uint8_t type = payload[idx + 1];
            if (type == 0x16 && len >= 3) { // Service Data - 16-bit UUID
                // UUID is at idx+2 (2 bytes, little-endian)
                uint16_t uuid = payload[idx + 2] | (payload[idx + 3] << 8);
                if (uuid == 0xFD3D) {
                    // service data payload starts at idx+4, length = len-3
                    uint8_t* sdata = payload + idx + 4;
                    size_t sLen = len - 3;
                    // find manufacturer data (type 0xFF) in the AD payload so we can pass it to decoder
                    uint8_t* mdata = nullptr; 
                    size_t mLen = 0;
                    size_t idx2 = 0;
                    while (idx2 + 1 < payloadLen) {
                        uint8_t l2 = payload[idx2];
                        if (l2 == 0) break;
                        if (idx2 + l2 >= payloadLen) break;
                        uint8_t t2 = payload[idx2 + 1];
                        if (t2 == 0xFF && l2 >= 2) { // Manufacturer Specific
                            mdata = payload + idx2 + 2;
                            mLen = l2 - 1;
                            break;
                        }
                        idx2 += (size_t)l2 + 1;
                    }
                    // Accept the usual 0x69 signature, or allow the special-case MAC
                    if (sLen >= 3 && (sdata[0] == 0x69 || payloadLen == 26)) {
                        if (decodeFD3D(advLower, sdata, sLen, mdata, mLen, payloadLen == 26)) {
                            decoded = true;
                            break;
                        }
                    }
                }
            }
            idx += (size_t)len + 1;
        }
        if (!decoded) {
            Serial.println("  No SwitchBot FD3D service data found to decode.");
            Serial.println("------------------------------------");
        }
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
}

void loop() {
  if (!pBLEScan->isScanning() && (timeHM() != lastTime)) {
    lastTime=timeHM();
    Serial.println("Scheduled: starting scan at: " + lastTime);
    // clear seen devices for this new scan so duplicates from previous scans are allowed
    Ndevices = 0;
    seenDevices.clear();
    digitalWrite(ledPin,LOW);
    pBLEScan->start(scanTime, false);
    digitalWrite(ledPin,HIGH);
    // Serial.printf("Scan complete: %u devices found\n", (unsigned)seenDevices.size());
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