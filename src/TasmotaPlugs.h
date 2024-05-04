#ifndef TASMOPLUGS_H
#define TASMOPLUGS_H

#include <vector>
#include <string>
#include <Arduino.h>
#include "Config.h" 
#include "DebugOutput.h"

struct PlugState {
    int ip_octet;         // Last octet of the IP address for the plug
    int sub_plug_index;   // Index of the plug at this IP address, used in SPI mode
    int pin;              // ESP digital pin number, used in pin control mode
    int pinState;         // Current state of the pin (HIGH or LOW), relevant in pin mode
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
    void begin(DebugOutput& Logger );
    void initPlugStates();
    int getPlugState(int ipIndex, int subPlugIndex) ;
    int getPlugState(const std::string& url);
    int setPlugState(int ipIndex, int subPlugIndex, bool state) ;
    int setPlugState(const std::string& url, bool state);
    int getRSSI(int ipIndex, int subPlugIndex);
    int getEnergyValues(int ipIndex, int subPlugIndex, EnergyValues& values);
    static const char* getErrorString(int errorCode);
    std::string getIPAddress(PlugState& plug); 
    void showPlugConfiguration();

    std::vector<std::vector<PlugState>> plugs;// Vector of all plug states managed by this class
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
    std::string ip_base_url = "http://192.168.4."; // Default base URL
    std::string full_ip_address;               // Temporary storage for the full IP address
    static constexpr int defaultPinState = -1; // Default state for pins if no configuration is available
    DebugOutput log; 

};

#endif // TASMOPLUGS_H
