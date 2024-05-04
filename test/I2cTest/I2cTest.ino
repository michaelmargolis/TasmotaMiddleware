#include <SPI.h>
#include "tasmotaI2c.h"

TasmotaI2c tasmota(PRIMARY_I2C_ADDR);  // Assuming the I2C address of the slave device is 0x66

void setup() {
  Serial.begin(9600);
  Serial.println("starting Tasmota I2C interface on "); Serial.println(PRIMARY_I2C_ADDR), HEX);
  tasmota.begin();
  // test();
  printInstructions();   // Print instructions for using the commands
}

void test(){
  tasmota.powerOn();  // Turn on the power
  delay(1000);           // Wait for 1 second
  tasmota.powerOff(); // Turn off the power
  uint8_t rssi = tasmota.getRSSI();
  Serial.print("RSSI: ");
  Serial.println(rssi);

  EnergyValues energy = tasmota.getEnergyValues();
  Serial.print("Energy - Voltage: ");
  Serial.print(energy.Voltage);
  Serial.print(", Current: ");
  Serial.print(energy.Current);
  Serial.print(", Power: ");
  Serial.println(energy.Power);
  
}

void loopx() {
   if (Serial.available() > 0) {
      char c = Serial.read();
      if(c == 'h'){
         int pin = Serial.parseInt();
         digitalWrite(pin, HIGH);
         Serial.print(pin); Serial.println(" set HIGH");
      }
      else if(c == 'l'){
         int pin = Serial.parseInt();
         digitalWrite(pin, LOW);
         Serial.print(pin); Serial.println(" set LOW");
      }
   }
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');  // Read the command from the serial monitor
    command.trim();  // Remove any extraneous whitespace or newlines

    if (command == "on") {
      int ret = tasmota.powerOn();
      Serial.println("Power turned on");
    } else if (command == "off") {
      tasmota.powerOff();
      Serial.println("Power turned off");
    } else if (command.startsWith("on ")) {
      int ipIndex = command.substring(3).toInt();
      tasmota.powerOn(ipIndex);
      Serial.println("Power turned on for device at index: " + String(ipIndex));
    } else if (command.startsWith("off ")) {
      int ipIndex = command.substring(4).toInt();
      tasmota.powerOff(ipIndex);
      Serial.println("Power turned off for device at index: " + String(ipIndex));
    } else if (command == "rssi") {
      uint8_t rssi = tasmota.getRSSI();
      Serial.println("RSSI Value: " + String(rssi));
    } else if (command == "energy") {
      EnergyValues values = tasmota.getEnergyValues();
      printEnergyValues(values);
    } else if (command == "help") {
      printInstructions();
    } else {
      Serial.println("Invalid command. Type 'help' for a list of commands.");
    }
  }
}

void printInstructions() {
  Serial.println("Welcome to the tasmota Control Interface!");
  Serial.println("Available commands:");
  Serial.println("  'on'       - Turn on the power for the first device");
  Serial.println("  'off'      - Turn off the power for the first device");
  Serial.println("  'on [n]'   - Turn on the power for the nth device");
  Serial.println("  'off [n]'  - Turn off the power for the nth device");
  Serial.println("  'rssi'     - Get the RSSI value for the first device");
  Serial.println("  'energy'   - Get energy values for the first device");
  Serial.println("Type 'help' to display this message again.");
}

void printEnergyValues(const EnergyValues& values) {
  Serial.println("Energy Values:");
  Serial.print("Voltage: "); Serial.print(values.Voltage); Serial.println(" V");
  Serial.print("Current: "); Serial.print(values.Current); Serial.println(" A");
  Serial.print("Power: "); Serial.print(values.Power); Serial.println(" W");
  Serial.print("Energy Yesterday: "); Serial.print(values.Yesterday); Serial.println(" kWh");
  Serial.print("Energy Today: "); Serial.print(values.Today); Serial.println(" kWh");
  Serial.print("Total Energy: "); Serial.print(values.Total); Serial.println(" kWh");
}
