/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * Configuring the GNSS to automatically send MON RF reports over I2C and display them using a callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * This example shows how to display a summary of the GNSS module's RF .
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

// Callback: newMONRF will be called when new MON RF data arrives
void newMONRF(ubxCallbackDataCommon_t *theData)
{
  ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

  Serial.println();

  Serial.println(F("New MON RF data received:"));

  Serial.print(F("Message version:      "));
  Serial.println((uint8_t)myGNSS.getUbxMessageFieldCallback(msg, "version"));
  
  Serial.print(F("Message source:       "));
  switch((uint8_t)myGNSS.getUbxMessageFieldCallback(msg, "msgSource"))
  {
    case 0:
      Serial.println("0 = Single antenna");
      break;
    case 1:
      Serial.println("1 = Antenna 1");
      break;
    case 2:
      Serial.println("2 = Antenna 2");
      break;
    case 3:
      Serial.println("3 = RESERVED");
      break;
    default:
      Serial.println("UNKNOWN");
      break;
  }

  for (uint8_t b = 0; b < myGNSS.getUbxMessageBlockCountCallback(msg); b++)
  {
    Serial.print(F("blockId:              "));
    Serial.println((uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, b, "blockId"));

    Serial.print(F("antStatus:            "));
    switch((uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, b, "antStatus"))
    {
      case 0:
        Serial.println("INIT");
        break;
      case 1:
        Serial.println("DONT KNOW");
        break;
      case 2:
        Serial.println("OK");
        break;
      case 3:
        Serial.println("SHORT");
        break;
      case 4:
        Serial.println("OPEN");
        break;
      default:
        Serial.println("UNKNOWN");
        break;
    }

    Serial.print(F("agcCnt (%):           "));
    // getUbxMessageFieldCallback returns double by default. No casting needed.
    double agcCnt = myGNSS.getUbxMessageBlockFieldCallback(msg, b, "agcCnt") / 81.91; //Convert cno to %
    Serial.println(agcCnt, 1);

    Serial.print(F("GNSS Band:            "));
    switch((uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, b, "rfBlockGnssBand"))
    {
      default:
      case 0:
        Serial.println("UNKNOWN");
        break;
      case 1:
        Serial.println("L1");
        break;
      case 2:
        Serial.println("L2");
        break;
      case 3:
        Serial.println("L3");
        break;
      case 4:
        Serial.println("L5");
        break;
    }
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
  myGNSS.setCfgValset(UBLOX_CFG_HW_ANT_CFG_SHORTDET, 1); // Enable antenna short detection
  myGNSS.setCfgValset(UBLOX_CFG_HW_ANT_CFG_OPENDET, 1);  // Enable antenna open detection

  // Enable the MON RF Message on I2C, every second navigation cycle
  myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_MON_RF_I2C, 2);

  // Set up a callback for MON RF messages. Call newMONRF() each time one arrives.
  // Note: this does not enable the MON RF message. The message is assumed to be periodic.
  // All this does is register the callback.
  myGNSS.setAutoCallbackPtr("MON", "RF", &newMONRF);
}

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  Serial.print(".");
  delay(50);

  static unsigned long lastAntennaStatus = 0;
  if ((millis() - lastAntennaStatus) > 5000) // Call getAntennaStatus every 5 seconds
  {
    Serial.println();
    Serial.print("Combined antenna status: ");
    switch(myGNSS.getAntennaStatus())
    {
      default:
      case SFE_UBLOX_ANTENNA_STATUS_DONTKNOW:
        Serial.println("DONT KNOW");
        break;
      case SFE_UBLOX_ANTENNA_STATUS_INIT:
        Serial.println("INIT");
        break;
      case SFE_UBLOX_ANTENNA_STATUS_OK:
        Serial.println("OK");
        break;
      case SFE_UBLOX_ANTENNA_STATUS_SHORT:
        Serial.println("SHORT");
        break;
      case SFE_UBLOX_ANTENNA_STATUS_OPEN:
        Serial.println("OPEN");
        break;
    }
    lastAntennaStatus = millis();
  }
}
