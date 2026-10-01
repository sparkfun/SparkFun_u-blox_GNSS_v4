/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Configuring the GNSS to automatically send RXM SFRBX and RAWX reports over I2C and log them to file on SD card
 * By: Paul Clark
 * SparkFun Electronics
 *
 * ESP-IDF version of the Arduino example DataloggingExample1_RAWX_and_SFRBX.ino
 *
 * This example shows how to configure the u-blox GNSS to send RXM SFRBX and RAWX reports automatically
 * and log the data to SD card in UBX format.
 *
 * ** Please note: this example will only work on u-blox ADR or High Precision GNSS or Time Sync products **
 *
 * Data is logged in u-blox UBX format. Please see the u-blox protocol specification for more details.
 * You can replay and analyze the data using u-center:
 * https://www.u-blox.com/en/product/u-center
 * Or you can use (e.g.) RTKLIB to analyze the data and extract your precise location or produce
 * Post-Processed Kinematic data:
 * https://rtklibexplorer.wordpress.com/
 * http://rtkexplorer.com/downloads/rtklib-code/
 *
 * The SD card is accessed over SPI using the ESP-IDF FATFS VFS (see sd_card.c) - the ESP-IDF
 * equivalent of the Arduino SD library. Files are then read and written with the standard C
 * functions (fopen, fwrite, fclose).
 *
 * The default pins match the SparkFun Thing Plus C (ESP32 WROOM USB-C):
 * https://www.sparkfun.com/sparkfun-thing-plus-esp32-wroom-usb-c.html
 * I2C: SDA 21, SCL 22. microSD: SCK 18, POCI 19, PICO 23, CS 5. LED: 13.
 * Change them with: idf.py menuconfig -> "Example Configuration"
 *
 * Hardware Connections:
 * Please see: https://learn.sparkfun.com/tutorials/esp32-thing-plus-usb-c-hookup-guide
 * Connect your GNSS breakout to the Thing Plus C using a Qwiic cable.
 * Insert a formatted micro-SD card into the socket on the Thing Plus.
 * Connect the Thing Plus to your computer using a USB-C cable.
 * Build, flash and monitor with: idf.py -p PORT flash monitor
 * Press any key in the monitor to start logging, and again to stop.
 *
 * To minimise I2C bus errors, it is a good idea to open the I2C pull-up split pad links on
 * the u-blox module breakout.
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
#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "sd_card.h"

#include "SparkFun_u-blox_GNSS_v4.h"

static SFE_UBLOX_GNSS myGNSS; // SFE_UBLOX_GNSS uses I2C

static const char *mountPoint = "/sdcard";                // The SD card is mounted here
static const char *fileName = "/sdcard/RXM_RAWX.ubx";     // FATFS uses 8.3 file names by default
static FILE *myFile = nullptr;                            // File that all GNSS data is written to

#define sdWriteSize 512      // Write data to the SD card in blocks of 512 bytes
#define fileBufferSize 16384 // Allocate 16KBytes of RAM for UBX message storage
static uint8_t *myBuffer;    // Use myBuffer to hold the data while we write it to SD card

static unsigned long lastPrint; // Record when the last print took place

static int numSFRBX = 0; // Keep count of how many SFRBX message groups have been received
static int numRAWX = 0;  // Keep count of how many RAWX message groups have been received

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

static void setLED(bool on)
{
    if (CONFIG_EXAMPLE_LED_GPIO >= 0)
        gpio_set_level((gpio_num_t)CONFIG_EXAMPLE_LED_GPIO, on ? 1 : 0);
}

// Freeze. (A 'while(1);' would trigger the task watchdog on ESP-IDF. vTaskDelay lets the idle task run.)
static void freeze(void)
{
    while (true)
        vTaskDelay(pdMS_TO_TICKS(1000));
}

extern "C" void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("SparkFun u-blox Example\n");

    if (CONFIG_EXAMPLE_LED_GPIO >= 0) // Flash the LED each time we write to the SD card
    {
        gpio_reset_pin((gpio_num_t)CONFIG_EXAMPLE_LED_GPIO);
        gpio_set_direction((gpio_num_t)CONFIG_EXAMPLE_LED_GPIO, GPIO_MODE_OUTPUT);
        setLED(false);
    }

    // Create the I2C bus
    i2c_master_bus_config_t busConfig = {};
    busConfig.i2c_port = I2C_NUM_0;
    busConfig.sda_io_num = (gpio_num_t)CONFIG_EXAMPLE_I2C_SDA_GPIO;
    busConfig.scl_io_num = (gpio_num_t)CONFIG_EXAMPLE_I2C_SCL_GPIO;
    busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
    busConfig.glitch_ignore_cnt = 7;
    busConfig.flags.enable_internal_pullup = false; // u-blox modules have their own internal active pull-ups
    i2c_master_bus_handle_t i2cBus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&busConfig, &i2cBus));

    // Make stdin (the console) non-blocking, so keyPressed() can check for a key press without waiting
    fcntl(fileno(stdin), F_SETFL, fcntl(fileno(stdin), F_GETFL, 0) | O_NONBLOCK);

    emptyKeyBuffer(); // Make sure the input buffer is empty

    printf("Press any key to start logging.\n");

    while (!keyPressed()) // Wait for the user to press a key
        vTaskDelay(pdMS_TO_TICKS(10));

    vTaskDelay(pdMS_TO_TICKS(100)); // Wait, just in case multiple characters were sent

    emptyKeyBuffer(); // Empty the input buffer

    printf("Initializing SD card...\n");

    // See if the card is present and can be initialized:
    sdmmc_card_t *card = nullptr;
    esp_err_t err = sd_card_mount(mountPoint, CONFIG_EXAMPLE_SD_SCK_GPIO, CONFIG_EXAMPLE_SD_POCI_GPIO,
                                  CONFIG_EXAMPLE_SD_PICO_GPIO, CONFIG_EXAMPLE_SD_CS_GPIO, &card);
    if (err != ESP_OK)
    {
        printf("Card failed, or not present (%s). Freezing...\n", esp_err_to_name(err));
        // don't do anything more:
        freeze();
    }
    printf("SD card initialized.\n");

    // Create or open a file called "RXM_RAWX.ubx" on the SD card.
    // If the file already exists, the new data is appended to the end of the file.
    myFile = fopen(fileName, "a");
    if (myFile == nullptr)
    {
        printf("Failed to create UBX data file! Freezing...\n");
        freeze();
    }

    // myGNSS.enableDebugging(); // Uncomment this line to enable lots of helpful GNSS debug messages on the console
    // myGNSS.enableDebugging(SparkFun_UBLOX_GNSS::sfeStdout, true); // Or, uncomment this line to enable only the important GNSS debug messages

    // RAWX messages can be over 3KBytes in size, so we need to make sure we allocate enough RAM to hold all the data.
    // SD cards can occasionally 'hiccup' and a write takes much longer than usual. The buffer needs to be big enough
    // to hold the backlog of data if/when this happens.
    // getMaxFileBufferAvail will tell us the maximum number of bytes which the file buffer has contained.
    myGNSS.setFileBufferSize(fileBufferSize); // setFileBufferSize must be called _before_ .begin

    while (myGNSS.begin(i2cBus) == false) // Connect to the u-blox module using the I2C bus
    {
        printf("u-blox GNSS not detected at default I2C address. Retrying...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // myGNSS.setI2CTransactionSize(128); // Optional: read more bytes per I2C transaction (default 32). Useful for stress testing

    // Uncomment the next line if you want to reset your module back to the default settings with 1Hz navigation rate
    // (This will also disable any "auto" messages that were enabled and saved by other examples and reduce the load on the I2C bus)
    // myGNSS.factoryDefault(); vTaskDelay(pdMS_TO_TICKS(5000));

    myGNSS.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)

    // setCfgValset() writes to the RAM and Battery-backed-RAM layers by default (VAL_LAYER_RAM_BBR).
    // To use a different layer, add it as the third parameter. E.g. for RAM only:
    //   setCfgValset(UBLOX_CFG_MSGOUT_..., n, VAL_LAYER_RAM)
    // VAL_LAYER_ALL also saves the setting in Flash (if the module has Flash).

    // Enable the RXM RAWX Message on I2C, every two navigation cycles
    myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_I2C, 2);

    // Set up a callback for RXM RAWX messages. Call newRXM() each time one arrives.
    // Note: this does not enable the RAWX message. The message is assumed to be periodic.
    // All this does is register the callback.
    myGNSS.setAutoCallbackPtr("RXM", "RAWX", &newRXM);

    myGNSS.logUBX("RXM", "RAWX"); // Enable RXM RAWX data logging

    // Enable the RXM SFRBX Message on I2C, every two navigation cycles
    myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_I2C, 2);

    // Use the same callback for RXM SFRBX messages. Call newRXM() each time one arrives.
    myGNSS.setAutoCallbackPtr("RXM", "SFRBX", &newRXM);

    myGNSS.logUBX("RXM", "SFRBX"); // Enable RXM SFRBX data logging

    myBuffer = new uint8_t[sdWriteSize]; // Create our own buffer to hold the data while we write it to SD card

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
            setLED(true); // Flash the LED each time we write to the SD card

            myGNSS.extractFileBufferData(myBuffer, sdWriteSize); // Extract exactly sdWriteSize bytes from the UBX file buffer and put them into myBuffer

            fwrite(myBuffer, 1, sdWriteSize, myFile); // Write exactly sdWriteSize bytes from myBuffer to the ubxDataFile on the SD card

            // In case the SD writing is slow or there is a lot of data to write, keep checking for the arrival of new data
            myGNSS.checkUblox();     // Check for the arrival of new data and process it.
            myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

            setLED(false); // Turn the LED off again
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        if ((sfe_millis() - lastPrint) > 1000) // Print the message count once per second
        {
            // Print how many message groups have been received
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
                setLED(true); // Flash the LED while we write to the SD card

                uint16_t bytesToWrite = remainingBytes; // Write the remaining bytes to SD card sdWriteSize bytes at a time
                if (bytesToWrite > sdWriteSize)
                {
                    bytesToWrite = sdWriteSize;
                }

                myGNSS.extractFileBufferData(myBuffer, bytesToWrite); // Extract bytesToWrite bytes from the UBX file buffer and put them into myBuffer

                fwrite(myBuffer, 1, bytesToWrite, myFile); // Write bytesToWrite bytes from myBuffer to the ubxDataFile on the SD card

                remainingBytes -= bytesToWrite; // Decrement remainingBytes
            }

            setLED(false); // Turn the LED off

            fclose(myFile); // Close the data file
            myFile = nullptr;

            sd_card_unmount(mountPoint, card); // Unmount the SD card. It is now safe to remove it

            printf("Logging stopped. Freezing...\n");
            freeze(); // Do nothing more
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        vTaskDelay(1); // Let other tasks (and the idle task / watchdog) run. 1 tick = 1ms with CONFIG_FREERTOS_HZ=1000
    }
}
