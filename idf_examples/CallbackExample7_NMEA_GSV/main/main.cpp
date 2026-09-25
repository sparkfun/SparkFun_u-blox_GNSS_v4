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
 * ESP-IDF version of the Arduino example CallbackExample7_NMEA_GSV.ino
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
 * On ESP-IDF, the NMEA field getters return std::string (String on Arduino).
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
 * Plug a Qwiic cable into the GNSS and your ESP32 board. Connect SDA, SCL and GND (and 3V3 if required)
 * Set the GPIO pins with: idf.py menuconfig -> "Example Configuration"
 * Build, flash and monitor with: idf.py -p PORT flash monitor
 */

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

#include "SparkFun_u-blox_GNSS_v4.h"

static SFE_UBLOX_GNSS myGNSS; // SFE_UBLOX_GNSS uses I2C

static volatile bool printHeader = true;

// Print a string, right-justified with space padding as needed
static void printPadded(const std::string &str, size_t padding)
{
    for (size_t p = str.length(); p < padding; p++)
        printf(" ");
    printf("%s", str.c_str());
}

static void printGSVdata(nmeaCallbackDataCommon_t *theData)
{
    if (printHeader)
    {
        printf("\n");
        printf("Signal             svid elv  az cno\n");
        printHeader = false;
    }

    nmeaMessage *msg = myGNSS.getNmeaMessagePtr(theData);

    std::string xxGSV = myGNSS.getNmeaMessageFieldCallback(msg, "xxGSV");       // Get the Talker ID + GSV
    std::string signalId = myGNSS.getNmeaMessageFieldCallback(msg, "signalId"); // Get the signal ID
    const char *signal;

    if (xxGSV == "GPGSV") // GPS / SBAS
    {
        if (signalId == "1") signal = "GPS/SBAS L1 C/A    ";
        else if (signalId == "6") signal = "GPS L2 CL          ";
        else if (signalId == "5") signal = "GPS L2 CM          ";
        else if (signalId == "7") signal = "GPS L5 I           ";
        else if (signalId == "8") signal = "GPS L5 Q           ";
        else signal = "GPS UNKNOWN        ";
    }
    else if (xxGSV == "GAGSV")
    {
        if (signalId == "7") signal = "Galileo E1 C/B     ";
        else if (signalId == "1") signal = "Galileo E5 aI/aQ   ";
        else if (signalId == "2") signal = "Galileo E5 bI/bQ   ";
        else if (signalId == "5") signal = "Galileo E6 B/C     ";
        else if (signalId == "4") signal = "Galileo E6 A       ";
        else signal = "Galileo UNKNOWN    ";
    }
    else if (xxGSV == "GBGSV")
    {
        if (signalId == "1") signal = "BeiDou B1|D1/B1|D2 ";
        else if (signalId == "B") signal = "BeiDou B2|D1/B2|D2 ";
        else if (signalId == "8") signal = "BeiDou B3|D1/B3|D2 ";
        else if (signalId == "3") signal = "BeiDou B1 Cp/Cd    ";
        else if (signalId == "5") signal = "BeiDou B2 ap/ad    ";
        else signal = "BeiDou UNKNOWN     ";
    }
    else if (xxGSV == "GQGSV")
    {
        if (signalId == "1") signal = "QZSS L1 C/A        ";
        else if (signalId == "4") signal = "QZSS L1 S          ";
        else if (signalId == "5") signal = "QZSS L2 CM         ";
        else if (signalId == "6") signal = "QZSS L2 CL         ";
        else if (signalId == "7") signal = "QZSS L5 I          ";
        else if (signalId == "8") signal = "QZSS L5 Q          ";
        else signal = "QZSS UNKNOWN       ";
    }
    else if (xxGSV == "GLGSV")
    {
        if (signalId == "1") signal = "GLONASS L1 OF      ";
        else if (signalId == "3") signal = "GLONASS L2 OF      ";
        else signal = "GLONASS UNKNOWN    ";
    }
    else if (xxGSV == "GIGSV")
    {
        if (signalId == "1") signal = "NavIC L5 A         ";
        else signal = "NavIC UNKNOWN      ";
    }
    else signal = "UNKNOWN            ";

    for (uint8_t block = 0; block < 4; block++) // GSV can hold up to 4 blocks
    {
        std::string svid_str = myGNSS.getNmeaMessageBlockFieldCallback(msg, block, "svid");
        if (svid_str.length() > 0)
        {
            printf("%s", signal);

            // Adjust the SV number if needed
            int svid = atoi(svid_str.c_str());
            if (xxGSV == "GPGSV") // SBAS SVs S120-S151 are numbered 33-64
                if ((svid >= 33) && (svid <= 64))
                    svid_str = std::to_string(svid + 87);
            if (xxGSV == "GLGSV") // GLONASS SVs R1-R32 are numbered 65-96
                svid_str = std::to_string(svid - 64);
            printPadded(svid_str, 4);

            printPadded(myGNSS.getNmeaMessageBlockFieldCallback(msg, block, "elv"), 4);
            printPadded(myGNSS.getNmeaMessageBlockFieldCallback(msg, block, "az"), 4);
            printPadded(myGNSS.getNmeaMessageBlockFieldCallback(msg, block, "cno"), 4);
            printf("\n");
        }
    }
}

extern "C" void app_main(void)
{
    printf("SparkFun u-blox Example\n");

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

    // myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on the console

    while (myGNSS.begin(i2cBus) == false) // Connect to the u-blox module using the I2C bus
    {
        printf("u-blox GNSS not detected at default I2C address. Retrying...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
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

    while (true)
    {
        myGNSS.checkUblox();     // Check for the arrival of new data and process it.
        myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

        printf(".");
        fflush(stdout); // stdout is line-buffered. Flush so the dots appear immediately
        vTaskDelay(pdMS_TO_TICKS(50));
        printHeader = true;
    }
}
