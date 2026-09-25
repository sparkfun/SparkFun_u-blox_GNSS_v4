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
 * ESP-IDF version of the Arduino example PollingExample2_PositionVelocityTime_Serial.ino
 *
 * This example shows how to poll the u-blox module position, velocity and time (PVT) data using Serial (UART).
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
 * Hook up the TX, RX and GND pins, plus 3V3 or 5V depending on your needs
 * Connect: GNSS TX to ESP32 RX; GNSS RX to ESP32 TX
 * Set the GPIO pins with: idf.py menuconfig -> "Example Configuration"
 * Build, flash and monitor with: idf.py -p PORT flash monitor
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/uart.h"

#include "SparkFun_u-blox_GNSS_v4.h"

static SFE_UBLOX_GNSS_SERIAL myGNSS; // SFE_UBLOX_GNSS_SERIAL uses Serial (UART). For I2C or SPI, see Example1 and Example3

extern "C" void app_main(void)
{
    printf("SparkFun u-blox Example\n");

    // Configure the UART. u-blox X20, F9 and M10 modules default to 38400 baud. Change this in menuconfig if required
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

    while (myGNSS.begin(gnssUart) == false) // Connect to the u-blox module using the UART
    {
        printf("u-blox GNSS not detected. Retrying...\n");
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

        vTaskDelay(pdMS_TO_TICKS(10)); // Let other tasks run
    }
}
