# Arduino controlled middleware for Tasmota smartplugs

## Overview

This project enables an Arduino sketch to control mains power through wireless conectivity with open source smartplugs using Tasmota protocol. The software in this repository is middleware running on an ESP32  that communicates with the controlling Arduino and one or two Tasmota smart plugs that can be associated with an arduino pin that when set HIGH turns the plug on, and off when LOW.<br>
An API is also available to control many smartplugs as well as reporting on power and voltage statistics provided by the smartplug. 

![Example wiring](hardware/uno_i2c_wiring.jpg)
In the example above, Arduino is connected to an ESP32 using logic level translator. Arduino pin A5 used as a digital output turns a plug on when HIGH, off when LOW. Pin A4 controls the plug on ip address 192.168.4.13.  Arduino pin A5 ontrols the plug on ip address 192.168.4.12. Note the level translator is not required when connected to 3.3 volt Arduino boards.

The diagram below shows The PCB that supports additional features such as reading plug energy usage and wireless signal strength. As many plugs can be controlled as can be connected to the ESP32 access point. 
![Example wiring using Tasmota library](hardware/Gateway_PCB_i2c.jpg)  

## Functional Overview

The middleware transforms the ESP32 into an access point, facilitating connections from Tasmota smart plugs. The SSID follows the pattern `plugAPxxxx`, utilizing the last four digits of the ESP32's MAC address. Upon becoming an access point, the middleware accepts connections from Tasmota smart plugs and responds to changes in Arduino pin states by sending appropriate HTTP commands to control the smart plugs' power.

### Pairing Middleware with Smart Plugs

New plugs (or plugs reset to modify configuration) must be paired using the Tasmota setup procedure, connecting them to the `plugAPxxxx` SSID broadcasted by the ESP32 middleware.

## Development Environment Setup

### Install VS Code and Platformio


### Cloning the Repository

Clone the repository to your local machine using: 

```
git clone https://github.com/michaelmargolis/TasmotaMiddleware
```

### Opening in PlatformIO

Open the cloned project folder in PlatformIO, available as a VS Code extension. This environment supports extensive library management and compatibility with various development boards.

## Configuring the Middleware

### Editing `config.json`

The `data` directory contains `config.json`, a template for your smart plug configuration. Customize it with the MAC addresses of your smart plugs and the corresponding control pins on the ESP32. Note these pin numbers refer to the ESP32, you can connect the Arduino pin of your choice to this pin. IMPORTANT - the ESP32 is not 5v tolerant, use voltage level translation with 5 volt Arduinos. 

In the example below, plugs at IP address 192.168.4.13 and 12 will be controlled by setting ESP pins 6 and 7 (unless the I2C jumper is set) 

```json
{
  "plug_ip": [13,12],
  "plugs_per_ip":[1,1],
  "esp_pin_map": [6,7]
}
```

### Uploading `config.json` to ESP32
PlatformIo will auto detect the USB serial port if a single device is connected. If the correct ESP is not auto detected you can specify a com port in the platformio.ini file by uncommenting  'upload_port = xxxx' and entering the correct port

With the ESP32 USB port connected to the computer running Platformio, PlatformIO's "Upload File System Image" task to upload your configuration file to the ESP32.

To do this:
  - Click:  PlatformIO (the 'alien head') icon in the sidebar
  - Under PROJECT TASKS, Click: esp32-c3 (assuming you are using the XIAO ESP32-C3)
  - Click: Upload Filesystem Image

This step is crucial for the middleware to recognize and control your smart plugs. Missing or incorrect config.json data is the most common cause of unresponsive plugs.

### Compiling and uploading the sketch to ESP32
Compile and upload by clicking the right-arrow icon on the lower toolbar. Other useful icons on this toolbar are:
  - checkmark - complile (but don't upload)
  - trashcan - clean (deletes compiled objects when a complete rebuild is necessary, such as after modifying build flags) 

## Pairing Middleware with Smart Plugs

### New Plugs

1. **Initial Connection**: Connect a computer or smartphone to the `tasmota-xxxx` SSID provided by the smart plug upon powering up.
2. **Activate Controller Access Point**: Power up the ESP32 controller to broadcast the `plugAPxxxx` SSID.
3. **Select Controller SSID**: Ensure visibility of the intended controller's SSID and connect to it.
4. **Configure Wi-Fi Credentials on the Plug**:
    - Navigate to `http://192.168.4.1` using a web browser.
    - In the Tasmota interface, set the SSID to `plugAPxxxx` and the password to `passxxxx`, corresponding to the controller's MAC address.
    - Ensure correctness of the case-sensitive SSID and password.
    - Save and allow the plug to restart and connect to the controller's SSID.
    - Note the IP address assigned by the DHCP server, this will be changed to a static IP address in the next step. 
5. **Configure the plug's IP address**:
    - Switch the computer's Wi-Fi to the controller's SSID ('PlugAPxxxx').
    - Access the plug's homepage (`http://192.168.4.2` or as indicated) and navigate to the Information section to note the last four digits of the plug's MAC address.
    - Click the Console button on the home page and enter the following commands:
      - type: ipaddress1 1.192.168.4.xxx (where xxx is the ip octet identifying this plug, this octet value and the MAC digits will be entered into the config.json)
      - type: restart 1 (this will apply the change and restart the plug)
      - navigate your browswer to the new ip address to verify the change
6. **Label the plug**
    - Mark the plug with the plug's MAC suffix and the last IP octet as well as the controller's SSID for easy identification.


## Usage

See Using the Tasmota gateway [here](hardware/Tasmota%20Gateway%20Getting%20Started.pdf)

## Contributing

Contributions are welcome. Please fork the repository, make your changes, and submit a pull request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
