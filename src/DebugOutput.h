#ifndef DEBUG_OUTPUT_H
#define DEBUG_OUTPUT_H

#include <iostream>

// Uncomment the following line to enable verbose output globally.
// #define VERBOSE_OUTPUT

class DebugOutput {
public:
    template<typename T>
    DebugOutput& operator<<(const T& msg) {
        #ifdef VERBOSE_OUTPUT
        std::cout << msg;
        #endif
        return *this;
    }

    // Specialization for std::ostream manipulators (e.g., std::endl)
    DebugOutput& operator<<(std::ostream& (*pf)(std::ostream&)) {
        #ifdef VERBOSE_OUTPUT
        std::cout << pf;
        #endif
        return *this;
    }
};

// Global debug object declaration
extern DebugOutput debug;

#endif // DEBUG_OUTPUT_H
