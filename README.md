# PZTrack NFC Reader — M5Stack Core2

M5Stack Core2 + RFID2 Unit port of the [pz-nfc-reader](https://github.com/Kels316/pz-nfc-reader) Pi module.

Same two-step scan-and-confirm flow, same API endpoints, no server changes required.

---

## Hardware

| Component | Details |
|---|---|
| M5Stack Core2 v1.3 | ESP32, 2" capacitive touchscreen, WiFi, built-in battery |
| M5Stack RFID2 Unit | WS1850S, ISO 14443A/B, I2C at 0x28 |
| Grove cable | Connects RFID2 to Core2 Port A |

Connect the RFID2 unit to **Port A** (the red Grove connector on the left side of the Core2).

---

## How It Works

Identical flow to the Pi version:

1. Boot → connect WiFi → discover gateway via `WiFi.gatewayIP()`
2. Login to PZTrack API (JWT)
3. Load all trackers from `GET /trackers/all` → build UID→devEUI map in memory
4. Show Ready screen — poll for NFC tags
5. Tag scanned → `GET /trackers/<devEUI>/competitor` → show vessel + ON/OFF WATER
6. Operator taps screen to confirm toggle
7. `POST /competitors/<id>/checkinstate` → show result → back to Ready

No server changes needed — uses only existing API endpoints.

---

## Libraries (install via Arduino Library Manager)

| Library | Author | Notes |
|---|---|---|
| M5Unified | M5Stack | Core2 hardware abstraction |
| MFRC522-I2C | arozcan | Search "MFRC522 I2C" |
| ArduinoJson | Benoit Blanchon | v6 |

---

## Setup

### 1. Install Arduino IDE and ESP32 board support

Add to Board Manager URLs:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

Install **esp32 by Espressif Systems**.

Select board: **M5Stack-Core2**

### 2. Install libraries

In Arduino IDE → Sketch → Include Library → Manage Libraries:
- Search and install: `M5Unified`, `MFRC522-I2C`, `ArduinoJson`

### 3. Configure

Edit `config.h`:
```cpp
#define WIFI_SSID       "PZ Network"
#define WIFI_PASSWORD   "your-wifi-password"
#define API_USERNAME    "admin"
#define API_PASSWORD    "your-api-password"
```

### 4. Flash

Connect Core2 via USB-C. Select the correct port. Upload.

---

## Screens

| Screen | When shown |
|---|---|
| `PZTrack / NFC Reader / Starting up...` | Boot |
| `Connecting... <IP>` | WiFi + API init |
| `Ready / Scan vessel tag` | Idle |
| `Tag Detected / Looking up...` | Tag scanned, API call in flight |
| Vessel name + green ON WATER badge | Confirming — vessel is on water |
| Vessel name + grey OFF WATER badge | Confirming — vessel is off water |
| Vessel name + badge + `Updated OK` | 2s confirmation after tap |
| `Unknown Tag / Not registered` | Tag not in database |
| `ERROR <message>` | Red screen — any failure |

---

## Tag Registration

Tags are still registered using `register_tag.py` from the Pi repo, run on a laptop or Pi connected to PZ Network. No change to that workflow.

---

## Files

| File | Purpose |
|---|---|
| `pz-nfc-reader-m5.ino` | Main loop + state machine |
| `config.h` | WiFi credentials, API settings |
| `nfc.h / nfc.cpp` | RFID2 unit polling, UID extraction |
| `display.h / display.cpp` | M5Stack screen management |
| `api.h / api.cpp` | HTTP calls to PZTrack API, boot cache |
