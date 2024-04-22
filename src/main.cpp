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
#include "TasmotaPlugs.h"
#include "DebugOutput.h"


#define  SPOOF_MAC "3341"  // overrides hardware MAC if defined 

const int VERBOSITY_LEVEL = 0; // 0 = no output, 1 = info only, 2 = info and debug
DebugOutput logger;

TasmotaPlugs tasmotaPlugs;

static char _ssid[13];    // "plugAP" + 4 hex digits + null terminator
static char _password[12]; // "pass" + 4 hex digits + null terminator
bool apCreated = false;    // flag indicates acess point created

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

void checkPinState(PlugState* plug, int index) {
    int currentPinState = digitalRead(plug->pin);
    if (currentPinState != plug->pinState) {
        int currentPlugState = tasmotaPlugs.getPlugState(plug);
        logger.debug("Current state of plug on pin %d is  %d\n",  plug->pin, currentPlugState);

        if (currentPlugState >= 0) {
            if ((currentPinState == HIGH && currentPlugState != 1) || 
                (currentPinState == LOW && currentPlugState != 0)) {
                if(currentPinState == HIGH )   
                   logger.info("Pin %d went HIGH, Sending 'Power On' command.\n", plug->pin );
                else
                   logger.info("Pin %d went LOW, Sending 'Power Off' command.\n", plug->pin);   
                tasmotaPlugs.setPlugState(plug, currentPinState == HIGH);
            }
            plug->pinState = currentPinState; // Update pinState
        } else {
            logger.info("Error getting plug status: %s\n", tasmotaPlugs.getErrorString(currentPlugState));
        }
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

int states[] = {-1,-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
void setupX() {
   Serial.begin(115200); 
   for(int i=0; i < 11; i++){
     pinMode(i, INPUT_PULLDOWN);
   }

   while(1){
      for(int i=0; i < 11; i++){
        int state = digitalRead(i);
        if(state != states[i]){
            Serial.printf("pin %d state changed to %d\n", i, state);
            states[i] = state;
        }         
      }
   }


}     
void setup() {
   
    logger.begin(VERBOSITY_LEVEL);
    if(VERBOSITY_LEVEL > 0)
      Serial.begin(115200);

    //delay(2000);
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

    for(int i = 0; i < tasmotaPlugs.plugs.size(); i++) {
      pinMode(tasmotaPlugs.plugs[i].pin, INPUT_PULLDOWN);  // Initialize sense pin as input with pull-down resistor
    }
    delay(100);
}

void loop() {
    if(WiFi.softAPgetStationNum() > 0) {
        // here if one or more stations are connected to this access point
        for(int index = 0; index < tasmotaPlugs.plugs.size(); index++) {
            // process any pin state change for configured smartplugs
            //logger.info("nbr plugs = %d, index = %d\n",tasmotaPlugs.plugs.size(), index);
            checkPinState(&tasmotaPlugs.plugs[index], index);
        }
    }
    // checkSerialEvents();

}
