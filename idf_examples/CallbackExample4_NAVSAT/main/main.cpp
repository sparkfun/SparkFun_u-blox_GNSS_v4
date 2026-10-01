/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Reading NAV-SAT using a Callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * ESP-IDF version of the Arduino example CallbackExample4_NAVSAT.ino
 *
 * This example shows how to use a callback to print the NAV-SAT SV signal strengths
 * (cno = Carrier to NOise ratio in dBHz).
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

// Print a uint8_t, right-justified with space padding as needed
static void printPadded(uint8_t val, int padding)
{
    printf("%*u", padding, (unsigned)val);
}

// Callback: newNAVSAT will be called when new NAV SAT data arrives
static void newNAVSAT(ubxCallbackDataCommon_t *theData)
{
    ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

    printf("\n");

    uint8_t numSvs = myGNSS.getUbxMessageFieldCallback(msg, "numSvs");
    printf("New NAV SAT data received. It contains data for %u %s\n", (unsigned)numSvs, numSvs == 1 ? "SV." : "SVs.");

    printf("gnssId  svId qual used cno Carrier_Noise_dBHz\n");

    for (uint8_t block = 0; block < numSvs; block++)
    {
        uint8_t gnssId = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "gnssId");
        switch (gnssId)
        {
            case 0: printf("GPS     "); break;
            case 1: printf("SBAS    "); break;
            case 2: printf("Galileo "); break;
            case 3: printf("BeiDou  "); break;
            case 4: printf("IMES    "); break;
            case 5: printf("QZSS    "); break;
            case 6: printf("GLONASS "); break;
            case 7: printf("NAVIC   "); break;
            default: printf("UNKNOWN "); break;
        }

        uint8_t svId = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "svId");
        printPadded(svId, 4);

        // Signal quality indicator:
        //  0 = no signal
        //  1 = searching signal
        //  2 = signal acquired
        //  3 = signal detected but unusable
        //  4 = code locked and time synchronized
        //  5, 6, 7 = code and carrier locked and time synchronized
        uint8_t qualityInd = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "qualityInd");
        printPadded(qualityInd, 5);

        bool svUsed = (bool)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "svUsed");
        printPadded((uint8_t)svUsed, 5);

        uint8_t cno = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, block, "cno");
        printPadded(cno, 4);

        // Print cno as a bar
        printf(" ");
        for (uint8_t bar = 0; bar < cno; bar++)
            printf("=");
        printf("\n");
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

    // Enable the NAV SAT Message on I2C, every two navigation cycles
    myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_SAT_I2C, 2);

    // Set up a callback for NAV SAT messages. Call newNAVSAT() each time one arrives.
    // Note: this does not enable the NAV SAT message. The message is assumed to be periodic.
    // All this does is register the callback.
    myGNSS.setAutoCallbackPtr("NAV", "SAT", &newNAVSAT);

    while (true)
    {
        myGNSS.checkUblox();     // Check for the arrival of new data and process it.
        myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

        printf(".");
        fflush(stdout); // stdout is line-buffered. Flush so the dots appear immediately
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
