/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Polling the NMEA ZDA message
 * By: Paul Clark
 * SparkFun Electronics
 *
 * ESP-IDF version of the Arduino example PollingExample5_GPZDA.ino
 *
 * u-blox GNSS modules output the NMEA GGA, GLL, GSA, GSV, RMC, and VTG messages
 * by default. ZDA (Time and Date) is not output by default. In this example, we
 * poll (request) it.
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

    while (true)
    {
        // ZDA is not output by default. We need to Poll it (or make it periodic)
        if (myGNSS.getNMEA("ZDA")) // This will poll (request) the ZDA message
        {
            nmeaMessage *msg = myGNSS.nmeaMessages.find("ZDA");

            printf("UTC time: %s", myGNSS.getNmeaMessageField(msg, "time").c_str()); // Print the UTC time

            printf(" %s", myGNSS.getNmeaMessageField(msg, "day").c_str());   // Print the day
            printf("/%s/", myGNSS.getNmeaMessageField(msg, "month").c_str()); // Print the month

            // getNmeaMessageField returns everything as a string (std::string on ESP-IDF, String on Arduino)
            // Use atoi(), atof() or strtod() to convert to numeric types as needed
            int year = atoi(myGNSS.getNmeaMessageField(msg, "year").c_str()); // Convert string to int
            printf("%d\n", year);
        }
    }
}
