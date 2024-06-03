/*********************************************************************************************************************
Example Program For TLE75008
*********************************************************************************************************************/

#include <Arduino.h>
#include "TLE75008_ESD.h"

// TLE75008 Chip Select & IDLE Pins
#define IDLE 7
#define CSA 10

// Define the 8 TLE75008 Relay Drivers
TLE75008_ESD SWA(CSA, IDLE);

void setup() {
  Serial.begin(9600);

  // Initilize the 8 TLE75008 Relay Drivers
  SWA.begin();

  SWA.toggleOutput(1, true);
  SWA.toggleOutput(2, false);
}

void loop() {
    for (byte channel = 1; channel <= 8; ++channel) {
        bool overload = SWA.getOverloadStatus(channel);
        bool openLoad = SWA.getOpenLoadStatus(channel);
        bool outputStatus = SWA.getOutputStatusMonitor(channel);

        Serial.print("Channel ");
        Serial.print(channel);
        Serial.print(": Overload=");
        Serial.print(overload);
        Serial.print(", OpenLoad=");
        Serial.print(openLoad);
        Serial.print(", OutputStatus=");
        Serial.println(outputStatus);
    }

    delay(1000);
}