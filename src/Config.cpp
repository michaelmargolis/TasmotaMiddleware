#include "Config.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "DebugOutput.h"

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

    JsonArray esp_pin_map_json = doc["esp_pin_map"];
    JsonArray plug_ip_json = doc["plug_ip"];
    JsonArray plugs_per_ip_json = doc["plugs_per_ip"];

    esp_pin_map.clear();
    plug_ip.clear();
    plugs_per_ip.clear();

    for (int pin : esp_pin_map_json) {
        esp_pin_map.push_back(pin);
    }
    for (int ip : plug_ip_json) {
        plug_ip.push_back(ip);
    }
    for (int count : plugs_per_ip_json) {
        plugs_per_ip.push_back(count);
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

    JsonArray esp_pin_map_json = root.createNestedArray("esp_pin_map");
    JsonArray plug_ip_json = root.createNestedArray("plug_ip");
    JsonArray plugs_per_ip_json = root.createNestedArray("plugs_per_ip");

    for (int pin : esp_pin_map) {
        esp_pin_map_json.add(pin);
    }
    for (int ip : plug_ip) {
        plug_ip_json.add(ip);
    }
    for (int count : plugs_per_ip) {
        plugs_per_ip_json.add(count);
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

    JsonArray esp_pin_map_json = root.createNestedArray("esp_pin_map");
    JsonArray plug_ip_json = root.createNestedArray("plug_ip");
    JsonArray plugs_per_ip_json = root.createNestedArray("plugs_per_ip");

    for (int pin : esp_pin_map) {
        esp_pin_map_json.add(pin);
    }
    for (int ip : plug_ip) {
        plug_ip_json.add(ip);
    }
    for (int count : plugs_per_ip) {
        plugs_per_ip_json.add(count);
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
    Serial.print("ESP Pin Map: ");
    for (auto pin : esp_pin_map) {
        Serial.print(pin); Serial.print(", "); 
    }

    Serial.print("\nPlug IP Octets: ");
    for (auto ip : plug_ip) {
        Serial.print(ip); Serial.print(", "); 
    }

    Serial.print("\nPlugs per IP Address: ");
    for (int count : plugs_per_ip) {
        Serial.print(count); Serial.print(", "); 
    }
    Serial.println("\n");
}
