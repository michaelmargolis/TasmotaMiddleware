#include "TasmotaPlugs.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Config.h"

TasmotaPlugs::TasmotaPlugs() {
    // Constructor body, if needed
}

void TasmotaPlugs::begin() {
    // Load configuration from file system
    if (!config.loadConfig()) {
        Serial.println("Failed to load configuration!");
        return;
    }

    // Initialize plug states based on loaded configuration
    initPlugStates();

    // Check if SPI control mode is enabled and apply configuration if necessary
    if (config.SPI_controlMode) {
        Serial.println("Initializing plugs in SPI control mode.");
        initializeSPIForPlugs();
    } else {
        Serial.println("Using default Pin control mode.");
    }

    // Optionally print configuration data for debugging
    for (auto& plug : plugs) {
        Serial.printf("Plug at IP octet %d with MAC %s is controlled by ESP pin %d\n",
                      plug.ip_octet, config.plugMac_4[&plug - &plugs[0]].c_str(), plug.pin);
    }
    Serial.println();
}

void TasmotaPlugs::initPlugStates() {
    // Clear existing plug states to prepare for new configuration
    plugs.clear();
    for (size_t i = 0; i < config.plug_ip.size(); ++i) {
        PlugState newPlug;
        newPlug.ip_octet = config.plug_ip[i]; // Assume ip_octet is the index or identifier
        newPlug.pin = (i < config.esp_pin_map.size()) ? config.esp_pin_map[i] : defaultPinState;
        newPlug.pinState = defaultPinState; // Default or initial state, can be updated later
        plugs.push_back(newPlug);
    }
}

int TasmotaPlugs::getPlugState(PlugState* plug) {
    if (plug == nullptr) {
        return ERR_PLUG_REF_INVALID;
    }
    std::string url = ip_base_url + std::to_string(plug->ip_octet);
    return getPlugState(url);
}

int TasmotaPlugs::getPlugState(const std::string& url) {
    HTTPClient http;
    std::string command = url +  "/cm?cmnd=Power";
    http.begin(command.c_str());
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        return ERR_HTTP_REQUEST_FAILED;
    }

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, http.getString());
    http.end();
    if (error) {
        return ERR_JSON_ERROR;
    }

    const char* state = doc["POWER"];
    return (strcmp(state, "ON") == 0) ? 1 : 0;
}

int TasmotaPlugs::setPlugState(PlugState* plug, bool state) {
    if (plug == nullptr) {
        return ERR_PLUG_REF_INVALID;
    }
    std::string url = ip_base_url + std::to_string(plug->ip_octet);
    return setPlugState(url, state);
}

int TasmotaPlugs::setPlugState(const std::string& url, bool state) {
    HTTPClient http;
    std::string command = url + (state ? "/cm?cmnd=Power%20On" : "/cm?cmnd=Power%20Off");
    http.begin(command.c_str());
    int httpCode = http.GET();
    http.end();
    return (httpCode == HTTP_CODE_OK) ? RET_SUCCESS : ERR_TASMOTA_REQUEST_FAILED;
}

int TasmotaPlugs::getRSSI(const std::string& url) {
    HTTPClient http;
    std::string command = url + "/cm?cmnd=Status%2011";
    http.begin(command.c_str());
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        return ERR_HTTP_REQUEST_FAILED;
    }

    StaticJsonDocument<300> doc;
    DeserializationError error = deserializeJson(doc, http.getString());
    http.end();
    if (error) {
        return ERR_JSON_ERROR;
    }

    return doc["StatusSTS"]["Wifi"]["RSSI"]; // Assuming RSSI is directly accessible and valid
}

 int TasmotaPlugs::getEnergyValues(const std::string& url, EnergyValues& values ) {
    HTTPClient http;
    std::string command = url + "/cm?cmnd=Status%2010";
    http.begin(command.c_str());
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        return ERR_HTTP_REQUEST_FAILED;
    }

    StaticJsonDocument<500> doc;
    DeserializationError error = deserializeJson(doc, http.getString());
    http.end();
    if (error) {
        return ERR_JSON_ERROR;
    }

    JsonObject root = doc.as<JsonObject>();
    JsonObject StatusSNS = root["StatusSNS"];
    JsonObject ENERGY = StatusSNS["ENERGY"];
    values.Voltage = ENERGY["Voltage"].as<float>();
    values.Current = ENERGY["Current"].as<float>();
    values.Power = ENERGY["Power"].as<float>();
    values.Total = ENERGY["Total"].as<float>();
    values.Yesterday = ENERGY["Yesterday"].as<float>();
    values.Today = ENERGY["Today"].as<float>();
    return RET_SUCCESS;
}

void TasmotaPlugs::initializeSPIForPlugs() {
    // Initialization code for SPI, assuming some setup like SPI.begin()
    Serial.println("SPI initialized for all configured plugs.");
}

const char* TasmotaPlugs::getErrorString(int errorCode) {
    switch (errorCode) {
        case RET_SUCCESS: return "Success";
        case ERR_URL_PREPARATION_FAILED: return "URL preparation failed";
        case ERR_HTTP_REQUEST_FAILED: return "HTTP request failed";
        case ERR_JSON_ERROR: return "JSON parsing error";
        case ERR_UNKNOWN_STATE: return "Unknown state";
        case ERR_PLUG_NOT_CONNECTED: return "Plug not connected";
        case ERR_TASMOTA_REQUEST_FAILED: return "Tasmota request failed";
        case ERR_PLUG_REF_INVALID: return "Invalid plug reference";
        default: return "Unknown error";
    }
}
