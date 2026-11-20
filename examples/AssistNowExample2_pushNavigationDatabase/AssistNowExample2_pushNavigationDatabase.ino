/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Push the AssistNow Autonomous / navigation database back to the MAX-M10S
 * By: Paul Clark
 * SparkFun Electronics
 *
 * Based on the v3 library example Example3_AssistNowAutonomous_DatabaseWrite.
 *
 * This example shows how to push the navigation database - read by AssistNowExample1_readNavigationDatabase -
 * back to the module with pushAssistNowData().
 *
 * Where does the database come from?
 *   On ESP32: the example first tries to read the database from the microSD card file written by
 *   AssistNowExample1 (/MGADBD.UBX). The file contains the raw UBX-MGA-DBD messages
 *   If the file can not be read (no card, no file, or no valid UBX-MGA-DBD messages in it) - or if this is not
 *   an ESP32 - the example pushes database[databaseLen] instead. Copy and paste the C array printed by
 *   AssistNowExample1 into database.h
 *
 * The module needs to know the time to make the best use of the database. On ESP32, the example connects to WiFi
 * and gets the time from NTP. Update secrets.h with your WiFi credentials. The time is pushed to the module with
 * setUTCTimeAssistance(), before the database. (On other processors, no time is pushed. You could use an RTC.)
 *
 * The example:
 *   checks the database: it counts the UBX-MGA-DBD messages and checks their checksums
 *   on ESP32: connects to WiFi and gets the time from NTP
 *   optionally performs a GNSS cold start (COLD_START_FIRST). This clears the navigation data the module already
 *   has. All SparkFun boards have a backup battery, which means the module keeps its navigation data if you
 *   disconnect the power. Without the cold start, you will not see much difference in the time to first fix
 *   enables acknowledgement of assistance data (CFG-NAVSPG-ACKAIDING)
 *   on ESP32: pushes the time (UBX-MGA-INI-TIME_UTC)
 *   pushes the database. Each message is acknowledged by the module with a UBX-MGA-ACK. With PUSH_DEBUG set to 1,
 *   the library prints one "pushAssistNowData:" line per message, saying if it was accepted, and the number of
 *   packets processed
 *   prints the fix type and SIV once per second, and the time to the first 3D fix
 *
 * Set PUSH_DATABASE to 0 for a control run. The example still performs the cold start and pushes the time, but does
 * not push the database. Compare the time to first fix with and without the database.
 *
 * Notes:
 *   "UBX-MGA-DBD messages are only intended to be sent back to the same receiver that generated them."
 *   (u-blox M10 SPG 5.10 Interface Description, section 3.13.4.2.) Push the database to the module which created it
 *   The database in database.h is stored in flash on ESP32, but some processors (e.g. AVR) copy const arrays
 *   into RAM. The database can be several kBytes long. An ESP32 is recommended
 *   This example will not work on the ZED-F9P. "The ZED-F9P supports AssistNow Online only."
 *
 * Feel like supporting open source hardware?
 * Buy a board from SparkFun!
 * https://www.sparkfun.com/sparkfun-gnss-receiver-breakout-max-m10s-qwiic.html
 *
 * Hardware Connections:
 * Plug a Qwiic cable into the GNSS and your microcontroller board (e.g. the SparkFun Thing Plus C - ESP32 WROOM)
 * On ESP32: insert the microSD card written by AssistNowExample1. The microSD (SPI) chip select pin is sd_cs
 * (5 on the Thing Plus C)
 * Open the serial monitor at 115200 baud to see the output
 */

#include "database.h" // <- Copy and paste the database printed by AssistNowExample1 into database.h

#include <Wire.h> //Needed for I2C to GNSS

#include <SparkFun_u-blox_GNSS_v4.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v4

SFE_UBLOX_GNSS myGNSS; // SFE_UBLOX_GNSS uses I2C

#define MAX_DATABASE_LENGTH 32768 // The maximum database file length (bytes)
#define COLD_START_FIRST 1        // Set to 0 to push the database without performing a cold start first
#define MGA_ACK_MAXWAIT 250       // pushAssistNowData / setUTCTimeAssistance: wait up to this many ms for each UBX-MGA-ACK
#define PUSH_DEBUG 1              // Set to 0 to disable the "pushAssistNowData:" debug messages
#define PUSH_DATABASE 1           // Set to 0 for a control run: everything else is the same, but the database is not pushed

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#if defined(ARDUINO_ARCH_ESP32)

#include "FS.h"
#include <SPI.h>
#include <SD.h>
const int sd_cs = 5; // microSD (SPI) chip select pin. 5 is correct for the SparkFun Thing Plus C. Change this if required
#define DATABASE_FILE_NAME "/MGADBD.UBX" // The database written by AssistNowExample1, in raw UBX format

#include <WiFi.h>
#include <sys/time.h>
#include "time.h"
#include "esp_sntp.h"
#include "secrets.h" // <- Update secrets.h with your WiFi credentials

const char *ntpServer1 = "pool.ntp.org"; // The Network Time Protocol servers
const char *ntpServer2 = "time.nist.gov";

#define WIFI_CONNECT_TIMEOUT 20000 // Wait this many ms for the WiFi connection
#define NTP_SYNC_TIMEOUT 20000     // Wait this many ms for the time from NTP

#endif

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Find the next complete UBX message in buf, starting at *pos.
// Returns false when there are no more complete messages. *strayBytes counts bytes which are not part of a message.
bool nextUbxMessage(const uint8_t *buf, size_t len, size_t *pos, size_t *msgStart, uint16_t *payloadLen, size_t *strayBytes, size_t *badChecksums)
{
  while (*pos + 8 <= len)
  {
    if ((buf[*pos] == 0xB5) && (buf[*pos + 1] == 0x62))
    {
      uint16_t plen = (uint16_t)buf[*pos + 4] | ((uint16_t)buf[*pos + 5] << 8);
      if (*pos + 8 + plen <= len) // Is the whole message in the buffer?
      {
        uint8_t ckA = 0, ckB = 0;
        for (size_t i = *pos + 2; i < *pos + 6 + plen; i++)
        {
          ckA += buf[i];
          ckB += ckA;
        }
        if ((ckA == buf[*pos + 6 + plen]) && (ckB == buf[*pos + 7 + plen]))
        {
          *msgStart = *pos;
          *payloadLen = plen;
          *pos += 8 + plen;
          return true;
        }
        (*badChecksums)++; // Checksum failure
      }
      // Not a valid (or complete) message. Keep looking, one byte further on
    }
    (*pos)++;
    (*strayBytes)++;
  }
  (*strayBytes) += len - *pos; // Count any trailing bytes too
  *pos = len;
  return false;
}

// Check the database: print how many UBX-MGA-DBD messages it contains. Returns the number of UBX-MGA-DBD messages
uint16_t checkDatabase(const char *title, const uint8_t *buf, size_t len)
{
  size_t pos = 0, msgStart, stray = 0, bad = 0;
  uint16_t plen, dbdCount = 0, otherCount = 0, dbdMin = 0xFFFF, dbdMax = 0;

  while (nextUbxMessage(buf, len, &pos, &msgStart, &plen, &stray, &bad))
  {
    if ((buf[msgStart + 2] == UBX_CLASS_MGA) && (buf[msgStart + 3] == UBX_MGA_DBD))
    {
      dbdCount++;
      if (plen < dbdMin)
        dbdMin = plen;
      if (plen > dbdMax)
        dbdMax = plen;
    }
    else
      otherCount++;
  }

  Serial.print(title);
  Serial.print(F(" ("));
  Serial.print(len);
  Serial.println(F(" bytes):"));
  Serial.print(F("  UBX-MGA-DBD messages: "));
  Serial.print(dbdCount);
  if (dbdCount > 0)
  {
    Serial.print(F(" (payload length min "));
    Serial.print(dbdMin);
    Serial.print(F(" max "));
    Serial.print(dbdMax);
    Serial.print(F(")"));
  }
  Serial.println();
  Serial.print(F("  Other UBX messages: "));
  Serial.print(otherCount);
  Serial.print(F(". Checksum failures: "));
  Serial.print(bad);
  Serial.print(F(". Bytes outside UBX messages: "));
  Serial.println(stray);
  return dbdCount;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#if defined(ARDUINO_ARCH_ESP32)

// Read the database from microSD. Returns a pointer to the database (allocated with new), or nullptr if the read fails
uint8_t *readDatabaseFile(size_t *fileLen)
{
  *fileLen = 0;

  Serial.println(F("Initializing the microSD card..."));
  if (!SD.begin(sd_cs))
  {
    Serial.println(F("microSD card failed, or not present"));
    return nullptr;
  }

  uint8_t *fileData = nullptr;

  File dbFile = SD.open(DATABASE_FILE_NAME, FILE_READ);
  if (!dbFile)
  {
    Serial.println(F("Could not open " DATABASE_FILE_NAME "!"));
  }
  else
  {
    size_t fileSize = dbFile.size();
    Serial.print(F(DATABASE_FILE_NAME " is "));
    Serial.print(fileSize);
    Serial.println(F(" bytes"));

    if ((fileSize == 0) || (fileSize > MAX_DATABASE_LENGTH))
    {
      Serial.println(F("The file is empty, or too large!"));
    }
    else
    {
      fileData = new uint8_t[fileSize];
      if (fileData == nullptr)
      {
        Serial.println(F("Could not allocate storage for the file!"));
      }
      else
      {
        size_t bytesRead = dbFile.read(fileData, fileSize);
        if (bytesRead != fileSize)
        {
          Serial.print(F("File read failed! Bytes read: "));
          Serial.println(bytesRead);
          delete[] fileData;
          fileData = nullptr;
        }
        else
          *fileLen = fileSize;
      }
    }
    dbFile.close();
  }

  SD.end();
  return fileData;
}

// Connect to WiFi and set the ESP32 system time from NTP. Returns true if the time was set
bool getNetworkTime()
{
  Serial.print(F("Connecting to WiFi network "));
  Serial.print(ssid);

  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  while ((WiFi.status() != WL_CONNECTED) && ((millis() - startTime) < WIFI_CONNECT_TIMEOUT))
  {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();

  bool timeIsSet = false;

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println(F("WiFi connection failed! Have you updated secrets.h?"));
  }
  else
  {
    Serial.println(F("WiFi connected"));

    // Start SNTP. Set the GMT and daylight offsets to zero. We need UTC, not local time
    configTime(0, 0, ntpServer1, ntpServer2);

    // Wait for SNTP to set the system time.
    // Note: sntp_get_sync_status() resets the status after returning SNTP_SYNC_STATUS_COMPLETED
    startTime = millis();
    while ((!timeIsSet) && ((millis() - startTime) < NTP_SYNC_TIMEOUT))
    {
      timeIsSet = (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED);
      if (!timeIsSet)
        delay(100);
    }

    if (timeIsSet)
    {
      struct tm timeinfo;
      if (getLocalTime(&timeinfo))
        Serial.println(&timeinfo, "Time from NTP (UTC): %A, %B %d %Y %H:%M:%S");
    }
    else
      Serial.println(F("Failed to obtain the time from NTP!"));

    esp_sntp_stop(); // The ESP32 RTC keeps the time. Stop SNTP before disconnecting
  }

  // Disconnect the WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println(F("WiFi disconnected"));

  return timeIsSet;
}

// Push the ESP32 system time (UTC) to the module
bool pushTimeAssistance()
{
  struct timeval tv;
  gettimeofday(&tv, nullptr); // Get the time, with microseconds
  struct tm utc;
  gmtime_r(&tv.tv_sec, &utc);

  Serial.println(&utc, "Pushing the time (UTC) to the module: %Y-%m-%d %H:%M:%S");

  // setUTCTimeAssistance uses a default time accuracy of 2 seconds which should be OK here.
  // Have a look at the library source code for more details.
  return myGNSS.setUTCTimeAssistance(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                                     utc.tm_hour, utc.tm_min, utc.tm_sec, ((uint32_t)tv.tv_usec) * 1000,
                                     defaultMGAINITIMEtAccS, defaultMGAINITIMEtAccNs, defaultMGAINITIMEsource,
                                     SFE_UBLOX_MGA_ASSIST_ACK_YES, MGA_ACK_MAXWAIT);
}

#endif

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

unsigned long pushedAt;    // millis() when the database was pushed
bool firstFixSeen = false; // Becomes true when the first 3D fix is seen

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("SparkFun u-blox Example"));

  while (Serial.available()) Serial.read(); // Empty the serial buffer
  Serial.println(F("Press any key to begin..."));
  while (!Serial.available()); // Wait for a keypress

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Find the database: from microSD (on ESP32), or from database[databaseLen]

  const uint8_t *pushData = nullptr; // Points to the database which will be pushed
  size_t pushLen = 0;
  uint8_t *fileData = nullptr; // The database read from microSD

#if defined(ARDUINO_ARCH_ESP32)
  size_t fileLen;
  fileData = readDatabaseFile(&fileLen);
  if (fileData != nullptr)
  {
    if (checkDatabase("The database read from " DATABASE_FILE_NAME, fileData, fileLen) > 0)
    {
      pushData = fileData;
      pushLen = fileLen;
    }
    else
    {
      Serial.println(F("The file does not contain any valid UBX-MGA-DBD messages!"));
      delete[] fileData;
      fileData = nullptr;
    }
  }
  if (pushData == nullptr)
    Serial.println(F("Using database[databaseLen] instead"));
#endif

  if (pushData == nullptr)
  {
    if ((databaseLen > 0) && (checkDatabase("database[databaseLen]", database, databaseLen) > 0))
    {
      pushData = database;
      pushLen = databaseLen;
    }
    else
    {
      Serial.println(F("database[databaseLen] does not contain any valid UBX-MGA-DBD messages!"));
      Serial.println(F("Copy and paste the database printed by AssistNowExample1 into database.h. Freezing..."));
      while (1);
    }
  }

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // On ESP32: connect to WiFi and get the time from NTP. The ESP32 RTC keeps the time after WiFi is disconnected

#if defined(ARDUINO_ARCH_ESP32)
  bool timeIsSet = getNetworkTime();
#endif

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Start I2C. Connect to the GNSS.

  Wire.begin(); // Start I2C

  //myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  while (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Retrying..."));
    delay (1000);
  }
  Serial.println(F("u-blox module connected"));

  // The settings below are made in RAM only. They are cleared when the module is powered off.

  myGNSS.setI2COutput(COM_TYPE_UBX, VAL_LAYER_RAM); // Turn off NMEA noise

  if (myGNSS.setAopCfg(1, 0, VAL_LAYER_RAM) == true) // Enable AssistNow Autonomous (CFG-ANA-USE_ANA)
    Serial.println(F("AssistNow Autonomous enabled"));
  else
    Serial.println(F("Could not enable AssistNow Autonomous!"));

  myGNSS.setNavigationFrequency(1, VAL_LAYER_RAM); // Produce one solution per second

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Optional: perform a GNSS cold start, to clear the module's own navigation data

#if COLD_START_FIRST
  // UBX-CFG-RST: navBbrMask 0xFFFF (cold start: clear all navigation data), resetMode 0x02 (controlled software
  // reset - GNSS only). Only the GNSS tasks are restarted. The configuration is not changed.
  uint8_t coldStart[4] = {0xFF, 0xFF, 0x02, 0x00};
  myGNSS.cfgRst(coldStart, 4);
  Serial.println(F("GNSS cold start performed. The module's navigation data has been cleared"));
  delay(1000); // Give the GNSS time to restart
#endif

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Push the time and the database to the module

  // Enable acknowledgement of assistance data (CFG-NAVSPG-ACKAIDING), so each message is acknowledged
  if (myGNSS.setAckAiding(1, VAL_LAYER_RAM) == false)
    Serial.println(F("Could not enable ACKAIDING!"));

#if PUSH_DEBUG
  Serial.println(F("Limited debug messages are enabled during the push:"));
  myGNSS.enableDebugging(Serial, true); // Enable "important" debug messages only
#endif

#if defined(ARDUINO_ARCH_ESP32)
  if (timeIsSet)
  {
    if (pushTimeAssistance() == false)
      Serial.println(F("Could not push the time!"));
  }
  else
    Serial.println(F("The time is not known. Pushing the database without it. This will not work as well..."));
#endif

#if PUSH_DATABASE
  Serial.println(F("Pushing the database to the module..."));

  unsigned long pushStart = millis();
  size_t bytesPushed = myGNSS.pushAssistNowData(pushData, pushLen, SFE_UBLOX_MGA_ASSIST_ACK_YES, MGA_ACK_MAXWAIT);
  pushedAt = millis();

#if PUSH_DEBUG
  myGNSS.disableDebugging();
#endif

  // Note: bytesPushed is the number of bytes sent to the module. The "pushAssistNowData:" debug messages show
  // how many messages were accepted (acknowledged) by the module
  Serial.print(F("Pushed "));
  Serial.print(bytesPushed);
  Serial.print(F(" of "));
  Serial.print(pushLen);
  Serial.print(F(" bytes of navigation database to the module. Push took "));
  Serial.print(pushedAt - pushStart);
  Serial.println(F(" ms"));

  if (bytesPushed != pushLen)
    Serial.println(F("Warning: bytesPushed does not match the database length! Maybe the database contains bad data? Or there was a communication error?"));
#else
  pushedAt = millis();

#if PUSH_DEBUG
  myGNSS.disableDebugging();
#endif

  (void)pushData; // pushData and pushLen are not used in the control run
  (void)pushLen;
  Serial.println(F("PUSH_DATABASE is 0. This is a control run: the database has NOT been pushed"));
#endif

  if (fileData != nullptr)
    delete[] fileData; // Free the file data

  Serial.println(F("Waiting for the first 3D fix..."));
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void loop()
{
  // Poll NAV-PVT once per second. Print the fix type and SIV, so we can see how quickly the fixType goes to 3D

  if (myGNSS.getNAVPVT() == true) // Use the helper method getNAVPVT() to poll NAV-PVT
  {
    float seconds = ((float)(millis() - pushedAt)) / 1000.0;
    uint8_t fixType = myGNSS.getFixType();

    Serial.print(seconds, 1);
    Serial.print(F("s  Fix: "));
    if (fixType == 0) Serial.print(F("No fix"));
    else if (fixType == 1) Serial.print(F("Dead reckoning"));
    else if (fixType == 2) Serial.print(F("2D"));
    else if (fixType == 3) Serial.print(F("3D"));
    else if (fixType == 4) Serial.print(F("GNSS + Dead reckoning"));
    else if (fixType == 5) Serial.print(F("Time only"));
    Serial.print(F("  SIV: "));
    Serial.print(myGNSS.getSIV());
    Serial.print(F("  Lat: "));
    Serial.print(myGNSS.getLatitude());
    Serial.print(F("  Long: "));
    Serial.print(myGNSS.getLongitude());
    Serial.println(F(" (degrees * 10^-7)"));

    if ((!firstFixSeen) && ((fixType == 3) || (fixType == 4)))
    {
      firstFixSeen = true;
      Serial.print(F("Time to first 3D fix (since the push): "));
      Serial.print(seconds, 1);
      Serial.println(F(" s"));
    }
  }

  delay(1000);
}
