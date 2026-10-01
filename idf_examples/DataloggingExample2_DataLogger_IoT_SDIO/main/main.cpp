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
 * ESP-IDF version of the Arduino example DataloggingExample2_DataLogger_IoT_SDIO.ino
 *
 * This example is written for the SparkFun DataLogger IoT - 9DoF:
 * https://www.sparkfun.com/sparkfun-datalogger-iot-9dof.html
 *
 * But it should also run on the SparkFun DataLogger IoT:
 * https://www.sparkfun.com/sparkfun-datalogger-iot.html
 *
 * It makes use of the fast SDIO microSD interface to increase the logging speed.
 * The SD card is accessed using the ESP-IDF SDMMC host and FATFS VFS (see sdmmc_card.c) - the ESP-IDF
 * equivalent of the Arduino SD_MMC library. Files are then written with the standard C functions
 * (fopen, fwrite, fclose). On the ESP32, the SDMMC slot 1 pins are fixed: CLK 14, CMD 15, D0 2, D1 4, D2 12, D3 13.
 *
 * ** Please note: this example will only work on u-blox ADR or High Precision GNSS or Time Sync products **
 *
 * Hardware set-up:
 * Close the DSEL jumper on the ZED-F9P breakout - to select SPI mode
 * Connect:
 * DataLogger IoT : ZED-F9P
 * GND              GND
 * 3V3_SW           3V3
 * SCK  (18)        SCK
 * PICO (23)        PICO (MOSI)
 * POCI (19)        POCI (MISO)
 * 33               CS
 *
 * The pins can be changed with: idf.py menuconfig -> "Example Configuration"
 * Build, flash and monitor with: idf.py -p PORT flash monitor
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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "sdmmc_card.h"

#include "SparkFun_u-blox_GNSS_v4.h"

#define GNSS_SPI_HOST SPI2_HOST // The SPI host used for the GNSS

static SFE_UBLOX_GNSS_SPI myGNSS;

static const char *mountPoint = "/sdcard"; // The SD card is mounted here
static FILE *myFile = nullptr;             // File that all GNSS data is written to

#define sdWriteSize 2048     // Write data to the SD card in blocks of n*512 bytes
#define fileBufferSize 65530 // Allocate just under 64KBytes of RAM for UBX message storage
#define navRate 20           // Set the Nav Rate (Frequency) to 20Hz
// #define ubxOnly           // Uncomment this line to log UBX (RAWX and SFRBX) only
static uint8_t *myBuffer;    // Use myBuffer to hold the data while we write it to SD card

static unsigned long lastPrint; // Record when the last print took place

static int numSFRBX = 0; // Keep count of how many SFRBX messages have been received (see note above)
static int numRAWX = 0;  // Keep count of how many RAWX messages have been received (see note above)

// Callback: newRXM will be called when new RXM RAWX or SFRBX data arrives
static void newRXM(ubxCallbackDataCommon_t *theData)
{
    if (theData->Class == UBX_CLASS_RXM)
    {
        if (theData->ID == UBX_RXM_RAWX) // Check if this is RAWX
            numRAWX++;                   // Increment the count
        else if (theData->ID == UBX_RXM_SFRBX) // Check if this is SFRBX
            numSFRBX++;                         // Increment the count
    }
}

// The ESP-IDF equivalent of Serial.available() / Serial.read() for the console:
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

// Configure a GPIO as an output and set its level. Does nothing if gpio is -1
static void pinOutput(int gpio, bool high)
{
    if (gpio < 0)
        return;
    gpio_reset_pin((gpio_num_t)gpio);
    gpio_set_direction((gpio_num_t)gpio, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)gpio, high ? 1 : 0);
}

static void setLED(bool on)
{
    if (CONFIG_EXAMPLE_STAT_LED_GPIO >= 0)
        gpio_set_level((gpio_num_t)CONFIG_EXAMPLE_STAT_LED_GPIO, on ? 1 : 0);
}

// Freeze. (A 'while(1);' would trigger the task watchdog on ESP-IDF. vTaskDelay lets the idle task run.)
static void freeze(void)
{
    while (true)
        vTaskDelay(pdMS_TO_TICKS(1000));
}

extern "C" void app_main(void)
{
    pinOutput(CONFIG_EXAMPLE_STAT_LED_GPIO, false); // Flash the STAT LED each time we write to the SD card

    pinOutput(CONFIG_EXAMPLE_GNSS_CS_GPIO, true); // Deselect all the SPI devices
    pinOutput(CONFIG_EXAMPLE_IMU_CS_GPIO, true);
    pinOutput(CONFIG_EXAMPLE_MAG_CS_GPIO, true);

    // Initialize the SPI bus. (This also sets up the SPI pins - replacing the Arduino example's "fake transaction")
    spi_bus_config_t busConfig = {};
    busConfig.mosi_io_num = CONFIG_EXAMPLE_GNSS_SPI_PICO_GPIO;
    busConfig.miso_io_num = CONFIG_EXAMPLE_GNSS_SPI_POCI_GPIO;
    busConfig.sclk_io_num = CONFIG_EXAMPLE_GNSS_SPI_SCK_GPIO;
    busConfig.quadwp_io_num = -1;
    busConfig.quadhd_io_num = -1;
    busConfig.max_transfer_sz = 0; // Use the default
    ESP_ERROR_CHECK(spi_bus_initialize(GNSS_SPI_HOST, &busConfig, SPI_DMA_CH_AUTO));

    pinOutput(CONFIG_EXAMPLE_EN_3V3_SW_GPIO, true); // Enable power for the microSD card and GNSS

    vTaskDelay(pdMS_TO_TICKS(3000)); // Allow time for the GNSS and SD card to start up and for Tera Term to reconnect
    printf("SparkFun DataLogger IoT GNSS Logging : SPI and SDIO\n");

    // Make stdin (the console) non-blocking, so keyPressed() can check for a key press without waiting
    fcntl(fileno(stdin), F_SETFL, fcntl(fileno(stdin), F_GETFL, 0) | O_NONBLOCK);

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Initialize the GNSS

    printf("Initializing the GNSS...\n");

    // myGNSS.enableDebugging(); // Uncomment this line to enable lots of helpful GNSS debug messages on the console
    // myGNSS.enableDebugging(SparkFun_UBLOX_GNSS::sfeStdout, true); // Uncomment this line to enable only the important GNSS debug messages

    // RAWX messages can be over 3KBytes in size, so we need to make sure we allocate enough RAM to hold all the data.
    myGNSS.setFileBufferSize(fileBufferSize); // setFileBufferSize must be called _before_ .begin

    // Connect to the u-blox module using the SPI host, CS pin and speed setting
    // ublox devices generally work up to 5MHz. We'll use 4MHz for this example:
    bool begun = false;
    do
    {
        begun = myGNSS.begin(GNSS_SPI_HOST, (gpio_num_t)CONFIG_EXAMPLE_GNSS_CS_GPIO, 4000000);
        if (!begun)
        {
            printf("u-blox GNSS not detected on SPI bus.\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    } while (!begun);

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Wait for a 3D fix. Get the date and time for the log file

    // myGNSS.factoryDefault(); vTaskDelay(pdMS_TO_TICKS(5000)); // Uncomment this line to reset the module back to its factory defaults

#ifdef ubxOnly
    myGNSS.setSPIOutput(COM_TYPE_UBX); // Set the SPI port to output only UBX
#else
    myGNSS.setSPIOutput(COM_TYPE_UBX | COM_TYPE_NMEA); // Set the SPI port to output both UBX and NMEA messages
#endif

    // myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); // Optional: save (only) the communications port settings to flash and BBR

    printf("Waiting for a 3D fix");
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t fix = 0;

    do
    {
        if (myGNSS.getNAVPVT())        // Use the helper method
            fix = myGNSS.getFixType(); // Use the helper method
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf(".");
        fflush(stdout); // stdout is line-buffered. Flush so the dots appear immediately
    } while (fix != 3); // Wait for a 3D fix

    printf("\n");

    // Read the date and time using the helper methods
    // from the most recent NAV-PVT data - collected
    // by getNAVPVT() above
    uint16_t y = myGNSS.getYear();
    uint8_t M = myGNSS.getMonth();
    uint8_t d = myGNSS.getDay();
    uint8_t h = myGNSS.getHour();
    uint8_t m = myGNSS.getMinute();
    uint8_t s = myGNSS.getSecond();

    // The file name includes the mount point. Long file names need CONFIG_FATFS_LFN_HEAP (see sdkconfig.defaults)
    char szBuffer[48] = {'\0'};
    snprintf(szBuffer, sizeof(szBuffer), "%s/%04d%02d%02d%02d%02d%02d.ubx", mountPoint, y, M, d, h, m, s);

    printf("GNSS initialized.\n");

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Initialize the SD card. Open the log file

    printf("Initializing SD card...\n");

    // Begin the SD card
    sdmmc_card_t *card = nullptr;
#ifdef CONFIG_EXAMPLE_SDMMC_4BIT
    const bool fourBit = true;
#else
    const bool fourBit = false;
#endif
#ifdef CONFIG_EXAMPLE_SDMMC_HIGHSPEED
    const bool highSpeed = true;
#else
    const bool highSpeed = false;
#endif
    esp_err_t err = sdmmc_card_mount(mountPoint, fourBit, highSpeed, &card);
    if (err != ESP_OK)
    {
        printf("Card mount failed (%s). Freezing...\n", esp_err_to_name(err));
        freeze();
    }

    // Open the log file for writing
    printf("Log file is: %s\n", szBuffer);
    myFile = fopen(szBuffer, "w");
    if (myFile == nullptr)
    {
        printf("Failed to open log file for writing. Freezing...\n");
        freeze();
    }

    printf("SD card initialized.\n");

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Wait for a key press

    emptyKeyBuffer(); // Make sure the input buffer is empty

    printf("Press any key to start logging.\n");

    while (!keyPressed()) // Wait for the user to press a key
        vTaskDelay(pdMS_TO_TICKS(10));

    vTaskDelay(pdMS_TO_TICKS(100)); // Wait, just in case multiple characters were sent

    emptyKeyBuffer(); // Empty the input buffer

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

    printf("Press any key to stop logging.\n");

    lastPrint = sfe_millis(); // Initialize lastPrint

    while (true)
    {
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        myGNSS.checkUblox();     // Check for the arrival of new data and process it.
        myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        while (myGNSS.fileBufferAvailable() >= sdWriteSize) // Check to see if we have at least sdWriteSize waiting in the buffer
        {
            setLED(true); // Flash the STAT LED each time we write to the SD card

            myGNSS.extractFileBufferData(myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer

            fwrite(myBuffer, 1, sdWriteSize, myFile); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card

            // In case the SD writing is slow or there is a lot of data to write, keep checking for the arrival of new data
            myGNSS.checkUblox();     // Check for the arrival of new data and process it.
            myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

            setLED(false); // Turn the STAT LED off again
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        if ((sfe_millis() - lastPrint) > 1000) // Print the message count once per second
        {
            // Print how many message groups have been received (see note above)
            printf("Number of message groups received: SFRBX: %d RAWX: %d\n", numSFRBX, numRAWX);

            uint16_t maxBufferBytes = myGNSS.getMaxFileBufferAvail(); // Get how full the file buffer has been (not how full it is now)

            // printf("The maximum number of bytes which the file buffer has contained is: %u\n", maxBufferBytes); // It is a fun thing to watch how full the buffer gets

            if (maxBufferBytes > ((fileBufferSize / 5) * 4)) // Warn the user if fileBufferSize was more than 80% full
            {
                printf("Warning: the file buffer has been over 80%% full. Some data may have been lost.\n");
            }

            lastPrint = sfe_millis(); // Update lastPrint
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        if (keyPressed()) // Check if the user wants to stop logging
        {
            uint16_t remainingBytes = myGNSS.fileBufferAvailable(); // Check if there are any bytes remaining in the file buffer

            while (remainingBytes > 0) // While there is still data in the file buffer
            {
                setLED(true); // Flash the STAT LED while we write to the SD card

                uint16_t bytesToWrite = remainingBytes; // Write the remaining bytes to SD card sdWriteSize bytes at a time
                if (bytesToWrite > sdWriteSize)
                {
                    bytesToWrite = sdWriteSize;
                }

                myGNSS.extractFileBufferData(myBuffer, bytesToWrite); // Extract bytesToWrite bytes from the UBX file buffer and put them into myBuffer

                fwrite(myBuffer, 1, bytesToWrite, myFile); // Write bytesToWrite bytes from myBuffer to the ubxDataFile on the SD card

                remainingBytes -= bytesToWrite; // Decrement remainingBytes
            }

            setLED(false); // Turn the STAT LED off

            fclose(myFile); // Close the data file
            myFile = nullptr;

            sdmmc_card_unmount(mountPoint, card); // Unmount the SD card. It is now safe to remove it

            myGNSS.setNavigationFrequency(1); // Set navigation rate to 1Hz

            myGNSS.newCfgValset(VAL_LAYER_RAM);
            myGNSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_SPI, 0);
            myGNSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_SPI, 0);
            myGNSS.sendCfgValset();

            myGNSS.setSPIOutput(COM_TYPE_UBX | COM_TYPE_NMEA); // Re-enable NMEA

            printf("Logging stopped. Freezing...\n");
            freeze(); // Do nothing more
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        // No extra delay here: when the GNSS has no data, checkUblox() waits for the SPI polling interval
        // (setSPIPollingWait, default 9ms) which lets other tasks (and the idle task / watchdog) run
    }
}
