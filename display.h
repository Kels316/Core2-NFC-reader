#pragma once

#include <Arduino.h>
#include <M5Unified.h>

// Colours — matches the visual intent of the Pi OLED screens but in colour.
// ON WATER = green (vessel is out, danger context, needs attention)
// OFF WATER = dark background (vessel is safe/home)
#define COLOUR_BG_DEFAULT   TFT_BLACK
#define COLOUR_BG_ON_WATER  0x0640    // dark green background
#define COLOUR_BG_OFF_WATER TFT_BLACK
#define COLOUR_BG_ERROR     TFT_RED
#define COLOUR_BG_SUCCESS   0x0640    // dark green

#define COLOUR_TEXT_DEFAULT TFT_WHITE
#define COLOUR_TEXT_ON      0x07E0    // bright green
#define COLOUR_TEXT_OFF     0xC618    // light grey
#define COLOUR_TEXT_ERROR   TFT_WHITE
#define COLOUR_TEXT_DIM     0x7BEF    // mid grey for secondary text

class DisplayManager {
public:
    DisplayManager();
    void begin();

    // Screens — mirrors every state in the Pi's display_manager.py
    void showStartup();
    void showConnecting(const String& ip);
    void showReady();
    void showScanning(const String& uidHex);
    void showCurrentStatus(const String& craftName, const String& state);
    void showSuccess(const String& craftName, const String& newState);
    void showUnknownTag(const String& uidHex);
    void showError(const String& message);
    void showShutdown();

    // Backlight control
    void setBacklight(uint8_t level);
    void dimBacklight();
    void fullBacklight();

private:
    void _clear(uint32_t bgColour = COLOUR_BG_DEFAULT);
    void _centreText(const String& text, int y, uint8_t size, uint32_t colour);
    void _drawStatusBadge(const String& state);
};
