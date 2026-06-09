/*
 * pz-nfc-reader-m5.ino
 * 
 * PZTrack NFC vessel check-in reader — M5Stack Core2 + RFID2 Unit
 * 
 * Hardware
 * --------
 * M5Stack Core2 v1.3
 * M5Stack RFID2 Unit (WS1850S, I2C 0x28) on Grove Port A
 * 
 * Operation — identical two-state flow to the Pi version
 * -------------------------------------------------------
 * STATE: SCANNING
 *   Polls RFID2 for a tag.
 *   On scan → looks up UID in boot-time cache → fetches live competitor
 *   state from API → enters CONFIRMING.
 * 
 * STATE: CONFIRMING
 *   Shows vessel name + ON/OFF WATER badge + "Tap to toggle".
 *   Tap anywhere on screen → toggle via API → show result → SCANNING.
 *   New tag scan → immediately look up new vessel (operator scanned wrong tag).
 *   Backlight timeout → dim screen (wake on tap or scan).
 * 
 * Boot sequence
 * -------------
 * 1. Show startup screen
 * 2. Connect to WiFi — gateway IP = WiFi.gatewayIP()
 * 3. Login to PZTrack API (JWT)
 * 4. Load all trackers from GET /trackers/all → build UID→devEUI map
 * 5. Show Ready
 * 
 * Libraries required (install via Arduino Library Manager)
 * --------------------------------------------------------
 * M5Unified         by M5Stack
 * MFRC522-I2C       by arozcan  (search "MFRC522 I2C")
 * ArduinoJson       by Benoit Blanchon (v6)
 */

#include <M5Unified.h>
#include <WiFi.h>
#include "config.h"
#include "nfc.h"
#include "display.h"
#include "api.h"

// ---------------------------------------------------------------------------
// State machine
// ---------------------------------------------------------------------------

enum State {
    STATE_BOOTING,
    STATE_CONNECTING,
    STATE_LOADING,
    STATE_SCANNING,
    STATE_CONFIRMING
};

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

DisplayManager display;
NFCReader      nfc;
APIClient      api;

State          state = STATE_BOOTING;
Competitor     pending;           // set when in CONFIRMING state
String         lastUID = "";      // for debounce
unsigned long  lastScanTime = 0;  // for debounce
unsigned long  lastActivityTime = 0; // for backlight timeout
bool           backlightDimmed = false;

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);

    // Initialise M5Stack hardware
    auto cfg = M5.config();
    M5.begin(cfg);

    display.begin();
    display.showStartup();
    delay(1000);

    // --- WiFi ---------------------------------------------------------------
    state = STATE_CONNECTING;
    Serial.println("[WiFi] Connecting to " + String(WIFI_SSID));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - wifiStart > 30000) {
            display.showError("WiFi Timeout");
            Serial.println("[WiFi] Connection timeout");
            // Retry after 10s rather than halting
            delay(10000);
            ESP.restart();
        }
        display.showConnecting(WiFi.localIP().toString());
        delay(500);
    }

    String gatewayIP = WiFi.gatewayIP().toString();
    Serial.println("[WiFi] Connected. Gateway: " + gatewayIP);

    // --- API + tracker cache ------------------------------------------------
    state = STATE_LOADING;
    display.showConnecting(gatewayIP);  // reuse screen — shows "Connecting..."

    if (!api.begin(gatewayIP)) {
        display.showError("API Connect Failed");
        Serial.println("[Boot] API init failed");
        delay(10000);
        ESP.restart();
    }

    // --- NFC ----------------------------------------------------------------
    if (!nfc.begin()) {
        display.showError("NFC Init Failed");
        Serial.println("[Boot] RFID2 init failed");
        delay(10000);
        ESP.restart();
    }

    // Ready
    state = STATE_SCANNING;
    display.showReady();
    lastActivityTime = millis();
    Serial.println("[Boot] Ready. Waiting for NFC tags...");
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

void loop() {
    M5.update();  // updates touch, buttons, power

    // --- Backlight management -----------------------------------------------
    if (BACKLIGHT_TIMEOUT_MS > 0) {
        unsigned long idle = millis() - lastActivityTime;
        if (!backlightDimmed && idle > BACKLIGHT_TIMEOUT_MS) {
            display.dimBacklight();
            backlightDimmed = true;
        }
    }

    // Wake on any touch
    if (M5.Touch.getCount() > 0 && backlightDimmed) {
        display.fullBacklight();
        backlightDimmed = false;
        lastActivityTime = millis();
        // Don't process this touch further — it was just a wake touch
        return;
    }

    // --- STATE: SCANNING ----------------------------------------------------
    if (state == STATE_SCANNING || state == STATE_CONFIRMING) {

        String uid = nfc.readUID();

        if (uid.length() > 0) {
            unsigned long now = millis();

            // Debounce — suppress same tag re-trigger, but only when not
            // already confirming (operator may re-scan to replace pending)
            if (state == STATE_SCANNING) {
                if (uid == lastUID && (now - lastScanTime) < DEBOUNCE_MS) {
                    uid = "";  // treat as no scan
                }
            }

            if (uid.length() > 0) {
                lastUID = uid;
                lastScanTime = now;
                lastActivityTime = now;
                if (backlightDimmed) {
                    display.fullBacklight();
                    backlightDimmed = false;
                }

                Serial.println("[NFC] Tag scanned: " + uid);
                display.showScanning(uid);

                Competitor competitor;
                if (!api.lookupUID(uid, competitor)) {
                    Serial.println("[NFC] Unknown tag: " + uid);
                    display.showUnknownTag(uid);
                    delay(2000);
                    state = STATE_SCANNING;
                    pending = Competitor();
                    display.showReady();
                } else {
                    competitor.uid = uid;
                    pending = competitor;
                    state = STATE_CONFIRMING;

                    Serial.println("[NFC] Vessel: " + competitor.craftName +
                                   " | State: " + competitor.checkinState);
                    display.showCurrentStatus(competitor.craftName, competitor.checkinState);
                }
            }
        }
    }

    // --- STATE: CONFIRMING — watch for touch --------------------------------
    if (state == STATE_CONFIRMING) {
        if (M5.Touch.getCount() > 0) {
            lastActivityTime = millis();

            Serial.println("[Touch] Confirm tap received for " + pending.craftName);

            if (api.toggleCheckinState(pending)) {
                // Determine new state for display
                String newState = (pending.checkinState == "checked_in")
                                  ? "checked_out" : "checked_in";
                display.showSuccess(pending.craftName, newState);
                Serial.println("[API] Toggle successful → " + newState);
            } else {
                display.showError("Update Failed");
                Serial.println("[API] Toggle failed");
            }

            delay(2000);
            state = STATE_SCANNING;
            pending = Competitor();
            lastUID = "";   // reset debounce so same tag can re-scan immediately
            display.showReady();
        }
    }
}
