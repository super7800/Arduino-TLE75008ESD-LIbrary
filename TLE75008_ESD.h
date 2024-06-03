#ifndef TLE75008_ESD_H
#define TLE75008_ESD_H

#include <Arduino.h>
#include <SPI.h>

class TLE75008_ESD {
public:
    TLE75008_ESD(uint8_t cs_pin, uint8_t idle_pin);
    void begin();
    void toggleOutput(byte channel, bool state);  // Method to set output ON or OFF

    // Diagnostic Functions
    bool getOverloadStatus(byte channel);
    bool getOpenLoadStatus(byte channel);
    bool getOutputStatusMonitor(byte channel);

private:
    uint8_t _cs_pin;
    uint8_t _idle_pin;
    void initialize();
    void writeRegister(byte reg, byte value);
    byte readRegister(byte reg);
};

#endif