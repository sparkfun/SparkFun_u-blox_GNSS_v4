/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Mount / unmount a microSD card over SPI, using the ESP-IDF FATFS VFS
 * (the ESP-IDF equivalent of the Arduino SD library). Written in C, as the
 * ESP-IDF sdmmc / sdspi default-configuration macros are C initializers.
 */

#pragma once

#include "esp_err.h"
#include "sdmmc_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the SPI bus and mount the SD card at mountPoint (e.g. "/sdcard").
// Files can then be accessed with the standard C functions: fopen("/sdcard/FILE.UBX", ...) etc.
esp_err_t sd_card_mount(const char *mountPoint, int sckGpio, int pociGpio, int picoGpio, int csGpio, sdmmc_card_t **card);

// Unmount the SD card and free the SPI bus
void sd_card_unmount(const char *mountPoint, sdmmc_card_t *card);

#ifdef __cplusplus
}
#endif
