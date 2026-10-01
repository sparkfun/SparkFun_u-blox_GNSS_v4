/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * Configuring the GNSS to automatically send RXM SFRBX and RAWX reports at 20Hz (!) over SPI
 * and log them to file on SD card using full 4-bit SDIO
 * By: Paul Clark
 * SparkFun Electronics
 *
 * This example is written for the SparkFun DataLogger IoT - 9DoF:
 * https://www.sparkfun.com/sparkfun-datalogger-iot-9dof.html
 * 
 * But it should also run on the SparkFun DataLogger IoT:
 * https://www.sparkfun.com/sparkfun-datalogger-iot.html
 * 
 * It makes use of the fast SDIO microSD interface to increase the logging speed.
 *
 * ** Please note: this example will only work on u-blox ADR or High Precision GNSS or Time Sync products **
 *
 * Hardware set-up:
 * Close the DSEL jumper on the ZED-F9P breakout - to select SPI mode
 * Connect:
 * OpenLog ESP32 : ZED-F9P
 * GND             GND
 * 3V3_SW          3V3
 * SCK  (18)       SCK
 * PICO (23)       PICO (MOSI)
 * POCI (19)       POCI (MISO)
 * 33              CS
 * 
 * Note that the numSFRBX and numRAWX may lag behind what is actually being logged
 * to microSD. For best results, to ensure numSFRBX and numRAWX are accurate:
 *   Edit u-blox_struct.h and increase UBX_RXM_SFRBX_CALLBACK_BUFFERS to ~60
 *   Edit ubxRXMRAWX.h and increase numCallbackCopies to ~4
 * 
 * Feel like supporting open source hardware?
 * Buy a board from SparkFun!
 * https://www.sparkfun.com/sparkfun-allband-gnss-rtk-breakout-zed-x20p-qwiic.html
 * https://www.sparkfun.com/sparkfun-gps-rtk2-board-zed-f9p-qwiic-gps-15136.html
 * https://www.sparkfun.com/sparkfun-gps-rtk-sma-breakout-zed-f9p-qwiic.html
 */

#include "FS.h"
#include <SPI.h> //Needed for SPI to GNSS
#include "FS.h"
#include "SD_MMC.h"
#include <Wire.h>

#define GNSS_CS 33   // Connect the ZED-F9P CS pin to OpenLog ESP32 pin 33
#define EN_3V3_SW 32 // The 3.3V_SW regulator Enable pin is connected to D32
#define STAT_LED 25  // The OpenLog ESP32 STAT LED is connected to pin 25
#define IMU_CS 5     // The ISM330 IMU CS is connected to pin 5
#define MAG_CS 27    // The MMC5983 Mag CS is connected to pin 27

#define spiPort SPI

#include <SparkFun_u-blox_GNSS_v4.h> //Click here to get the library: http://librarymanager/All#SparkFun_u-blox_GNSS_v4
SFE_UBLOX_GNSS_SPI myGNSS;

File myFile; //File that all GNSS data is written to

#define sdWriteSize 2048     // Write data to the SD card in blocks of n*512 bytes
#define fileBufferSize 65530 // Allocate just under 64KBytes of RAM for UBX message storage
#define navRate 20           // Set the Nav Rate (Frequency) to 20Hz
//#define ubxOnly            // Uncomment this line to log UBX (RAWX and SFRBX) only
uint8_t *myBuffer;           // Use myBuffer to hold the data while we write it to SD card

unsigned long lastPrint; // Record when the last Serial print took place

int numSFRBX = 0; // Keep count of how many SFRBX messages have been received (see note above)
int numRAWX = 0;  // Keep count of how many RAWX messages have been received (see note above)

// Set the status LED
void statLED(bool on)
{
  if (STAT_LED >= 0)
    digitalWrite(STAT_LED, on ? HIGH : LOW);
}

// Callback: newRXM will be called when new RXM RAWX or SFRBX data arrives
void newRXM(ubxCallbackDataCommon_t *theData)
{
  if (theData->Class == UBX_CLASS_RXM)
  {
    if (theData->ID == UBX_RXM_RAWX) // Check if this is RAWX
      numRAWX++; // Increment the count
    else if (theData->ID == UBX_RXM_SFRBX) // Check if this is SFRBX
      numSFRBX++; // Increment the count
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(STAT_LED, OUTPUT); // Flash the STAT LED each time we write to the SD card
  statLED(false);

  pinMode(GNSS_CS, OUTPUT);
  digitalWrite(GNSS_CS, HIGH);
  pinMode(IMU_CS, OUTPUT);
  digitalWrite(IMU_CS, HIGH);
  pinMode(MAG_CS, OUTPUT);
  digitalWrite(MAG_CS, HIGH);

  spiPort.begin();

  // Do a fake transaction to initialize the SPI pins
  spiPort.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  spiPort.transfer(0);
  spiPort.endTransaction();

  pinMode(EN_3V3_SW, OUTPUT); // Enable power for the microSD card and GNSS
  digitalWrite(EN_3V3_SW, HIGH);
  
  delay(3000); // Allow time for the GNSS and SD card to start up and for Tera Term to reconnect
  Serial.println(F("SparkFun OpenLog ESP32 GNSS Logging : SPI and SDIO"));

  // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Initialize the GNSS

  Serial.println(F("Initializing the GNSS..."));

  //myGNSS.enableDebugging(); // Uncomment this line to enable lots of helpful GNSS debug messages on Serial
  //myGNSS.enableDebugging(Serial, true); // Uncomment this line to enable only the important GNSS debug messages on Serial

  // RAWX messages can be over 3KBytes in size, so we need to make sure we allocate enough RAM to hold all the data.
  myGNSS.setFileBufferSize(fileBufferSize); // setFileBufferSize must be called _before_ .begin

  // Connect to the u-blox module using SPI port, csPin and speed setting
  // ublox devices generally work up to 5MHz. We'll use 4MHz for this example:
  bool begun = false;
  do
  {
    begun = myGNSS.begin(spiPort, GNSS_CS, 4000000);
    if (!begun)
    {
      Serial.println(F("u-blox GNSS not detected on SPI bus."));
      delay(1000);
    }
  }
  while (!begun);
  
  // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Wait for a 3D fix. Get the date and time for the log file

  //myGNSS.factoryDefault(); delay(5000); // Uncomment this line to reset the module back to its factory defaults

#ifdef ubxOnly
  myGNSS.setSPIOutput(COM_TYPE_UBX); //Set the SPI port to output only UBX
#else
  myGNSS.setSPIOutput(COM_TYPE_UBX | COM_TYPE_NMEA); //Set the SPI port to output both UBX and NMEA messages
#endif

  //myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Optional: save (only) the communications port settings to flash and BBR

  Serial.print(F("Waiting for a 3D fix"));
  delay(100);

  uint8_t fix = 0;

  do
  {
    if (myGNSS.getNAVPVT()) // Use the helper method
      fix = myGNSS.getFixType(); // Use the helper method
    delay(1000);
    Serial.print(F("."));
  }
  while ( fix != 3 ); // Wait for a 3D fix

  Serial.println();

  // Read the date and time using the helper methods
  // from the most recent NAV-PVT data - collected
  // by getNAVPVT() above
  uint16_t y = myGNSS.getYear();
  uint8_t M = myGNSS.getMonth();
  uint8_t d = myGNSS.getDay();
  uint8_t h = myGNSS.getHour();
  uint8_t m = myGNSS.getMinute();
  uint8_t s = myGNSS.getSecond();

  char szBuffer[40] = {'\0'};
  snprintf(szBuffer, sizeof(szBuffer), "/%04d%02d%02d%02d%02d%02d.ubx", y, M, d, h, m, s);

  Serial.println(F("GNSS initialized."));

  // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Initialize the SD card. Open the log file

  Serial.println(F("Initializing SD card..."));

  // Begin the SD card
  if(!SD_MMC.begin())
  {
    Serial.println(F("Card mount failed. Freezing..."));
    while(1);
  }

  // Open the log file for writing
  Serial.printf("Log file is: %s\r\n", szBuffer);
  myFile = SD_MMC.open((const char *)szBuffer, FILE_WRITE);
  if(!myFile)
  {
    Serial.println(F("Failed to open log file for writing. Freezing..."));
    while(1);
  }

  Serial.println(F("SD card initialized."));

  // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Wait for a key press

  while (Serial.available()) // Make sure the Serial buffer is empty
  {
    Serial.read();
  }

  Serial.println(F("Press any key to start logging."));

  while (!Serial.available()) // Wait for the user to press a key
  {
    ; // Do nothing
  }

  delay(100); // Wait, just in case multiple characters were sent

  while (Serial.available()) // Empty the Serial buffer
  {
    Serial.read();
  }

  // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Enable RAWX and SFRBX

  // setCfgValset() writes to the RAM and Battery-backed-RAM layers by default (VAL_LAYER_RAM_BBR).
  // To use a different layer, add it as the third parameter. E.g. for RAM only:
  //   setCfgValset(UBLOX_CFG_MSGOUT_..., n, VAL_LAYER_RAM)
  // VAL_LAYER_ALL also saves the setting in Flash (if the module has Flash).

  // Enable the RXM RAWX Message on SPI, every navigation cycle
  myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_SPI, 1);

  // Set up a callback for RXM RAWX messages. Call newRXM() each time one arrives.
  // Note: this does not enable the RAWX message. The message is assumed to be periodic.
  // All this does is register the callback.
  myGNSS.setAutoCallbackPtr("RXM", "RAWX", &newRXM);

  myGNSS.logUBX("RXM", "RAWX"); // Enable RXM RAWX data logging

  // Enable the RXM SFRBX Message on SPI, every navigation cycle
  myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_SPI, 1);

  // Use the same callback for RXM SFRBX messages. Call newRXM() each time one arrives.
  myGNSS.setAutoCallbackPtr("RXM", "SFRBX", &newRXM);

  myGNSS.logUBX("RXM", "SFRBX"); // Enable RXM SFRBX data logging

  myBuffer = new uint8_t[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card  

#ifndef ubxOnly
  // newCfgValset() writes to the RAM and Battery-backed-RAM layers by default (VAL_LAYER_RAM_BBR).
  // To use a different layer, pass it as the parameter. E.g. for RAM only:
  //   newCfgValset(VAL_LAYER_RAM)
  // VAL_LAYER_ALL also saves the settings in Flash (if the module has Flash).
  myGNSS.newCfgValset(VAL_LAYER_RAM);
  myGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_SPI, navRate); // Ensure the GxGGA (Global positioning system fix data) message is enabled. Send every second.
  myGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_SPI, navRate); // Ensure the GxGSA (GNSS DOP and Active satellites) message is enabled. Send every second.
  myGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_SPI, navRate); // Ensure the GxGSV (GNSS satellites in view) message is enabled. Send every second.
  myGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GST_SPI, navRate); // Ensure the GxGST (Position error statistics) message is enabled. Send every second.
  myGNSS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_SPI, navRate); // Ensure the GxRMC (Recommended minimum: position, velocity and time) message is enabled. Send every second.
  myGNSS.sendCfgValset();
  myGNSS.setNMEALoggingMask(SFE_UBLOX_FILTER_NMEA_GGA | SFE_UBLOX_FILTER_NMEA_GSA | SFE_UBLOX_FILTER_NMEA_GSV | SFE_UBLOX_FILTER_NMEA_GST | SFE_UBLOX_FILTER_NMEA_RMC); // Log only these NMEA messages
#endif

  myGNSS.setNavigationFrequency(navRate); // Set navigation rate

  Serial.println(F("Press any key to stop logging."));

  lastPrint = millis(); // Initialize lastPrint
}

void loop()
{
  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  while (myGNSS.fileBufferAvailable() >= sdWriteSize) // Check to see if we have at least sdWriteSize waiting in the buffer
  {
    statLED(true); // Flash STAT_LED each time we write to the SD card

    myGNSS.extractFileBufferData(myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer

    myFile.write(myBuffer, sdWriteSize); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card

    // In case the SD writing is slow or there is a lot of data to write, keep checking for the arrival of new data
    myGNSS.checkUblox(); // Check for the arrival of new data and process it.
    myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

    statLED(false); // Turn STAT_LED off again
  }

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  if ((millis() - lastPrint) > 1000) // Print the message count once per second
  {
    Serial.print(F("Number of message groups received: SFRBX: ")); // Print how many message groups have been received (see note above)
    Serial.print(numSFRBX);
    Serial.print(F(" RAWX: "));
    Serial.println(numRAWX);

    uint16_t maxBufferBytes = myGNSS.getMaxFileBufferAvail(); // Get how full the file buffer has been (not how full it is now)

    //Serial.print(F("The maximum number of bytes which the file buffer has contained is: ")); // It is a fun thing to watch how full the buffer gets
    //Serial.println(maxBufferBytes);

    if (maxBufferBytes > ((fileBufferSize / 5) * 4)) // Warn the user if fileBufferSize was more than 80% full
    {
      Serial.println(F("Warning: the file buffer has been over 80% full. Some data may have been lost."));
    }

    lastPrint = millis(); // Update lastPrint
  }

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  if (Serial.available()) // Check if the user wants to stop logging
  {
    uint16_t remainingBytes = myGNSS.fileBufferAvailable(); // Check if there are any bytes remaining in the file buffer

    while (remainingBytes > 0) // While there is still data in the file buffer
    {
      statLED(true); // Flash STAT_LED each time we write to the SD card

      uint16_t bytesToWrite = remainingBytes; // Write the remaining bytes to SD card sdWriteSize bytes at a time
      if (bytesToWrite > sdWriteSize)
      {
        bytesToWrite = sdWriteSize;
      }

      myGNSS.extractFileBufferData(myBuffer, bytesToWrite); // Extract bytesToWrite bytes from the UBX file buffer and put them into myBuffer

      myFile.write(myBuffer, bytesToWrite); // Write bytesToWrite bytes from myBuffer to the ubxDataFile on the SD card

      remainingBytes -= bytesToWrite; // Decrement remainingBytes
    }

    statLED(false); // Turn STAT_LED off again

    myFile.close(); // Close the data file

    myGNSS.setNavigationFrequency(1); // Set navigation rate to 1Hz

    myGNSS.newCfgValset(VAL_LAYER_RAM);
    myGNSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_SPI, 0);
    myGNSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_SPI, 0);
    myGNSS.sendCfgValset();

    myGNSS.setSPIOutput(COM_TYPE_UBX | COM_TYPE_NMEA); // Re-enable NMEA

    Serial.println(F("Logging stopped. Freezing..."));
    while(1); // Do nothing more
  }

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
}
