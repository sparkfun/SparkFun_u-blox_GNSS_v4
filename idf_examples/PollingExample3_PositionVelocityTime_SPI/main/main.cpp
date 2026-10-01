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
 * ESP-IDF version of the Arduino example PollingExample3_PositionVelocityTime_SPI.ino
 *
 * This example shows how to poll the u-blox module position, velocity and time (PVT) data using SPI.
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
 * Hook up the PICO, POCI, SCK, CS and GND pins, plus 3V3 or 5V depending on your needs
 * Connect: GNSS PICO to ESP32 PICO (MOSI); GNSS POCI to ESP32 POCI (MISO)
 * Note: the GNSS module's SPI interface must be enabled (D_SEL). See the module's integration manual
 * Set the GPIO pins with: idf.py menuconfig -> "Example Configuration"
 * Build, flash and monitor with: idf.py -p PORT flash monitor
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/spi_master.h"

#include "SparkFun_u-blox_GNSS_v4.h"

static SFE_UBLOX_GNSS_SPI myGNSS; // SFE_UBLOX_GNSS_SPI uses SPI. For I2C or Serial, see Example1 and Example2

extern "C" void app_main(void)
{
    printf("SparkFun u-blox Example\n");

    // Initialize the SPI bus
    spi_bus_config_t busConfig = {};
    busConfig.mosi_io_num = CONFIG_EXAMPLE_SPI_PICO_GPIO;
    busConfig.miso_io_num = CONFIG_EXAMPLE_SPI_POCI_GPIO;
    busConfig.sclk_io_num = CONFIG_EXAMPLE_SPI_SCK_GPIO;
    busConfig.quadwp_io_num = -1;
    busConfig.quadhd_io_num = -1;
    busConfig.max_transfer_sz = 0; // Use the default
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &busConfig, SPI_DMA_CH_AUTO));

    // myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on the console

    // Connect to the u-blox module using SPI. The library drives the chip select pin
    while (myGNSS.begin(SPI2_HOST, (gpio_num_t)CONFIG_EXAMPLE_SPI_CS_GPIO) == false)
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
