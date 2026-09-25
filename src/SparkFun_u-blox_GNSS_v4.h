/**
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * Please see LICENSE.md for more details
 * 
 * An Arduino Library which allows you to communicate seamlessly with u-blox GNSS modules using the Configuration Interface
 * 
 * SparkFun sells these at its website: www.sparkfun.com
 * Do you like this library? Help support SparkFun. Buy a board!
 * https://www.sparkfun.com/sparkfun-allband-gnss-rtk-breakout-zed-x20p-qwiic.html
 * https://www.sparkfun.com/sparkfun-gps-rtk2-board-zed-f9p-qwiic-gps-15136.html
 * https://www.sparkfun.com/sparkfun-gps-rtk-sma-breakout-zed-f9p-qwiic.html
 * https://www.sparkfun.com/sparkfun-gnss-receiver-breakout-max-m10s-qwiic.html
 * https://www.sparkfun.com/sparkfun-gps-rtk-dead-reckoning-breakout-zed-f9r-qwiic-gps-22693.html
 *
 * Original version by Nathan Seidle @ SparkFun Electronics, September 6th, 2018
 * v2.0 rework by Paul Clark @ SparkFun Electronics, December 31st, 2020
 * v3.0 rework by Paul Clark @ SparkFun Electronics, December 8th, 2022
 * v4.0 rework by Claude, directed by Paul Clark @ SparkFun Electronics, September 2026
 *
 * https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4
 * 
 * @file SparkFun_u-blox_GNSS_v4.h
 */

#pragma once

#include "sfe_platform.h"

#include "u-blox_GNSS.h"
#include "u-blox_external_typedefs.h"
#include "sfe_bus.h"

class SFE_UBLOX_GNSS : public DevUBLOXGNSS
{
public:
  /**
   * @brief Construct an SFE_UBLOX_GNSS object configured for I2C communication.
   */
  SFE_UBLOX_GNSS() { _commType = COMM_TYPE_I2C; }

#if defined(SFE_ARDUINO)

  ///////////////////////////////////////////////////////////////////////
  // begin()
  //
  // This method is called to initialize the SFE_UBLOX_GNSS library and connect to
  // the GNSS device. This method must be called before calling any other method
  // that interacts with the device.
  //
  // Begin will then return true if "signs of life" have been seen: reception of _any_ valid UBX packet or _any_ valid NMEA header.
  //
  // This method follows the standard startup pattern in SparkFun Arduino
  // libraries.
  //
  //  Parameter   Description
  //  ---------   ----------------------------
  //  wirePort    optional. The Wire port. If not provided, the default port is used
  //  address     optional. I2C Address. If not provided, the default address is used.
  //  retval      true on success, false on startup failure
  //
  // This methond is overridden, implementing two versions.
  //
  // Version 1:
  // User skips passing in an I2C object which then defaults to Wire.
  /**
   * @brief Initialize I2C communication with the GNSS module using the default Wire port.
   *
   * Must be called before any other method that interacts with the device.
   *
   * @param deviceAddress I2C address of the module. Defaults to kUBLOXGNSSDefaultAddress.
   * @param maxWait Timeout in milliseconds for module communication. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @param assumeSuccess If true, skip waiting for "signs of life" and assume the module is present. Defaults to false.
   * @return true if signs of life were seen (or assumeSuccess is true), false on startup failure.
   */
  bool begin(uint8_t deviceAddress = kUBLOXGNSSDefaultAddress, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    // Setup  I2C object and pass into the superclass
    setCommunicationBus(_i2cBus);

    // Initialize the I2C buss class i.e. setup default Wire port
    _i2cBus.init(deviceAddress);

    // Initialize the system - return results
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

  // Version 2:
  //  User passes in an I2C object and an address (optional).
  /**
   * @brief Initialize I2C communication with the GNSS module using a caller-supplied Wire port.
   *
   * Must be called before any other method that interacts with the device.
   *
   * @param wirePort The TwoWire (Wire) instance to use.
   * @param deviceAddress I2C address of the module. Defaults to kUBLOXGNSSDefaultAddress.
   * @param maxWait Timeout in milliseconds for module communication. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @param assumeSuccess If true, skip waiting for "signs of life" and assume the module is present. Defaults to false.
   * @return true if signs of life were seen (or assumeSuccess is true), false on startup failure.
   */
  bool begin(TwoWire &wirePort, uint8_t deviceAddress = kUBLOXGNSSDefaultAddress, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    // Setup  I2C object and pass into the superclass
    setCommunicationBus(_i2cBus);

    // Give the I2C port provided by the user to the I2C bus class.
    _i2cBus.init(wirePort, deviceAddress);

    // Initialize the system - return results
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

#elif defined(SFE_ESP_IDF)

  /**
   * @brief Initialize I2C communication with the GNSS module (ESP-IDF).
   *
   * The I2C bus must already have been created with i2c_new_master_bus(). The module is added
   * to the bus as a device (default clock 400kHz - see menuconfig).
   *
   * Returns true if "signs of life" have been seen: reception of _any_ valid UBX packet or _any_ valid NMEA header.
   *
   * @param bus The I2C master bus handle.
   * @param deviceAddress The module's 7-bit I2C address. Defaults to kUBLOXGNSSDefaultAddress (0x42).
   * @param maxWait Maximum time, in ms, to wait for the module to respond.
   * @param assumeSuccess If true, return true even if no response was received.
   * @return true on success, false on startup failure.
   */
  bool begin(i2c_master_bus_handle_t bus, uint8_t deviceAddress = kUBLOXGNSSDefaultAddress, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    setCommunicationBus(_i2cBus);

    if (!_i2cBus.init(bus, deviceAddress))
      return false;

    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

#endif

private:
  // I2C bus class
  SparkFun_UBLOX_GNSS::SfeI2C _i2cBus;
};

class SFE_UBLOX_GNSS_SPI : public DevUBLOXGNSS
{
public:
  /**
   * @brief Construct an SFE_UBLOX_GNSS_SPI object configured for SPI communication.
   */
  SFE_UBLOX_GNSS_SPI() { _commType = COMM_TYPE_SPI; }

#if defined(SFE_ARDUINO)

  ///////////////////////////////////////////////////////////////////////
  // begin()
  //
  // This method is called to initialize the ISM330DHCX library and connect to
  // the ISM330DHCX device. This method must be called before calling any other method
  // that interacts with the device.
  //
  // This method follows the standard startup pattern in SparkFun Arduino
  // libraries.
  //
  //  Parameter   Description
  //  ---------   ----------------------------
  //  spiPort     optional. The SPI port. If not provided, the default port is used
  //  SPISettings optional. SPI "transaction" settings are need for every data transfer.
  //												Default used if not provided.
  //  Chip Select mandatory. The chip select pin ("CS") can't be guessed, so must be provided.
  //  retval      true on success, false on startup failure
  //
  // This methond is overridden, implementing three versions.
  //
  // Version 1:
  // User skips passing in an SPI object which then defaults to SPI.

  /**
   * @brief Initialize SPI communication with the GNSS module using the default SPI port and settings.
   *
   * Must be called before any other method that interacts with the device.
   *
   * @param cs The chip-select pin to drive for this module (required).
   * @param maxWait Timeout in milliseconds for module communication. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @param assumeSuccess If true, skip waiting for "signs of life" and assume the module is present. Defaults to false.
   * @return true if signs of life were seen (or assumeSuccess is true), false on startup failure.
   */
  bool begin(uint8_t cs, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    // Setup a SPI object and pass into the superclass
    setCommunicationBus(_spiBus);

    // Initialize the SPI bus class with the chip select pin, SPI port defaults to SPI,
    // and SPI settings are set to class defaults.
    _spiBus.init(cs);

    // Initialize the system - return results
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

  // Version 2:
  // User passes in an SPI object and SPISettings (optional).
  /**
   * @brief Initialize SPI communication using a caller-supplied SPI port and transaction settings.
   *
   * Must be called before any other method that interacts with the device.
   *
   * @param spiPort The SPIClass instance to use.
   * @param cs The chip-select pin to drive for this module (required).
   * @param ismSettings SPISettings (clock, bit order, mode) applied on every transaction.
   * @param maxWait Timeout in milliseconds for module communication. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @param assumeSuccess If true, skip waiting for "signs of life" and assume the module is present. Defaults to false.
   * @return true if signs of life were seen (or assumeSuccess is true), false on startup failure.
   */
  bool begin(SPIClass &spiPort, uint8_t cs, SPISettings ismSettings, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    // Setup a SPI object and pass into the superclass
    setCommunicationBus(_spiBus);

    // Initialize the SPI bus class with provided SPI port, SPI setttings, and chip select pin.
    _spiBus.init(spiPort, ismSettings, cs, true);

    // Initialize the system - return results
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

  // Version 3:
  // User passes in an SPI object and SPI speed (optional).
  /**
   * @brief Initialize SPI communication using a caller-supplied SPI port and clock speed.
   *
   * Must be called before any other method that interacts with the device.
   *
   * @param spiPort The SPIClass instance to use.
   * @param cs The chip-select pin to drive for this module (required).
   * @param spiSpeed SPI clock speed in Hz.
   * @param maxWait Timeout in milliseconds for module communication. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @param assumeSuccess If true, skip waiting for "signs of life" and assume the module is present. Defaults to false.
   * @return true if signs of life were seen (or assumeSuccess is true), false on startup failure.
   */
  bool begin(SPIClass &spiPort, uint8_t cs, uint32_t spiSpeed, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    // Setup a SPI object and pass into the superclass
    setCommunicationBus(_spiBus);

    // Initialize the SPI bus class with provided SPI port, SPI setttings, and chip select pin.
    _spiBus.init(spiPort, spiSpeed, cs, true);

    // Initialize the system - return results
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

#elif defined(SFE_ESP_IDF)

  /**
   * @brief Initialize SPI communication with the GNSS module (ESP-IDF).
   *
   * The SPI bus must already have been initialized with spi_bus_initialize(). The module is added
   * to the bus as a device (SPI mode 0). Chip select is driven by the library as a GPIO.
   *
   * @param host The SPI host the bus was initialized on (e.g. SPI2_HOST).
   * @param cs The chip select GPIO.
   * @param spiSpeed The SPI clock speed in Hz. Defaults to 4MHz.
   * @param maxWait Maximum time, in ms, to wait for the module to respond.
   * @param assumeSuccess If true, return true even if no response was received.
   * @return true on success, false on startup failure.
   */
  bool begin(spi_host_device_t host, gpio_num_t cs, uint32_t spiSpeed = 4000000, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    setCommunicationBus(_spiBus);

    if (!_spiBus.init(host, cs, spiSpeed))
      return false;

    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

  /**
   * @brief Initialize SPI communication using an SPI device the user has already added to the bus (ESP-IDF).
   *
   * The device must have been added with spics_io_num = -1 (chip select is driven by the library) and SPI mode 0.
   *
   * @param device The SPI device handle.
   * @param cs The chip select GPIO.
   * @param maxWait Maximum time, in ms, to wait for the module to respond.
   * @param assumeSuccess If true, return true even if no response was received.
   * @return true on success, false on startup failure.
   */
  bool begin(spi_device_handle_t device, gpio_num_t cs, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    setCommunicationBus(_spiBus);

    if (!_spiBus.init(device, cs))
      return false;

    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

#endif

private:
  // SPI bus class
  SparkFun_UBLOX_GNSS::SfeSPI _spiBus;
};

class SFE_UBLOX_GNSS_SERIAL : public DevUBLOXGNSS
{
public:
  /**
   * @brief Construct an SFE_UBLOX_GNSS_SERIAL object configured for Serial (UART) communication.
   */
  SFE_UBLOX_GNSS_SERIAL() { _commType = COMM_TYPE_SERIAL; }

#if defined(SFE_ARDUINO)

  ///////////////////////////////////////////////////////////////////////
  // begin()
  //
  // This method is called to initialize the SFE_UBLOX_GNSS library and connect to
  // the GNSS device. This method must be called before calling any other method
  // that interacts with the device.
  //
  // Begin will then return true if "signs of life" have been seen: reception of _any_ valid UBX packet or _any_ valid NMEA header.
  //
  // This method follows the standard startup pattern in SparkFun Arduino
  // libraries.
  //
  //  Parameter   Description
  //  ---------   ----------------------------
  //  serialPort  The Serial Stream
  //  retval      true on success, false on startup failure
  //
  /**
   * @brief Initialize Serial communication with the GNSS module using a caller-supplied Stream.
   *
   * Must be called before any other method that interacts with the device.
   *
   * @param serialPort The Stream (e.g. a HardwareSerial/SoftwareSerial) to use.
   * @param maxWait Timeout in milliseconds for module communication. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @param assumeSuccess If true, skip waiting for "signs of life" and assume the module is present. Defaults to false.
   * @return true if signs of life were seen (or assumeSuccess is true), false on startup failure.
   */
  bool begin(Stream &serialPort, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    // Setup Serial object and pass into the superclass
    setCommunicationBus(_serialBus);

    // Initialize the Serial bus class
    _serialBus.init(serialPort);

    // Initialize the system - return results
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

#elif defined(SFE_ESP_IDF)

  /**
   * @brief Initialize Serial (UART) communication with the GNSS module (ESP-IDF).
   *
   * The UART must already have been configured with uart_driver_install(), uart_param_config()
   * and uart_set_pin(). Use an RX buffer of at least 1024 bytes (2048+ recommended).
   *
   * Returns true if "signs of life" have been seen: reception of _any_ valid UBX packet or _any_ valid NMEA header.
   *
   * @param port The UART port (e.g. UART_NUM_1).
   * @param maxWait Maximum time, in ms, to wait for the module to respond.
   * @param assumeSuccess If true, return true even if no response was received.
   * @return true on success, false on startup failure.
   */
  bool begin(uart_port_t port, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    setCommunicationBus(_serialBus);

    if (!_serialBus.init(port))
      return false;

    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

#endif

private:
  // I2C bus class
  SparkFun_UBLOX_GNSS::SfeSerial _serialBus;
};

class SFE_UBLOX_GNSS_SUPER : public DevUBLOXGNSS // A Super Class - all three in one
{
public:
  /**
   * @brief Construct an SFE_UBLOX_GNSS_SUPER object. Communication bus is chosen by which begin() overload is called.
   */
  SFE_UBLOX_GNSS_SUPER(){};

#if defined(SFE_ARDUINO)

  /**
   * @brief Initialize I2C communication with the GNSS module using the default Wire port.
   *
   * Selects the I2C bus for this instance. Must be called before any other method that
   * interacts with the device.
   *
   * @param deviceAddress I2C address of the module. Defaults to kUBLOXGNSSDefaultAddress.
   * @param maxWait Timeout in milliseconds for module communication. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @param assumeSuccess If true, skip waiting for "signs of life" and assume the module is present. Defaults to false.
   * @return true if signs of life were seen (or assumeSuccess is true), false on startup failure.
   */
  bool begin(uint8_t deviceAddress = kUBLOXGNSSDefaultAddress, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
     _commType = COMM_TYPE_I2C;

    // Setup  I2C object and pass into the superclass
    setCommunicationBus(_i2cBus);

    // Initialize the I2C buss class i.e. setup default Wire port
    _i2cBus.init(deviceAddress);

    // Initialize the system - return results
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

  /**
   * @brief Initialize I2C communication with the GNSS module using a caller-supplied Wire port.
   *
   * Selects the I2C bus for this instance. Must be called before any other method that
   * interacts with the device.
   *
   * @param wirePort The TwoWire (Wire) instance to use.
   * @param deviceAddress I2C address of the module. Defaults to kUBLOXGNSSDefaultAddress.
   * @param maxWait Timeout in milliseconds for module communication. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @param assumeSuccess If true, skip waiting for "signs of life" and assume the module is present. Defaults to false.
   * @return true if signs of life were seen (or assumeSuccess is true), false on startup failure.
   */
  bool begin(TwoWire &wirePort, uint8_t deviceAddress = kUBLOXGNSSDefaultAddress, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
     _commType = COMM_TYPE_I2C;

    // Setup  I2C object and pass into the superclass
    setCommunicationBus(_i2cBus);

    // Give the I2C port provided by the user to the I2C bus class.
    _i2cBus.init(wirePort, deviceAddress);

    // Initialize the system - return results
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

  /**
   * @brief Initialize SPI communication using a caller-supplied SPI port and transaction settings.
   *
   * Selects the SPI bus for this instance. Must be called before any other method that
   * interacts with the device.
   *
   * @param spiPort The SPIClass instance to use.
   * @param cs The chip-select pin to drive for this module (required).
   * @param ismSettings SPISettings (clock, bit order, mode) applied on every transaction.
   * @param maxWait Timeout in milliseconds for module communication. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @param assumeSuccess If true, skip waiting for "signs of life" and assume the module is present. Defaults to false.
   * @return true if signs of life were seen (or assumeSuccess is true), false on startup failure.
   */
  bool begin(SPIClass &spiPort, uint8_t cs, SPISettings ismSettings, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
     _commType = COMM_TYPE_SPI;

    // Setup a SPI object and pass into the superclass
    setCommunicationBus(_spiBus);

    // Initialize the SPI bus class with provided SPI port, SPI setttings, and chip select pin.
    _spiBus.init(spiPort, ismSettings, cs, true);

    // Initialize the system - return results
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

  /**
   * @brief Initialize SPI communication using a caller-supplied SPI port and clock speed.
   *
   * Selects the SPI bus for this instance. Must be called before any other method that
   * interacts with the device.
   *
   * @param spiPort The SPIClass instance to use.
   * @param cs The chip-select pin to drive for this module (required).
   * @param spiSpeed SPI clock speed in Hz. Defaults to 4 MHz.
   * @param maxWait Timeout in milliseconds for module communication. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @param assumeSuccess If true, skip waiting for "signs of life" and assume the module is present. Defaults to false.
   * @return true if signs of life were seen (or assumeSuccess is true), false on startup failure.
   */
  bool begin(SPIClass &spiPort, uint8_t cs, uint32_t spiSpeed = 4000000, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
     _commType = COMM_TYPE_SPI;

    // Setup a SPI object and pass into the superclass
    setCommunicationBus(_spiBus);

    // Initialize the SPI bus class with provided SPI port, SPI setttings, and chip select pin.
    _spiBus.init(spiPort, spiSpeed, cs, true);

    // Initialize the system - return results
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

  /**
   * @brief Initialize Serial communication with the GNSS module using a caller-supplied Stream.
   *
   * Selects the Serial bus for this instance. Must be called before any other method that
   * interacts with the device.
   *
   * @param serialPort The Stream (e.g. a HardwareSerial/SoftwareSerial) to use.
   * @param maxWait Timeout in milliseconds for module communication. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @param assumeSuccess If true, skip waiting for "signs of life" and assume the module is present. Defaults to false.
   * @return true if signs of life were seen (or assumeSuccess is true), false on startup failure.
   */
  bool begin(Stream &serialPort, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
     _commType = COMM_TYPE_SERIAL;

    // Setup Serial object and pass into the superclass
    setCommunicationBus(_serialBus);

    // Initialize the Serial bus class
    _serialBus.init(serialPort);

    // Initialize the system - return results
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

#elif defined(SFE_ESP_IDF)

  bool begin(i2c_master_bus_handle_t bus, uint8_t deviceAddress = kUBLOXGNSSDefaultAddress, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    _commType = COMM_TYPE_I2C;
    setCommunicationBus(_i2cBus);
    if (!_i2cBus.init(bus, deviceAddress))
      return false;
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

  bool begin(spi_host_device_t host, gpio_num_t cs, uint32_t spiSpeed = 4000000, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    _commType = COMM_TYPE_SPI;
    setCommunicationBus(_spiBus);
    if (!_spiBus.init(host, cs, spiSpeed))
      return false;
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

  bool begin(spi_device_handle_t device, gpio_num_t cs, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    _commType = COMM_TYPE_SPI;
    setCommunicationBus(_spiBus);
    if (!_spiBus.init(device, cs))
      return false;
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

  bool begin(uart_port_t port, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false)
  {
    _commType = COMM_TYPE_SERIAL;
    setCommunicationBus(_serialBus);
    if (!_serialBus.init(port))
      return false;
    return this->DevUBLOXGNSS::init(maxWait, assumeSuccess);
  }

#endif

private:
  SparkFun_UBLOX_GNSS::SfeI2C _i2cBus;
  SparkFun_UBLOX_GNSS::SfeSPI _spiBus;
  SparkFun_UBLOX_GNSS::SfeSerial _serialBus;
};
