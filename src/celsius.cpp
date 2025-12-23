#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string>
#include <vector>
#include <unordered_set>

// Configuration via #define
#define SCAN_TIME_S 5
#define SCAN_INTERVAL_MS 60000UL

// (TARGET_ADDRESS removed — use MAC_WHITELIST to control which devices are processed)
// Edit to add the addresses you want to target (lowercase, colon-separated).
static const std::vector<std::string> MAC_WHITELIST = {
    "f7:86:17:6f:ad:57",//notte
    "ca:c8:11:8d:e2:c6",//giardino
    "f1:39:38:e5:68:0a",//soggiorno
    "c0:23:17:1f:65:4f"//tavernetta
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
unsigned long lastScanMillis = 0;
const unsigned long scanIntervalMs = SCAN_INTERVAL_MS; // 1 minute

// Track devices seen during the current scan to avoid duplicate prints
static std::unordered_set<std::string> seenDevices;

// ----------------------------------------------------------------------
// 1. DATA CALLBACKS (AdvertisedDeviceCallbacks):
//    Handles processing data when a device is found.
// ----------------------------------------------------------------------

// Decode SwitchBot FD3D service data. Returns true if decoded and printed.
static bool decodeFD3D(const std::string &advLower, uint8_t* sdata, size_t sLen, uint8_t* mdata=nullptr, size_t mLen=0, bool useAlt=false) {
    int battery = -1;
    float temperature = NAN;
    float humidity = NAN;
    if (sLen >= 3) battery = sdata[2];

    auto inRange = [](float v){ return v > -40.0f && v < 85.0f; };

        // Special-case alternate model at MAC f1:39:38:e5:68:0a
    if (useAlt) {
        // Use manufacturer-data mapping observed in captures (PayloadLen==26):
        // mdata layout: [0..1]=company id, [2..7]=MAC, [8]=battery, [9]=?, [10]=temp_frac, [11]=temp_int+128, [12]=humidity|flags
        Serial.println("  Using alternate decoder for special meter model (manufacturer-data mapping)");
        if (mdata != nullptr && mLen >= 13) {
            int m_battery = (int)mdata[8];
            int temp_int = (int)mdata[11] - 128;
            float temp_frac = ((int)mdata[10]) / 10.0f;
            float t = (temp_int >= 0) ? (float)temp_int + temp_frac : (float)temp_int - temp_frac;
            float h = (float)(mdata[12] & 0x7F);
            if (inRange(t)) temperature = t;
            humidity = (h >= 0.0f && h <= 100.0f) ? h : NAN;
            Serial.printf("  Battery: %d%%\n", m_battery);
            Serial.printf("  Temperature: %.1f°C\n", temperature);
            Serial.printf("  Humidity: %.0f%%\n", humidity);
            Serial.println("------------------------------------");
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
    Serial.println("------------------------------------");
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
            
            Serial.println("--- SwitchBot Meter (T/H) FOUND & DECODING ---");
            Serial.printf("  Name: %s\n", advertisedDevice->getName().c_str());
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
    delay(1000); 
    Serial.println("Starting BLE Scan for SwitchBot Meter...");

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
    
    Serial.printf("Starting scan for %d seconds...\n", scanTime);
    // start initial scan (non-continuous) — we'll restart periodically in loop()
    // clear any previous seen set before a new scan
    seenDevices.clear();
    pBLEScan->start(scanTime, false);
    lastScanMillis = millis();
}

void loop() {
    // Periodically start a scan every `scanIntervalMs` milliseconds.
    if (!pBLEScan->isScanning() && (millis() - lastScanMillis >= scanIntervalMs)) {
        Serial.println("Scheduled: starting scan...");
        // clear seen devices for this new scan so duplicates from previous scans are allowed
        seenDevices.clear();
        pBLEScan->start(scanTime, false);
        lastScanMillis = millis();
    }
}