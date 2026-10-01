/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * NEO-D9C QZSS-L6 receiver example
 * By: Paul Clark
 * SparkFun Electronics
 *
 * This example shows how to display a summary of the NEO-D9C's UBX-RXM-QZSSL6 data.
 * It also enables UBX-RXM-QZSSL6 message output on both UART1 and UART2 at 38400 baud
 * so you can feed the corrections directly to (e.g.) a ZED-F9P.
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

SFE_UBLOX_GNSS myQZSS;

// Callback: newQZSSL6 will be called when new RXM QZSSL6 data arrives
void newQZSSL6(ubxCallbackDataCommon_t *theData)
{
  ubxMessage *msg = myQZSS.getUbxMessagePtr(theData);

  Serial.println();

  Serial.println(F("New QZSSL6 data received:"));

  Serial.print(F("QZSSL6 message version: "));
  Serial.println((uint8_t)myQZSS.getUbxMessageFieldCallback(msg, "version"));
  
  Serial.print(F("svId:                   "));
  Serial.println((uint8_t)myQZSS.getUbxMessageFieldCallback(msg, "svId"));
  
  Serial.print(F("cno (dBHz):             "));
  // getUbxMessageFieldCallback returns double by default. No casting needed.
  double ebno = myQZSS.getUbxMessageFieldCallback(msg, "cno") * 0.00390625; //Convert cno to dB : multiply by 2^-8
  Serial.println(ebno, 3);

  Serial.print(F("bitErrCorr:             "));
  Serial.println((uint8_t)myQZSS.getUbxMessageFieldCallback(msg, "bitErrCorr"));
  
  Serial.print(F("Receiver channel:       "));
  Serial.println((uint8_t)myQZSS.getUbxMessageFieldCallback(msg, "chn"));
  
  Serial.print(F("Message name:           "));
  Serial.println((uint8_t)myQZSS.getUbxMessageFieldCallback(msg, "msgName") == 0 ? "L6D" : "L6E");
  
  Serial.print(F("Channel name:           "));
  Serial.println((uint8_t)myQZSS.getUbxMessageFieldCallback(msg, "chName") == 0 ? "A" : "B");
  
  Serial.println();
}

void setup()
{
  Serial.begin(115200);
  delay(1000); 
  Serial.println("SparkFun u-blox NEO-D9C Example");

  Wire.begin(); // Start I2C

  //myQZSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  while (myQZSS.begin(Wire, 0x43) == false) //Connect to the u-blox NEO-D9C using Wire port. The D9C default I2C address is 0x43 (not 0x42)
  {
    Serial.println("u-blox NEO-D9C not detected at default I2C address. Retrying...");
    delay (1000);
  }

  Serial.println(F("u-blox NEO-D9C connected"));

  // newCfgValset() writes to the RAM and Battery-backed-RAM layers by default (VAL_LAYER_RAM_BBR).
  // To use a different layer, pass it as the parameter. E.g. for RAM only:
  //   newCfgValset(VAL_LAYER_RAM)
  // VAL_LAYER_ALL also saves the settings in Flash (if the module has Flash).
  myQZSS.newCfgValset(); // Create a new Configuration Interface message - this defaults to VAL_LAYER_RAM_BBR (change in RAM and BBR)
  myQZSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_QZSSL6_I2C,   1);     // Output QZSS-L6 message on the I2C port 
  myQZSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_QZSSL6_UART1, 1);     // Output QZSS-L6 message on UART1
  myQZSS.addCfgValset(UBLOX_CFG_UART1_BAUDRATE,              38400); // Match UART1 baudrate with ZED
  myQZSS.addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX,            1);     // Enable UBX output on UART2
  myQZSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_QZSSL6_UART2, 1);     // Output QZSS-L6 message on UART2
  myQZSS.addCfgValset(UBLOX_CFG_UART2_BAUDRATE,              38400); // Match UART2 baudrate with ZED
  bool ok = myQZSS.sendCfgValset(); // Apply the settings

  Serial.print(F("QZSS-L6: configuration "));
  Serial.println(ok ? "OK" : "NOT OK");

  myQZSS.softwareResetGNSSOnly(); // Do a restart

  // Set up a callback for RXM QZSSL6 messages. Call newQZSSL6() each time one arrives.
  // Note: this does not enable the QZSSL6 message. The message is assumed to be periodic.
  // All this does is register the callback.
  myQZSS.setAutoCallbackPtr("RXM", "QZSSL6", &newQZSSL6);
}

void loop()
{
  myQZSS.checkUblox(); // Check for the arrival of new data and process it.
  myQZSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  static int dotsPrinted = 0;
  Serial.print(".");
  if (dotsPrinted++ == 50)
  {
    Serial.println();
    dotsPrinted = 0;
  }
  delay(250);
}
