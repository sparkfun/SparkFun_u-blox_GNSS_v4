/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Reading Position, Velocity and Time (PVT) via UBX binary commands
 * By: Paul Clark
 * SparkFun Electronics
 *
 * ESP-IDF version of the Arduino example PollingExample1_PositionVelocityTime.ino
 *
 * This example shows how to poll the u-blox module position, velocity and time (PVT) data.
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

#include "SparkFun_u-blox_GNSS_v4.h"

static SFE_UBLOX_GNSS myGNSS; // SFE_UBLOX_GNSS uses I2C. For Serial or SPI, see Example2 and Example3

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
        // Poll the position, velocity and time (PVT) information.
        // getNAVPVT() returns true when new data is received.
        if (myGNSS.getNAVPVT() == true) // Use the helper method getNAVPVT()
        {
            int32_t latitude = myGNSS.getLatitude();    // Use the helper method
            int32_t longitude = myGNSS.getLongitude();  // Use the helper method
            int32_t altitude = myGNSS.getAltitudeMSL(); // Helper method for Altitude above Mean Sea Level
            printf("Lat: %ld Long: %ld (degrees * 10^-7) Alt: %ld (mm)\n", (long)latitude, (long)longitude, (long)altitude);
        }

        // Poll the position, velocity and time (PVT) information.
        // getUBX() returns true when new data is received.
        if (myGNSS.getUBX("NAV", "PVT") == true)
        {
            ubxMessage *msg = myGNSS.ubxMessages.findByName("NAV", "PVT");

            // getUbxMessageField converts everything to double. Convert lat back to int32_t
            int32_t latitude = (int32_t)myGNSS.getUbxMessageField(msg, "lat");

            // Or, we could read the true "I4" (int32_t) directly, without going through double
            // To do that, we need to use the ubxAnyType struct
            ubxAnyType ubxAnyTypeLon = myGNSS.getUbxMessageField(msg, "lon");

            int32_t altitude = (int32_t)myGNSS.getUbxMessageField(msg, "hMSL");

            printf("Lat: %ld Long: %ld (degrees * 10^-7) Alt: %ld (mm)\n", (long)latitude, (long)ubxAnyTypeLon.I4, (long)altitude);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Let other tasks run
    }
}
