#include <Arduino.h>
#include <Wire.h>
#include "TasmotaPlugs.h"
#include "DebugOutput.h"


constexpr int8_t PRIMARY_I2C_ADDR = 0X35;
constexpr int8_t SECONDARY_I2C_ADDR = 0X55;

// API states
enum State {
    ReadyForCmd,
    ReadyForService,
    ReadyForReply,
    PayloadReady,  // State when ready to send detailed payload data
};

class I2cInterface {
private:
    static I2cInterface* instance;  // Static instance pointer

    static TasmotaPlugs* plugPtr;  // Pointer to a TasmotaPlugs instance
    static DebugOutput* logPtr;
    static byte deviceAddress; 
    static byte commandBuffer[3];
    static volatile State currentState;
    static int8_t lastCompletionCode;
    static EnergyValues lastValues;

public:
    I2cInterface() {
        instance = this;  // Set the instance pointer
    }

    ~I2cInterface() {
        instance = nullptr;  // Clear the instance pointer
    }

    // API Return codes 
    static constexpr int8_t RET_SUCCESS = 0;
    static constexpr int8_t ERR_URL_PREPARATION_FAILED = -1;
    static constexpr int8_t ERR_HTTP_REQUEST_FAILED = -2;
    static constexpr int8_t ERR_UNHANDLED_CASE = -3;
    static constexpr int8_t ERR_JSON_ERROR = -101;
    static constexpr int8_t ERR_UNKNOWN_STATE = -102;
    static constexpr int8_t ERR_PLUG_NOT_CONNECTED = -103;
    static constexpr int8_t ERR_TASMOTA_REQUEST_FAILED = -104;
    static constexpr int8_t ERR_PLUG_REF_INVALID = -105;
    static constexpr int8_t ERR_I2C_TIMEOUT = -106;
    static constexpr int8_t ERR_UNKNOWN_COMMAND = -107;
    static constexpr int8_t ERR_BUSY = -108;

    static void begin(byte address, TasmotaPlugs& plugs, DebugOutput& logger) {
        deviceAddress = address;
        plugPtr = &plugs;
        logPtr = &logger;
        Wire.begin(deviceAddress);
        Wire.onReceive(receiveEvent);  // Register the receive event handler
        Wire.onRequest(requestEvent);  // Register the request event handler
        logPtr->info("I2C interface started at address 0X%x\n", deviceAddress);
        currentState = ReadyForCmd;
    }

    static void receiveEvent(int howMany) {
        if (Wire.available() == 3) {
            commandBuffer[0] = Wire.read();
            logPtr->debug("got cmd %c\n", commandBuffer[0]);
            if(validateCommand(commandBuffer[0])) {
                // only fill command buffer and change state if command is  valid
                for (int i = 1; i < 3; i++) {
                    commandBuffer[i] = Wire.read();
                }
                currentState = ReadyForService;
            }
        }
    }

    static bool validateCommand(byte cmd) {
       switch (cmd) {
            case 'H': 
            case 'L': 
            case 'R': 
            case 'E':
                return true;
            defualt: 
                return false;    
            }
        return false;    
    }
    
    static void requestEvent() {
        logPtr->debug("requestEvent in state: %d\n", currentState);   
        switch (currentState) {
            case ReadyForService:
                logPtr->debug("event requested in state: ReadyForService\n");
                Wire.write(ERR_BUSY);
                break; 
            case ReadyForReply:
                logPtr->debug("Completion Code: %d\n", lastCompletionCode);           
                Wire.write(lastCompletionCode);
                if (commandBuffer[0] == 'E' && lastCompletionCode == RET_SUCCESS) {
                    currentState = PayloadReady;  // Transition to payload ready state
                } else {
                    currentState = ReadyForCmd;  // Reset to idle after sending response
                }
                break;
            case PayloadReady:
                Wire.write(reinterpret_cast<const uint8_t*>(&lastValues), sizeof(lastValues));
                currentState = ReadyForCmd;  // Return to idle after sending the payload
                break;
            default:
                logPtr->error("unknown state in requstEvent: %d\n", currentState);
                Wire.write(ERR_UNKNOWN_STATE);  // Handle unexpected state by reporting an error
                break;
             }
    }
 
     void service() {
        if (currentState == ReadyForService) {
            char cmd = commandBuffer[0];
            int index = commandBuffer[1];
            int subIndex = commandBuffer[2];
            handleCmd(cmd, index, subIndex);
        }
    }

    static void handleCmd(char cmd, int index, int subIndex) {
        switch (cmd) {
            case 'H': lastCompletionCode = plugPtr->setPlugState(index, subIndex, true); break;
            case 'L': lastCompletionCode = plugPtr->setPlugState(index, subIndex, false); break;
            case 'R': lastCompletionCode = plugPtr->getRSSI(index, subIndex); break;
            case 'E': lastCompletionCode = plugPtr->getEnergyValues(index, subIndex, lastValues); break;
            default:
                logPtr->error("ERROR, unexpected command: %c\n, cmd");
                lastCompletionCode = ERR_UNKNOWN_COMMAND;
                currentState = ReadyForCmd; 
                return;   
        }
        currentState = ReadyForReply; 
    }

};

// Initialize the static member
I2cInterface* I2cInterface::instance = nullptr;
DebugOutput* I2cInterface::logPtr = nullptr;
TasmotaPlugs* I2cInterface::plugPtr = nullptr; 
byte I2cInterface::deviceAddress = PRIMARY_I2C_ADDR;  // Default value initialization
byte I2cInterface::commandBuffer[3] = {0};
volatile State I2cInterface::currentState = ReadyForCmd;  
int8_t I2cInterface::lastCompletionCode = ERR_UNKNOWN_STATE;
EnergyValues I2cInterface::lastValues = {};