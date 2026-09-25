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
 * @file sfe_bus_esp_idf.cpp
 *
 * Native ESP-IDF implementation of the hardware bus classes. See sfe_bus_esp_idf.h.
 * The Arduino implementation is in sfe_bus.cpp.
 */

#include "sfe_platform.h"

#if defined(SFE_ESP_IDF) // This file contains the ESP-IDF implementation. See sfe_bus.cpp for Arduino

#include "sfe_bus.h"
#include "esp_log.h"

static const char *TAG = "sfe_ublox_gnss";

namespace SparkFun_UBLOX_GNSS
{

  SfeStdoutOutput sfeStdout;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // I2C
  //
  // From the u-blox integration manual:
  // "There are two forms of DDC read transfer. The "random access" form includes a peripheral register
  //  address and thus allows any register to be read. The second "current address" form omits the
  //  register address. If this second form is used, then an address pointer in the receiver is used to
  //  determine which register to read. This address pointer will increment after each read unless it
  //  is already pointing at register 0xFF, the highest addressable register, in which case it remains
  //  unaltered."

  SfeI2C::SfeI2C(void) : _bus{nullptr}, _dev{nullptr}, _address{0}, _clockHz{0}
  {
  }

  SfeI2C::~SfeI2C(void)
  {
    removeDevice();
  }

  void SfeI2C::removeDevice(void)
  {
    if (_dev != nullptr)
    {
      i2c_master_bus_rm_device(_dev);
      _dev = nullptr;
    }
  }

  bool SfeI2C::init(i2c_master_bus_handle_t bus, uint8_t address, uint32_t clockHz)
  {
    if (bus == nullptr)
      return false;

    // Nothing to do if we are already attached with these settings
    if ((_dev != nullptr) && (bus == _bus) && (address == _address) && (clockHz == _clockHz))
      return true;

    removeDevice(); // Remove the old device if the address (or bus or speed) has changed

    i2c_device_config_t devConfig = {};
    devConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    devConfig.device_address = address;
    devConfig.scl_speed_hz = clockHz;

    esp_err_t err = i2c_master_bus_add_device(bus, &devConfig, &_dev);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
      _dev = nullptr;
      return false;
    }

    _bus = bus;
    _address = address;
    _clockHz = clockHz;
    return true;
  }

  // Is a device connected? Address-only transaction, check for an ACK
  bool SfeI2C::ping()
  {
    if (_bus == nullptr)
      return false;

    return (i2c_master_probe(_bus, _address, CONFIG_SFE_UBLOX_GNSS_I2C_TIMEOUT_MS) == ESP_OK);
  }

  // Checks how many bytes are waiting in the GNSS's I2C buffer, by reading registers 0xFD and 0xFE.
  // This is a single write-then-read transaction, with a repeated start.
  uint16_t SfeI2C::available()
  {
    if (_dev == nullptr)
      return 0;

    uint8_t reg = 0xFD; // 0xFD (MSB) and 0xFE (LSB) are the registers that contain number of bytes available
    uint8_t buf[2] = {0, 0};
    if (i2c_master_transmit_receive(_dev, &reg, 1, buf, 2, CONFIG_SFE_UBLOX_GNSS_I2C_TIMEOUT_MS) != ESP_OK)
      return 0; // Sensor did not ACK

    return (((uint16_t)buf[0]) << 8) | buf[1];
  }

  // Write data to register 0xFF in a single transmission
  uint8_t SfeI2C::writeBytes(uint8_t *data, uint8_t length)
  {
    if ((_dev == nullptr) || (length == 0))
      return 0;

    if (i2c_master_transmit(_dev, data, length, CONFIG_SFE_UBLOX_GNSS_I2C_TIMEOUT_MS) != ESP_OK)
      return 0;

    return length;
  }

  // "Current address" read - from register 0xFF
  uint8_t SfeI2C::readBytes(uint8_t *data, uint8_t length)
  {
    if ((_dev == nullptr) || (length == 0))
      return 0;

    if (i2c_master_receive(_dev, data, length, CONFIG_SFE_UBLOX_GNSS_I2C_TIMEOUT_MS) != ESP_OK)
      return 0;

    return length;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // SPI

  // Some ESP32 SPI configurations (no DMA) cannot transfer more than 64 bytes in one transaction.
  // Chip select is driven manually, so splitting a transfer into several transactions is invisible to the module.
  static const size_t kSpiMaxChunk = 64;

  SfeSPI::SfeSPI(void) : _dev{nullptr}, _ownDevice{false}, _cs{GPIO_NUM_NC}, _busAcquired{false}
  {
  }

  SfeSPI::~SfeSPI(void)
  {
    removeDevice();
  }

  void SfeSPI::removeDevice(void)
  {
    if ((_dev != nullptr) && _ownDevice)
      spi_bus_remove_device(_dev);
    _dev = nullptr;
    _ownDevice = false;
  }

  bool SfeSPI::configureCS(gpio_num_t cs)
  {
    // The chip select pin can vary from platform to platform and project to project
    // and so it must be given by the user.
    if ((cs == GPIO_NUM_NC) || !GPIO_IS_VALID_OUTPUT_GPIO(cs))
    {
      ESP_LOGE(TAG, "SPI: invalid chip select GPIO %d", (int)cs);
      return false;
    }

    _cs = cs;
    sfe_pin_output((int)_cs);
    sfe_pin_write((int)_cs, true); // Deselect
    return true;
  }

  bool SfeSPI::init(spi_host_device_t host, gpio_num_t cs, uint32_t clockHz)
  {
    if (_dev != nullptr) // Already initialized
      return true;

    if (!configureCS(cs))
      return false;

    spi_device_interface_config_t devConfig = {};
    devConfig.mode = 0;             // u-blox modules use SPI mode 0
    devConfig.clock_speed_hz = (int)clockHz;
    devConfig.spics_io_num = -1;    // Chip select is driven manually
    devConfig.queue_size = 1;

    esp_err_t err = spi_bus_add_device(host, &devConfig, &_dev);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(err));
      _dev = nullptr;
      return false;
    }

    _ownDevice = true;
    return true;
  }

  bool SfeSPI::init(spi_device_handle_t device, gpio_num_t cs)
  {
    if (device == nullptr)
      return false;

    if (_dev == device) // Already initialized
      return true;

    removeDevice();

    if (!configureCS(cs))
      return false;

    _dev = device;
    _ownDevice = false;
    return true;
  }

  // Transfer length bytes. CS must already be low and the bus acquired. rxData can be nullptr
  bool SfeSPI::transfer(const uint8_t *txData, uint8_t *rxData, size_t length)
  {
    while (length > 0)
    {
      size_t chunk = (length > kSpiMaxChunk) ? kSpiMaxChunk : length;

      spi_transaction_t t = {};
      t.length = chunk * 8; // In bits
      t.tx_buffer = txData;
      t.rx_buffer = rxData;
      if (spi_device_polling_transmit(_dev, &t) != ESP_OK)
        return false;

      if (txData != nullptr)
        txData += chunk;
      if (rxData != nullptr)
        rxData += chunk;
      length -= chunk;
    }
    return true;
  }

  // Writing bytes also reads bytes simultaneously. Read data is _ignored_ here
  uint8_t SfeSPI::writeBytes(uint8_t *data, uint8_t length)
  {
    if ((_dev == nullptr) || (length == 0))
      return 0;

    if (spi_device_acquire_bus(_dev, portMAX_DELAY) != ESP_OK)
      return 0;
    sfe_pin_write((int)_cs, false); // Signal communication start

    bool ok = transfer(data, nullptr, length);

    sfe_pin_write((int)_cs, true); // End communication
    spi_device_release_bus(_dev);

    return ok ? length : 0;
  }

  // Read bytes while writing 0xFF
  uint8_t SfeSPI::readBytes(uint8_t *data, uint8_t length)
  {
    if ((_dev == nullptr) || (length == 0))
      return 0;

    uint8_t ones[kSpiMaxChunk];
    memset(ones, 0xFF, sizeof(ones));

    if (spi_device_acquire_bus(_dev, portMAX_DELAY) != ESP_OK)
      return 0;
    sfe_pin_write((int)_cs, false);

    bool ok = true;
    uint8_t remaining = length;
    uint8_t *ptr = data;
    while (ok && (remaining > 0))
    {
      uint8_t chunk = (remaining > kSpiMaxChunk) ? (uint8_t)kSpiMaxChunk : remaining;
      ok = transfer(ones, ptr, chunk);
      ptr += chunk;
      remaining -= chunk;
    }

    sfe_pin_write((int)_cs, true);
    spi_device_release_bus(_dev);

    return ok ? length : 0;
  }

  // Writing bytes also reads bytes simultaneously. Read data is returned in readData
  uint8_t SfeSPI::writeReadBytes(const uint8_t *data, uint8_t *readData, uint8_t length)
  {
    if ((_dev == nullptr) || (length == 0))
      return 0;

    if (spi_device_acquire_bus(_dev, portMAX_DELAY) != ESP_OK)
      return 0;
    sfe_pin_write((int)_cs, false);

    bool ok = transfer(data, readData, length);

    sfe_pin_write((int)_cs, true);
    spi_device_release_bus(_dev);

    return ok ? length : 0;
  }

  // Byte-at-a-time transactions: startWriteReadByte(), writeReadByte() ... endWriteReadByte()
  void SfeSPI::startWriteReadByte()
  {
    if (_dev == nullptr)
      return;

    if (spi_device_acquire_bus(_dev, portMAX_DELAY) != ESP_OK)
      return;
    _busAcquired = true;
    sfe_pin_write((int)_cs, false);
  }

  void SfeSPI::writeReadByte(const uint8_t *data, uint8_t *readData)
  {
    writeReadByte(*data, readData);
  }

  void SfeSPI::writeReadByte(const uint8_t data, uint8_t *readData)
  {
    *readData = 0xFF;
    if ((_dev == nullptr) || !_busAcquired)
      return;

    spi_transaction_t t = {};
    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    t.length = 8;
    t.tx_data[0] = data;
    if (spi_device_polling_transmit(_dev, &t) == ESP_OK)
      *readData = t.rx_data[0];
  }

  void SfeSPI::endWriteReadByte()
  {
    if ((_dev == nullptr) || !_busAcquired)
      return;

    sfe_pin_write((int)_cs, true);
    spi_device_release_bus(_dev);
    _busAcquired = false;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Serial (UART)

  SfeSerial::SfeSerial(void) : _port{UART_NUM_0}, _initialized{false} // uart_port_t is an enum in ESP-IDF v6 (it was an int in v5)
  {
  }

  bool SfeSerial::init(uart_port_t port)
  {
    if (!uart_is_driver_installed(port))
    {
      ESP_LOGE(TAG, "UART%d: uart_driver_install() has not been called", (int)port);
      return false;
    }

    _port = port;
    _initialized = true;

    // Get rid of any stale serial data already in the processor's RX buffer
    uart_flush_input(_port);

    return true;
  }

  uint16_t SfeSerial::available()
  {
    if (!_initialized)
      return 0;

    size_t len = 0;
    if (uart_get_buffered_data_len(_port, &len) != ESP_OK)
      return 0;

    return (len > 0xFFFF) ? 0xFFFF : (uint16_t)len;
  }

  uint8_t SfeSerial::writeBytes(uint8_t *data, uint8_t length)
  {
    if ((!_initialized) || (length == 0))
      return 0;

    int written = uart_write_bytes(_port, (const void *)data, length);
    return (written < 0) ? 0 : (uint8_t)written;
  }

  uint8_t SfeSerial::readBytes(uint8_t *data, uint8_t length)
  {
    if ((!_initialized) || (length == 0))
      return 0;

    // The library only reads bytes it knows are available, so don't wait
    int bytesRead = uart_read_bytes(_port, data, length, 0);
    return (bytesRead < 0) ? 0 : (uint8_t)bytesRead;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Output ports

  size_t SfeStdoutOutput::write(const uint8_t *buffer, size_t length)
  {
    return fwrite(buffer, 1, length, stdout);
  }

  size_t SfeUartOutput::write(const uint8_t *buffer, size_t length)
  {
    int written = uart_write_bytes(_port, (const void *)buffer, length);
    return (written < 0) ? 0 : (size_t)written;
  }

}

#endif // SFE_ESP_IDF
