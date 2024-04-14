/*
ArduinoGatewayTest.inno

Test sketch assumes LDR sensors are connected on analog pins 0 and 4
with respective outputs on digital pins 11 and 12

Connect ICSP pins from Arduino to Gatway
Note orientation of cable to ensure +5 volt pins connected together. 
*/
 

class LDRSensor {
public:
    int analogPin;        // Analog pin where the LDR is connected
    int controlPin;       // Digital pin to control (e.g., an LED or a relay)
    int maxReading;       // Maximum light reading, adjusted dynamically
    int minReading;       // Minimum light reading, adjusted dynamically
    int threshold;        // Light threshold for switching the control pin
    float thresholdPercent; // Percentage to calculate threshold between min and max
    int hysteresis;       // Hysteresis margin to avoid flickering
    bool lastState;       // Last state of the control pin

    LDRSensor(int analogPin, int controlPin) : analogPin(analogPin), controlPin(controlPin), maxReading(0), minReading(60), thresholdPercent(0.5), hysteresis(50), lastState(false) {}

    void begin() {
        pinMode(controlPin, OUTPUT);
        int initialReading = analogRead(analogPin);
        maxReading = initialReading;  // Set initial maximum reading
        minReading = initialReading < minReading ? initialReading : minReading;  // Set initial minimum reading if below default
        updateThreshold(); // Initialize threshold based on initial readings
        Serial.begin(9600);    // Start serial communication at 9600 bps
    }

    // Update threshold based on current max and min readings
    void updateThreshold() {
        threshold = minReading + (int)((maxReading - minReading) * thresholdPercent);
    }

    // Update sensor state and control the digital output based on light level
    void update() {
        int reading = analogRead(analogPin);

        // Dynamically adjust max and min readings
        if (reading > maxReading) {
            maxReading = reading;
            updateThreshold();
        } else if (reading < minReading) {
            minReading = reading;
            updateThreshold();
        }

        bool newState;
        if (reading < threshold - hysteresis) {
            newState = true;  // Turn on the control pin if below lower threshold
        } else if (reading > threshold + hysteresis) {
            newState = false; // Turn off the control pin if above upper threshold
        } else {
            newState = lastState;  // Maintain the last state if within the hysteresis band
        }

        if (newState != lastState) {
            digitalWrite(controlPin, newState ? HIGH : LOW);
            lastState = newState;
            // Print the sensor reading and new state to the serial monitor
            Serial.print("Sensor on pin ");
            Serial.print(analogPin);
            Serial.print(": Reading = ");
            Serial.print(reading);
            Serial.print(", New State = ");
            Serial.println(newState ? "ON" : "OFF");
        }
    }
};

LDRSensor sensor1(0, 11);  // LDR is connected to A0 and control pin is 11
LDRSensor sensor2(4, 12);  // LDR is connected to A4 and control pin is 12

void setup() {
    Serial.begin(9600);    // Start serial communication at 9600 bps
    sensor1.begin();       // Initialize sensor 1
    sensor2.begin();       // Initialize sensor 2
}

void loop() {
    sensor1.update();      // Update sensor 1 state
    sensor2.update();      // Update sensor 2 state
    //Serial.print(analogRead(0));   Serial.print(", "); Serial.println(analogRead(4)); // uncomment to show sensor readings
    delay(100); 
}
