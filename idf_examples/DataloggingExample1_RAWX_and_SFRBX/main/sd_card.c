/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Mount / unmount a microSD card over SPI, using the ESP-IDF FATFS VFS. See sd_card.h
 */

#include <stdio.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sd_card.h"

esp_err_t sd_card_mount(const char *mountPoint, int sckGpio, int pociGpio, int picoGpio, int csGpio, sdmmc_card_t **card)
{
    esp_vfs_fat_sdmmc_mount_config_t mountConfig = {
        .format_if_mount_failed = false, // Do not format the card if mounting fails
        .max_files = 2,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT(); // SPI2_HOST

    spi_bus_config_t busConfig = {
        .mosi_io_num = picoGpio,
        .miso_io_num = pociGpio,
        .sclk_io_num = sckGpio,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    esp_err_t err = spi_bus_initialize((spi_host_device_t)host.slot, &busConfig, SDSPI_DEFAULT_DMA);
    if (err != ESP_OK)
        return err;

    sdspi_device_config_t slotConfig = SDSPI_DEVICE_CONFIG_DEFAULT();
    slotConfig.gpio_cs = (gpio_num_t)csGpio;
    slotConfig.host_id = (spi_host_device_t)host.slot;

    err = esp_vfs_fat_sdspi_mount(mountPoint, &host, &slotConfig, &mountConfig, card);
    if (err != ESP_OK)
    {
        spi_bus_free((spi_host_device_t)host.slot);
        return err;
    }

    sdmmc_card_print_info(stdout, *card); // Print the card's name, type, speed and size
    return ESP_OK;
}

void sd_card_unmount(const char *mountPoint, sdmmc_card_t *card)
{
    spi_host_device_t hostId = (spi_host_device_t)card->host.slot;
    esp_vfs_fat_sdcard_unmount(mountPoint, card);
    spi_bus_free(hostId);
}
