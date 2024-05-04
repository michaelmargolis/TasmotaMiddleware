#include "TasmotaPlugs.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Config.h"

TasmotaPlugs::TasmotaPlugs() {
    // Constructor body, if needed
}

void TasmotaPlugs::begin(DebugOutput& Logger) {
    log = Logger;
    // Load configuration from file system
    if (!config.loadConfig()) {
        log.info("Failed to load configuration!\n");
        return;
    }

    // Initialize plug states based on loaded configuration
    initPlugStates();

    // Optionally print configuration data for debugging
    showPlugConfiguration();
}

void TasmotaPlugs::showPlugConfiguration() {
    for (size_t ipIndex = 0; ipIndex < plugs.size(); ++ipIndex) {
        for (size_t subPlugIndex = 0; subPlugIndex < plugs[ipIndex].size(); ++subPlugIndex) {
            PlugState &plug = plugs[ipIndex][subPlugIndex];
            log.info("Device at IP %s%d has index %zu, subIndex %zu\n",  ip_base_url.c_str(), plug.ip_octet, ipIndex, subPlugIndex);
        }
    }
    log.info("\n");  
}

void TasmotaPlugs::initPlugStates() {
    plugs.clear();
    for (size_t ipIndex = 0; ipIndex < config.plug_ip.size(); ++ipIndex) {
        std::vector<PlugState> subPlugs;
        for (int subPlugIndex = 0; subPlugIndex < config.plugs_per_ip[ipIndex]; ++subPlugIndex) {
            PlugState newPlug;
            newPlug.ip_octet = config.plug_ip[ipIndex];
            newPlug.sub_plug_index = subPlugIndex;
            newPlug.pin = config.esp_pin_map[ipIndex];
            newPlug.pinState = defaultPinState;
            subPlugs.push_back(newPlug);
        }
        plugs.push_back(subPlugs);
    }
}

int TasmotaPlugs::getPlugState(int ipIndex, int subPlugIndex) {
    if (ipIndex >= plugs.size() || subPlugIndex >= plugs[ipIndex].size()) {
        return ERR_PLUG_REF_INVALID;
    }
    PlugState* plug = &plugs[ipIndex][subPlugIndex];
    return getPlugState(getIPAddress(*plug));
}

int TasmotaPlugs::getPlugState(const std::string& url) {
    log.debug("getting state for plag at %s\n", url.c_str());
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

int TasmotaPlugs::setPlugState(int ipIndex, int subPlugIndex, bool state) {
    if (ipIndex >= plugs.size() || subPlugIndex >= plugs[ipIndex].size()) {
        return ERR_PLUG_REF_INVALID;
    }
    PlugState* plug = &plugs[ipIndex][subPlugIndex];
    return setPlugState(getIPAddress(*plug), state);
}

int TasmotaPlugs::setPlugState(const std::string& url, bool state) {
    HTTPClient http;
    std::string command = url + (state ? "/cm?cmnd=Power%20On" : "/cm?cmnd=Power%20Off");
    http.begin(command.c_str());
    int httpCode = http.GET();
    http.end();
    return (httpCode == HTTP_CODE_OK) ? RET_SUCCESS : ERR_TASMOTA_REQUEST_FAILED;
}

 int TasmotaPlugs::getRSSI(int ipIndex, int subPlugIndex){
    PlugState* plug = &plugs[ipIndex][subPlugIndex];
 
    HTTPClient http;
    std::string command = getIPAddress(*plug) + "/cm?cmnd=Status%2011";
    http.begin(command.c_str());
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        return ERR_HTTP_REQUEST_FAILED;
    }

    StaticJsonDocument<600> doc;
    DeserializationError error = deserializeJson(doc, http.getString());
    http.end();
    if (error) {
        log.error("DeserializationError: %s\n", error.c_str());
        log.error("heap free = %ld\n", ESP.getMaxAllocHeap());
        return ERR_JSON_ERROR;
    }

    return doc["StatusSTS"]["Wifi"]["RSSI"]; // Assuming RSSI is directly accessible and valid
}

 int TasmotaPlugs::getEnergyValues(int ipIndex, int subPlugIndex, EnergyValues& values) {
    PlugState* plug = &plugs[ipIndex][subPlugIndex];
    HTTPClient http;
    std::string command = getIPAddress(*plug) + "/cm?cmnd=Status%2010";
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

std::string TasmotaPlugs::getIPAddress(PlugState& plug) {
    // Construct the full URL using the ip_octet from the plug
    full_ip_address = ip_base_url + std::to_string(plug.ip_octet);
    return full_ip_address;
}
