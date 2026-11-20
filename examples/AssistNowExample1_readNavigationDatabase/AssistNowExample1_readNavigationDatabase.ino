/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Read the AssistNow Autonomous / navigation database from the MAX-M10S
 * By: Paul Clark
 * SparkFun Electronics
 *
 * Based on the v3 library example Example2_AssistNowAutonomous_DatabaseRead.
 *
 * This example shows how to:
 *   enable AssistNow Autonomous (CFG-ANA-USE_ANA) and watch its progress with NAV-SAT (aopAvail) and NAV-AOPSTATUS
 *   read the navigation database from the module with readNavigationDatabase()
 *   print the database to Serial, as a C array which can be copied into your own code
 *   on ESP32: write the database to a microSD card file in raw UBX format (/MGADBD.UBX)
 * The next example (AssistNowExample2) will read the file and push the database back to the module.
 *
 * Note: UBX-NAV-AOPSTATUS is documented in the M10 SPG 5.10 Interface Description, but not in SPG 5.00
 * (only its CFG-MSGOUT keys are). Even so, a MAX-M10S whose UBX-MON-VER reports "ROM SPG 5.00" does output it:
 * this example was tested on one.
 *
 * readNavigationDatabase() polls UBX-MGA-DBD. The module sends its whole database as a series of UBX-MGA-DBD
 * messages, then a UBX-MGA-ACK whose msgPayloadStart field contains the number of UBX-MGA-DBD messages it sent.
 * (u-blox M10 SPG 5.10 Interface Description, section 3.13.4.1.) The database returned by readNavigationDatabase()
 * is those UBX-MGA-DBD messages, complete with sync chars and checksums - i.e. raw UBX - so the file size is
 * databaseLen bytes.
 *
 * Diagnostics for GitHub issue #5 (https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/issues/5):
 * on a MAX-M10S, readNavigationDatabase() was reported to receive one fewer UBX-MGA-DBD message than the
 * closing UBX-MGA-ACK says were sent, and to then wait for the full maxWait. To help find out why:
 *   Limited ("important only") debug messages are enabled during readNavigationDatabase(). Look for lines
 *   starting "readNavigationDatabase:" and "processUBX:" (e.g. "processUBX: buffer overrun detected!", which
 *   includes the message Len)
 *   Every byte received during readNavigationDatabase() is also captured, using setOutputPort(). The example
 *   then counts the UBX-MGA-DBD messages the module actually sent, compares them one by one with the database
 *   the library returned, and prints any which are missing - with their length
 *   The time taken is printed. If it is close to maxWait, readNavigationDatabase() did not see a matching ACK
 * Set CAPTURE_RAW_DATA to 0 to disable the capture (it needs another CAPTURE_BUFFER_LENGTH bytes of RAM).
 *
 * Note: this example needs plenty of RAM. The database can be several kBytes long. An ESP32 is recommended.
 *
 * Feel like supporting open source hardware?
 * Buy a board from SparkFun!
 * https://www.sparkfun.com/sparkfun-gnss-receiver-breakout-max-m10s-qwiic.html
 *
 * Hardware Connections:
 * Plug a Qwiic cable into the GNSS and your microcontroller board (e.g. the SparkFun Thing Plus C - ESP32 WROOM)
 * On ESP32: insert a formatted microSD card. The microSD (SPI) chip select pin is sd_cs (5 on the Thing Plus C)
 * Open the serial monitor at 115200 baud to see the output
 */

#include <Wire.h> //Needed for I2C to GNSS

#include <SparkFun_u-blox_GNSS_v4.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v4

#if defined(ARDUINO_ARCH_ESP32)
#include "FS.h"
#include <SPI.h>
#include <SD.h>
const int sd_cs = 5; // microSD (SPI) chip select pin. 5 is correct for the SparkFun Thing Plus C. Change this if required
#define DATABASE_FILE_NAME "/MGADBD.UBX" // The database is written to this file, in raw UBX format
#define CAPTURE_FILE_NAME "/MGARAW.UBX"  // Diagnostics: the raw data received during readNavigationDatabase()
#endif

SFE_UBLOX_GNSS myGNSS; // SFE_UBLOX_GNSS uses I2C

#define MAX_DATABASE_LENGTH 32768    // Allocate 32kBytes to store the navigation database
#define DATABASE_READ_MAXWAIT 3100   // readNavigationDatabase maxWait (ms). The library default is 3100

#define CAPTURE_RAW_DATA 1           // Diagnostics: set to 0 to disable the capture of the raw data
#define CAPTURE_BUFFER_LENGTH 32768  // Allocate 32kBytes to capture the raw data

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Diagnostics: capture every byte the library receives.
// The library writes every received byte to the port set with setOutputPort(). RawCapture is a Print
// which stores those bytes in a buffer - but only while enabled is true.

class RawCapture : public Print
{
public:
  uint8_t *buffer = nullptr; // The captured bytes
  size_t size = 0;           // The size of buffer
  size_t length = 0;         // The number of bytes captured
  size_t lost = 0;           // The number of bytes which did not fit in buffer
  bool enabled = false;      // Only capture while enabled is true

  size_t write(uint8_t c) override
  {
    if (enabled)
    {
      if (length < size)
        buffer[length++] = c;
      else
        lost++;
    }
    return 1;
  }
};

RawCapture rawCapture;

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Diagnostics: helpers to find and compare the UBX messages in a buffer

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
  return false;
}

// Find the next UBX-MGA-DBD message in buf
bool nextDbdMessage(const uint8_t *buf, size_t len, size_t *pos, size_t *msgStart, uint16_t *payloadLen)
{
  size_t stray = 0, bad = 0;
  while (nextUbxMessage(buf, len, pos, msgStart, payloadLen, &stray, &bad))
  {
    if ((buf[*msgStart + 2] == UBX_CLASS_MGA) && (buf[*msgStart + 3] == UBX_MGA_DBD))
      return true;
  }
  return false;
}

// Print a summary of the UBX messages in buf. Returns the number of UBX-MGA-DBD messages
uint16_t summarizeUbx(const char *title, const uint8_t *buf, size_t len, bool printLengths)
{
  size_t pos = 0, msgStart, stray = 0, bad = 0;
  uint16_t plen, dbdCount = 0, otherCount = 0, dbdMin = 0xFFFF, dbdMax = 0;
  uint8_t lengthsOnLine = 0; // The number of payload lengths printed on the current line

  Serial.println();
  Serial.print(title);
  Serial.print(F(" ("));
  Serial.print(len);
  Serial.println(F(" bytes):"));

  if (printLengths)
    Serial.println(F("  UBX-MGA-DBD payload lengths:"));

  while (nextUbxMessage(buf, len, &pos, &msgStart, &plen, &stray, &bad))
  {
    uint8_t cls = buf[msgStart + 2];
    uint8_t id = buf[msgStart + 3];
    if ((cls == UBX_CLASS_MGA) && (id == UBX_MGA_DBD))
    {
      if (printLengths)
      {
        if (lengthsOnLine == 0)
          Serial.print(F("   "));
        Serial.print(F(" "));
        Serial.print(plen);
        if (++lengthsOnLine == 16)
        {
          Serial.println();
          lengthsOnLine = 0;
        }
      }
      dbdCount++;
      if (plen < dbdMin)
        dbdMin = plen;
      if (plen > dbdMax)
        dbdMax = plen;
    }
    else if ((cls == UBX_CLASS_MGA) && (id == UBX_MGA_ACK_DATA0) && (plen == 8))
    {
      if (printLengths && (lengthsOnLine > 0))
      {
        Serial.println(); // End the current line of lengths
        lengthsOnLine = 0;
      }
      // UBX-MGA-ACK-DATA0: type, version, infoCode, msgId, msgPayloadStart (U1[4])
      uint32_t msgPayloadStart = (uint32_t)buf[msgStart + 10] | ((uint32_t)buf[msgStart + 11] << 8) | ((uint32_t)buf[msgStart + 12] << 16) | ((uint32_t)buf[msgStart + 13] << 24);
      Serial.print(F("  UBX-MGA-ACK: type "));
      Serial.print(buf[msgStart + 6]);
      Serial.print(F(" infoCode "));
      Serial.print(buf[msgStart + 8]);
      Serial.print(F(" msgId 0x"));
      Serial.print(buf[msgStart + 9], HEX);
      Serial.print(F(" msgPayloadStart "));
      Serial.print(msgPayloadStart);
      if (buf[msgStart + 9] == UBX_MGA_DBD)
        Serial.print(F(" <-- the number of UBX-MGA-DBD messages the module sent"));
      Serial.println();
    }
    else
      otherCount++;
  }
  if (printLengths && (lengthsOnLine > 0))
    Serial.println(); // End the last line of lengths

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

// Compare the UBX-MGA-DBD messages in the raw capture with those in the database, one by one.
// Print any which are missing from the database.
void compareDbdMessages(const uint8_t *raw, size_t rawLen, const uint8_t *db, size_t dbLen)
{
  size_t rawPos = 0, dbPos = 0, rawStart, dbStart = 0;
  uint16_t rawPlen, dbPlen = 0;
  uint16_t rawIndex = 0, missing = 0;
  bool dbValid = nextDbdMessage(db, dbLen, &dbPos, &dbStart, &dbPlen);

  Serial.println();
  Serial.println(F("Comparing the captured UBX-MGA-DBD messages with the database, one by one:"));

  while (nextDbdMessage(raw, rawLen, &rawPos, &rawStart, &rawPlen))
  {
    if (dbValid && (dbPlen == rawPlen) && (memcmp(raw + rawStart, db + dbStart, 8 + rawPlen) == 0))
    {
      dbValid = nextDbdMessage(db, dbLen, &dbPos, &dbStart, &dbPlen); // Match. Move on to the next database message
    }
    else
    {
      missing++;
      Serial.print(F("  Message #"));
      Serial.print(rawIndex + 1);
      Serial.print(F(" is MISSING from the database. Payload length "));
      Serial.print(rawPlen);
      Serial.print(F(". First payload bytes:"));
      for (uint16_t i = 0; (i < 8) && (i < rawPlen); i++)
      {
        Serial.print(F(" "));
        if (raw[rawStart + 6 + i] < 0x10)
          Serial.print(F("0"));
        Serial.print(raw[rawStart + 6 + i], HEX);
      }
      Serial.println();
    }
    rawIndex++;
  }

  if (missing == 0)
    Serial.println(F("  Every captured UBX-MGA-DBD message is in the database"));
  if (dbValid)
    Serial.println(F("  The database contains UBX-MGA-DBD message(s) which were not captured! (Was the capture buffer full?)"));
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Callback: newNAVSAT will be called when new NAV SAT data arrives
void newNAVSAT(ubxCallbackDataCommon_t *theData)
{
  ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

  uint8_t numSvs = myGNSS.getUbxMessageFieldCallback(msg, "numSvs");

  uint8_t numAopAvail = 0; // Count how many SVs have AssistNow Autonomous data available
  for (uint8_t block = 0; block < numSvs; block++)
  {
    if ((bool)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "aopAvail"))
      numAopAvail++;
  }

  Serial.println();
  Serial.print(F("UBX-NAV-SAT contains data for "));
  Serial.print(numSvs);
  Serial.println(numSvs == 1 ? F(" SV") : F(" SVs"));
  Serial.print(F("AssistNow Autonomous data is available for "));
  Serial.print(numAopAvail);
  Serial.println(numAopAvail == 1 ? F(" SV") : F(" SVs"));
}

// Callback: newNAVAOPSTATUS will be called when new NAV AOPSTATUS data arrives
void newNAVAOPSTATUS(ubxCallbackDataCommon_t *theData)
{
  ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

  uint8_t status = myGNSS.getUbxMessageFieldCallback(msg, "status");
  Serial.print(F("AOPSTATUS status is "));
  Serial.print(status);
  Serial.println(status == 0 ? F(" (idle)") : F(" (running)"));
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("SparkFun u-blox Example"));

  while (Serial.available()) Serial.read(); // Empty the serial buffer
  Serial.println(F("Press any key to begin..."));
  while (!Serial.available()); // Wait for a keypress

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

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Enable AssistNow Autonomous data collection (CFG-ANA-USE_ANA)

  if (myGNSS.setAopCfg(1, 0, VAL_LAYER_RAM) == true)
    Serial.println(F("AssistNow Autonomous enabled"));
  else
    Serial.println(F("Could not enable AssistNow Autonomous!"));

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Enable periodic UBX-NAV-SAT and UBX-NAV-AOPSTATUS messages and set up the callbacks

  myGNSS.setNavigationFrequency(1, VAL_LAYER_RAM); // Produce one solution per second

  myGNSS.setAutoUBXrate("NAV", "SAT", 1, true, VAL_LAYER_RAM);       // Enable NAV-SAT on I2C, every epoch
  myGNSS.setAutoUBXrate("NAV", "AOPSTATUS", 1, true, VAL_LAYER_RAM); // Enable NAV-AOPSTATUS on I2C, every epoch
  myGNSS.setAutoCallbackPtr("NAV", "SAT", &newNAVSAT);               // Call newNAVSAT() when NAV-SAT arrives
  myGNSS.setAutoCallbackPtr("NAV", "AOPSTATUS", &newNAVAOPSTATUS);   // Call newNAVAOPSTATUS() when NAV-AOPSTATUS arrives

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Keep displaying NAV SAT and AOPSTATUS until the user presses a key

  Serial.println(F("AssistNow Autonomous data collection is in progress. Press any key to quit and read the database."));

  while (Serial.available()) Serial.read(); // Empty the serial buffer

  while (!Serial.available()) // Wait for the arrival of a keypress
  {
    myGNSS.checkUblox();     // Check for the arrival of new data and process it.
    myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

    Serial.print(".");
    delay(50);
  }

  Serial.println();

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Disable the periodic UBX-NAV-SAT and UBX-NAV-AOPSTATUS messages

  myGNSS.setAutoUBXrate("NAV", "SAT", 0, true, VAL_LAYER_RAM);
  myGNSS.setAutoUBXrate("NAV", "AOPSTATUS", 0, true, VAL_LAYER_RAM);
  delay(1100);
  myGNSS.checkUblox(); // Process (discard) any messages which were already on their way

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Allocate storage for the database (and the raw data capture)

  uint8_t *database = new uint8_t[MAX_DATABASE_LENGTH]; // The database will be stored here
  if (database == nullptr)
  {
    Serial.println(F("Could not allocate storage for the database! Freezing..."));
    while (1);
  }
  Serial.println(F("Storage has been allocated for the database."));

#if CAPTURE_RAW_DATA
  rawCapture.buffer = new uint8_t[CAPTURE_BUFFER_LENGTH];
  if (rawCapture.buffer != nullptr)
  {
    rawCapture.size = CAPTURE_BUFFER_LENGTH;
    myGNSS.setOutputPort(rawCapture); // The library will write every received byte to rawCapture
  }
  else
    Serial.println(F("Could not allocate storage for the raw data capture. Continuing without it..."));
#endif

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Read the navigation database from the module

  Serial.println(F("Reading the database. Limited debug messages are enabled during the read:"));
  Serial.flush();

  myGNSS.enableDebugging(Serial, true); // Enable "important" debug messages only
  rawCapture.enabled = true;            // Start capturing (if the capture buffer was allocated)

  unsigned long readStart = millis();
  size_t databaseLen = myGNSS.readNavigationDatabase(database, MAX_DATABASE_LENGTH, DATABASE_READ_MAXWAIT); // Read the database
  unsigned long readTime = millis() - readStart;

  rawCapture.enabled = false; // Stop capturing
  myGNSS.disableDebugging();

  Serial.print(F("The navigation database length is "));
  Serial.print(databaseLen);
  Serial.println(F(" bytes"));
  Serial.print(F("readNavigationDatabase took "));
  Serial.print(readTime);
  Serial.print(F(" ms (maxWait is "));
  Serial.print(DATABASE_READ_MAXWAIT);
  Serial.println(F(" ms)"));
  if (readTime >= DATABASE_READ_MAXWAIT)
    Serial.println(F("readNavigationDatabase timed out: it did not see a matching UBX-MGA-ACK"));

  if (databaseLen == MAX_DATABASE_LENGTH)
    Serial.println(F("There was not enough memory to store the entire database. Some data will have been lost!"));

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Diagnostics: compare what the module sent with what the library stored

  summarizeUbx("The database returned by readNavigationDatabase", database, databaseLen, false);

#if CAPTURE_RAW_DATA
  if (rawCapture.buffer != nullptr)
  {
    summarizeUbx("The raw data received during readNavigationDatabase", rawCapture.buffer, rawCapture.length, true);
    if (rawCapture.lost > 0)
    {
      Serial.print(F("  The capture buffer was full. Bytes not captured: "));
      Serial.println(rawCapture.lost);
    }
    compareDbdMessages(rawCapture.buffer, rawCapture.length, database, databaseLen);
  }
#endif

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Print the database so it can be copied into your own code

  Serial.println();
  Serial.println(F("The database, as a C array:"));
  Serial.println();
  Serial.print(F("size_t databaseLen = "));
  Serial.print(databaseLen);
  Serial.println(F(";"));
  Serial.print(F("const uint8_t database["));
  Serial.print(databaseLen);
  Serial.println(F("] = {"));
  for (size_t i = 0; i < databaseLen; i++)
  {
    if ((i % 32) == 0)
      Serial.print(F("  0x"));
    if (database[i] < 0x10) // Print leading zero
      Serial.print(F("0"));
    Serial.print(database[i], HEX);
    if (i == (databaseLen - 1))
      Serial.println();
    else if ((i % 32) == 31)
      Serial.println(F(","));
    else
      Serial.print(F(", 0x"));
  }
  Serial.println(F("};"));

  //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // On ESP32: write the database to microSD card, in raw UBX format

#if defined(ARDUINO_ARCH_ESP32)
  Serial.println();
  Serial.println(F("Initializing the microSD card..."));
  if (!SD.begin(sd_cs))
  {
    Serial.println(F("microSD card failed, or not present. The database has not been saved"));
  }
  else
  {
    File dbFile = SD.open(DATABASE_FILE_NAME, FILE_WRITE); // FILE_WRITE overwrites any existing file
    if (!dbFile)
    {
      Serial.println(F("Could not create " DATABASE_FILE_NAME "!"));
    }
    else
    {
      size_t written = dbFile.write(database, databaseLen);
      dbFile.close();

      // Check the file size is databaseLen
      dbFile = SD.open(DATABASE_FILE_NAME, FILE_READ);
      size_t fileSize = dbFile ? dbFile.size() : 0;
      if (dbFile)
        dbFile.close();

      Serial.print(F("The database has been written to " DATABASE_FILE_NAME ". Bytes written: "));
      Serial.print(written);
      Serial.print(F(". File size: "));
      Serial.print(fileSize);
      Serial.println((fileSize == databaseLen) ? F(" (correct)") : F(" (INCORRECT - should be databaseLen!)"));
    }

#if CAPTURE_RAW_DATA
    // Diagnostics: also save the raw data, so it can be attached to the GitHub issue
    if ((rawCapture.buffer != nullptr) && (rawCapture.length > 0))
    {
      File rawFile = SD.open(CAPTURE_FILE_NAME, FILE_WRITE);
      if (rawFile)
      {
        rawFile.write(rawCapture.buffer, rawCapture.length);
        rawFile.close();
        Serial.print(F("The raw data received during readNavigationDatabase has been written to " CAPTURE_FILE_NAME ": "));
        Serial.print(rawCapture.length);
        Serial.println(F(" bytes"));
      }
    }
#endif

    SD.end();
  }
#endif

  Serial.println();
  Serial.println(F("Done!"));
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void loop()
{
  // Nothing to do here
}
