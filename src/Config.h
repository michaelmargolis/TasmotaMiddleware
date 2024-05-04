#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <vector>
#include <string>
#include <Stream.h>
#include "FS.h"

class Config {
public:
    bool loadConfig();
    void writeConfigToStream(const std::string& ssid, Stream &outputStream = Serial);
    bool readConfigFromStream(Stream& inputStream);
    bool saveConfig();
    void printConfig();

    std::vector<int> esp_pin_map;
    std::vector<int> plug_ip;
    std::vector<int> plugs_per_ip;

private:
    bool internalLoadConfig();
    bool internalSaveConfig();
};

#endif // CONFIG_H
