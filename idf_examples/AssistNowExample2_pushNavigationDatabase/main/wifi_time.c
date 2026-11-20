/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Connect to WiFi, set the ESP32 system time from NTP (SNTP), then disconnect.
 * Based on the ESP-IDF examples: wifi/getting_started/station and protocols/sntp
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "nvs_flash.h"

#include "wifi_time.h"

#define WIFI_CONNECTED_BIT BIT0 // Connected to the access point, with an IP address
#define WIFI_FAIL_BIT BIT1      // Failed to connect after WIFI_MAXIMUM_RETRY retries
#define WIFI_MAXIMUM_RETRY 5

static EventGroupHandle_t wifiEventGroup = NULL;
static int wifiRetries = 0;

static void wifiEventHandler(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData)
{
    (void)arg;
    if ((eventBase == WIFI_EVENT) && (eventId == WIFI_EVENT_STA_START))
    {
        esp_wifi_connect();
    }
    else if ((eventBase == WIFI_EVENT) && (eventId == WIFI_EVENT_STA_DISCONNECTED))
    {
        if (wifiRetries < WIFI_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            wifiRetries++;
            printf("Retrying the WiFi connection...\n");
        }
        else
            xEventGroupSetBits(wifiEventGroup, WIFI_FAIL_BIT);
    }
    else if ((eventBase == IP_EVENT) && (eventId == IP_EVENT_STA_GOT_IP))
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)eventData;
        printf("WiFi connected. IP address: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        wifiRetries = 0;
        xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_time_sync(const char *ssid, const char *password, const char *ntpServer, int connectTimeoutMs, int syncTimeoutMs)
{
    // Initialize NVS. The WiFi driver uses it to store calibration data
    esp_err_t err = nvs_flash_init();
    if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND))
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
        return err;

    wifiEventGroup = xEventGroupCreate();
    wifiRetries = 0;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *staNetif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t initConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&initConfig));

    esp_event_handler_instance_t instanceAnyId;
    esp_event_handler_instance_t instanceGotIp;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, NULL, &instanceAnyId));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, NULL, &instanceGotIp));

    // The authmode threshold defaults to WIFI_AUTH_OPEN, which is treated as WPA2 if the password is 8 or more characters
    wifi_config_t wifiConfig = {0};
    strncpy((char *)wifiConfig.sta.ssid, ssid, sizeof(wifiConfig.sta.ssid));
    strncpy((char *)wifiConfig.sta.password, password, sizeof(wifiConfig.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("Connecting to WiFi network %s...\n", ssid);

    // Wait until the connection is established (WIFI_CONNECTED_BIT), or failed (WIFI_FAIL_BIT), or we time out
    EventBits_t bits = xEventGroupWaitBits(wifiEventGroup, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(connectTimeoutMs));

    if (bits & WIFI_CONNECTED_BIT)
    {
        // Start SNTP and wait for it to set the system time
        esp_sntp_config_t sntpConfig = ESP_NETIF_SNTP_DEFAULT_CONFIG(ntpServer);
        esp_netif_sntp_init(&sntpConfig);
        err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(syncTimeoutMs)); // ESP_OK, or ESP_ERR_TIMEOUT
        esp_netif_sntp_deinit();
    }
    else
    {
        printf("WiFi connection failed! Have you updated secrets.h?\n");
        err = ESP_FAIL;
    }

    // Disconnect the WiFi as it's no longer needed. Unregister the handlers first, so the disconnect is not retried
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instanceGotIp);
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instanceAnyId);
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(staNetif);
    esp_event_loop_delete_default();
    vEventGroupDelete(wifiEventGroup);
    wifiEventGroup = NULL;
    printf("WiFi disconnected\n");

    return err;
}
