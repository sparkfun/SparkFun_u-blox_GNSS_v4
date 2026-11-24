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
 * ESP-IDF version of the Arduino example AssistNowExample1_readNavigationDatabase.ino
 * Based on the v3 library example Example2_AssistNowAutonomous_DatabaseRead.
 *
 * This example shows how to:
 *   enable AssistNow Autonomous (CFG-ANA-USE_ANA) and watch its progress with NAV-SAT (aopAvail) and NAV-AOPSTATUS
 *   read the navigation database from the module with readNavigationDatabase()
 *   print the database to the console, as a C array which can be copied into your own code
 *   write the database to a microSD card file in raw UBX format (/sdcard/MGADBD.UBX)
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
 * The microSD card is accessed over SPI using the ESP-IDF FATFS VFS (see sd_card.c), and written with the
 * standard C file functions (fopen, fwrite, fclose). If the card can not be mounted, the database is still
 * printed to the console.
 *
 * Feel like supporting open source hardware?
 * Buy a board from SparkFun!
 * https://www.sparkfun.com/sparkfun-gnss-receiver-breakout-max-m10s-qwiic.html
 *
 * Hardware Connections:
 * Plug a Qwiic cable into the GNSS and your ESP32 board (e.g. the SparkFun Thing Plus C - ESP32 WROOM)
 * Insert a formatted microSD card
 * Set the GPIO pins with: idf.py menuconfig -> "Example Configuration"
 * Build, flash and monitor with: idf.py -p PORT flash monitor
 * Press any key in the monitor when prompted
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

#include "sd_card.h"

#include "SparkFun_u-blox_GNSS_v4.h"

static SFE_UBLOX_GNSS myGNSS; // SFE_UBLOX_GNSS uses I2C

static const char *mountPoint = "/sdcard";                  // The microSD card is mounted here
static const char *databaseFileName = "/sdcard/MGADBD.UBX"; // The database is written to this file, in raw UBX format
static const char *captureFileName = "/sdcard/MGARAW.UBX";  // Diagnostics: the raw data received during readNavigationDatabase()
                                                            // (FATFS uses 8.3 file names by default)

#define MAX_DATABASE_LENGTH 32768    // Allocate 32kBytes to store the navigation database
#define DATABASE_READ_MAXWAIT 3100   // readNavigationDatabase maxWait (ms). The library default is 3100

#define CAPTURE_RAW_DATA 1           // Diagnostics: set to 0 to disable the capture of the raw data
#define CAPTURE_BUFFER_LENGTH 32768  // Allocate 32kBytes to capture the raw data

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Diagnostics: capture every byte the library receives.
// The library writes every received byte to the port set with setOutputPort(). captureBytes() is called
// through an SfeCallbackOutput, and stores those bytes in a buffer - but only while enabled is true.

typedef struct
{
    uint8_t *buffer; // The captured bytes
    size_t size;     // The size of buffer
    size_t length;   // The number of bytes captured
    size_t lost;     // The number of bytes which did not fit in buffer
    bool enabled;    // Only capture while enabled is true
} rawCapture_t;

static rawCapture_t rawCapture = {};

static void captureBytes(const uint8_t *buffer, size_t length, void *userContext)
{
    rawCapture_t *capture = (rawCapture_t *)userContext;
    if (!capture->enabled)
        return;
    for (size_t i = 0; i < length; i++)
    {
        if (capture->length < capture->size)
            capture->buffer[capture->length++] = buffer[i];
        else
            capture->lost++;
    }
}

static SparkFun_UBLOX_GNSS::SfeCallbackOutput captureOutput(captureBytes, &rawCapture);

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Diagnostics: helpers to find and compare the UBX messages in a buffer

// Find the next complete UBX message in buf, starting at *pos.
// Returns false when there are no more complete messages. *strayBytes counts bytes which are not part of a message.
static bool nextUbxMessage(const uint8_t *buf, size_t len, size_t *pos, size_t *msgStart, uint16_t *payloadLen, size_t *strayBytes, size_t *badChecksums)
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
static bool nextDbdMessage(const uint8_t *buf, size_t len, size_t *pos, size_t *msgStart, uint16_t *payloadLen)
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
static uint16_t summarizeUbx(const char *title, const uint8_t *buf, size_t len, bool printLengths)
{
    size_t pos = 0, msgStart, stray = 0, bad = 0;
    uint16_t plen, dbdCount = 0, otherCount = 0, dbdMin = 0xFFFF, dbdMax = 0;
    uint8_t lengthsOnLine = 0; // The number of payload lengths printed on the current line

    printf("\n%s (%u bytes):\n", title, (unsigned)len);

    if (printLengths)
        printf("  UBX-MGA-DBD payload lengths:\n");

    while (nextUbxMessage(buf, len, &pos, &msgStart, &plen, &stray, &bad))
    {
        uint8_t cls = buf[msgStart + 2];
        uint8_t id = buf[msgStart + 3];
        if ((cls == UBX_CLASS_MGA) && (id == UBX_MGA_DBD))
        {
            if (printLengths)
            {
                if (lengthsOnLine == 0)
                    printf("   ");
                printf(" %u", (unsigned)plen);
                if (++lengthsOnLine == 16)
                {
                    printf("\n");
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
                printf("\n"); // End the current line of lengths
                lengthsOnLine = 0;
            }
            // UBX-MGA-ACK-DATA0: type, version, infoCode, msgId, msgPayloadStart (U1[4])
            uint32_t msgPayloadStart = (uint32_t)buf[msgStart + 10] | ((uint32_t)buf[msgStart + 11] << 8) | ((uint32_t)buf[msgStart + 12] << 16) | ((uint32_t)buf[msgStart + 13] << 24);
            printf("  UBX-MGA-ACK: type %u infoCode %u msgId 0x%02X msgPayloadStart %lu%s\n",
                   (unsigned)buf[msgStart + 6], (unsigned)buf[msgStart + 8], (unsigned)buf[msgStart + 9], (unsigned long)msgPayloadStart,
                   (buf[msgStart + 9] == UBX_MGA_DBD) ? " <-- the number of UBX-MGA-DBD messages the module sent" : "");
        }
        else
            otherCount++;
    }
    if (printLengths && (lengthsOnLine > 0))
        printf("\n"); // End the last line of lengths

    printf("  UBX-MGA-DBD messages: %u", (unsigned)dbdCount);
    if (dbdCount > 0)
        printf(" (payload length min %u max %u)", (unsigned)dbdMin, (unsigned)dbdMax);
    printf("\n");
    printf("  Other UBX messages: %u. Checksum failures: %u. Bytes outside UBX messages: %u\n",
           (unsigned)otherCount, (unsigned)bad, (unsigned)stray);
    return dbdCount;
}

// Compare the UBX-MGA-DBD messages in the raw capture with those in the database, one by one.
// Print any which are missing from the database.
static void compareDbdMessages(const uint8_t *raw, size_t rawLen, const uint8_t *db, size_t dbLen)
{
    size_t rawPos = 0, dbPos = 0, rawStart, dbStart = 0;
    uint16_t rawPlen, dbPlen = 0;
    uint16_t rawIndex = 0, missing = 0;
    bool dbValid = nextDbdMessage(db, dbLen, &dbPos, &dbStart, &dbPlen);

    printf("\nComparing the captured UBX-MGA-DBD messages with the database, one by one:\n");

    while (nextDbdMessage(raw, rawLen, &rawPos, &rawStart, &rawPlen))
    {
        if (dbValid && (dbPlen == rawPlen) && (memcmp(raw + rawStart, db + dbStart, 8 + rawPlen) == 0))
        {
            dbValid = nextDbdMessage(db, dbLen, &dbPos, &dbStart, &dbPlen); // Match. Move on to the next database message
        }
        else
        {
            missing++;
            printf("  Message #%u is MISSING from the database. Payload length %u. First payload bytes:", (unsigned)(rawIndex + 1), (unsigned)rawPlen);
            for (uint16_t i = 0; (i < 8) && (i < rawPlen); i++)
                printf(" %02X", (unsigned)raw[rawStart + 6 + i]);
            printf("\n");
        }
        rawIndex++;
    }

    if (missing == 0)
        printf("  Every captured UBX-MGA-DBD message is in the database\n");
    if (dbValid)
        printf("  The database contains UBX-MGA-DBD message(s) which were not captured! (Was the capture buffer full?)\n");
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Callback: newNAVSAT will be called when new NAV SAT data arrives
static void newNAVSAT(ubxCallbackDataCommon_t *theData)
{
    ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

    uint8_t numSvs = myGNSS.getUbxMessageFieldCallback(msg, "numSvs");

    uint8_t numAopAvail = 0; // Count how many SVs have AssistNow Autonomous data available
    for (uint8_t block = 0; block < numSvs; block++)
    {
        if ((bool)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "aopAvail"))
            numAopAvail++;
    }

    printf("\nUBX-NAV-SAT contains data for %u %s\n", (unsigned)numSvs, numSvs == 1 ? "SV" : "SVs");
    printf("AssistNow Autonomous data is available for %u %s\n", (unsigned)numAopAvail, numAopAvail == 1 ? "SV" : "SVs");
}

// Callback: newNAVAOPSTATUS will be called when new NAV AOPSTATUS data arrives
static void newNAVAOPSTATUS(ubxCallbackDataCommon_t *theData)
{
    ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

    uint8_t status = myGNSS.getUbxMessageFieldCallback(msg, "status");
    printf("AOPSTATUS status is %u%s\n", (unsigned)status, status == 0 ? " (idle)" : " (running)");
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// stdin is made non-blocking (see app_main), so fgetc returns EOF when no key has been pressed
static bool keyPressed(void)
{
    int c = fgetc(stdin);
    if (c == EOF)
    {
        clearerr(stdin); // Clear the EOF / error flag so we can read again later
        return false;
    }
    return true;
}

static void emptyKeyBuffer(void)
{
    while (keyPressed())
        ; // Discard any waiting characters
}

static unsigned long millisNow(void)
{
    return (unsigned long)pdTICKS_TO_MS(xTaskGetTickCount()); // 1ms per tick with CONFIG_FREERTOS_HZ=1000 (sdkconfig.defaults)
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

extern "C" void app_main(void)
{
    // Give the native USB serial port time to (re)connect, so the console does not miss the first messages
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("SparkFun u-blox Example\n");

    // Make stdin (the console) non-blocking, so keyPressed() can check for a key press without waiting
    fcntl(fileno(stdin), F_SETFL, fcntl(fileno(stdin), F_GETFL, 0) | O_NONBLOCK);

    emptyKeyBuffer();
    printf("Press any key to begin...\n");
    while (!keyPressed()) // Wait for a keypress
        vTaskDelay(pdMS_TO_TICKS(50));

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Create the I2C bus. Connect to the GNSS.

    i2c_master_bus_config_t busConfig = {};
    busConfig.i2c_port = I2C_NUM_0;
    busConfig.sda_io_num = (gpio_num_t)CONFIG_EXAMPLE_I2C_SDA_GPIO;
    busConfig.scl_io_num = (gpio_num_t)CONFIG_EXAMPLE_I2C_SCL_GPIO;
    busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
    busConfig.glitch_ignore_cnt = 7;
    busConfig.flags.enable_internal_pullup = false; // u-blox modules have their own internal active pull-ups
    i2c_master_bus_handle_t i2cBus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&busConfig, &i2cBus));

    // myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on the console

    while (myGNSS.begin(i2cBus) == false) // Connect to the u-blox module using the I2C bus
    {
        printf("u-blox GNSS not detected at default I2C address. Retrying...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("u-blox module connected\n");

    // The settings below are made in RAM only. They are cleared when the module is powered off.

    myGNSS.setI2COutput(COM_TYPE_UBX, VAL_LAYER_RAM); // Turn off NMEA noise

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Enable AssistNow Autonomous data collection (CFG-ANA-USE_ANA)

    if (myGNSS.setAopCfg(1, 0, VAL_LAYER_RAM) == true)
        printf("AssistNow Autonomous enabled\n");
    else
        printf("Could not enable AssistNow Autonomous!\n");

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Enable periodic UBX-NAV-SAT and UBX-NAV-AOPSTATUS messages and set up the callbacks

    myGNSS.setNavigationFrequency(1, VAL_LAYER_RAM); // Produce one solution per second

    myGNSS.setAutoUBXrate("NAV", "SAT", 1, true, VAL_LAYER_RAM);       // Enable NAV-SAT on I2C, every epoch
    myGNSS.setAutoUBXrate("NAV", "AOPSTATUS", 1, true, VAL_LAYER_RAM); // Enable NAV-AOPSTATUS on I2C, every epoch
    myGNSS.setAutoCallbackPtr("NAV", "SAT", &newNAVSAT);               // Call newNAVSAT() when NAV-SAT arrives
    myGNSS.setAutoCallbackPtr("NAV", "AOPSTATUS", &newNAVAOPSTATUS);   // Call newNAVAOPSTATUS() when NAV-AOPSTATUS arrives

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Keep displaying NAV SAT and AOPSTATUS until the user presses a key

    printf("AssistNow Autonomous data collection is in progress. Press any key to quit and read the database.\n");

    emptyKeyBuffer();

    while (!keyPressed()) // Wait for the arrival of a keypress
    {
        myGNSS.checkUblox();     // Check for the arrival of new data and process it.
        myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

        printf(".");
        fflush(stdout); // stdout is line-buffered. Flush so the dots appear immediately
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    printf("\n");

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Disable the periodic UBX-NAV-SAT and UBX-NAV-AOPSTATUS messages

    myGNSS.setAutoUBXrate("NAV", "SAT", 0, true, VAL_LAYER_RAM);
    myGNSS.setAutoUBXrate("NAV", "AOPSTATUS", 0, true, VAL_LAYER_RAM);
    vTaskDelay(pdMS_TO_TICKS(1100));
    myGNSS.checkUblox(); // Process (discard) any messages which were already on their way

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Allocate storage for the database (and the raw data capture)

    uint8_t *database = new uint8_t[MAX_DATABASE_LENGTH]; // The database will be stored here
    if (database == nullptr)
    {
        printf("Could not allocate storage for the database! Freezing...\n");
        while (1)
            vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("Storage has been allocated for the database.\n");

#if CAPTURE_RAW_DATA
    rawCapture.buffer = new uint8_t[CAPTURE_BUFFER_LENGTH];
    if (rawCapture.buffer != nullptr)
    {
        rawCapture.size = CAPTURE_BUFFER_LENGTH;
        myGNSS.setOutputPort(captureOutput); // The library will write every received byte to captureBytes()
    }
    else
        printf("Could not allocate storage for the raw data capture. Continuing without it...\n");
#endif

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Read the navigation database from the module

    printf("Reading the database. Limited debug messages are enabled during the read:\n");
    fflush(stdout);

    myGNSS.enableDebugging(SparkFun_UBLOX_GNSS::sfeStdout, true); // Enable "important" debug messages only
    rawCapture.enabled = true;                                     // Start capturing (if the capture buffer was allocated)

    unsigned long readStart = millisNow();
    size_t databaseLen = myGNSS.readNavigationDatabase(database, MAX_DATABASE_LENGTH, DATABASE_READ_MAXWAIT); // Read the database
    unsigned long readTime = millisNow() - readStart;

    rawCapture.enabled = false; // Stop capturing
    myGNSS.disableDebugging();

    printf("The navigation database length is %u bytes\n", (unsigned)databaseLen);
    printf("readNavigationDatabase took %lu ms (maxWait is %u ms)\n", readTime, (unsigned)DATABASE_READ_MAXWAIT);
    if (readTime >= DATABASE_READ_MAXWAIT)
        printf("readNavigationDatabase timed out: it did not see a matching UBX-MGA-ACK\n");

    if (databaseLen == MAX_DATABASE_LENGTH)
        printf("There was not enough memory to store the entire database. Some data will have been lost!\n");

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Diagnostics: compare what the module sent with what the library stored

    summarizeUbx("The database returned by readNavigationDatabase", database, databaseLen, false);

#if CAPTURE_RAW_DATA
    if (rawCapture.buffer != nullptr)
    {
        summarizeUbx("The raw data received during readNavigationDatabase", rawCapture.buffer, rawCapture.length, true);
        if (rawCapture.lost > 0)
            printf("  The capture buffer was full. Bytes not captured: %u\n", (unsigned)rawCapture.lost);
        compareDbdMessages(rawCapture.buffer, rawCapture.length, database, databaseLen);
    }
#endif

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Print the database so it can be copied into your own code

    printf("\nThe database, as a C array:\n\n");
    printf("size_t databaseLen = %u;\n", (unsigned)databaseLen);
    printf("const uint8_t database[%u] = {\n", (unsigned)databaseLen);
    for (size_t i = 0; i < databaseLen; i++)
    {
        if ((i % 32) == 0)
            printf("  ");
        printf("0x%02X", (unsigned)database[i]);
        if (i == (databaseLen - 1))
            printf("\n");
        else if ((i % 32) == 31)
            printf(",\n");
        else
            printf(", ");
    }
    printf("};\n");

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Write the database to microSD card, in raw UBX format

    printf("\nMounting the microSD card...\n");
    sdmmc_card_t *card = nullptr;
    esp_err_t err = sd_card_mount(mountPoint, CONFIG_EXAMPLE_SD_SCK_GPIO, CONFIG_EXAMPLE_SD_POCI_GPIO,
                                  CONFIG_EXAMPLE_SD_PICO_GPIO, CONFIG_EXAMPLE_SD_CS_GPIO, &card);
    if (err != ESP_OK)
    {
        printf("microSD card failed, or not present (%s). The database has not been saved\n", esp_err_to_name(err));
    }
    else
    {
        FILE *dbFile = fopen(databaseFileName, "wb"); // "wb" overwrites any existing file
        if (dbFile == nullptr)
        {
            printf("Could not create %s!\n", databaseFileName);
        }
        else
        {
            size_t written = fwrite(database, 1, databaseLen, dbFile);
            fclose(dbFile);

            // Check the file size is databaseLen
            struct stat st = {};
            long fileSize = (stat(databaseFileName, &st) == 0) ? (long)st.st_size : -1;

            printf("The database has been written to %s. Bytes written: %u. File size: %ld%s\n",
                   databaseFileName, (unsigned)written, fileSize,
                   (fileSize == (long)databaseLen) ? " (correct)" : " (INCORRECT - should be databaseLen!)");
        }

#if CAPTURE_RAW_DATA
        // Diagnostics: also save the raw data, so it can be attached to the GitHub issue
        if ((rawCapture.buffer != nullptr) && (rawCapture.length > 0))
        {
            FILE *rawFile = fopen(captureFileName, "wb");
            if (rawFile != nullptr)
            {
                fwrite(rawCapture.buffer, 1, rawCapture.length, rawFile);
                fclose(rawFile);
                printf("The raw data received during readNavigationDatabase has been written to %s: %u bytes\n",
                       captureFileName, (unsigned)rawCapture.length);
            }
        }
#endif

        sd_card_unmount(mountPoint, card); // Unmount the microSD card. It is now safe to remove it
    }

    printf("\nDone!\n");

    while (true) // Nothing more to do
        vTaskDelay(pdMS_TO_TICKS(1000));
}
