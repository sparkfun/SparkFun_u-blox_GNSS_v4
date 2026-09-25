idf_component_register(SRCS "main.cpp" "sd_card.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES fatfs sdmmc esp_driver_sdspi esp_driver_spi esp_driver_gpio esp_driver_i2c)
