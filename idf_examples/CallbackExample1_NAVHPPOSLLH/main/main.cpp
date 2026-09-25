/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Reading HPPOSLLH using a Callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * ESP-IDF version of the Arduino example CallbackExample1_NAVHPPOSLLH.ino
 *
 * This example shows how to use a callback to print the High Precision PVT LLH
 * data from the GNSS.
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

static void printPVTdata(ubxCallbackDataCommon_t *theData)
{
    printf("\n");

    ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

    // getUbxMessageField returns everything as double. Cast to other types as needed
    unsigned long timeOfWeek = (unsigned long)myGNSS.getUbxMessageFieldCallback(msg, "iTOW");
    printf("TimeOfWeek: %lu (ms)", timeOfWeek); // Print the Time Of Week

    long latitude = (long)myGNSS.getUbxMessageFieldCallback(msg, "lat");
    printf(" Lat: %ld", latitude); // Print the latitude

    // Or, we could read the true "I4" (int32_t) directly, without going through double
    // To do that, we need to use the ubxAnyType struct
    ubxAnyType ubxAnyTypeLon = myGNSS.getUbxMessageFieldCallback(msg, "lon");
    printf(" Long: %ld (degrees * 10^-7)", (long)ubxAnyTypeLon.I4); // Print the longitude directly as int32_t

    float hAcc = (float)myGNSS.getUbxMessageFieldCallback(msg, "hAcc");
    printf(" Horiz Acc: %.1f (mm)\n", hAcc / 10.0); // Print the horizontal accuracy estimate
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

    // Enable the NAV HPPOSLLH Message on I2C at the navigation rate
    myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSLLH_I2C, 1);

    // Set up a callback for NAV HPPOSLLH messages. Call printPVTdata() each time one arrives.
    // Note: this does not enable the HPPOSLLH message. The message is assumed to be periodic.
    // All this does is register the callback.
    myGNSS.setAutoCallbackPtr("NAV", "HPPOSLLH", &printPVTdata);

    while (true)
    {
        myGNSS.checkUblox();     // Check for the arrival of new data and process it.
        myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

        printf(".");
        fflush(stdout); // stdout is line-buffered. Flush so the dots appear immediately
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
