#include "TLE75008_ESD.h"

/*********************************************************************************************************************
Arduino driver class for the Infineon TLE75008-ESD 8-channel low-side driver.

Version 1.1

Written By SUPER7800 2/27/2025

SPI Register Addresses (ADDR0, ADDR1)
datasheet section "10.6 SPI Registers Overview"
We combine them as: address = (ADDR0 << 2) | ADDR1

Features included:
- 16-bit SPI frame commands
- Full register read/write
- Output (channel) control
- Basic diagnostics (Overload, Open Load, etc.)
*********************************************************************************************************************/

static const uint8_t ADDR0_OUT       = 0x0; // OUT  (ADDR0=0000B, ADDR1=00B)
static const uint8_t ADDR0_MAPIN0    = 0x1; // MAPIN0
static const uint8_t ADDR0_MAPIN1    = 0x2; // MAPIN1
static const uint8_t ADDR0_INST      = 0x1; // INST has (ADDR0=0001B, ADDR1=10B) => combined 0x06
static const uint8_t ADDR0_DIAG_IOL  = 0x2; // DIAG_IOL
static const uint8_t ADDR0_DIAG_OSM  = 0x2; // DIAG_OSM (ADDR1=01 => combined 0x09)
static const uint8_t ADDR0_HWCR      = 0x3; // HWCR
static const uint8_t ADDR0_HWCR_OCL  = 0x3; // HWCR_OCL

// For convenience, we define combined addresses:
static const uint8_t REG_OUT        = ((ADDR0_OUT      << 2) | 0x0); // 0x00
static const uint8_t REG_MAPIN0     = ((ADDR0_MAPIN0   << 2) | 0x0); // 0x04
static const uint8_t REG_MAPIN1     = ((ADDR0_MAPIN1   << 2) | 0x1); // 0x09 (actually 0x08 if ADDR1=01?)
static const uint8_t REG_INST       = ((ADDR0_INST     << 2) | 0x2); // 0x06
static const uint8_t REG_DIAG_IOL   = ((ADDR0_DIAG_IOL << 2) | 0x0); // 0x08
static const uint8_t REG_DIAG_OSM   = ((ADDR0_DIAG_OSM << 2) | 0x1); // 0x09
static const uint8_t REG_HWCR       = ((ADDR0_HWCR     << 2) | 0x0); // 0x0C
static const uint8_t REG_HWCR_OCL   = ((ADDR0_HWCR_OCL << 2)| 0x1);  // 0x0D

// Helper macros for building read/write frames:
#define SPI_CMD_READ  0x4000u  // 01 as top bits => 0x40xx => 0b0100 0000 ...
#define SPI_CMD_WRITE 0x8000u  // 10 as top bits => 0x80xx => 0b1000 0000 ...

// Constructor
TLE75008_ESD::TLE75008_ESD(uint8_t csPin, uint8_t idlePin)
    : _csPin(csPin), _idlePin(idlePin)
{
}

// Begin function: sets up pins, configures SPI, and initializes the TLE75008-ESD.
void TLE75008_ESD::begin() {

    pinMode(_csPin,   OUTPUT);
    pinMode(_idlePin, OUTPUT);

    // Initially, ensure device is not driven from floating pins
    digitalWrite(_csPin,   HIGH);
    // Typically to “wake” device in normal operation, IDLE = HIGH => IDLE pin high = IDLE or ACTIVE mode
    // but let's put it HIGH first so we can do registers config
    digitalWrite(_idlePin, HIGH);
    delayMicroseconds(100);

    // Start Arduino SPI, up to 5 MHz if VDD > 4.5V. 
    // Mode: Data is latched on falling edge, shifted out on rising => CPOL=0, CPHA=1 => SPI_MODE1
    // The datasheet states: “Data is sampled in on the falling edge of SCLK, shifted out on rising edge” => that is Mode1.
    SPI.begin();
    SPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE1));
    // End the transaction.
    SPI.endTransaction();

    // Run device initialization steps
    initializeDevice();
}

// Internal init routine: clear latched errors, set device to Active, enable default config.
void TLE75008_ESD::initializeDevice() {

    // Clear any latched errors (HWCR_OCL register = 0xFF to clear all)
    clearAllErrors();

    // set input mapping to default (channel2->IN0, channel3->IN1)
    writeRegister(REG_MAPIN0, 0x04);  // default for channel 2
    writeRegister(REG_MAPIN1, 0x08);  // default for channel 3

    // Enable open load diagnostic current. Or only enable channels you want to check. 0xFF => all
    writeRegister(REG_DIAG_IOL, 0x00); // For example: 0x00 => disable all IOL for now

    // Set the HWCR register to normal settings. 
    // bit7=ACT=1 => always keep device in Active mode. 
    // bit6=RST =>0 => no SW reset
    // bits5..4 => reserved =>0
    // bits3..0 => PAR => 0 => no parallel channel linking
    // => 0x80 => 1000 0000b => but wait: the 8 bits is 0x80 => bit 7=1 => ACT=1
    writeRegister(REG_HWCR, 0x80);

    // 5) Turn OFF all outputs initially
    writeRegister(REG_OUT, 0x00);
}

// Put device fully into Sleep mode (IDLE pin LOW + no channels on).
void TLE75008_ESD::enterSleep() {

    // Turn off all outputs
    writeRegister(REG_OUT, 0x00);

    // Now drive IDLE pin LOW => Sleep mode
    digitalWrite(_idlePin, LOW);
    delayMicroseconds(100);  // give time to settle
}

// Put device in Active mode from Idle mode by setting IDLE=HIGH, plus writing HWCR.ACT=1
void TLE75008_ESD::enterActive() {

    // Drive IDLE pin HIGH
    digitalWrite(_idlePin, HIGH);
    delayMicroseconds(50);

    // Force HWCR.ACT=1
    // read current HWCR, set bit7, write back
    uint8_t hwcr = readRegister(REG_HWCR);
    hwcr |= 0x80; // set bit7
    writeRegister(REG_HWCR, hwcr);
}

// Read the 8-bit OUT register: which channels are ON or OFF.
uint8_t TLE75008_ESD::readOutputs() {
    return readRegister(REG_OUT);
}

// Write the 8-bit OUT register in one step: turns channels ON or OFF in parallel.
// mask  bit0 => channel0, bit1 => channel1, etc. 1=ON, 0=OFF
void TLE75008_ESD::writeOutputs(uint8_t mask) {
    writeRegister(REG_OUT, mask);
}

// Set a single channel (0-7) ON or OFF, leaving others unchanged.
// This reads the current OUT register, modifies one bit, writes it back.
void TLE75008_ESD::setChannel(uint8_t channel, bool on) {

    if(channel > 7) return;

    uint8_t current = readOutputs();
    if(on)  current |=  (1 << channel);
    else    current &= ~(1 << channel);
    writeOutputs(current);
}

// Set a single channel (1-8) ON or OFF, leaving others unchanged.
// This reads the current OUT register, modifies one bit, writes it back.
void TLE75008_ESD::toggleOutput(uint8_t channel, bool on) {

    channel--;

    if(channel > 7) return;

    uint8_t current = readOutputs();
    if(on)  current |=  (1 << channel);
    else    current &= ~(1 << channel);
    writeOutputs(current);
}

// Clear latched error bits for ALL channels.
void TLE75008_ESD::clearAllErrors() {
    // Writing 1 to each channel bit in HWCR_OCL register clears that channel’s error latch
    // So 0xFF => clear all 8 channels at once
    writeRegister(REG_HWCR_OCL, 0xFF);
}

// Clear latched error bit for one channel
void TLE75008_ESD::clearError(uint8_t channel) {
    if(channel > 7) return;
    uint8_t mask = (1 << channel);
    writeRegister(REG_HWCR_OCL, mask);
}

// Read the standard diagnosis 16-bit word.
// Bits: [15:0] = 0 UVRVS, 1 LOPVDD, 2..3 MODE, 4..7 ???, 8 OLOFF, 9.. bit about ERR channels, etc. datasheet Table 14 (section 10.6.1).
uint16_t TLE75008_ESD::readStandardDiagnosis() {
 
    // we can do a "READ STANDARD DIAGNOSIS" by sending 0xxxxxxxxxxxxx01B on bits[15..0].
    // Easiest approach: just do a "dummy read" from an address that is not used 
    // or by the recommended "0x4002" approach. The chip responds with standard diag next time.
    //
    // An easy trick: "Reading from address 0" with read bits can also yield standard diag on next frame if it’s invalid.
    // The official way is: 1) send "0x4002" => 0100 0000 0000 0010 => read standard diag
    // 2) next 16-bit is the standard diag.
    //
    // So we do 2-step:
    // (1) xfer16(0x4002) => discarding old data
    // (2) diag = xfer16(0x0000) => that’s the standard diag
    //
    // Then the *next* 16-bit response is also standard diag from the read, but we can read once more if needed.
    uint16_t dummy = spiTransfer16(0x4002u); // request read standard diag
    (void)dummy; // discard

    uint16_t diag = spiTransfer16(0x0000u); // now get it
    return diag;
}

// Read DIAG_OSM register (0010B 01B => combined 0x09). This register indicates per-channel if VDS < threshold => "1", or "0" if above threshold.
// return 8-bit data (lowest bit => channel0, etc.)
uint8_t TLE75008_ESD::readOutputStatusMonitor() {
    return readRegister(REG_DIAG_OSM);
}

// Enable/disable open-load current sources on channels. 1 => enable, 0 => disable, per bit.
void TLE75008_ESD::writeDiagnosticCurrent(uint8_t mask) {
    writeRegister(REG_DIAG_IOL, mask);
}

// Read the DIAG_IOL register to see which channels have open-load current enabled.
uint8_t TLE75008_ESD::readDiagnosticCurrent() {
    return readRegister(REG_DIAG_IOL);
}

// Low-Level SPI R/W Functions
// Transfer exactly 16 bits over SPI, MSB first, capturing the 16 bits that come back from the TLE75008-ESD.
uint16_t TLE75008_ESD::spiTransfer16(uint16_t txData) {

    // Use Arduino’s SPI.transfer16 if available. If not, do two SPI.transfer calls.
    // The TLE75008-ESD always expects 16 SCLK pulses per frame.
    // The returned 16 bits correspond to what the chip outputs *during* these 16 SCLK cycles,
    // typically the response to the previous command.

    SPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE1));
    digitalWrite(_csPin, LOW);

#if defined(SPI_HAS_TRANSFER16)
    uint16_t rxData = SPI.transfer16(txData);
#else
    // Manually do two transfers if hardware doesn't have transfer16
    uint8_t msb = (txData >> 8) & 0xFF;
    uint8_t lsb = (txData & 0xFF);

    uint8_t rxMsb = SPI.transfer(msb);
    uint8_t rxLsb = SPI.transfer(lsb);
    uint16_t rxData = ((uint16_t)rxMsb << 8) | rxLsb;
#endif

    digitalWrite(_csPin, HIGH);
    SPI.endTransaction();

    return rxData;
}

// Write an 8-bit register: 16-bit SPI frame = [10aaaaab b cccccccc]
void TLE75008_ESD::writeRegister(uint8_t address, uint8_t value) {

    // Frame:
    //  bit15..14 = 10 (write)
    //  bit13..8  = address (6 bits)
    //  bit7..0   = data
    // => 0x8000 | (address<<8) | value
    uint16_t cmd = 0x8000u;
    cmd |= ((uint16_t)address << 8) & 0x3F00u; // only 6 bits of address
    cmd |= (uint16_t)value;

    // Send that 16-bit frame => the response is the standard diag from the PREVIOUS command
    spiTransfer16(cmd);

    // The *next* 16-bit response is the standard diag from THIS command but we typically read that if we want to keep track. Let’s just do a dummy read for now:
    (void)spiTransfer16(0x0000u);
}

// Read an 8-bit register with the 2-frame read protocol
uint8_t TLE75008_ESD::readRegister(uint8_t address) {

    // 1) Send the "read command" frame: 
    //    bits[15..14] = 01 => read
    //    bits[13..8]  = address
    //    bits[1..0]   = 10 => as recommended 
    // => example: 0x4002 => 0100 0000 0000 0010
    // but we incorporate the address:
    // readFrame = 0x4000 | (address<<8) & 0x3F00 | 0x02
    uint16_t cmd = 0x4000u;
    cmd |= ((uint16_t)address << 8) & 0x3F00u;
    cmd |= 0x0002u; // put '10' in the last 2 bits

    // This first transfer initiates the read request
    spiTransfer16(cmd);

    // The next 16 bits we clock out is the actual register content => "10aaaaab b cccccccc"
    uint16_t regData = spiTransfer16(0x0000u);

    // The lower 8 bits is the data
    uint8_t value = (uint8_t)(regData & 0x00FF);

    // Now the next 16 bits from the device (if we do a subsequent spiTransfer16) would be standard diagnosis. We can do a dummy read if we want:
    (void)spiTransfer16(0x0000u);

    return value;
}
