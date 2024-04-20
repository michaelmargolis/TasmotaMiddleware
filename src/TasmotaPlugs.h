#ifndef TASMOPLUGS_H
#define TASMOPLUGS_H

#include <vector>
#include <string>
#include <Arduino.h>
#include "Config.h"  // Include the Config class to handle configuration tasks

struct PlugState {
    int pin;        // GPIO pin number used to control this plug
    int ip_octet;   // Last octet of the IP address for the plug
    int pinState;   // Current state of the pin (HIGH or LOW)
};

struct EnergyValues {
  float Voltage;     // Volts
  float Current;     // Amps
  float Power;       // Watts 
  float Yesterday;   // kWatt hours
  float Today;       // kWatt hours   
  float Total;       // kWatt hours   
};

class TasmotaPlugs {
public:
    TasmotaPlugs();
    void begin();
    void initPlugStates();
    int getPlugState(PlugState* plug);
    int getPlugState(const std::string& url);
    int setPlugState(PlugState* plug, bool state);
    int setPlugState(const std::string& url, bool state);
    int getRSSI(const std::string& url);
    int getEnergyValues(const std::string& url, EnergyValues& values);
    static const char* getErrorString(int errorCode);

    std::vector<PlugState> plugs; // Vector of all plug states managed by this class
    Config config;  // Configuration object to manage config data

    // Constants for operation status codes
    static constexpr int RET_SUCCESS = 0;
    static constexpr int ERR_URL_PREPARATION_FAILED = -1;
    static constexpr int ERR_HTTP_REQUEST_FAILED = -2;
    static constexpr int ERR_UNHANDLED_CASE = -3;
    static constexpr int ERR_JSON_ERROR = -101;
    static constexpr int ERR_UNKNOWN_STATE = -102;
    static constexpr int ERR_PLUG_NOT_CONNECTED = -103;
    static constexpr int ERR_TASMOTA_REQUEST_FAILED = -104;
    static constexpr int ERR_PLUG_REF_INVALID = -105;

private:
    void initializeSPIForPlugs(); // Initializes SPI if SPI control mode is enabled
    
    std::string ip_base_url = "http://192.168.4."; // Default base URL
    static constexpr int defaultPinState = -1; // Default state for pins if no configuration is available

};

#endif // TASMOPLUGS_H
