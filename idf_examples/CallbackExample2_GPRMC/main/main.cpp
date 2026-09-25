/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Reading NMEA RMC (GPRMC) using a Callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * ESP-IDF version of the Arduino example CallbackExample2_GPRMC.ino
 *
 * u-blox GNSS modules output the NMEA GGA, GLL, GSA, GSV, RMC, and VTG messages
 * by default. We do not need to enable them - unless they were previously disabled.
 * In this example, we tell the library to trigger a callback when the RMC message
 * arrives.
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

#include "SparkFun_u-blox_GNSS_v4.h"

static SFE_UBLOX_GNSS myGNSS; // SFE_UBLOX_GNSS uses I2C

static void printNMEAdata(nmeaCallbackDataCommon_t *theData)
{
    printf("\n");

    nmeaMessage *msg = myGNSS.getNmeaMessagePtr(theData);

    printf("UTC time: %s", myGNSS.getNmeaMessageFieldCallback(msg, "time").c_str()); // Print the UTC time

    printf(" Date: %s", myGNSS.getNmeaMessageFieldCallback(msg, "date").c_str()); // UTC date: DDMMYY

    printf(" Lat: %s", myGNSS.getNmeaMessageFieldCallback(msg, "lat").c_str()); // Print the latitude
    printf(" %s", myGNSS.getNmeaMessageFieldCallback(msg, "NS").c_str());       // Print the North / South indicator

    // getNmeaMessageFieldCallback returns everything as a string (std::string on ESP-IDF, String on Arduino)
    // Use atoi(), atof() or strtod() to convert to numeric types as needed
    float longitude = atof(myGNSS.getNmeaMessageFieldCallback(msg, "lon").c_str()); // Convert string to float
    printf(" Long: %.8f", longitude);                                                 // Print the longitude with 8 decimal places
    printf(" %s (degrees)\n", myGNSS.getNmeaMessageFieldCallback(msg, "EW").c_str()); // Print the East / West indicator
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

    // Set up a callback for NMEA RMC messages. Call printNMEAdata() each time one arrives.
    // Note: this does not enable the RMC message. The message is assumed to be periodic.
    // All this does is register the callback.
    myGNSS.setNmeaCallbackPtr("RMC", &printNMEAdata);

    while (true)
    {
        myGNSS.checkUblox();     // Check for the arrival of new data and process it.
        myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

        printf(".");
        fflush(stdout); // stdout is line-buffered. Flush so the dots appear immediately
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
