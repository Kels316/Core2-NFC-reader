#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Competitor data returned from the API
struct Competitor {
    String competitorId;
    String craftName;
    String teamName;
    String checkinState;    // "checked_in" | "checked_out" | ""
    String devEUI;          // tracker devEUI used to find this competitor
    String uid;             // NFC UID that triggered this lookup
};

class APIClient {
public:
    APIClient();

    // Call once at boot — loads all trackers, connects to API, authenticates
    // Returns false if WiFi or API connection fails
    bool begin(const String& gatewayIP);

    // Look up a scanned UID in the boot-time cache
    // Returns true and fills out if found, false if unknown tag
    bool lookupUID(const String& uid, Competitor& out);

    // Toggle checkin state for a competitor
    // Returns true on success, false on failure
    bool toggleCheckinState(const Competitor& competitor);

    // Refresh JWT if close to expiry — call before any API request
    bool refreshTokenIfNeeded();

    // Gateway IP discovered at boot
    String gatewayIP() const { return _gatewayIP; }

private:
    String _gatewayIP;
    String _baseURL;
    String _token;
    unsigned long _tokenExpiry;   // millis() when token expires

    // Boot-time cache: NFC UID (uppercase hex) → devEUI (16 hex chars)
    // Built from GET /trackers/all at startup
    std::map<String, String> _uidToDevEUI;

    bool _login();
    bool _loadTrackers();
    bool _getCompetitorByDevEUI(const String& devEUI, Competitor& out);

    // HTTP helpers
    int _httpGet(const String& path, String& responseBody);
    int _httpPost(const String& path, const String& body, String& responseBody);
};
