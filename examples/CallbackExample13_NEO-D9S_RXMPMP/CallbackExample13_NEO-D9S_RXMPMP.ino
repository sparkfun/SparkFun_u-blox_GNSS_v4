/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * NEO-D9S L-Band receiver example
 * By: Paul Clark
 * SparkFun Electronics
 *
 * This example shows how to display the NEO-D9S's received signal imbalance and magnitude,
 * plus a summary of any received PMP data.
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

SFE_UBLOX_GNSS myLBand;

const uint32_t myLBandFreq = 1556290000; // Uncomment this line to use the US SPARTN 1.8 service
//const uint32_t myLBandFreq = 1545260000; // Uncomment this line to use the EU SPARTN 1.8 service

// Callback: newPMP will be called when new RXM PMP data arrives
void newPMP(ubxCallbackDataCommon_t *theData)
{
  ubxMessage *msg = myLBand.getUbxMessagePtr(theData);

  Serial.println();

  Serial.println(F("New PMP data received:"));

  Serial.print(F("PMP message version: "));
  Serial.println((uint8_t)myLBand.getUbxMessageFieldCallback(msg, "version"));
  
  Serial.print(F("numBytesUserData :   "));
  Serial.println((uint16_t)myLBand.getUbxMessageFieldCallback(msg, "numBytesUserData"));
  
  Serial.print(F("serviceIdentifier:   0x"));
  Serial.println((uint16_t)myLBand.getUbxMessageFieldCallback(msg, "serviceIdentifer"), HEX);
  
  Serial.print(F("uniqueWordBitErrors: "));
  Serial.println((uint8_t)myLBand.getUbxMessageFieldCallback(msg, "uniqueWordBitErrors"));
  
  Serial.print(F("fecBits:             "));
  Serial.println((uint16_t)myLBand.getUbxMessageFieldCallback(msg, "fecBits"));
  
  Serial.print(F("ebno (dB):           "));
  // getUbxMessageFieldCallback returns double by default. No casting needed.
  double ebno = myLBand.getUbxMessageFieldCallback(msg, "ebno") * 0.125; //Convert ebno to dB : multiply by 2^-3
  Serial.println(ebno, 3);

  Serial.println();
}

void setup()
{
  Serial.begin(115200);
  delay(1000); 
  Serial.println("SparkFun u-blox NEO-D9S Example");

  Wire.begin(); // Start I2C

  //myLBand.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  while (myLBand.begin(Wire, 0x43) == false) //Connect to the u-blox NEO-D9S using Wire port. The D9S default I2C address is 0x43 (not 0x42)
  {
    Serial.println("u-blox NEO-D9S not detected at default I2C address. Retrying...");
    delay (1000);
  }

  Serial.println(F("u-blox NEO-D9S connected"));

  // newCfgValset() writes to the RAM and Battery-backed-RAM layers by default (VAL_LAYER_RAM_BBR).
  // To use a different layer, pass it as the parameter. E.g. for RAM only:
  //   newCfgValset(VAL_LAYER_RAM)
  // VAL_LAYER_ALL also saves the settings in Flash (if the module has Flash).
  myLBand.newCfgValset(); // Create a new Configuration Interface message - this defaults to VAL_LAYER_RAM_BBR (change in RAM and BBR)
  myLBand.addCfgValset(UBLOX_CFG_PMP_CENTER_FREQUENCY,     myLBandFreq); // Default 1539812500 Hz
  myLBand.addCfgValset(UBLOX_CFG_PMP_SEARCH_WINDOW,        2200);        // Default 2200 Hz
  myLBand.addCfgValset(UBLOX_CFG_PMP_USE_SERVICE_ID,       0);           // Default 1 
  myLBand.addCfgValset(UBLOX_CFG_PMP_SERVICE_ID,           21845);       // Default 50821
  myLBand.addCfgValset(UBLOX_CFG_PMP_DATA_RATE,            2400);        // Default 2400 bps
  myLBand.addCfgValset(UBLOX_CFG_PMP_USE_DESCRAMBLER,      1);           // Default 1
  myLBand.addCfgValset(UBLOX_CFG_PMP_DESCRAMBLER_INIT,     26969);       // Default 23560
  myLBand.addCfgValset(UBLOX_CFG_PMP_USE_PRESCRAMBLING,    0);           // Default 0
  myLBand.addCfgValset(UBLOX_CFG_PMP_UNIQUE_WORD,          16238547128276412563ull); 
  myLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_I2C,   1); // Ensure UBX-RXM-PMP is enabled on the I2C port 
  myLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_UART1, 1); // Output UBX-RXM-PMP on UART1
  myLBand.addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX,         1); // Enable UBX output on UART2
  myLBand.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_UART2, 1); // Output UBX-RXM-PMP on UART2
  myLBand.addCfgValset(UBLOX_CFG_UART1_BAUDRATE,           38400); // match baudrate with ZED default
  myLBand.addCfgValset(UBLOX_CFG_UART2_BAUDRATE,           38400); // match baudrate with ZED default
  bool ok = myLBand.sendCfgValset(); // Apply the settings

  Serial.print(F("L-Band: configuration "));
  Serial.println(ok ? "OK" : "NOT OK");

  myLBand.softwareResetGNSSOnly(); // Do a restart

  // Set up a callback for RXM PMP messages. Call newPMP() each time one arrives.
  // Note: this does not enable the PMP message. The message is assumed to be periodic.
  // All this does is register the callback.
  myLBand.setAutoCallbackPtr("RXM", "PMP", &newPMP);
}

void loop()
{
  myLBand.checkUblox(); // Check for the arrival of new data and process it.
  myLBand.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  static int dotsPrinted = 0;
  Serial.print(".");
  if (dotsPrinted++ == 50)
  {
    Serial.println();
    dotsPrinted = 0;
  }
  delay(250);
}
