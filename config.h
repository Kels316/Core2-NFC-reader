#pragma once

// =============================================================================
// PZTrack NFC Reader — M5Stack Core2 configuration
// =============================================================================
// Copy this file and fill in your values before flashing.
// This file is gitignored — never commit credentials.

// WiFi — must be the PZ Network SSID for gateway auto-discovery to work
#define WIFI_SSID       "PZ Network"
#define WIFI_PASSWORD   "your-wifi-password"

// PZTrack API credentials — must match the API server config
#define API_USERNAME    "admin"
#define API_PASSWORD    "your-api-password"

// API port — PZTrack Docker default
#define API_PORT        5000

// JWT token lifetime in seconds (10 days per global_config.json)
// Refresh 5 minutes before expiry, same as the Pi version
#define JWT_LIFETIME_S  864000
#define JWT_REFRESH_MARGIN_S 300

// NFC debounce — minimum seconds before the same tag can trigger again
// Matches debounce_seconds in the Pi version's config.json
#define DEBOUNCE_MS     3000

// Display backlight timeout — dims after this many ms of inactivity
// Set to 0 to disable dimming
#define BACKLIGHT_TIMEOUT_MS  30000
#define BACKLIGHT_DIM_LEVEL   64    // 0-255
#define BACKLIGHT_FULL_LEVEL  255
