/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Connect to WiFi, set the ESP32 system time from NTP (SNTP), then disconnect.
 * Written in C, as the ESP-IDF WiFi and SNTP default-configuration macros are C initializers.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Connect to the WiFi network ssid, set the system time (UTC) using SNTP and the ntpServer, then disconnect the WiFi.
// Returns ESP_OK if the time was set, ESP_ERR_TIMEOUT if the time was not received from the server, or
// ESP_FAIL if the WiFi connection failed. (Other esp_err_t errors are possible too.)
// The ESP32 RTC keeps the time after the WiFi is disconnected: read it with gettimeofday().
esp_err_t wifi_time_sync(const char *ssid, const char *password, const char *ntpServer, int connectTimeoutMs, int syncTimeoutMs);

#ifdef __cplusplus
}
#endif
