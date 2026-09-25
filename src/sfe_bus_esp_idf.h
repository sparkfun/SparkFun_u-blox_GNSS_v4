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
 * @file sfe_bus_esp_idf.h
 *
 * Native ESP-IDF implementation of the hardware bus classes (SfeI2C, SfeSPI, SfeSerial) and of the
 * debug / message output ports (SfeOutput, SfePrint).
 *
 * Included by sfe_bus.h when building as an ESP-IDF component (SFE_ESP_IDF). Do not include directly.
 * The Arduino equivalents are in sfe_bus.h / sfe_bus.cpp.
 *
 * The user creates and owns the bus (i2c_new_master_bus(), spi_bus_initialize(),
 * uart_driver_install()). These classes attach the GNSS module to that bus.
 */

#pragma once

#include "sfe_platform.h"

#if defined(SFE_ESP_IDF)

#include "sdkconfig.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "driver/gpio.h"

// Defaults - can be changed with idf.py menuconfig (see Kconfig)
#ifndef CONFIG_SFE_UBLOX_GNSS_I2C_CLOCK_HZ
#define CONFIG_SFE_UBLOX_GNSS_I2C_CLOCK_HZ 400000
#endif
#ifndef CONFIG_SFE_UBLOX_GNSS_I2C_TIMEOUT_MS
#define CONFIG_SFE_UBLOX_GNSS_I2C_TIMEOUT_MS 100
#endif

namespace SparkFun_UBLOX_GNSS
{

  // SfeI2C: I2C, using the ESP-IDF i2c_master driver.
  class SfeI2C : public GNSSDeviceBus
  {
  public:
    SfeI2C(void);
    ~SfeI2C(void);

    /**
     * @brief Attach the GNSS module to an I2C bus created by the user with i2c_new_master_bus().
     *
     * Adds the module to the bus as a device (i2c_master_bus_add_device). The address is always
     * (re-)applied: calling init() again with a different address removes and re-adds the device.
     *
     * @param bus The I2C master bus handle.
     * @param address The 7-bit I2C address of the GNSS module.
     * @param clockHz The I2C clock speed for this device. Defaults to CONFIG_SFE_UBLOX_GNSS_I2C_CLOCK_HZ (400kHz).
     * @return true on success.
     */
    bool init(i2c_master_bus_handle_t bus, uint8_t address, uint32_t clockHz = CONFIG_SFE_UBLOX_GNSS_I2C_CLOCK_HZ);

    bool ping();
    uint16_t available();
    uint8_t writeBytes(uint8_t *data, uint8_t length);
    uint8_t writeReadBytes(const uint8_t *data, uint8_t *readData, uint8_t length)
    { (void)data; (void)readData; (void)length; return 0; }
    void startWriteReadByte(){};
    void writeReadByte(const uint8_t *data, uint8_t *readData){ (void)data; (void)readData; }
    void writeReadByte(const uint8_t data, uint8_t *readData){ (void)data; (void)readData; }
    void endWriteReadByte(){};
    uint8_t readBytes(uint8_t *data, uint8_t length);

  private:
    void removeDevice(void);

    i2c_master_bus_handle_t _bus;
    i2c_master_dev_handle_t _dev;
    uint8_t _address;
    uint32_t _clockHz;
  };

  // SfeSPI: SPI, using the ESP-IDF spi_master driver.
  // Chip select is driven manually as a GPIO (like the Arduino implementation), so that the
  // byte-at-a-time startWriteReadByte() / writeReadByte() / endWriteReadByte() sequence works.
  class SfeSPI : public GNSSDeviceBus
  {
  public:
    SfeSPI(void);
    ~SfeSPI(void);

    /**
     * @brief Attach the GNSS module to an SPI bus initialized by the user with spi_bus_initialize().
     *
     * Adds the module to the bus as a device (spi_bus_add_device), SPI mode 0, with chip select
     * driven by this class as a GPIO.
     *
     * @param host The SPI host the bus was initialized on (e.g. SPI2_HOST).
     * @param cs The chip select GPIO.
     * @param clockHz The SPI clock speed. Defaults to 4MHz. (u-blox modules support up to 5.5MHz.)
     * @return true on success.
     */
    bool init(spi_host_device_t host, gpio_num_t cs, uint32_t clockHz = 4000000);

    /**
     * @brief Use an SPI device the user has already added to the bus.
     *
     * The device must have been added with spics_io_num = -1 (chip select is driven by this class)
     * and SPI mode 0.
     *
     * @param device The SPI device handle.
     * @param cs The chip select GPIO.
     * @return true on success.
     */
    bool init(spi_device_handle_t device, gpio_num_t cs);

    bool ping() { return false; }
    uint16_t available() { return 0; }
    uint8_t writeBytes(uint8_t *data, uint8_t length);
    uint8_t writeReadBytes(const uint8_t *data, uint8_t *readData, uint8_t length);
    void startWriteReadByte();
    void writeReadByte(const uint8_t *data, uint8_t *readData);
    void writeReadByte(const uint8_t data, uint8_t *readData);
    void endWriteReadByte();
    uint8_t readBytes(uint8_t *data, uint8_t length);

  private:
    bool configureCS(gpio_num_t cs);
    bool transfer(const uint8_t *txData, uint8_t *rxData, size_t length); // CS must already be low and the bus acquired
    void removeDevice(void);

    spi_device_handle_t _dev;
    bool _ownDevice; // true if we added the device - and so must remove it
    gpio_num_t _cs;
    bool _busAcquired;
  };

  // SfeSerial: UART, using the ESP-IDF uart driver.
  class SfeSerial : public GNSSDeviceBus
  {
  public:
    SfeSerial(void);

    /**
     * @brief Use a UART port configured by the user.
     *
     * The user must first call uart_driver_install() (with an RX buffer of at least 1024 bytes -
     * 2048 or more is recommended for high navigation rates or RAWX), uart_param_config() and
     * uart_set_pin(). Any stale RX data is discarded.
     *
     * @param port The UART port (e.g. UART_NUM_1).
     * @return true on success.
     */
    bool init(uart_port_t port);

    bool ping() { return false; }
    uint16_t available();
    uint8_t writeBytes(uint8_t *data, uint8_t length);
    uint8_t writeReadBytes(const uint8_t *data, uint8_t *readData, uint8_t length)
    { (void)data; (void)readData; (void)length; return 0; }
    void startWriteReadByte(){};
    void writeReadByte(const uint8_t *data, uint8_t *readData){ (void)data; (void)readData; }
    void writeReadByte(const uint8_t data, uint8_t *readData){ (void)data; (void)readData; }
    void endWriteReadByte(){};
    uint8_t readBytes(uint8_t *data, uint8_t length);

  private:
    uart_port_t _port;
    bool _initialized;
  };

  // SfeOutput: the ESP-IDF equivalent of the Arduino Print class. It is the destination for debug
  // messages (enableDebugging) and for the NMEA / RTCM / UBX message pass-through
  // (setNMEAOutputPort, setRTCMOutputPort, setUBXOutputPort, setOutputPort).
  // Derive your own class from SfeOutput to send the data anywhere you like (a file, a socket, a queue).
  class SfeOutput
  {
  public:
    virtual ~SfeOutput() {}
    virtual size_t write(const uint8_t *buffer, size_t length) = 0;
  };

  // Output to stdout - the ESP-IDF console (UART0 or USB Serial/JTAG)
  class SfeStdoutOutput : public SfeOutput
  {
  public:
    size_t write(const uint8_t *buffer, size_t length);
  };

  // Output to a UART. The UART driver must already be installed.
  class SfeUartOutput : public SfeOutput
  {
  public:
    SfeUartOutput(uart_port_t port) : _port(port) {}
    size_t write(const uint8_t *buffer, size_t length);

  private:
    uart_port_t _port;
  };

  // Output to a user callback function
  class SfeCallbackOutput : public SfeOutput
  {
  public:
    typedef void (*callback_t)(const uint8_t *buffer, size_t length, void *userContext);
    SfeCallbackOutput(callback_t callback, void *userContext = nullptr) : _callback(callback), _userContext(userContext) {}
    size_t write(const uint8_t *buffer, size_t length)
    {
      if (_callback != nullptr)
        _callback(buffer, length, _userContext);
      return length;
    }

  private:
    callback_t _callback;
    void *_userContext;
  };

  // The default debug output port
  extern SfeStdoutOutput sfeStdout;

  // SfePrint: the library's internal print helper. Wraps an SfeOutput and does the formatting.
  class SfePrint
  {
  public:
    SfePrint(void) { _outputPort = nullptr; }

    void init(SfeOutput &outputPort) { _outputPort = &outputPort; }
    inline bool operator==(SfePrint const &other) const { return _outputPort == other._outputPort; }
    inline bool operator!=(SfePrint const &other) const { return !(*this == other); }

    void write(uint8_t c)
    {
      if (_outputPort != nullptr)
        _outputPort->write(&c, 1);
    }
    void print(const char *c)
    {
      if ((_outputPort != nullptr) && (c != nullptr))
        _outputPort->write((const uint8_t *)c, strlen(c));
    }
    void print(uint32_t value, int base = DEC)
    {
      char buf[12];
      if (base == HEX)
        snprintf(buf, sizeof(buf), "%lX", (unsigned long)value);
      else
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)value);
      print(buf);
    }
    void println() { print("\r\n"); }
    void println(const char *c)
    {
      print(c);
      println();
    }
    void println(uint32_t value, int base = DEC)
    {
      print(value, base);
      println();
    }

  private:
    SfeOutput *_outputPort;
  };

};

// The type used by the public API for debug and message output ports: enableDebugging(), setNMEAOutputPort(), etc.
typedef SparkFun_UBLOX_GNSS::SfeOutput sfe_print_t;

#endif // SFE_ESP_IDF
