#include "DebugOutput.h"

DebugOutput::DebugOutput() : verbosityLevel(0), outputStream(nullptr) {}

void DebugOutput::begin(uint8_t level, Stream& stream) {
    this->verbosityLevel = level;
    this->outputStream = &stream;
}

void DebugOutput::info(const char* format, ...) {
    if (this->verbosityLevel >= 1) {
        va_list args;
        va_start(args, format);
        printFormatted(format, args);
        va_end(args);
    }
}

void DebugOutput::debug(const char* format, ...) {
    if (this->verbosityLevel >= 2) {
        va_list args;
        va_start(args, format);
        printFormatted(format, args);
        va_end(args);
    }
}

void DebugOutput::printFormatted(const char* format, va_list args) {
    if (this->outputStream != nullptr) {
        char buf[256];
        vsnprintf(buf, sizeof(buf), format, args);
        this->outputStream->print(buf);
    }
}
