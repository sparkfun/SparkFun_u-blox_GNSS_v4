/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * Reading HPPOSLLH using a Callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * This example shows how to use a callback to print the High Precision PVT LLH
 * data from the GNSS.
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

void printPVTdata(ubxCallbackDataCommon_t *theData)
{
    Serial.println();

    ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

    // getUbxMessageField returns everything as double. Cast to other types as needed
    unsigned long timeOfWeek = (unsigned long)myGNSS.getUbxMessageFieldCallback(msg, "iTOW");
    Serial.print("TimeOfWeek: ");
    Serial.print(timeOfWeek); // Print the Time Of Week
    Serial.print(" (ms)");

    long latitude = (long)myGNSS.getUbxMessageFieldCallback(msg, "lat");
    Serial.print(" Lat: ");
    Serial.print(latitude); // Print the latitude

    // Or, we could read the true "I4" (int32_t) directly, without going through double
    // To do that, we need to use the ubxAnyType struct
    ubxAnyType ubxAnyTypeLon = myGNSS.getUbxMessageFieldCallback(msg, "lon");
    Serial.print(" Long: ");
    Serial.print(ubxAnyTypeLon.I4); // Print the longitude directly as int32_t
    Serial.print(" (degrees * 10^-7)");

    float hAcc = (float)myGNSS.getUbxMessageFieldCallback(msg, "hAcc");
    Serial.print(" Horiz Acc: ");
    Serial.print(hAcc / 10.0, 1); // Print the horizontal accuracy estimate
    Serial.println(" (mm)");
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

  // Enable the NAV HPPOSLLH Message on I2C at the navigation rate
  myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSLLH_I2C, 1);

  // Set up a callback for NAV HPPOSLLH messages. Call printPVTdata() each time one arrives.
  // Note: this does not enable the HPPOSLLH message. The message is assumed to be periodic.
  // All this does is register the callback.
  myGNSS.setAutoCallbackPtr("NAV", "HPPOSLLH", &printPVTdata);
}

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  Serial.print(".");
  delay(50);
}
