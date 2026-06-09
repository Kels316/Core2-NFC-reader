#include "api.h"
#include "config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <map>

APIClient::APIClient()
    : _tokenExpiry(0)
{}

// ---------------------------------------------------------------------------
// Boot initialisation
// ---------------------------------------------------------------------------

bool APIClient::begin(const String& gatewayIP) {
    _gatewayIP = gatewayIP;
    _baseURL = "http://" + gatewayIP + ":" + String(API_PORT);
    Serial.println("[API] Base URL: " + _baseURL);

    if (!_login()) return false;
    if (!_loadTrackers()) return false;

    Serial.println("[API] Ready. " + String(_uidToDevEUI.size()) + " trackers cached.");
    return true;
}

// ---------------------------------------------------------------------------
// Tag lookup — purely local, no network call
// ---------------------------------------------------------------------------

bool APIClient::lookupUID(const String& uid, Competitor& out) {
    String upperUID = uid;
    upperUID.toUpperCase();

    auto it = _uidToDevEUI.find(upperUID);
    if (it == _uidToDevEUI.end()) {
        return false;  // unknown tag
    }

    // We have the devEUI — now fetch live competitor data including checkinState
    return _getCompetitorByDevEUI(it->second, out);
}

// ---------------------------------------------------------------------------
// Toggle checkin state
// Mirrors PZTrackClient.set_checkin_state() in api_client.py
// ---------------------------------------------------------------------------

bool APIClient::toggleCheckinState(const Competitor& competitor) {
    if (!refreshTokenIfNeeded()) return false;

    // Determine new state — opposite of current
    String newState;
    if (competitor.checkinState == "checked_in") {
        newState = "checked_out";
    } else {
        newState = "checked_in";
    }

    Serial.println("[API] Toggling " + competitor.craftName +
                   " (" + competitor.competitorId + "): " +
                   competitor.checkinState + " → " + newState);

    String body = "{\"checkin_state\":\"" + newState + "\"}";
    String response;
    int code = _httpPost(
        "/competitors/" + competitor.competitorId + "/checkinstate",
        body,
        response
    );

    if (code == 200 || code == 204) {
        Serial.println("[API] State updated OK.");
        return true;
    }

    Serial.println("[API] Toggle failed, HTTP " + String(code) + ": " + response);
    return false;
}

// ---------------------------------------------------------------------------
// Token management — mirrors refresh_if_needed() in api_client.py
// ---------------------------------------------------------------------------

bool APIClient::refreshTokenIfNeeded() {
    // Refresh if token absent or within margin of expiry
    unsigned long now = millis();
    if (_token.isEmpty() || now >= _tokenExpiry) {
        Serial.println("[API] Refreshing JWT token.");
        return _login();
    }
    return true;
}

// ---------------------------------------------------------------------------
// Private: login — mirrors PZTrackClient.login() in api_client.py
// ---------------------------------------------------------------------------

bool APIClient::_login() {
    String body = "{\"username\":\"" + String(API_USERNAME) +
                  "\",\"password\":\"" + String(API_PASSWORD) + "\"}";
    String response;
    int code = _httpPost("/login", body, response);

    if (code != 200) {
        Serial.println("[API] Login failed, HTTP " + String(code));
        return false;
    }

    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, response) != DeserializationError::Ok) {
        Serial.println("[API] Login JSON parse error");
        return false;
    }

    _token = doc["access_token"].as<String>();
    // Token lifetime from config, refresh margin applied same as Pi version
    _tokenExpiry = millis() + ((unsigned long)(JWT_LIFETIME_S - JWT_REFRESH_MARGIN_S)) * 1000UL;

    Serial.println("[API] JWT token acquired.");
    return true;
}

// ---------------------------------------------------------------------------
// Private: load trackers at boot — builds UID → devEUI cache
// Uses GET /trackers/all which returns rfid field per tracker
// ---------------------------------------------------------------------------

bool APIClient::_loadTrackers() {
    if (!refreshTokenIfNeeded()) return false;

    String response;
    int code = _httpGet("/trackers/all", response);

    if (code != 200) {
        Serial.println("[API] GET /trackers/all failed, HTTP " + String(code));
        return false;
    }

    // Response: { "trackers": [ { "devEUI": "...", "name": "...", "rfid": "..." }, ... ] }
    // rfid may be absent if not yet registered
    DynamicJsonDocument doc(32768);  // large enough for a full event fleet
    if (deserializeJson(doc, response) != DeserializationError::Ok) {
        Serial.println("[API] Tracker list JSON parse error");
        return false;
    }

    _uidToDevEUI.clear();
    JsonArray trackers = doc["trackers"].as<JsonArray>();
    int registered = 0;

    for (JsonObject tracker : trackers) {
        if (!tracker.containsKey("rfid") || tracker["rfid"].isNull()) continue;

        String devEUI = tracker["devEUI"].as<String>();
        String rfid   = tracker["rfid"].as<String>();
        devEUI.toUpperCase();
        rfid.toUpperCase();

        _uidToDevEUI[rfid] = devEUI;
        registered++;
    }

    Serial.println("[API] Loaded " + String(registered) + " registered NFC tags.");
    return true;
}

// ---------------------------------------------------------------------------
// Private: get competitor from tracker devEUI
// Uses GET /trackers/<devEUI>/competitor
// ---------------------------------------------------------------------------

bool APIClient::_getCompetitorByDevEUI(const String& devEUI, Competitor& out) {
    if (!refreshTokenIfNeeded()) return false;

    String response;
    String devEUILower = devEUI;
    devEUILower.toLowerCase();  // API expects lowercase hex
    int code = _httpGet("/trackers/" + devEUILower + "/competitor", response);

    if (code == 404) {
        Serial.println("[API] No competitor linked to devEUI " + devEUI);
        return false;
    }
    if (code != 200) {
        Serial.println("[API] GET competitor failed, HTTP " + String(code));
        return false;
    }

    // Response: { "competitor": { "competitorId", "craftName", "checkinState", ... } }
    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, response) != DeserializationError::Ok) {
        Serial.println("[API] Competitor JSON parse error");
        return false;
    }

    JsonObject c = doc["competitor"];
    out.competitorId = c["competitorId"].as<String>();
    out.craftName    = c["craftName"].as<String>();
    out.teamName     = c["teamName"].as<String>();
    out.checkinState = c["checkinState"].isNull() ? "checked_out" : c["checkinState"].as<String>();
    out.devEUI       = devEUI;

    return true;
}

// ---------------------------------------------------------------------------
// Private HTTP helpers — thin wrappers around HTTPClient
// ---------------------------------------------------------------------------

int APIClient::_httpGet(const String& path, String& responseBody) {
    HTTPClient http;
    http.begin(_baseURL + path);
    if (!_token.isEmpty()) {
        http.addHeader("Authorization", "Bearer " + _token);
    }
    http.setTimeout(10000);

    int code = http.GET();
    responseBody = (code > 0) ? http.getString() : "";
    http.end();
    return code;
}

int APIClient::_httpPost(const String& path, const String& body, String& responseBody) {
    HTTPClient http;
    http.begin(_baseURL + path);
    http.addHeader("Content-Type", "application/json");
    if (!_token.isEmpty()) {
        http.addHeader("Authorization", "Bearer " + _token);
    }
    http.setTimeout(10000);

    int code = http.POST(body);
    responseBody = (code > 0) ? http.getString() : "";
    http.end();
    return code;
}
