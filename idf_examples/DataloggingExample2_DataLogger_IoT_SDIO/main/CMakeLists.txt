idf_component_register(SRCS "main.cpp" "sdmmc_card.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES fatfs sdmmc esp_driver_sdmmc esp_driver_spi esp_driver_gpio)
