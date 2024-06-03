#include "TLE75008_ESD.h"

// TLE75008-ESD Registers
#define OUT_REGISTER   0x00
#define MAPIN0_REGISTER 0x01
#define MAPIN1_REGISTER 0x02
#define INST_REGISTER 0x06
#define DIAG_IOL_REGISTER 0x08
#define DIAG_OSM_REGISTER 0x09
#define HWCR_REGISTER 0x0C
#define HWCR_OCL_REGISTER 0x0D

// TLE75008-ESD Commands
#define READ_COMMAND  0x01
#define WRITE_COMMAND 0x80

TLE75008_ESD::TLE75008_ESD(uint8_t cs_pin, uint8_t idle_pin) {

  _cs_pin = cs_pin;
  _idle_pin = idle_pin;

}

void TLE75008_ESD::begin() {

  // Initialize the SPI bus and chip select pin
  pinMode(_cs_pin, OUTPUT);
  pinMode(_idle_pin, OUTPUT);
  digitalWrite(_cs_pin, HIGH);
  digitalWrite(_idle_pin, LOW);  // Enter Limp Home mode initially

  SPI.begin();
  SPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE1));

  // Initialize the TLE75008-ESD chip
  initialize();
}

void TLE75008_ESD::initialize() {
  // Ensure the chip is not in Limp Home mode
  digitalWrite(_idle_pin, HIGH);
  delay(1);  // Allow time for the mode transition
  
  // Clear any latched errors
  writeRegister(HWCR_OCL_REGISTER, 0xFF);
  
  // Configure input mapping registers
  writeRegister(MAPIN0_REGISTER, 0x04);  // Map IN0 to channel 2 (default)
  writeRegister(MAPIN1_REGISTER, 0x08);  // Map IN1 to channel 3 (default)
  
  // Enable open load diagnostic current
  writeRegister(DIAG_IOL_REGISTER, 0xFF);
  
  // Set the hardware configuration register (optional based on application)
  writeRegister(HWCR_REGISTER, 0x01);  // Set the device to active mode

  // Configure necessary registers to enable diagnostics
  writeRegister(DIAG_OSM_REGISTER, 0xFF); // Enable output status monitoring
}

void TLE75008_ESD::toggleOutput(byte channel, bool state) {

  channel = channel - 1;

  if (channel > 7) return;  // Channel out of range

  byte currentOutputState = readRegister(OUT_REGISTER);
  if (state) {
    currentOutputState |= (1 << channel);  // Set bit to 1
  } else {
    currentOutputState &= ~(1 << channel); // Set bit to 0
  }
  writeRegister(OUT_REGISTER, currentOutputState);
}

// Diagnostic Functions
bool TLE75008_ESD::getOverloadStatus(byte channel) {

  channel = channel - 1;

  if (channel > 7) return false;  // Channel out of range
  byte status = readRegister(INST_REGISTER);
  return (status & (1 << channel)) != 0;
}

bool TLE75008_ESD::getOpenLoadStatus(byte channel) {

  channel = channel - 1;

  if (channel > 7) return false;  // Channel out of range
  byte status = readRegister(DIAG_OSM_REGISTER);
  return (status & (1 << channel)) != 0;
}

bool TLE75008_ESD::getOutputStatusMonitor(byte channel) {
    if (channel > 7) return false;  // Channel out of range
    byte status = readRegister(DIAG_IOL_REGISTER);
    return (status & (1 << channel)) != 0;
}

void TLE75008_ESD::writeRegister(byte reg, byte value) {
  digitalWrite(_cs_pin, LOW);
  SPI.transfer(WRITE_COMMAND | reg);
  SPI.transfer(value);
  digitalWrite(_cs_pin, HIGH);
}

byte TLE75008_ESD::readRegister(byte reg) {
  digitalWrite(_cs_pin, LOW);
  SPI.transfer(READ_COMMAND | reg);
  byte result = SPI.transfer(0x00);
  digitalWrite(_cs_pin, HIGH);
  return result;
}