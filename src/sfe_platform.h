/**
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Please see LICENSE.md for more details
 *
 * https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4
 *
 * @file sfe_platform.h
 *
 * Platform portability layer.
 *
 * This library builds both as an Arduino Library and as an Espressif ESP-IDF Component.
 * Everything that is platform-specific - apart from the hardware bus classes in sfe_bus.h /
 * sfe_bus.cpp (Arduino) and sfe_bus_esp_idf.h / sfe_bus_esp_idf.cpp (ESP-IDF) - lives here:
 *
 *   sfe_millis()              milliseconds since boot (wraps like Arduino millis())
 *   sfe_micros()              microseconds since boot (wraps like Arduino micros())
 *   sfe_delay(ms)             delay / yield to the RTOS
 *   sfe_pin_output(pin)       configure a GPIO as an output
 *   sfe_pin_write(pin, high)  drive a GPIO high or low
 *   sfe_string_t              the string type used by the NMEA field getters etc.
 *                             (Arduino String, or std::string on ESP-IDF)
 *   sfe_string_from_double()  format a double with a given number of decimal places
 *   DEC / HEX                 number bases for debugPrint()
 *
 * The platform is selected automatically:
 *   ARDUINO defined           -> Arduino (this includes arduino-esp32 used as an ESP-IDF component)
 *   ESP_PLATFORM defined      -> native ESP-IDF
 */

#pragma once

#if defined(ARDUINO)

// ---------------------------------------------------------------------------------------------
// Arduino
// ---------------------------------------------------------------------------------------------

#define SFE_ARDUINO 1

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

typedef String sfe_string_t;

static inline unsigned long sfe_millis(void) { return millis(); }
static inline unsigned long sfe_micros(void) { return micros(); }
static inline void sfe_delay(unsigned long ms) { delay(ms); }
static inline void sfe_pin_output(int pin) { pinMode(pin, OUTPUT); }
static inline void sfe_pin_write(int pin, bool high) { digitalWrite(pin, high ? HIGH : LOW); }
static inline void sfe_string_from_double(sfe_string_t &str, double value, int decimalPlaces)
{
  str = String(value, decimalPlaces); // Pass int, as the original code did: the second parameter type differs between cores (unsigned int on ESP32, unsigned char on AVR)
}

#elif defined(ESP_PLATFORM)

// ---------------------------------------------------------------------------------------------
// Native ESP-IDF
// ---------------------------------------------------------------------------------------------

#define SFE_ESP_IDF 1

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"

#ifndef DEC
#define DEC 10
#endif
#ifndef HEX
#define HEX 16
#endif

typedef std::string sfe_string_t;

static inline unsigned long sfe_millis(void) { return (unsigned long)(esp_timer_get_time() / 1000ULL); }
static inline unsigned long sfe_micros(void) { return (unsigned long)esp_timer_get_time(); }

// sfe_delay: the library uses delay(1) inside its polling loops to "allow an RTOS to get an elbow in".
// With the ESP-IDF default FreeRTOS tick of 100Hz, pdMS_TO_TICKS(1) is zero - and vTaskDelay(0) only
// yields to tasks of equal priority. So: delays shorter than one tick are done as a short busy-wait
// followed by a yield; longer delays use vTaskDelay. Setting CONFIG_FREERTOS_HZ=1000 (recommended - see
// the examples' sdkconfig.defaults) makes every delay a true sleep.
static inline void sfe_delay(unsigned long ms)
{
  if (ms == 0)
  {
    taskYIELD();
    return;
  }
  TickType_t ticks = pdMS_TO_TICKS(ms);
  if (ticks == 0)
  {
    esp_rom_delay_us((uint32_t)(ms * 1000UL));
    taskYIELD();
  }
  else
  {
    vTaskDelay(ticks);
  }
}

static inline void sfe_pin_output(int pin)
{
  gpio_reset_pin((gpio_num_t)pin);
  gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
}
static inline void sfe_pin_write(int pin, bool high) { gpio_set_level((gpio_num_t)pin, high ? 1 : 0); }

static inline void sfe_string_from_double(sfe_string_t &str, double value, int decimalPlaces)
{
  char buf[40];
  snprintf(buf, sizeof(buf), "%.*f", decimalPlaces, value);
  str = buf;
}

#else

#error "SparkFun u-blox GNSS v4: unsupported platform. Arduino (ARDUINO) or ESP-IDF (ESP_PLATFORM) is required."

#endif
