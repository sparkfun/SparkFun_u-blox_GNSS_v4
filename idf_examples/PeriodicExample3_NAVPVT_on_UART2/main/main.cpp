/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Making NAV-PVT periodic ("Automatic") on the module's UART2 port
 * By: Paul Clark
 * SparkFun Electronics
 *
 * ESP-IDF version of the Arduino example PeriodicExample3_NAVPVT_on_UART2.ino
 *
 * This example connects to the u-blox module's UART2 port (not UART1) and makes the NAV-PVT
 * message periodic on UART2 by calling setAutoUBXrate("NAV", "PVT", 1, true, VAL_LAYER_RAM);
 * It reproduces the code from GitHub issue #4:
 * https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/issues/4
 *
 * connectedToUART2() tells the library that the ESP32 is connected to the module's UART2.
 * The library then uses the UART2 configuration keys - e.g. UBLOX_CFG_MSGOUT_UBX_NAV_PVT_UART2
 * when setAutoUBX() or setAutoUBXrate() enables a periodic message.
 * (Note: the module's UART2 can be connected to any ESP32 UART. EXAMPLE_UART_PORT_NUM selects
 * the ESP32 UART. connectedToUART2() refers to the module's UART.)
 *
 * Why setUART2Output(COM_TYPE_UBX) is needed:
 *
 * By default, the X20's UART2 accepts UBX messages but does not output them. From the u-blox X20 HPG 2.11
 * Interface Description, "Configuration defaults" (Tables 109 and 110):
 *   CFG-UART2INPROT-UBX   = 1 (true)  - UART2 accepts UBX messages
 *   CFG-UART2OUTPROT-UBX  = 0 (false) - UART2 does not output UBX messages
 *   CFG-UART2OUTPROT-NMEA = 0 (false) - or NMEA messages
 *
 * Even so, begin() connects over UART2 with the default configuration. begin() checks the connection
 * by polling a configuration value (UBX-CFG-VALGET) and waiting for the reply and its acknowledgement
 * (UBX-ACK-ACK). The module sends replies and acknowledgements back to the sender - on the port the
 * request arrived on - even when UBX output is disabled on that port. (Tested on a ZED-X20P running
 * HPG 2.11.)
 *
 * The OUTPROT settings control the messages the module outputs by itself, like the periodic NAV-PVT.
 * So, like the code in issue #4, the example calls setUART2Output(COM_TYPE_UBX) to enable UBX output on
 * UART2. Without it, setAutoUBXrate() enables NAV-PVT on UART2, but the module does not output it.
 * Note: setUART2Output(COM_TYPE_UBX) also disables NMEA and RTCM3 output on UART2.
 *
 * The example also calls setUART2Input() with UBX, NMEA, RTCM3 and SPARTN - the X20 defaults - to make
 * sure UBX input is enabled on UART2. (If UBX input has been disabled on UART2, the library can not talk
 * to the module over UART2. Use u-center, or another port, to enable it first.)
 *
 * All of these settings are made in RAM only. They are cleared when the module is powered off.
 *
 * After setAutoUBXrate(), the example reads back the NAV-PVT output rate for each port, so you
 * can see which port's configuration key was written. You should see "UART2: 1". (Before the
 * library fixed issue #4, setAutoUBXrate wrote the SPI key instead: "SPI: 1", "UART2: 0".)
 * Power-cycle the module before running this example, so that the rates start from the defaults.
 *
 * Feel like supporting open source hardware?
 * Buy a board from SparkFun!
 * https://www.sparkfun.com/sparkfun-allband-gnss-rtk-breakout-zed-x20p-qwiic.html
 * https://www.sparkfun.com/sparkfun-gps-rtk2-board-zed-f9p-qwiic-gps-15136.html
 * https://www.sparkfun.com/sparkfun-gps-rtk-sma-breakout-zed-f9p-qwiic.html
 * https://www.sparkfun.com/sparkfun-gps-rtk-dead-reckoning-breakout-zed-f9r-qwiic-gps-22693.html
 * (The MAX-M10S does not have a UART2)
 *
 * Hardware Connections:
 * Connect: GNSS TX2 to ESP32 RX; GNSS RX2 to ESP32 TX; and GND. Plus 3V3 or 5V depending on your needs
 * Set the GPIO pins with: idf.py menuconfig -> "Example Configuration"
 * Build, flash and monitor with: idf.py -p PORT flash monitor
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/uart.h"

#include "SparkFun_u-blox_GNSS_v4.h"

static SFE_UBLOX_GNSS_SERIAL myGNSS; // SFE_UBLOX_GNSS_SERIAL uses Serial (UART). This is the object from issue #4

#define GNSS_NAV_FREQUENCY 1 // The navigation rate (Hz). (The value used in issue #4 is not known)

// Read back and print the NAV-PVT output rate for one port
static void printPVTRate(const char *portName, uint32_t key)
{
    uint8_t rate;
    if (myGNSS.getVal8(key, &rate, VAL_LAYER_RAM))
        printf("%s: %u  ", portName, (unsigned)rate);
    else
        printf("%s: ?  ", portName);
}

extern "C" void app_main(void)
{
    printf("SparkFun u-blox Example\n");

    // Configure the ESP32 UART connected to the module's UART2.
    // u-blox X20 and F9 modules default to 38400 baud. Change this in menuconfig if required
    const uart_port_t gnssUart = (uart_port_t)CONFIG_EXAMPLE_UART_PORT_NUM;
    uart_config_t uartConfig = {};
    uartConfig.baud_rate = CONFIG_EXAMPLE_UART_BAUD_RATE;
    uartConfig.data_bits = UART_DATA_8_BITS;
    uartConfig.parity = UART_PARITY_DISABLE;
    uartConfig.stop_bits = UART_STOP_BITS_1;
    uartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uartConfig.source_clk = UART_SCLK_DEFAULT;
    ESP_ERROR_CHECK(uart_driver_install(gnssUart, 2048, 0, 0, NULL, 0)); // 2048 byte RX buffer, no TX buffer
    ESP_ERROR_CHECK(uart_param_config(gnssUart, &uartConfig));
    ESP_ERROR_CHECK(uart_set_pin(gnssUart, CONFIG_EXAMPLE_UART_TX_GPIO, CONFIG_EXAMPLE_UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on the console

    myGNSS.connectedToUART2(); // Tell the library we are connected to the module's UART2 (not UART1). Call this before begin()

    while (myGNSS.begin(gnssUart) == false) // Connect to the u-blox module using the UART
    {
        printf("u-blox GNSS not detected on UART2. Retrying...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("Connected to the module on UART2\n");

    // Enable UBX output (OUTPROT) on UART2 - needed for the periodic NAV-PVT. This also turns off NMEA and RTCM3 output
    myGNSS.setUART2Output(COM_TYPE_UBX, VAL_LAYER_RAM);

    // Make sure UBX input (INPROT) is enabled on UART2. Keep NMEA, RTCM3 and SPARTN input enabled (the X20 defaults)
    myGNSS.setUART2Input(COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3 | COM_TYPE_SPARTN, VAL_LAYER_RAM);

    myGNSS.setNavigationFrequency(GNSS_NAV_FREQUENCY, VAL_LAYER_RAM); // Set the navigation rate

    // Enable periodic ("automatic") NAV-PVT messages on UART2: every navigation epoch, with implicit update
    if (myGNSS.setAutoUBXrate("NAV", "PVT", 1, true, VAL_LAYER_RAM) == true)
        printf("setAutoUBXrate(\"NAV\", \"PVT\") succeeded\n");
    else
        printf("setAutoUBXrate(\"NAV\", \"PVT\") failed!\n");

    // Read back the NAV-PVT output rate for each port. We expect UART2: 1
    printf("NAV-PVT output rate - ");
    printPVTRate("I2C", UBLOX_CFG_MSGOUT_UBX_NAV_PVT_I2C);
    printPVTRate("SPI", UBLOX_CFG_MSGOUT_UBX_NAV_PVT_SPI);
    printPVTRate("UART1", UBLOX_CFG_MSGOUT_UBX_NAV_PVT_UART1);
    printPVTRate("UART2", UBLOX_CFG_MSGOUT_UBX_NAV_PVT_UART2);
    printf("\n");

    TickType_t lastPVT = xTaskGetTickCount(); // When the last NAV-PVT arrived

    while (true)
    {
        // Because we called setAutoUBXrate("NAV", "PVT", ...), the library knows
        // NAV-PVT messages will be arriving periodically on UART2.
        // getNAVPVT() does not poll. It returns true (once) when a fresh NAV-PVT message has arrived.
        // Read the PVT fields within the `if getNAVPVT()`:

        if (myGNSS.getNAVPVT() == true)
        {
            lastPVT = xTaskGetTickCount();

            printf("\n");
            printf("TOW: %lu (ms) Fix: %u SIV: %u Lat: %ld Long: %ld (degrees * 10^-7) Alt: %ld (mm)\n",
                   (unsigned long)myGNSS.getTimeOfWeek(), // Use the helper methods
                   (unsigned)myGNSS.getFixType(),
                   (unsigned)myGNSS.getSIV(),
                   (long)myGNSS.getLatitude(),
                   (long)myGNSS.getLongitude(),
                   (long)myGNSS.getAltitudeMSL());
        }

        // NAV-PVT should arrive every 1/GNSS_NAV_FREQUENCY seconds. Warn if it stops arriving
        if ((xTaskGetTickCount() - lastPVT) > pdMS_TO_TICKS(3000))
        {
            printf("\nNo NAV-PVT received on UART2 for 3 seconds!\n");
            lastPVT = xTaskGetTickCount();
        }

        printf(".");
        fflush(stdout); // stdout is line-buffered. Flush so the dots appear immediately
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
