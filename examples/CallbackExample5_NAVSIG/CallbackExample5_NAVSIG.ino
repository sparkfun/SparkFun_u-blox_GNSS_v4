/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * Reading NAV-SIG using a Callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * This example shows how to use a callback to print the NAV-SIG SV signal strengths
 * (cno = Carrier to NOise ratio in dBHz).
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

#include <Wire.h> //Needed for I2C to GNSS

#include <SparkFun_u-blox_GNSS_v4.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v4

SFE_UBLOX_GNSS myGNSS; // SFE_UBLOX_GNSS uses I2C

void newNAVSIG(ubxCallbackDataCommon_t *theData)
{
  ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

  Serial.println();

  uint8_t numSigs = myGNSS.getUbxMessageFieldCallback(msg, "numSigs");
  Serial.print(F("New NAV SIG data received. It contains data for "));
  Serial.print(numSigs);
  Serial.println(numSigs == 1 ? F(" SV.") : F(" SVs."));

  Serial.println("gnssId  svId sigId       qual hlth pr cr cno Carrier_Noise_dBHz");

  for (uint8_t block = 0; block < numSigs; block++)
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

    // Signal quality indicator:
    //  0 = no signal
    //  1 = searching signal
    //  2 = signal acquired
    //  3 = signal detected but unusable
    //  4 = code locked and time synchronized
    //  5, 6, 7 = code and carrier locked and time synchronized
    uint8_t qualityInd = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "qualityInd");
    printPadded(qualityInd, 4);

    // Signal health flag:
    //  0 = unknown
    //  1 = healthy
    //  2 = unhealthy
    uint8_t health = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "health");
    printPadded(health, 5);

    bool prUsed = (bool)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "prUsed");
    printPadded(prUsed, 3);

    bool crUsed = (bool)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "crUsed");
    printPadded(crUsed, 3);

    uint8_t cno = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "cno");
    printPadded(cno, 4);

    Serial.print(F(" "));
    for (uint8_t bar = 0; bar < cno; bar++)
      Serial.print(F("="));
    Serial.println();
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000); 
  Serial.println(F("SparkFun u-blox Example"));

  Wire.begin(); // Start I2C

  //myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  while (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Retrying..."));
    delay (1000);
  }

  // setCfgValset() writes to the RAM and Battery-backed-RAM layers by default (VAL_LAYER_RAM_BBR).
  // To use a different layer, add it as the third parameter. E.g. for RAM only:
  //   setCfgValset(UBLOX_CFG_MSGOUT_..., n, VAL_LAYER_RAM)
  // VAL_LAYER_ALL also saves the setting in Flash (if the module has Flash).

  // Enable the NAV SIG Message on I2C, every two navigation cycles
  myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_SIG_I2C, 2);

  // Set up a callback for NAV SIG messages. Call newNAVSIG() each time one arrives.
  // Note: this does not enable the NAV SIG message. The message is assumed to be periodic.
  // All this does is register the callback.
  myGNSS.setAutoCallbackPtr("NAV", "SIG", &newNAVSIG);
}

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  Serial.print(F("."));
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
    Serial.print(F(" "));
  Serial.print(val);
}
