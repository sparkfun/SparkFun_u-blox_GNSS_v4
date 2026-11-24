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
 * ESP-IDF version of the Arduino example AssistNowExample2_pushNavigationDatabase.ino
 * Based on the v3 library example Example3_AssistNowAutonomous_DatabaseWrite.
 *
 * This example shows how to push the navigation database - read by AssistNowExample1_readNavigationDatabase -
 * back to the module with pushAssistNowData().
 *
 * Where does the database come from?
 *   The example first tries to read the database from the microSD card file written by AssistNowExample1
 *   (/sdcard/MGADBD.UBX). The file contains the raw UBX-MGA-DBD messages
 *   If the file can not be read (no card, no file, or no valid UBX-MGA-DBD messages in it), the example pushes
 *   database[databaseLen] instead. Copy and paste the C array printed by AssistNowExample1 into main/database.h
 *
 * The module needs to know the time to make the best use of the database. The example connects to WiFi and gets
 * the time from NTP (see wifi_time.c). Update main/secrets.h with your WiFi credentials. The time is pushed to the
 * module with setUTCTimeAssistance(), before the database.
 *
 * The example:
 *   checks the database: it counts the UBX-MGA-DBD messages and checks their checksums
 *   connects to WiFi and gets the time from NTP
 *   optionally performs a GNSS cold start (COLD_START_FIRST). This clears the navigation data the module already
 *   has. All SparkFun boards have a backup battery, which means the module keeps its navigation data if you
 *   disconnect the power. Without the cold start, you will not see much difference in the time to first fix
 *   enables acknowledgement of assistance data (CFG-NAVSPG-ACKAIDING)
 *   pushes the time (UBX-MGA-INI-TIME_UTC)
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
 *   The database in database.h is const, so it is stored in flash
 *   This example will not work on the ZED-F9P. "The ZED-F9P supports AssistNow Online only."
 *
 * The microSD card is accessed over SPI using the ESP-IDF FATFS VFS (see sd_card.c), and read with the
 * standard C file functions (fopen, fread, fclose).
 *
 * Feel like supporting open source hardware?
 * Buy a board from SparkFun!
 * https://www.sparkfun.com/sparkfun-gnss-receiver-breakout-max-m10s-qwiic.html
 *
 * Hardware Connections:
 * Plug a Qwiic cable into the GNSS and your ESP32 board (e.g. the SparkFun Thing Plus C - ESP32 WROOM)
 * Insert the microSD card written by AssistNowExample1
 * Set the GPIO pins with: idf.py menuconfig -> "Example Configuration"
 * Build, flash and monitor with: idf.py -p PORT flash monitor
 * Press any key in the monitor when prompted
 */

#include "database.h" // <- Copy and paste the database printed by AssistNowExample1 into database.h
#include "secrets.h"  // <- Update secrets.h with your WiFi credentials

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

#include "sd_card.h"
#include "wifi_time.h"

#include "SparkFun_u-blox_GNSS_v4.h"

static SFE_UBLOX_GNSS myGNSS; // SFE_UBLOX_GNSS uses I2C

static const char *mountPoint = "/sdcard";                  // The microSD card is mounted here
static const char *databaseFileName = "/sdcard/MGADBD.UBX"; // The database written by AssistNowExample1, in raw UBX format

static const char *ntpServer = "pool.ntp.org"; // The Network Time Protocol server

#define WIFI_CONNECT_TIMEOUT 20000 // Wait this many ms for the WiFi connection
#define NTP_SYNC_TIMEOUT 20000     // Wait this many ms for the time from NTP

#define MAX_DATABASE_LENGTH 32768 // The maximum database file length (bytes)
#define COLD_START_FIRST 1        // Set to 0 to push the database without performing a cold start first
#define MGA_ACK_MAXWAIT 250       // pushAssistNowData / setUTCTimeAssistance: wait up to this many ms for each UBX-MGA-ACK
#define PUSH_DEBUG 1              // Set to 0 to disable the "pushAssistNowData:" debug messages
#define PUSH_DATABASE 1           // Set to 0 for a control run: everything else is the same, but the database is not pushed

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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
    (*strayBytes) += len - *pos; // Count any trailing bytes too
    *pos = len;
    return false;
}

// Check the database: print how many UBX-MGA-DBD messages it contains. Returns the number of UBX-MGA-DBD messages
static uint16_t checkDatabase(const char *title, const uint8_t *buf, size_t len)
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

    printf("%s (%u bytes):\n", title, (unsigned)len);
    printf("  UBX-MGA-DBD messages: %u", (unsigned)dbdCount);
    if (dbdCount > 0)
        printf(" (payload length min %u max %u)", (unsigned)dbdMin, (unsigned)dbdMax);
    printf("\n");
    printf("  Other UBX messages: %u. Checksum failures: %u. Bytes outside UBX messages: %u\n",
           (unsigned)otherCount, (unsigned)bad, (unsigned)stray);
    return dbdCount;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Read the database from microSD. Returns a pointer to the database (allocated with new), or nullptr if the read fails
static uint8_t *readDatabaseFile(size_t *fileLen)
{
    *fileLen = 0;

    printf("Mounting the microSD card...\n");
    sdmmc_card_t *card = nullptr;
    esp_err_t err = sd_card_mount(mountPoint, CONFIG_EXAMPLE_SD_SCK_GPIO, CONFIG_EXAMPLE_SD_POCI_GPIO,
                                  CONFIG_EXAMPLE_SD_PICO_GPIO, CONFIG_EXAMPLE_SD_CS_GPIO, &card);
    if (err != ESP_OK)
    {
        printf("microSD card failed, or not present (%s)\n", esp_err_to_name(err));
        return nullptr;
    }

    uint8_t *fileData = nullptr;

    struct stat st = {};
    if (stat(databaseFileName, &st) != 0)
    {
        printf("Could not open %s!\n", databaseFileName);
    }
    else
    {
        size_t fileSize = (size_t)st.st_size;
        printf("%s is %u bytes\n", databaseFileName, (unsigned)fileSize);

        if ((fileSize == 0) || (fileSize > MAX_DATABASE_LENGTH))
        {
            printf("The file is empty, or too large!\n");
        }
        else
        {
            FILE *dbFile = fopen(databaseFileName, "rb");
            if (dbFile == nullptr)
            {
                printf("Could not open %s!\n", databaseFileName);
            }
            else
            {
                fileData = new uint8_t[fileSize];
                if (fileData == nullptr)
                {
                    printf("Could not allocate storage for the file!\n");
                }
                else
                {
                    size_t bytesRead = fread(fileData, 1, fileSize, dbFile);
                    if (bytesRead != fileSize)
                    {
                        printf("File read failed! Bytes read: %u\n", (unsigned)bytesRead);
                        delete[] fileData;
                        fileData = nullptr;
                    }
                    else
                        *fileLen = fileSize;
                }
                fclose(dbFile);
            }
        }
    }

    sd_card_unmount(mountPoint, card); // Unmount the microSD card. It is now safe to remove it
    return fileData;
}

// Push the ESP32 system time (UTC) to the module
static bool pushTimeAssistance(void)
{
    struct timeval tv;
    gettimeofday(&tv, nullptr); // Get the time, with microseconds
    struct tm utc;
    gmtime_r(&tv.tv_sec, &utc);

    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &utc);
    printf("Pushing the time (UTC) to the module: %s\n", timeStr);

    // setUTCTimeAssistance uses a default time accuracy of 2 seconds which should be OK here.
    // Have a look at the library source code for more details.
    return myGNSS.setUTCTimeAssistance(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                                       utc.tm_hour, utc.tm_min, utc.tm_sec, ((uint32_t)tv.tv_usec) * 1000,
                                       defaultMGAINITIMEtAccS, defaultMGAINITIMEtAccNs, defaultMGAINITIMEsource,
                                       SFE_UBLOX_MGA_ASSIST_ACK_YES, MGA_ACK_MAXWAIT);
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
    // Find the database: from microSD, or from database[databaseLen]

    const uint8_t *pushData = nullptr; // Points to the database which will be pushed
    size_t pushLen = 0;

    size_t fileLen;
    uint8_t *fileData = readDatabaseFile(&fileLen); // The database read from microSD
    if (fileData != nullptr)
    {
        if (checkDatabase("The database read from /sdcard/MGADBD.UBX", fileData, fileLen) > 0)
        {
            pushData = fileData;
            pushLen = fileLen;
        }
        else
        {
            printf("The file does not contain any valid UBX-MGA-DBD messages!\n");
            delete[] fileData;
            fileData = nullptr;
        }
    }

    if (pushData == nullptr)
    {
        printf("Using database[databaseLen] instead\n");
        if ((databaseLen > 0) && (checkDatabase("database[databaseLen]", database, databaseLen) > 0))
        {
            pushData = database;
            pushLen = databaseLen;
        }
        else
        {
            printf("database[databaseLen] does not contain any valid UBX-MGA-DBD messages!\n");
            printf("Copy and paste the database printed by AssistNowExample1 into main/database.h. Freezing...\n");
            while (1)
                vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Connect to WiFi and get the time from NTP. The ESP32 RTC keeps the time after WiFi is disconnected

    esp_err_t timeErr = wifi_time_sync(ssid, password, ntpServer, WIFI_CONNECT_TIMEOUT, NTP_SYNC_TIMEOUT);
    bool timeIsSet = (timeErr == ESP_OK);
    if (timeIsSet)
    {
        time_t now = time(nullptr);
        struct tm utc;
        gmtime_r(&now, &utc);
        char timeStr[48];
        strftime(timeStr, sizeof(timeStr), "%A, %B %d %Y %H:%M:%S", &utc);
        printf("Time from NTP (UTC): %s\n", timeStr);
    }
    else
        printf("Failed to obtain the time from NTP! (%s)\n", esp_err_to_name(timeErr));

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

    if (myGNSS.setAopCfg(1, 0, VAL_LAYER_RAM) == true) // Enable AssistNow Autonomous (CFG-ANA-USE_ANA)
        printf("AssistNow Autonomous enabled\n");
    else
        printf("Could not enable AssistNow Autonomous!\n");

    myGNSS.setNavigationFrequency(1, VAL_LAYER_RAM); // Produce one solution per second

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Optional: perform a GNSS cold start, to clear the module's own navigation data

#if COLD_START_FIRST
    // UBX-CFG-RST: navBbrMask 0xFFFF (cold start: clear all navigation data), resetMode 0x02 (controlled software
    // reset - GNSS only). Only the GNSS tasks are restarted. The configuration is not changed.
    uint8_t coldStart[4] = {0xFF, 0xFF, 0x02, 0x00};
    myGNSS.cfgRst(coldStart, 4);
    printf("GNSS cold start performed. The module's navigation data has been cleared\n");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Give the GNSS time to restart
#endif

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Push the time and the database to the module

    // Enable acknowledgement of assistance data (CFG-NAVSPG-ACKAIDING), so each message is acknowledged
    if (myGNSS.setAckAiding(1, VAL_LAYER_RAM) == false)
        printf("Could not enable ACKAIDING!\n");

#if PUSH_DEBUG
    printf("Limited debug messages are enabled during the push:\n");
    myGNSS.enableDebugging(SparkFun_UBLOX_GNSS::sfeStdout, true); // Enable "important" debug messages only
#endif

    if (timeIsSet)
    {
        if (pushTimeAssistance() == false)
            printf("Could not push the time!\n");
    }
    else
        printf("The time is not known. Pushing the database without it. This will not work as well...\n");

#if PUSH_DATABASE
    printf("Pushing the database to the module...\n");

    unsigned long pushStart = millisNow();
    size_t bytesPushed = myGNSS.pushAssistNowData(pushData, pushLen, SFE_UBLOX_MGA_ASSIST_ACK_YES, MGA_ACK_MAXWAIT);
    unsigned long pushedAt = millisNow();

#if PUSH_DEBUG
    myGNSS.disableDebugging();
#endif

    // Note: bytesPushed is the number of bytes sent to the module. The "pushAssistNowData:" debug messages show
    // how many messages were accepted (acknowledged) by the module
    printf("Pushed %u of %u bytes of navigation database to the module. Push took %lu ms\n",
           (unsigned)bytesPushed, (unsigned)pushLen, pushedAt - pushStart);

    if (bytesPushed != pushLen)
        printf("Warning: bytesPushed does not match the database length! Maybe the database contains bad data? Or there was a communication error?\n");
#else
    unsigned long pushedAt = millisNow();

#if PUSH_DEBUG
    myGNSS.disableDebugging();
#endif

    (void)pushData; // pushData and pushLen are not used in the control run
    (void)pushLen;
    printf("PUSH_DATABASE is 0. This is a control run: the database has NOT been pushed\n");
#endif

    if (fileData != nullptr)
        delete[] fileData; // Free the file data

    printf("Waiting for the first 3D fix...\n");

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Poll NAV-PVT once per second. Print the fix type and SIV, so we can see how quickly the fixType goes to 3D

    bool firstFixSeen = false; // Becomes true when the first 3D fix is seen

    while (true)
    {
        if (myGNSS.getNAVPVT() == true) // Use the helper method getNAVPVT() to poll NAV-PVT
        {
            float seconds = ((float)(millisNow() - pushedAt)) / 1000.0f;
            uint8_t fixType = myGNSS.getFixType();

            const char *fixName = "Unknown";
            if (fixType == 0) fixName = "No fix";
            else if (fixType == 1) fixName = "Dead reckoning";
            else if (fixType == 2) fixName = "2D";
            else if (fixType == 3) fixName = "3D";
            else if (fixType == 4) fixName = "GNSS + Dead reckoning";
            else if (fixType == 5) fixName = "Time only";

            printf("%.1fs  Fix: %s  SIV: %u  Lat: %ld  Long: %ld (degrees * 10^-7)\n",
                   seconds, fixName, (unsigned)myGNSS.getSIV(),
                   (long)myGNSS.getLatitude(), (long)myGNSS.getLongitude());

            if ((!firstFixSeen) && ((fixType == 3) || (fixType == 4)))
            {
                firstFixSeen = true;
                printf("Time to first 3D fix (since the push): %.1f s\n", seconds);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
