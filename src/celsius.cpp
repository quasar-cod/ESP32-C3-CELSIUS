#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#define ADDR  "celsius" 

//dati per display
#include <U8g2lib.h>
#include <Wire.h>
#define SDA_PIN 5
#define SCL_PIN 6
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);   // EastRising 0.42" OLED

String BOARD = "2";
int i,j;
const int ledPin = 8; 
int pt = 1;
String MAC = "";
String sensorName = "";
float temperature = NAN;
float humidity = NAN;
int battery = -1;
String RSSI = "";

// Map MAC (lowercase, colon-separated) to human-friendly sensor name
static const std::unordered_map<std::string, std::string> MAC_NAMES = {
    {"f7:86:17:6f:ad:57", "SWBT01"},
    {"f1:39:38:e5:68:0a", "SWBT02"},
    {"c0:23:17:1f:65:4f", "SWBT03"},
    {"ca:c8:11:8d:e2:c6", "SWBT04"}
};

#include "time.h"                 // for time() ctime()
#include "miotime.h"                   // for time() ctime()
#define MY_NTP_SERVER "it.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   

// Configuration via #define
#define SCAN_TIME_S 10
#define SCAN_INTERVAL_MS 60000UL

// (TARGET_ADDRESS removed — use MAC_WHITELIST to control which devices are processed)
// Edit to add the addresses you want to target (lowercase, colon-separated).
static const std::vector<std::string> MAC_WHITELIST = {
    "f7:86:17:6f:ad:57",
    "f1:39:38:e5:68:0a",
    "c0:23:17:1f:65:4f",
    "ca:c8:11:8d:e2:c6"
};

// List MACs to ignore when whitelist is empty (lowercase, colon-separated).
static const std::vector<std::string> MAC_BLACKLIST = {
    "2b:ed:64:24:44:d5",
    "38:01:95:1c:47:33",
    "1e:f6:22:94:c3:3e",
    "e2:da:33:12:4b:79",
    "66:aa:b9:10:ea:87"
};

int scanTime = SCAN_TIME_S; // Scan duration in seconds (Scan will restart automatically)
NimBLEScan* pBLEScan;
String lastTime;
// Track devices seen during the current scan to avoid duplicate prints
static std::unordered_set<std::string> seenDevices;

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
//   http.begin(client,"http://hp-i3/celsius/updatedata.php");  //--> Specify request destination
  http.begin(client,"http://myhomesmart.altervista.org/celsius/updatedata.php");  //--> Specify request destination
  //NB va usato http e non https
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");  //--> Specify content-type header   
  httpCode = http.POST(postData); //--> Send the request
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
        // Special-case alternate model at MAC f1:39:38:e5:68:0a
    if (useAlt) {
        // Use manufacturer-data mapping observed in captures (PayloadLen==26):
        // mdata layout: [0..1]=company id, [2..7]=MAC, [8]=battery, [9]=?, [10]=temp_frac, [11]=temp_int+128, [12]=humidity|flags
            if (mdata != nullptr && mLen >= 13) {
            int8_t raw_b = (int8_t)mdata[8];
            int m_battery = raw_b < 0 ? -raw_b : (int)raw_b; // some captures encode as signed byte (0xC5 == -59)
            if (m_battery < 0) m_battery = 0;
            if (m_battery > 100) m_battery = 100;
            int temp_int = (int)mdata[11] - 128;
            float temp_frac = ((int)mdata[10]) / 10.0f;
            float t = (temp_int >= 0) ? (float)temp_int + temp_frac : (float)temp_int - temp_frac;
            float h = (float)(mdata[12] & 0x7F);
            if (inRange(t)) temperature = t;
            humidity = (h >= 0.0f && h <= 100.0f) ? h : NAN;
            Serial.printf("  Battery: %d%%\n", battery);
            Serial.printf("  m_Battery: %d%%\n", m_battery);
            Serial.printf("  Temperature: %.1f°C\n", temperature);
            Serial.printf("  Humidity: %.0f%%\n", humidity);
            Serial.println("TIPO 1 MINI");
            Serial.println("------------------------------------");
            updatedata();
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
    // Default decoder (existing heuristic)
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
        // If a whitelist is configured, only process those MACs
        if (!MAC_WHITELIST.empty()) {
            bool allowed = false;
            for (const auto &w : MAC_WHITELIST) if (advLower == toLower(w)) { allowed = true; break; }
            if (!allowed) return; // not in whitelist
        } else {
            // Ignore blacklisted MACs when whitelist is empty
            for (const auto &b : MAC_BLACKLIST) if (advLower == toLower(b)) return;
        }
        // Deduplicate within a single scan: skip if we've already processed this MAC
        if (seenDevices.find(advLower) != seenDevices.end()) return;
        seenDevices.insert(advLower);
        // Debug: print summary for every (allowed) discovered device
        size_t payloadLenDbg = advertisedDevice->getPayloadLength();
        std::string nameDbg = advertisedDevice->getName();
        Serial.printf("Found device: %s  RSSI: %d dBm  PayloadLen: %u  Name: %s\n",
                      advertisedDevice->getAddress().toString().c_str(),
                      advertisedDevice->getRSSI(), (unsigned)payloadLenDbg,
                      nameDbg.c_str());
        uint8_t* payloadDbg = advertisedDevice->getPayload();
        if (payloadDbg != nullptr && payloadLenDbg > 0) {
            Serial.print("  Payload: ");
            for (size_t i = 0; i < payloadLenDbg && i < 32; ++i) Serial.printf("%02X ", payloadDbg[i]);
            Serial.println();
        } else {
            Serial.println("  Payload: <none>");
        }
        // At this point the device is allowed (whitelist/blacklist checks passed)
        {
            RSSI=advertisedDevice->getRSSI();
            MAC= advertisedDevice->getAddress().toString().c_str();
            Serial.println("--- SwitchBot Meter (T/H) FOUND & DECODING ---");
                        // Prefer configured friendly name for this MAC, fall back to advertised name
                        if (MAC_NAMES.find(advLower) != MAC_NAMES.end()) sensorName = MAC_NAMES.at(advLower).c_str();
                        else sensorName = advertisedDevice->getName().c_str();
                        Serial.printf("  Name: %s\n", sensorName.c_str());
            Serial.printf("  MAC: %s\n", advertisedDevice->getAddress().toString().c_str());
            Serial.printf("  RSSI: %d dBm\n", advertisedDevice->getRSSI());
            uint8_t* payload = advertisedDevice->getPayload();
            size_t payloadLen = advertisedDevice->getPayloadLength();
            if (payload == nullptr || payloadLen == 0) {
                Serial.println("  ERROR: No payload to decode.");
                return;
            }
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
                        uint8_t* mdata = nullptr; size_t mLen = 0;
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
    }
};
AdvertisedDeviceCallbacks advCallbacks;
// ----------------------------------------------------------------------
// 2. SCAN CONTROLLER:
//    Use the `start(duration, true)` overload for continuous scanning
//    to avoid relying on scan-complete callback signatures.
// ----------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("\nInitialized serial communications");
  pinMode(ledPin, OUTPUT);  
  digitalWrite(ledPin, HIGH);     
  const char* ssid = "TIM-39751438";
  const char* password = "EFuPktKzk6utU2y5a5SEkUUQ";
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("-------------");
  Serial.print("Connecting to wifi");
  int connecting_process_timed_out = 20; // 10 seconds.
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
    digitalWrite(ledPin,LOW);
    delay(250);
    digitalWrite(ledPin,HIGH);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {//Countdown "connecting_process_timed_out".
      ESP.restart();
    }
  }
  Serial.println();
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("-------------");
  Serial.println("Abilito dns");
  if (MDNS.begin(ADDR)) {
    Serial.println("Abilitato");
  }
  Serial.println("-------------");
  configTime(0,0, MY_NTP_SERVER); // --> Here is the IMPORTANT ONE LINER needed in your sketch!
  setenv("TZ","CET-1CEST,M3.5.0/02,M10.5.0/03" ,1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
  Serial.println("NTP TZ DST - wait 1 minute");
// by default, the NTP will be started after 60 secs
  for (int i =0;i<60;i++){
    Serial.print(".");
    delay(500);
    digitalWrite(ledPin,LOW);
    delay(500);
    digitalWrite(ledPin,HIGH);
  }
  Serial.println("\norario timezone NTP");
  Serial.println(timeHMS());
  Serial.println("\nStarting BLE Scan for SwitchBot Meter...");
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
    // Serial.printf("Starting scan for %d seconds...\n", scanTime);
    // start initial scan (non-continuous) — we'll restart periodically in loop()
    // clear any previous seen set before a new scan
    // seenDevices.clear();
    // pBLEScan->start(scanTime, false);
    lastTime=timeS();
//dati per display
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.setFont(u8g2_font_timR14_tr);	// choose a suitable font  
  u8g2.clearBuffer();
}

void loop() {
//dati per display
  u8g2.setCursor(0, 34);
  u8g2.print(timeHMS());
  u8g2.sendBuffer();
  if (!pBLEScan->isScanning() && (timeHM() != lastTime)) {
    digitalWrite(ledPin,LOW);
    lastTime=timeHM();
    Serial.println("Scheduled: starting scan at: " + lastTime);
    // clear seen devices for this new scan so duplicates from previous scans are allowed
    seenDevices.clear();
    pBLEScan->start(scanTime, false);
    digitalWrite(ledPin,HIGH);
  }
}
