#include <Wire.h>


const int8_t PRIMARY_I2C_ADDR = 0X35;
const int8_t SECONDARY_I2C_ADDR = 0X55;
    
struct EnergyValues {
  float Voltage;     
  float Current;     
  float Power;       
  float Yesterday;   
  float Today;       
  float Total;       
};

class TasmotaI2c {
private:
    byte deviceAddress;  // I2C address of the slave device
    unsigned long responseTimeout = 10000;  // Extend timeout to 10 seconds for HTTP requests

public:
    TasmotaI2c(byte address) : deviceAddress(address) {}

    void begin() {
        Wire.begin();  // Initialize I2C as master
        Wire.setClock(100000);  // Set I2C clock speed to 100 kHz
    }

    bool powerOn(int8_t ipIndex = 0, int8_t subPlugIndex = 0) {
        return checkCompletionCode(sendCommand('H', ipIndex, subPlugIndex));
    }

    bool powerOff(int8_t ipIndex = 0, int8_t subPlugIndex = 0) {
        return checkCompletionCode(sendCommand('L', ipIndex, subPlugIndex));
    }

    int8_t getRSSI(int8_t ipIndex = 0, int8_t subPlugIndex = 0) {
        return sendCommand('R', ipIndex, subPlugIndex);  // Directly return the RSSI or error code
    }

    EnergyValues getEnergyValues(int8_t ipIndex = 0, int8_t subPlugIndex = 0) {
        int8_t resultCode = sendCommand('E', ipIndex, subPlugIndex);
        if (resultCode == RET_SUCCESS) {  // Check if resultCode indicates success
            return retrieveEnergyValues();
        }
        return EnergyValues(); // Return empty struct if error code received
    }

private:
    int8_t sendCommand(char cmd, int8_t ipIndex, int8_t subPlugIndex) {
        Wire.beginTransmission(deviceAddress);
        Wire.write(cmd);
        Wire.write(ipIndex);
        Wire.write(subPlugIndex);
        Wire.endTransmission();

        // Use adaptive polling to wait for response
        return pollForResponse();
    }

    int8_t pollForResponse() {
        unsigned long startTime = millis();
        while (millis() - startTime < responseTimeout) {
            Wire.requestFrom((int)deviceAddress, 1);
            if (Wire.available()) {
                return Wire.read();  // Read the response
            }
            delay(100);  // Adaptive delay to allow slave time to process the request
        }
        return -100;  // Timeout error code
    }

    bool checkCompletionCode(int8_t resultCode) {
        if (resultCode < 0) {
            Serial.print("Error: ");
            Serial.println(resultCode);
            return false;
        }
        return true;
    }

    EnergyValues retrieveEnergyValues() {
        EnergyValues values;
        Wire.requestFrom((int)deviceAddress, sizeof(EnergyValues));
        if (Wire.available() == sizeof(EnergyValues)) {
            Wire.readBytes((char*)&values, sizeof(values));  // Read the data into the structure
        }
        return values;
    }
};
