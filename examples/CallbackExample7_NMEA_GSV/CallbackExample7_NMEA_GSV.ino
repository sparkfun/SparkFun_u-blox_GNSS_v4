/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * Reading NMEA GSV using a Callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * When NMEA GSV messages are enabled, u-blox GNSS modules output multiple messages as
 * defined by the NMEA 0183 specification:
 * GPGSV indicates the message contains information for GPS SVs. "GP" is the Talker ID.
 * GLGSV indicates the message contains information for GLONASS SVs. "GL" is the Talker ID.
 * Messages are output in groups, of up to 9 messages per constellation.
 * In theory, the module could output up to 54 messages (up to 9 messages for each of 6
 * constellations).
 * This example demonstrates how to read multiple GSV messages using a callback.
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

volatile bool printHeader = true;

void printGSVdata(nmeaCallbackDataCommon_t *theData)
{
  if (printHeader)
  {
    Serial.println();
    Serial.println("Signal             svid elv  az cno");
    printHeader = false;
  }

  nmeaMessage *msg = myGNSS.getNmeaMessagePtr(theData);

  String xxGSV = myGNSS.getNmeaMessageFieldCallback(msg, "xxGSV"); // Get the Talker ID + GSV
  String signalId = myGNSS.getNmeaMessageFieldCallback(msg, "signalId"); // Get the signal ID
  String signal;

  if (xxGSV == "GPGSV") // GPS / SBAS
  {
    if (signalId == "1") signal = String("GPS/SBAS L1 C/A    ");
    else if (signalId == "6") signal = String("GPS L2 CL          ");
    else if (signalId == "5") signal = String("GPS L2 CM          ");
    else if (signalId == "7") signal = String("GPS L5 I           ");
    else if (signalId == "8") signal = String("GPS L5 Q           ");
    else signal = String("GPS UNKNOWN        ");
  }
  else if (xxGSV == "GAGSV")
  {
    if (signalId == "7") signal = String("Galileo E1 C/B     ");
    else if (signalId == "1") signal = String("Galileo E5 aI/aQ   ");
    else if (signalId == "2") signal = String("Galileo E5 bI/bQ   ");
    else if (signalId == "5") signal = String("Galileo E6 B/C     ");
    else if (signalId == "4") signal = String("Galileo E6 A       ");
    else signal = String("Galileo UNKNOWN    ");
  }     
  else if (xxGSV == "GBGSV")
  {
    if (signalId == "1") signal = String("BeiDou B1|D1/B1|D2 ");
    else if (signalId == "B") signal = String("BeiDou B2|D1/B2|D2 ");
    else if (signalId == "8") signal = String("BeiDou B3|D1/B3|D2 ");
    else if (signalId == "3") signal = String("BeiDou B1 Cp/Cd    ");
    else if (signalId == "5") signal = String("BeiDou B2 ap/ad    ");
    else signal = String("BeiDou UNKNOWN     ");
  }  
  else if (xxGSV == "GQGSV")
  {
    if (signalId == "1") signal = String("QZSS L1 C/A        ");
    else if (signalId == "4") signal = String("QZSS L1 S          ");
    else if (signalId == "5") signal = String("QZSS L2 CM         ");
    else if (signalId == "6") signal = String("QZSS L2 CL         ");
    else if (signalId == "7") signal = String("QZSS L5 I          ");
    else if (signalId == "8") signal = String("QZSS L5 Q          ");
    else signal = String("QZSS UNKNOWN       ");
  }
  else if (xxGSV == "GLGSV")
  {
    if (signalId == "1") signal = String("GLONASS L1 OF      ");
    else if (signalId == "3") signal = String("GLONASS L2 OF      ");
    else signal = String("GLONASS UNKNOWN    ");
  }     
  else if (xxGSV == "GIGSV")
  {
    if (signalId == "1") signal = String("NavIC L5 A         ");
    else signal = String("NavIC UNKNOWN      ");
  }
  else signal = String("UNKNOWN            ");

  for (uint8_t block = 0; block < 4; block++) // GSV can hold up to 4 blocks
  {
    if (myGNSS.getNmeaMessageBlockFieldCallback(msg, block, "svid").length() > 0)
    {
      Serial.print(signal);

      // Adjust the SV number if needed
      String svid_str = myGNSS.getNmeaMessageBlockFieldCallback(msg, block, "svid");
      int svid = atoi(myGNSS.getNmeaMessageBlockFieldCallback(msg, block, "svid").c_str());
      if (xxGSV == "GPGSV") // SBAS SVs S120-S151 are numbered 33-64
        if ((svid >= 33) && (svid <= 64))
          svid_str = String(svid + 87);
      if (xxGSV == "GLGSV") // GLONASS SVs R1-R32 are numbered 65-96
        svid_str = String(svid - 64);
      printPadded(svid_str, 4);

      printPadded(myGNSS.getNmeaMessageBlockFieldCallback(msg, block, "elv"), 4);
      printPadded(myGNSS.getNmeaMessageBlockFieldCallback(msg, block, "az"), 4);
      printPadded(myGNSS.getNmeaMessageBlockFieldCallback(msg, block, "cno"), 4);
      Serial.println();
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

  // Enable the NMEA GSV Message on I2C, every two navigation cycles
  myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_I2C, 2);

  // Set up a callback for NMEA GSV messages. Call printGSVdata() each time one arrives.
  // Note: this does not enable the GSV message. The message is assumed to be periodic.
  // All this does is register the callback.
  myGNSS.setNmeaCallbackPtr("GSV", &printGSVdata);
}

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  Serial.print(".");
  delay(50);
  printHeader = true;
}

// Print a String, right-justified with space padding as needed
void printPadded(String str, uint8_t padding)
{
  for (uint8_t p = str.length(); p < padding; p++)
    Serial.print(" ");
  Serial.print(str);
}
