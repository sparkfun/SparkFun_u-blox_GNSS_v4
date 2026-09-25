/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Mount / unmount a microSD card using the ESP32 SDMMC (SDIO) host, using the ESP-IDF FATFS VFS
 * (the ESP-IDF equivalent of the Arduino SD_MMC library). Written in C, as the ESP-IDF sdmmc
 * default-configuration macros are C initializers.
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "sdmmc_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

// Mount the SD card at mountPoint (e.g. "/sdcard") using SDMMC slot 1.
// On the ESP32 the slot 1 pins are fixed: CLK 14, CMD 15, D0 2, D1 4, D2 12, D3 13.
// Files can then be accessed with the standard C functions: fopen("/sdcard/...", ...) etc.
esp_err_t sdmmc_card_mount(const char *mountPoint, bool fourBit, bool highSpeed, sdmmc_card_t **card);

// Unmount the SD card
void sdmmc_card_unmount(const char *mountPoint, sdmmc_card_t *card);

#ifdef __cplusplus
}
#endif
