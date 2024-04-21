#ifndef DEBUGOUTPUT_H
#define DEBUGOUTPUT_H

#include <Arduino.h>
#include <stdarg.h>

class DebugOutput {
private:
    uint8_t verbosityLevel;
    Stream* outputStream;

public:
    // Constructor
    DebugOutput();

    // Begin method to set up the logging system
    void begin(uint8_t level, Stream& stream = Serial);

    // Methods for printing messages
    void info(const char* format, ...);
    void debug(const char* format, ...);

private:
    // Helper method for processing variadic arguments
    void printFormatted(const char* format, va_list args);
};

#endif // DEBUGOUTPUT_H

