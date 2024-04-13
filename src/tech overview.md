# Codebase Documentation Overview

## General Information
This codebase is designed to manage and control Tasmota smart plugs via an ESP32 module, which acts as an access point. It reads configurations from a JSON file, monitors pin states for controlling the plugs, and handles dynamic IP assignments. The functionality is split across several modules, each responsible for a distinct aspect of the system.

## File Descriptions

### `main.cpp`
**Purpose**: Serves as the main entry point of the application. It sets up the WiFi environment, initializes the filesystem and plug management, and contains the main loop that processes pin state changes and serial commands.

**Key Components**:
- `setupWiFi()`: Establishes a WiFi access point with a unique SSID derived from the ESP32's MAC address.
- `checkPinState()`: Monitors changes in pin states and sends commands to the smart plugs accordingly.
- `configEvent()`: Listens for the "Probe" command over serial and responds with the current configuration prepended by the SSID.

### `PlugNetworkManager.cpp`
**Purpose**: Manages network tasks such as IP resolution for plugs and generating command URLs based on device MAC addresses.

**Key Components**:
- `refreshIpMacMappings()`: Updates the IP-MAC mappings for all connected devices.
- `getConnectedStations()`: Retrieves a list of IP addresses for all devices currently connected to the ESP32 access point.
- `prepareCommandUrl()`: Constructs a URL for sending a command to a specific plug.

### `TasmotaPlugs.cpp`
**Purpose**: Handles the state management of each plug, including sending commands to control plug power states and initializing plug configurations.

**Key Components**:
- `begin()`: Initializes the plug states based on the loaded configuration.
- `getPlugState()`, `setPlugState()`: Get and set the power state of a plug.
- `initializeSPIForPlugs()`: Optionally initializes SPI for plug control, if enabled.

### `config.json`
**Structure**:
```json
{
  "SPI_controlMode": false,
  "smartplugs": [
    {
      "plugMac_4": "E946",
      "pin": 8
    },
    {
      "plugMac_4": "EA2D",
      "pin": 9
    }
  ]
}
