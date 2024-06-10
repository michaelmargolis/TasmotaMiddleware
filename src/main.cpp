/*
   main.cpp file for the tasmota plug controller
   This module creates an ESP32 access point for communicating with a Tasmota smartplug
   This version sets the plug state to turn on and off depending on the HIGH or LOW state
   of an ESP32 input pin. Typically an Arduino sketch controls the plug by setting a pin
   connected to the ESP32.  Up to three seperate pins are monitored for independant
   control of three smartplugs.
   A config file on the ESP32 read at startup defines the MAC address of the plug and
   the pin number of the ESP32 control pin paired with that plug.

   Created 2 April 2024 Michael Margolis
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "LittleFS.h"
#include "DebugOutput.h"
#include "TasmotaPlugs.h"
#include "i2cInterface.h"


//#define  SPOOF_MAC "3341"  // if defined, overrides hardware MAC for testing fixed SSID 

static const int PRIMARY_I2C_ADDR_PIN = 21;    // jumper this pin low to use primary I2C address
static const int SECONDARY_I2C_ADDR_PIN  = 5;  // or jumper this pin low to use secondary I2C address
                                               // or if nether is low, use Pin Control

const int  VERBOSITY_LEVEL = 1; // -1 = no output, 0 = errors only,  1 = errors and info, 2 = errors, info and debug
DebugOutput logger;

TasmotaPlugs tasmotaPlugs;
I2cInterface i2cInterface;


static char _ssid[13];    // "plugAP" + 4 hex digits + null terminator
static char _password[12]; // "pass" + 4 hex digits + null terminator
bool apCreated = false;    // flag indicates acess point created
bool pinControl = false;   // false results in I2C control

int setupWiFi() {
    if (!apCreated) {
        WiFi.mode(WIFI_MODE_APSTA);

        #if defined SPOOF_MAC
          snprintf(_ssid, sizeof(_ssid), "plugAP%s", SPOOF_MAC);
          snprintf(_password, sizeof(_password), "pass%s", SPOOF_MAC);
        #else
          // use ESP hardware MAC
          uint8_t mac[6];
          WiFi.softAPmacAddress(mac);
          snprintf(_ssid, sizeof(_ssid), "plugAP%02X%02X", mac[4], mac[5]);
          snprintf(_password, sizeof(_password), "pass%02X%02X", mac[4], mac[5]);
        #endif 
        
        if (!WiFi.softAP(_ssid, _password)) {
            logger.info("Failed to create access point\n");
            return TasmotaPlugs::ERR_HTTP_REQUEST_FAILED; 
        } else {
            apCreated = true;
            logger.info("Access Point created with SSID: %s \n", _ssid);
        }
    }
    return TasmotaPlugs::RET_SUCCESS; 
}

void checkPinState(int ipIndex) {
    PlugState& plug = tasmotaPlugs.plugs[ipIndex][0];  // Pin mode assumes a single subindex
    if (plug.pin >= 0) {
        int currentPinState = digitalRead(plug.pin);

        // Check if the read state from the hardware pin differs from the stored state
        if (currentPinState != plug.pinState) {
            //logger.debug("Getting state of plug  at %s controlled by pin %d\n", tasmotaPlugs.getIPAddress(plug).c_str(), plug.pin);
            int currentPlugState = tasmotaPlugs.getPlugState(ipIndex, 0);
            //logger.debug("Current state of plug on pin %d is %d\n", plug.pin, currentPlugState);

            // Check if the state fetch was successful
            if (currentPlugState >= 0) {
                logger.debug("plug  at %s has state %d, pin %d has state %d\n", tasmotaPlugs.getIPAddress(plug).c_str(),
                          currentPlugState, plug.pin, currentPinState);
                // Determine if there's a mismatch between the physical pin state and the logical state
                if ((currentPinState == HIGH && currentPlugState != 1) ||
                    (currentPinState == LOW && currentPlugState != 0)) {

                    // change plug state to match logical state
                    if (currentPinState == HIGH) {
                        logger.info("Pin %d went HIGH, Sending 'Power On' command.\n", plug.pin);
                        tasmotaPlugs.setPlugState(ipIndex, 0, true);  // Corrected to use updated method signature
                    } else {
                        logger.info("Pin %d went LOW, Sending 'Power Off' command.\n", plug.pin);
                        tasmotaPlugs.setPlugState(ipIndex, 0, false);  // Corrected to use updated method signature
                    }
                    // Update the stored pin state to reflect the current physical state
                    plug.pinState = currentPinState;
                }
            } else {
                // log errors in fetching the current plug state
                logger.info("Error getting status for plug at %s: %s\n", 
                    tasmotaPlugs.getIPAddress(plug).c_str(), tasmotaPlugs.getErrorString(currentPlugState));
            }
        }
    } else {
        logger.error("Invalid pin number %d for plug at IP index %d, subindex 0\n", plug.pin, ipIndex);
        delay(1000);
    }
}

void processConfigUpdate(String newConfig){
    logger.info("new config is [%s]\n", newConfig.c_str());
}

void checkSerialEvents() {
    if (Serial.available()) {
        String incomingData = Serial.readStringUntil('\n'); // Read data until newline
        incomingData.trim(); // Trim any whitespace

        if (incomingData.indexOf("Probe") != -1) {
            tasmotaPlugs.config.writeConfigToStream(_ssid, Serial);
        }
        else if(incomingData.indexOf("config|") != -1) {
            processConfigUpdate(incomingData.substring(incomingData.indexOf("config|")));
        }
    }
}


void setup() {
   
    logger.begin(VERBOSITY_LEVEL);
    if(VERBOSITY_LEVEL >= 0)
      Serial.begin(115200);

    delay(2000);
    logger.info("Starting\n");

    while (setupWiFi() != TasmotaPlugs::RET_SUCCESS) {
        logger.info("Retrying WiFi startup sequence\n");
    }
    String ipString = WiFi.softAPIP().toString();
    logger.info("Access Point started, IP Address: %s\n", ipString.c_str());

    if (!LittleFS.begin()) {
        logger.info("An Error has occurred while mounting LittleFS\n");
    }
    delay(1000);
    tasmotaPlugs.begin(logger);
    tasmotaPlugs.config.printConfig();
    
    pinMode(PRIMARY_I2C_ADDR_PIN , INPUT_PULLUP);
    pinMode(SECONDARY_I2C_ADDR_PIN, INPUT_PULLUP);

    if(digitalRead(PRIMARY_I2C_ADDR_PIN) == LOW) {  
        i2cInterface.begin(PRIMARY_I2C_ADDR, tasmotaPlugs, logger); 
    }
    else if(digitalRead(SECONDARY_I2C_ADDR_PIN) == LOW) {  
        i2cInterface.begin(SECONDARY_I2C_ADDR, tasmotaPlugs, logger); 
    }
    else {
       // neither I2C jumper is enabled 
       logger.info("Pin control is enabled)\n");
       pinControl = true;
       size_t nbrPins = tasmotaPlugs.config.esp_pin_map.size();
       for(size_t i = 0; i <  nbrPins; i++){
            int pin = tasmotaPlugs.config.esp_pin_map[i];
            pinMode(pin, INPUT_PULLDOWN);
            logger.debug("Setting ESP pin %d to INPUT_PULLDOWN\n", pin);
       }
    }
    delay(100);
}

static int prevNbrStations = -1;

void loop() {
    int nbrStations = WiFi.softAPgetStationNum() ;
    if(nbrStations != prevNbrStations){ // report and store changes in found stations
        logger.info("Number of active stations is %d\n", nbrStations);
        prevNbrStations = nbrStations; 
    }
    if(true) { //nbrStations > 0) {  fixme
       // here if one or more stations are connected to this access point
       if(pinControl){
            for (size_t ipIndex = 0; ipIndex < tasmotaPlugs.plugs.size(); ++ipIndex) {
                // process any pin state change for configured smartplugs
                //logger.info("nbr plugs = %d, index = %d\n",tasmotaPlugs.plugs.size(), index);
                checkPinState(ipIndex);
            }
       }
       else{
           i2cInterface.service();
       }
    }
    // checkSerialEvents();

    delay(50);
}
