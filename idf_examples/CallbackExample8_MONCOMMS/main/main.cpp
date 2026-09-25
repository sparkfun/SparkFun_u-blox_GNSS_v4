/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Configuring the GNSS to automatically send MON COMMS reports over I2C and display them using a callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * ESP-IDF version of the Arduino example CallbackExample8_MONCOMMS.ino
 *
 * This example shows how to configure the u-blox GNSS to send MON COMMS reports automatically
 * and access the data via a callback.
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

// Callback: newMONCOMMS will be called when new MON COMMS data arrives
static void newMONCOMMS(ubxCallbackDataCommon_t *theData)
{
    ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

    printf("\n");

    uint8_t nPorts = (uint8_t)myGNSS.getUbxMessageFieldCallback(msg, "nPorts");
    printf("New MON COMMS data received. It contains data for %u %s\n", (unsigned)nPorts, nPorts == 1 ? "port." : "ports.");

    // Mimic the data shown in u-center
    for (uint8_t port = 0; port < nPorts; port++) // For each port
    {
        uint16_t portId = (uint16_t)myGNSS.getUbxMessageBlockFieldCallback(msg, port, "portId");

        switch (portId) // Print the port ID. Skip unknown / reserved ports
        {
            case COM_PORT_ID_I2C:
                printf("I2C       ");
                break;
            case COM_PORT_ID_UART1:
                printf("UART1     ");
                break;
            case COM_PORT_ID_UART2: // X20P uses 0x0200
                printf("UART2 X20 ");
                break;
            case COM_PORT_ID_UART2 + 1: // ZED-F9P uses 0x0201
                printf("UART2 F9  ");
                break;
            case COM_PORT_ID_USB:
                printf("USB       ");
                break;
            case COM_PORT_ID_SPI:
                printf("SPI       ");
                break;
            default:
                // printf("Unknown / reserved portId 0x%04X\n", portId);
                continue;
        }

        printf(": txBytes %-10lu", (unsigned long)(uint32_t)myGNSS.getUbxMessageBlockFieldCallback(msg, port, "txBytes"));
        printf(" : rxBytes %-10lu", (unsigned long)(uint32_t)myGNSS.getUbxMessageBlockFieldCallback(msg, port, "rxBytes"));

        for (int i = 0; i < 4; i++)
        {
            char protId_str[sizeof("protId0")];
            snprintf(protId_str, sizeof(protId_str), "protId%d", i);
            uint8_t protId = (uint8_t)myGNSS.getUbxMessageFieldCallback(msg, protId_str);
            if (protId < 0xFF)
            {
                switch (protId)
                {
                    case 0: printf(" : UBX     "); break;
                    case 1: printf(" : NMEA    "); break;
                    case 2: printf(" : RTCM2   "); break;
                    case 5: printf(" : RTCM3   "); break;
                    case 6: printf(" : SPARTN  "); break;
                    default: printf(" : UNKNOWN "); break;
                }
                char msgs_str[sizeof("msgs0")];
                snprintf(msgs_str, sizeof(msgs_str), "msgs%d", i);
                printf("%-5u", (unsigned)(uint16_t)myGNSS.getUbxMessageBlockFieldCallback(msg, port, msgs_str));
            }
        }

        printf(" : skipped %lu\n", (unsigned long)(uint32_t)myGNSS.getUbxMessageBlockFieldCallback(msg, port, "skipped"));
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

    // Enable the MON COMMS Message on I2C, every two navigation cycles
    myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_MON_COMMS_I2C, 2);

    // Set up a callback for MON COMMS messages. Call newMONCOMMS() each time one arrives.
    // Note: this does not enable the COMMS message. The message is assumed to be periodic.
    // All this does is register the callback.
    myGNSS.setAutoCallbackPtr("MON", "COMMS", &newMONCOMMS);

    while (true)
    {
        myGNSS.checkUblox();     // Check for the arrival of new data and process it.
        myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

        printf(".");
        fflush(stdout); // stdout is line-buffered. Flush so the dots appear immediately
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
