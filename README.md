# Arduino-TLE75008TLE75008ESD-LIbrary

This library is for the TLE75008ESD in SPI mode, no daisy chain. You can have as many of these as you want. The project this was designed for uses 12, each with seperat CS pins. You should
make your connections to the chip following the picture, i.e. IN0 and IN1 left floating.

This library should work with any arduino compatable. i used it with a board that used the adafruit Metro M4 bootloader with a SAMD51J19 Microcontroller. No special things are used, so the
arduino UNO ETC. should work with no issue.

The STATUS functionality has only been basically tested. i cannot guerentee that it works fully or completely correclty. The toggleOutput is the only function you really need anyways.

Im not a software engineer. i neither know or care about liscences. Do whatever you want with this code, i could not care less. I will try to keep this library up to date if anyone has problems.
