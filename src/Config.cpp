#include "Config.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

bool Config::loadConfig() {
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount file system");
        return false;
    }

    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("Failed to open config file for reading");
        return false;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        Serial.println("Failed to parse config file: " + String(error.c_str()));
        return false;
    }

    SPI_controlMode = doc["SPI_controlMode"];
    JsonArray esp_pin_map_json = doc["esp_pin_map"];
    JsonArray plug_ip_json = doc["plug_ip"];
    JsonArray plug_mac4_json = doc["plug_mac4"];

    esp_pin_map.clear();
    plug_ip.clear();
    plugMac_4.clear();

    for (int pin : esp_pin_map_json) {
        esp_pin_map.push_back(pin);
    }
    for (int ip : plug_ip_json) {
        plug_ip.push_back(ip);
    }
    for (const char* mac : plug_mac4_json) {
        plugMac_4.push_back(std::string(mac));
    }

    return true;
}

void Config::writeConfigToStream(const std::string& ssid, Stream &outputStream) {
    if (esp_pin_map.empty()) {
        if (!loadConfig()) {
            outputStream.println("Failed to load config for streaming");
            return;
        }
    }

    StaticJsonDocument<1024> doc;
    JsonObject root = doc.to<JsonObject>();
    root["plugApMac4"] = ssid;  // Add SSID to JSON object
    root["SPI_controlMode"] = SPI_controlMode;

    JsonArray esp_pin_map_json = root.createNestedArray("esp_pin_map");
    JsonArray plug_ip_json = root.createNestedArray("plug_ip");
    JsonArray plug_mac4_json = root.createNestedArray("plug_mac4");

    for (int pin : esp_pin_map) {
        esp_pin_map_json.add(pin);
    }
    for (int ip : plug_ip) {
        plug_ip_json.add(ip);
    }
    for (const std::string& mac : plugMac_4) {
        plug_mac4_json.add(mac.c_str());
    }

    serializeJson(doc, outputStream);
    outputStream.println();
}

bool Config::readConfigFromStream(Stream& inputStream) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, inputStream);

    if (error) {
        Serial.println("Failed to parse JSON from stream: " + String(error.c_str()));
        return false;
    }

    return saveConfig();
}

bool Config::saveConfig() {
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return false;
    }

    StaticJsonDocument<1024> doc;
    JsonObject root = doc.to<JsonObject>();
    root["SPI_controlMode"] = SPI_controlMode;

    JsonArray esp_pin_map_json = root.createNestedArray("esp_pin_map");
    JsonArray plug_ip_json = root.createNestedArray("plug_ip");
    JsonArray plug_mac4_json = root.createNestedArray("plug_mac4");

    for (int pin : esp_pin_map) {
        esp_pin_map_json.add(pin);
    }
    for (int ip : plug_ip) {
        plug_ip_json.add(ip);
    }
    for (const std::string& mac : plugMac_4) {
        plug_mac4_json.add(mac.c_str());
    }

    if (serializeJson(doc, configFile) == 0) {
        Serial.println("Failed to write to config file");
        configFile.close();
        return false;
    }

    configFile.close();
    return true;
}

void Config::printConfig() {
    Serial.println("Configuration Data:");
    Serial.print("SPI Control Mode: ");
    Serial.println(SPI_controlMode ? "Enabled" : "Disabled");

    Serial.println("ESP Pin Map:");
    for (auto pin : esp_pin_map) {
        Serial.println(pin);
    }

    Serial.println("Plug IP Octets:");
    for (auto ip : plug_ip) {
        Serial.println(ip);
    }

    Serial.println("Plug MAC Addresses:");
    for (auto& mac : plugMac_4) {
        Serial.println(mac.c_str());
    }
}
