idf_component_register(SRCS "main.cpp" "sd_card.c" "wifi_time.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES fatfs sdmmc esp_driver_sdspi esp_driver_spi esp_driver_gpio esp_driver_i2c esp_wifi esp_netif esp_event nvs_flash)
