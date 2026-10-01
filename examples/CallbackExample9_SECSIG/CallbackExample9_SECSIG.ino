/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * Configuring the GNSS to automatically send SEC SIG reports over I2C and display them using a callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * This example shows how to configure the u-blox GNSS to send SEC SIG reports automatically
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

// Callback: newSECSIG will be called when new SEC SIG data arrives
void newSECSIG(ubxCallbackDataCommon_t *theData)
{
  ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

  Serial.println();

  uint8_t version = (uint8_t)myGNSS.getUbxMessageFieldCallback(msg, "version");
  Serial.print(F("New SEC SIG data received. It contains version "));
  Serial.print(version);
  Serial.println(F(" data."));

  if (version >= 2)
  {
    Serial.print(F("Jamming detection is "));
    bool jamDetEnabled = (bool)myGNSS.getUbxMessageFieldCallback(msg, "jamDetEnabled");
    Serial.println(jamDetEnabled ? "enabled" : "disabled");
    if (jamDetEnabled)
    {
      Serial.print(F("Jamming state: "));
      switch ((uint8_t)myGNSS.getUbxMessageFieldCallback(msg, "jamState"))
      {
        case 1:
          Serial.println(F("no jamming indicated"));
          break;
        case 2:
          Serial.println(F("warning (jamming indicated)"));
          break;
        case 0:
        default:
          Serial.println(F("unknown"));
          break;
      }
    }
    Serial.print(F("Spoofing detection is "));
    bool spfDetEnabled = (bool)myGNSS.getUbxMessageFieldCallback(msg, "spfDetEnabled");
    Serial.println(spfDetEnabled ? "enabled" : "disabled");
    if (spfDetEnabled)
    {
      Serial.print(F("Spoofing state: "));
      switch ((uint8_t)myGNSS.getUbxMessageFieldCallback(msg, "spfState"))
      {
        case 1:
          Serial.println(F("no spoofing indicated"));
          break;
        case 2:
          Serial.println(F("spoofing suspected"));
          break;
        case 3:
          Serial.println(F("spoofing detected"));
          break;
        case 0:
        default:
          Serial.println(F("unknown"));
          break;
      }
    }
    Serial.print(F("Number of jamming center frequencies: "));
    uint8_t jamNumCentFreqs = (uint8_t)myGNSS.getUbxMessageFieldCallback(msg, "jamNumCentFreqs");
    Serial.println(jamNumCentFreqs);
    if (jamNumCentFreqs > 0)
    {
      for (uint8_t i = 0; i < jamNumCentFreqs; i++)
      {
        Serial.print(F("Center frequency: "));
        Serial.print((uint32_t)myGNSS.getUbxMessageBlockFieldCallback(msg, i, "centFreq"));
        Serial.print(F(" kHz "));
        if ((bool)myGNSS.getUbxMessageBlockFieldCallback(msg, i, "jammed"))
          Serial.print("- jammed");
        Serial.println();
      }
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

  // Enable the SEC SIG Message on I2C, every two navigation cycles
  myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_SEC_SIG_I2C, 2);

  // Set up a callback for SEC SIG messages. Call newSECSIG() each time one arrives.
  // Note: this does not enable the SEC SIG message. The message is assumed to be periodic.
  // All this does is register the callback.
  myGNSS.setAutoCallbackPtr("SEC", "SIG", &newSECSIG);
}

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  Serial.print(".");
  delay(50);
}
