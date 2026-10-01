/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * Reading NAV-SAT using a Callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * This example shows how to use a callback to print the NAV-SAT SV signal strengths
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

void newNAVSAT(ubxCallbackDataCommon_t *theData)
{
  ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

  Serial.println();

  uint8_t numSvs = myGNSS.getUbxMessageFieldCallback(msg, "numSvs");
  Serial.print(F("New NAV SAT data received. It contains data for "));
  Serial.print(numSvs);
  Serial.println(numSvs == 1 ? F(" SV.") : F(" SVs."));

  Serial.println("gnssId  svId qual used cno Carrier_Noise_dBHz");

  for (uint8_t block = 0; block < numSvs; block++)
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

    // Signal quality indicator:
    //  0 = no signal
    //  1 = searching signal
    //  2 = signal acquired
    //  3 = signal detected but unusable
    //  4 = code locked and time synchronized
    //  5, 6, 7 = code and carrier locked and time synchronized
    uint8_t qualityInd = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "qualityInd");
    printPadded(qualityInd, 5);

    bool svUsed = (bool)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "svUsed");
    printPadded((uint8_t)svUsed, 5);

    uint8_t cno = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "cno");
    printPadded(cno, 4);

    // Print cno as a bar
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

  // Enable the NAV SAT Message on I2C, every two navigation cycles
  myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_SAT_I2C, 2);

  // Set up a callback for NAV SAT messages. Call newNAVSAT() each time one arrives.
  // Note: this does not enable the NAV SAT message. The message is assumed to be periodic.
  // All this does is register the callback.
  myGNSS.setAutoCallbackPtr("NAV", "SAT", &newNAVSAT);
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
