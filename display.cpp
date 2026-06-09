#include "display.h"
#include "config.h"

DisplayManager::DisplayManager() {}

void DisplayManager::begin() {
    M5.Lcd.fillScreen(COLOUR_BG_DEFAULT);
    M5.Lcd.setTextDatum(TC_DATUM);  // top-centre alignment default
    fullBacklight();
}

// ---------------------------------------------------------------------------
// Screens
// ---------------------------------------------------------------------------

// Boot screen — shown immediately on power-on
void DisplayManager::showStartup() {
    _clear();
    _centreText("PZTrack", 60, 3, COLOUR_TEXT_DEFAULT);
    _centreText("NFC Reader", 90, 2, COLOUR_TEXT_DIM);
    _centreText("Starting up...", 150, 1, COLOUR_TEXT_DIM);
}

// Waiting for gateway — shows IP being tried (mirrors "Connecting... <IP>")
void DisplayManager::showConnecting(const String& ip) {
    _clear();
    _centreText("Connecting...", 80, 2, COLOUR_TEXT_DIM);
    _centreText(ip, 120, 1, COLOUR_TEXT_DIM);
    _centreText("Waiting for gateway", 150, 1, COLOUR_TEXT_DIM);
}

// Idle — waiting for a tag scan
void DisplayManager::showReady() {
    _clear();
    _centreText("Ready", 70, 3, COLOUR_TEXT_DEFAULT);
    _centreText("Scan vessel tag", 115, 2, COLOUR_TEXT_DIM);
    // Small NFC icon hint at bottom
    M5.Lcd.drawRoundRect(140, 190, 40, 30, 5, COLOUR_TEXT_DIM);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(COLOUR_TEXT_DIM);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("NFC", 160, 205);
    M5.Lcd.setTextDatum(TC_DATUM);
}

// Immediately after a scan — shows UID while API call is in flight
void DisplayManager::showScanning(const String& uidHex) {
    _clear();
    _centreText("Tag Detected", 70, 2, COLOUR_TEXT_DEFAULT);
    _centreText("Looking up...", 105, 2, COLOUR_TEXT_DIM);
    _centreText(uidHex, 160, 1, COLOUR_TEXT_DIM);
}

// CONFIRMING state — vessel name + status badge + tap instruction
// Mirrors the Pi's inverted-display for ON WATER
void DisplayManager::showCurrentStatus(const String& craftName, const String& state) {
    bool onWater = (state == "checked_in");
    uint32_t bg = onWater ? COLOUR_BG_ON_WATER : COLOUR_BG_OFF_WATER;
    _clear(bg);

    // Craft name — large, prominent
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(3);
    // Word-wrap long names at 18 chars
    if (craftName.length() <= 12) {
        M5.Lcd.drawString(craftName, 160, 40);
    } else {
        // Split at last space before char 12
        int split = craftName.lastIndexOf(' ', 12);
        if (split < 0) split = 12;
        M5.Lcd.drawString(craftName.substring(0, split), 160, 30);
        M5.Lcd.drawString(craftName.substring(split + 1), 160, 65);
    }

    // Status badge
    _drawStatusBadge(state);

    // Tap-to-confirm instruction at bottom
    _centreText("Tap screen to toggle", 195, 1, COLOUR_TEXT_DIM);
}

// Post-confirmation — shows new state for 2 seconds (mirrors "Updated OK")
void DisplayManager::showSuccess(const String& craftName, const String& newState) {
    bool onWater = (newState == "checked_in");
    uint32_t bg = onWater ? COLOUR_BG_ON_WATER : COLOUR_BG_OFF_WATER;
    _clear(bg);

    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString(craftName, 160, 50);

    _drawStatusBadge(newState);

    _centreText("Updated OK", 185, 2, TFT_GREEN);
}

// Tag not in database
void DisplayManager::showUnknownTag(const String& uidHex) {
    _clear();
    _centreText("Unknown Tag", 70, 2, TFT_YELLOW);
    _centreText("Not registered", 105, 2, COLOUR_TEXT_DIM);
    _centreText(uidHex, 160, 1, COLOUR_TEXT_DIM);
}

// Any failure — red inverted screen (mirrors Pi's inverted ERROR screen)
void DisplayManager::showError(const String& message) {
    _clear(COLOUR_BG_ERROR);
    _centreText("ERROR", 70, 3, TFT_WHITE);
    _centreText(message, 120, 1, TFT_WHITE);
}

// Clean shutdown
void DisplayManager::showShutdown() {
    _clear();
    _centreText("Shutting down...", 110, 2, COLOUR_TEXT_DIM);
}

// ---------------------------------------------------------------------------
// Backlight
// ---------------------------------------------------------------------------

void DisplayManager::setBacklight(uint8_t level) {
    M5.Lcd.setBrightness(level);
}

void DisplayManager::dimBacklight() {
    setBacklight(BACKLIGHT_DIM_LEVEL);
}

void DisplayManager::fullBacklight() {
    setBacklight(BACKLIGHT_FULL_LEVEL);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void DisplayManager::_clear(uint32_t bgColour) {
    M5.Lcd.fillScreen(bgColour);
}

void DisplayManager::_centreText(const String& text, int y, uint8_t size, uint32_t colour) {
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.setTextColor(colour);
    M5.Lcd.setTextSize(size);
    M5.Lcd.drawString(text, 160, y);  // 160 = horizontal centre of 320px screen
}

// Draws the ON WATER / OFF WATER badge in the middle of the screen
void DisplayManager::_drawStatusBadge(const String& state) {
    bool onWater = (state == "checked_in");
    uint32_t badgeColour = onWater ? COLOUR_TEXT_ON : COLOUR_TEXT_OFF;
    String label = onWater ? "ON WATER" : "OFF WATER";

    M5.Lcd.drawRoundRect(60, 115, 200, 50, 8, badgeColour);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(badgeColour);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString(label, 160, 140);
    M5.Lcd.setTextDatum(TC_DATUM);
}
