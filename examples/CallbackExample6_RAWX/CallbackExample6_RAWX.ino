/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * Configuring the GNSS to automatically send RXM RAWX reports over I2C and display them using a callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * This example shows how to configure the u-blox GNSS to send RXM RAWX reports automatically
 * and access the data via a callback.
 *  
 * Feel like supporting open source hardware?
 * Buy a board from SparkFun!
 * https://www.sparkfun.com/sparkfun-allband-gnss-rtk-breakout-zed-x20p-qwiic.html
 * https://www.sparkfun.com/sparkfun-gps-rtk2-board-zed-f9p-qwiic-gps-15136.html
 * https://www.sparkfun.com/sparkfun-gps-rtk-sma-breakout-zed-f9p-qwiic.html
 * https://www.sparkfun.com/sparkfun-gnss-receiver-breakout-max-m10s-qwiic.html
 * https://www.sparkfun.com/sparkfun-gps-rtk-dead-reckoning-breakout-zed-f9r-qwiic-gps-22693.html
 *
 * Hardware Connections:
 * Plug a Qwiic cable into the GNSS and your microcontroller board
 * Open the serial monitor at 115200 baud to see the output
 */

#include <Wire.h> //Needed for I2C to GPS

#include <SparkFun_u-blox_GNSS_v4.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v4

SFE_UBLOX_GNSS myGNSS;

// Callback: newRAWX will be called when new RXM RAWX data arrives
void newRAWX(ubxCallbackDataCommon_t *theData)
{
  ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

  Serial.println();

  uint8_t numMeas = (uint8_t)myGNSS.getUbxMessageFieldCallback(msg, "numMeas");
  Serial.print(F("New RAWX data received. It contains "));
  Serial.print(numMeas); // Print numMeas (Number of measurements / blocks)
  Serial.print(F(" measurements."));

  if (sizeof(double) == 8) // Check if our processor supports 64-bit double
  {
    uint16_t week = (uint16_t)myGNSS.getUbxMessageFieldCallback(msg, "week");
    // getUbxMessageBlockFieldCallback returns double. No type conversion needed
    double timeOfWeek = myGNSS.getUbxMessageFieldCallback(msg, "rcvTow");
    Serial.print(F(" Week:TOW: "));
    Serial.print(week);
    Serial.print(":");
    Serial.print(timeOfWeek, 1);
  }

  Serial.println();

  Serial.println("gnssId  svId sigId       cno Pseudorange (m) Carrier Phase (cycles)");

  for (uint8_t block = 0; block < numMeas; block++)
  {
    uint8_t gnssId = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "gnssId");
    switch (gnssId)
    {
      case 0: Serial.print(F("GPS     ")); break;
      case 1: Serial.print(F("SBAS    ")); break;
      case 2: Serial.print(F("Galileo ")); break;
      case 3: Serial.print(F("BeiDou  ")); break;
      case 4: Serial.print(F("IMES    ")); break;
      case 5: Serial.print(F("QZSS    ")); break;
      case 6: Serial.print(F("GLONASS ")); break;
      case 7: Serial.print(F("NAVIC   ")); break;
      default: Serial.print(F("UNKNOWN ")); break;
    }

    uint8_t svId = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "svId");
    printPadded(svId, 4);
    Serial.print(F(" "));

    uint8_t sigId = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "sigId");
    switch (gnssId)
    {
      case 0: // GPS
        switch (sigId)
        {
          case 0: Serial.print(F("L1 C/A      ")); break;
          case 3: Serial.print(F("L2 CL       ")); break;
          case 4: Serial.print(F("L2 CM       ")); break;
          case 6: Serial.print(F("L5 I        ")); break;
          case 7: Serial.print(F("L5 Q        ")); break;
          default: Serial.print(F("UNKNOWN     ")); break;
        }
        break;
      case 1: // SBAS
        switch (sigId)
        {
          case 0: Serial.print(F("L1 C/A      ")); break;
          default: Serial.print(F("UNKNOWN     ")); break;
        }
        break;
      case 2: // GALILEO
        switch (sigId)
        {
          case 0: Serial.print(F("E1 C        ")); break;
          case 1: Serial.print(F("E1 B        ")); break;
          case 3: Serial.print(F("E5 aI       ")); break;
          case 4: Serial.print(F("E5 aQ       ")); break;
          case 5: Serial.print(F("E5 bI       ")); break;
          case 6: Serial.print(F("E5 bQ       ")); break;
          case 8: Serial.print(F("E6 B        ")); break;
          case 9: Serial.print(F("E6 C        ")); break;
          case 10: Serial.print(F("E6 A        ")); break;
          default: Serial.print(F("UNKNOWN     ")); break;
        }
        break;
      case 3: // BeiDou
        switch (sigId)
        {
          case 0: Serial.print(F("B1|D1       ")); break;
          case 1: Serial.print(F("B2|D2       ")); break;
          case 2: Serial.print(F("B2|D1       ")); break;
          case 3: Serial.print(F("B2|D2       ")); break;
          case 4: Serial.print(F("B3|D1       ")); break;
          case 10: Serial.print(F("B3|D2       ")); break;
          case 5: Serial.print(F("B1 Cp pilot ")); break;
          case 6: Serial.print(F("B1 Cd data  ")); break;
          case 7: Serial.print(F("B2 ap pilot ")); break;
          case 8: Serial.print(F("B2 ad data  ")); break;
          default: Serial.print(F("UNKNOWN     ")); break;
        }
        break;
      case 5: // QZSS
        switch (sigId)
        {
          case 0: Serial.print(F("L1 C/A      ")); break;
          case 1: Serial.print(F("L1 S        ")); break;
          case 4: Serial.print(F("L2 CM       ")); break;
          case 5: Serial.print(F("L2 CL       ")); break;
          case 8: Serial.print(F("L5 I        ")); break;
          case 9: Serial.print(F("L5 Q        ")); break;
          case 12: Serial.print(F("L1 C/B      ")); break;
          default: Serial.print(F("UNKNOWN     ")); break;
        }
        break;
      case 6: // GLONASS
        switch (sigId)
        {
          case 0: Serial.print(F("L1 OF       ")); break;
          case 2: Serial.print(F("L2 OF       ")); break;
          default: Serial.print(F("UNKNOWN     ")); break;
        }
        break;
      case 7: // NavIC
        switch (sigId)
        {
          case 0: Serial.print(F("L5 A        ")); break;
          default: Serial.print(F("UNKNOWN     ")); break;
        }
        break;
      default: Serial.print(F("UNKNOWN     ")); break;
    }

    uint8_t cno = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "cno");
    printPadded(cno, 3);

    if (sizeof(double) == 8) // Check if our processor supports 64-bit double
    {
      // getUbxMessageBlockFieldCallback returns double. No type conversion needed
      double pseudorange = myGNSS.getUbxMessageBlockFieldCallback(msg, block, "prMes");
      Serial.print(F(" "));
      Serial.print(pseudorange, 3);
      Serial.print(F("    "));

      // getUbxMessageBlockFieldCallback returns double. No type conversion needed
      double carrierPhase = myGNSS.getUbxMessageBlockFieldCallback(msg, block, "cpMes");
      Serial.print(carrierPhase, 3);
    }

    Serial.println();
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000); 
  Serial.println("SparkFun u-blox Example");

  Wire.begin(); // Start I2C

  //myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  while (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println("u-blox GNSS not detected at default I2C address. Retrying...");
    delay (1000);
  }

  // setCfgValset() writes to the RAM and Battery-backed-RAM layers by default (VAL_LAYER_RAM_BBR).
  // To use a different layer, add it as the third parameter. E.g. for RAM only:
  //   setCfgValset(UBLOX_CFG_MSGOUT_..., n, VAL_LAYER_RAM)
  // VAL_LAYER_ALL also saves the setting in Flash (if the module has Flash).

  // Enable the RXM RAWX Message on I2C, every two navigation cycles
  myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_I2C, 2);

  // Set up a callback for RXM RAWX messages. Call newRAWX() each time one arrives.
  // Note: this does not enable the RAWX message. The message is assumed to be periodic.
  // All this does is register the callback.
  myGNSS.setAutoCallbackPtr("RXM", "RAWX", &newRAWX);
}

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  Serial.print(".");
  delay(50);
}

// Print a uint8_t, right-justified with space padding as needed
void printPadded(uint8_t val, uint8_t padding)
{
  uint8_t digits = 1;
  if (val >= 100)
    digits = 3;
  else if (val >= 10)
    digits = 2;
  for (uint8_t p = padding; p > digits; p--)
    Serial.print(" ");
  Serial.print(val);
}
