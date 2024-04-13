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

DebugOutput debug;  // for debug uncomment VERBOSE_OUTPUT in DebugOutput.h

TasmotaPlugs tasmotaPlugs;

static char _ssid[13];    // "plugAP" + 4 hex digits + null terminator
static char _password[12]; // "pass" + 4 hex digits + null terminator
bool apCreated = false;    // flag indicates acess point created

int setupWiFi() {
    if (!apCreated) {
        WiFi.mode(WIFI_MODE_APSTA);

        uint8_t mac[6];
        WiFi.softAPmacAddress(mac);
        snprintf(_ssid, sizeof(_ssid), "plugAP%02X%02X", mac[4], mac[5]);
        snprintf(_password, sizeof(_password), "pass%02X%02X", mac[4], mac[5]);

        //snprintf(_ssid, sizeof(_ssid), "plugAP46ED");
        //snprintf(_password, sizeof(_password), "pass46ED");


        if (!WiFi.softAP(_ssid, _password)) {
            Serial.println("Failed to create access point");
            return TasmotaPlugs::ERR_HTTP_REQUEST_FAILED; 
        } else {
            apCreated = true;
            Serial.printf("Access Point created with SSID: %s \n", _ssid);
        }
    }
    return TasmotaPlugs::RET_SUCCESS; 
}

void checkPinState(PlugState* plug, int index) {
    int currentPinState = digitalRead(plug->pin);
    if (currentPinState != plug->pinState) {
        int currentPlugState = tasmotaPlugs.getPlugState(plug);
        //debug << "Current state of plug on pin " << (int)plug->pin << " with MAC " <<  tasmotaPlugs.plugMac_4[index] << " is " << currentPlugState << "\n";

        if (currentPlugState >= 0) {
            if ((currentPinState == HIGH && currentPlugState != 1) || 
                (currentPinState == LOW && currentPlugState != 0)) {
                if(currentPinState == HIGH )   
                   Serial.printf("Pin %d went HIGH, Sending 'Power On' command.\n", plug->pin );
                else
                   Serial.printf("Pin %d went LOW, Sending 'Power Off' command.\n", plug->pin);   
                tasmotaPlugs.setPlugState(plug, currentPinState == HIGH);
            }
            plug->pinState = currentPinState; // Update pinState
        } else {
            Serial.print("Error getting plug status: ");
            Serial.println(tasmotaPlugs.getErrorString(currentPlugState));
        }
    }
}


void processConfigUpdate(String newConfig){
    Serial.printf("new config is [%s]\n", newConfig.c_str());
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
    Serial.begin(115200);
    Serial.println("Starting");
    while (setupWiFi() != TasmotaPlugs::RET_SUCCESS) {
        Serial.println("Retrying WiFi startup sequence");
    }
    Serial.print("Access Point started, IP Address: "); 
    Serial.println(WiFi.softAPIP());

    if (!LittleFS.begin()) {
        Serial.println("An Error has occurred while mounting LittleFS");
    }
    delay(1000);
    tasmotaPlugs.begin();
    tasmotaPlugs.config.printConfig();
    Serial.flush();
    for(int i = 0; i < tasmotaPlugs.plugs.size(); i++) {
      pinMode(tasmotaPlugs.plugs[i].pin, INPUT_PULLDOWN);  // Initialize sense pin as input with pull-down resistor
    }
    delay(100);
}

void loop() {
    if (WiFi.softAPgetStationNum() > 0) {
        // here if one or more stations are connected to this access point
        for(int index = 0; index < tasmotaPlugs.plugs.size(); index++) {
            // process any pin state change for configured smartplugs
            checkPinState(&tasmotaPlugs.plugs[index], index);
        }
    }
    checkSerialEvents();

}
