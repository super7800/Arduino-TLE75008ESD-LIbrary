#ifndef TLE75008_ESD_H
#define TLE75008_ESD_H

#include <Arduino.h>
#include <SPI.h>

/*********************************************************************************************************************
Arduino driver class for the Infineon TLE75008-ESD 8-channel low-side driver.

Written By J Soucek NVE 2/27/2025

Features included:
- 16-bit SPI frame commands
- Full register read/write
- Output (channel) control
- Basic diagnostics (Overload, Open Load, etc.)
*********************************************************************************************************************/

class TLE75008_ESD {
public:

    TLE75008_ESD(uint8_t csPin, uint8_t idlePin);

    // Initialize SPI settings and configure the TLE75008-ESD.
    // Call this once in setup().
    void begin();

    // All outputs off, minimal current consumption.
    // Next calls requiring channels ON or Active mode will forcibly wake it.
    void enterSleep();

    // Put device into Active mode (vs. Idle mode).
    // The TLE75008-ESD can drive outputs only in Active (or Limp Home) modes.
    void enterActive();

    // Read back the device’s current 8-channel output state (OUT register).
    // return 8-bit mask of the outputs. Bit 0 = channel 0, bit 1 = channel 1, etc.
    uint8_t readOutputs();

    // Write an 8-bit mask to the OUT register to turn channels on/off in one shot.
    // mask 8-bit mask: 1 = ON, 0 = OFF. Bit 0 = channel 0, bit 1 = channel 1, etc.
    void writeOutputs(uint8_t mask);

    // Set a single channel ON or OFF, leaving the others untouched.
    // channel Channel number [0..7]
    // on      true = ON, false = OFF
    void setChannel(uint8_t channel, bool on);

    // Set a single channel ON or OFF, leaving the others untouched.
    // channel Channel number [1..8]
    // on      true = ON, false = OFF
    void toggleOutput(uint8_t channel, bool on);

    // ---------------------- Diagnostics ----------------------

    // Clear latched error bits for ALL channels.
    void clearAllErrors();

    // Clear latched error bits for a single channel.
    // channel [0..7]
    void clearError(uint8_t channel);

    // Read standard diagnostic bits (UVRVS, LOPVDD, MODE, TER, OLOFF, ERRn).
    // return The 16-bit standard diagnostic frame. See datasheet section 10.6.1
    uint16_t readStandardDiagnosis();

    // Read the Output Status Monitor register (DIAG_OSM).
    // This is useful for detecting open load (OFF) or verifying VDS < threshold (ON).
     // return 8-bit result, each bit corresponds to a channel 0..7
    uint8_t readOutputStatusMonitor();

    // Enable or disable the internal current source for open-load detection on each channel.
    // mask 8-bit mask: 1 = enable IOL, 0 = disable IOL
    void writeDiagnosticCurrent(uint8_t mask);

    // Read the DIAG_IOL register (which bits have open-load current enabled).
    // return 8-bit mask
    uint8_t readDiagnosticCurrent();

private:

    /*********************************************************************************************************************
    Private SPI Helpers

    Send and receive a single 16-bit frame over SPI.

    The TLE75008-ESD always expects 16 bits per frame. 
    The response from this call is the chip’s output during these 16 SCLKs, 
    which typically corresponds to the previous command’s result.

    txData 16 bits to transmit (MSB first)
    return 16 bits received (MSB is bit 15)
    *********************************************************************************************************************/

    uint16_t spiTransfer16(uint16_t txData);

    /*********************************************************************************************************************
    Write an 8-bit register with a single 16-bit SPI frame.

    TLE75008-ESD uses the “write” command:  10aaaaab b cccccccc
    - Bits [15:14] = 10 (write)
    - Bits [13:10] = ADDR0 (4 bits)
    - Bits [9:8]   = ADDR1 (2 bits)
    - Bits [7:0]   = data
    The next 16-bit transfer from the device returns the standard diagnosis.

    address   6-bit combined address (4 bits ADDR0 + 2 bits ADDR1)
    value     8-bit data
    *********************************************************************************************************************/

    void writeRegister(uint8_t address, uint8_t value);

    /*********************************************************************************************************************
    Read an 8-bit register with the two-frame SPI read protocol.

    1) First 16-bit frame: “read command” = 01aaaaab b xxxxxx10
    2) Second 16-bit frame (dummy tx = 0x0000) to get the data = 10aaaaab b cccccccc

    The *next* 16-bit transfer from the device returns standard diagnosis.

    address 6-bit combined address
    return 8-bit read data
    *********************************************************************************************************************/

    uint8_t readRegister(uint8_t address);

    /*********************************************************************************************************************
    Combine the 4-bit ADDR0 + 2-bit ADDR1 into a single 6-bit address.

    For example, if the datasheet says:
    - ADDR0 = 0011 (0x3)
    - ADDR1 = 01   (0x1)
    then combined =  (0011 << 2) | (01) = 0x0D (decimal 13).

    addr0 Lower 4 bits
    addr1 Next 2 bits
    *********************************************************************************************************************/

    static constexpr uint8_t makeAddress(uint8_t addr0, uint8_t addr1) {
        return ((addr0 & 0x0F) << 2) | (addr1 & 0x03);
    }

    // Internal initialization steps: clearing errors, enabling Active mode, etc.
    void initializeDevice();

private:
    uint8_t _csPin;
    uint8_t _idlePin;
};

#endif // TLE75008_ESD_H
