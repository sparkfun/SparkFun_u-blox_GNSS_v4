/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Mount / unmount a microSD card using the SDMMC (SDIO) host. See sdmmc_card.h
 */

#include <stdio.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_card.h"

esp_err_t sdmmc_card_mount(const char *mountPoint, bool fourBit, bool highSpeed, sdmmc_card_t **card)
{
    // The same defaults as the Arduino SD_MMC library: do not format, 5 open files
    esp_vfs_fat_sdmmc_mount_config_t mountConfig = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT(); // SDMMC slot 1, 20MHz
    if (highSpeed)
        host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // 40MHz

    sdmmc_slot_config_t slotConfig = SDMMC_SLOT_CONFIG_DEFAULT();
    slotConfig.width = fourBit ? 4 : 1;
    slotConfig.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP; // As Arduino SD_MMC does. External pull-ups are still recommended

    esp_err_t err = esp_vfs_fat_sdmmc_mount(mountPoint, &host, &slotConfig, &mountConfig, card);
    if (err != ESP_OK)
        return err;

    sdmmc_card_print_info(stdout, *card); // Print the card's name, type, speed and size
    return ESP_OK;
}

void sdmmc_card_unmount(const char *mountPoint, sdmmc_card_t *card)
{
    esp_vfs_fat_sdcard_unmount(mountPoint, card);
}
