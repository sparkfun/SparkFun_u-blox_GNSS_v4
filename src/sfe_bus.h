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
 * 
 * @file sfe_bus.h
 */

#pragma once

#include "sfe_platform.h"

namespace SparkFun_UBLOX_GNSS
{

  // The following abstract class is used an interface for upstream implementation.
  class GNSSDeviceBus
  {
  public:
    // For I2C, ping the _address
    // Not Applicable for SPI and Serial
    /**
     * @brief Check whether a GNSS module is present/responding on this bus.
     *
     * Meaningful for I2C only (sends an address-only transmission and checks for an ACK); SPI
     * and Serial cannot detect presence this way and their overrides simply return false.
     *
     * @return true if a device responded, false otherwise (or if not applicable to this bus).
     */
    virtual bool ping() = 0;

    // For Serial, return Serial.available()
    // For I2C, read registers 0xFD and 0xFE. Return bytes available as uint16_t
    // Not Applicable for SPI
    /**
     * @brief Get the number of bytes currently available to read from this bus.
     *
     * For Serial, this is Serial.available(). For I2C, this reads the module's 0xFD/0xFE
     * byte-count registers. Not applicable for SPI (always returns 0).
     *
     * @return Number of bytes available to read.
     */
    virtual uint16_t available() = 0;

    // For Serial, do Serial.write
    // For I2C, push data to register 0xFF. Chunkify if necessary. Prevent single byte writes as these are illegal
    // For SPI, writing bytes will also read bytes simultaneously. Read data is _ignored_ here. Use writeReadBytes
    /**
     * @brief Write a block of bytes out over this bus.
     *
     * For Serial, this is Serial.write(). For I2C, this pushes the data to register 0xFF in one
     * transmission. For SPI, writing also reads simultaneously, but the read data is discarded
     * here - use writeReadBytes() to keep it.
     *
     * @param data Pointer to the bytes to write.
     * @param length Number of bytes to write.
     * @return The number of bytes actually written (0 on failure).
     */
    virtual uint8_t writeBytes(uint8_t *data, uint8_t length) = 0;

    // For SPI, writing bytes will also read bytes simultaneously. Read data is returned in readData
    /**
     * @brief Write a block of bytes while simultaneously capturing the bytes read back.
     *
     * Meaningful for SPI, where every write is inherently also a read. Not applicable for I2C
     * or Serial (their overrides are no-ops that return 0).
     *
     * @param data Pointer to the bytes to write.
     * @param readData Buffer to receive the bytes read back, same length as 'data'.
     * @param length Number of bytes to transfer.
     * @return The number of bytes actually transferred (0 on failure or if not applicable).
     */
    virtual uint8_t writeReadBytes(const uint8_t *data, uint8_t *readData, uint8_t length) = 0;
    /**
     * @brief Begin a byte-at-a-time write/read transaction (SPI: beginTransaction() + assert CS).
     *
     * Paired with endWriteReadByte(); not applicable for I2C or Serial (no-ops there).
     */
    virtual void startWriteReadByte() = 0;                                  // beginTransaction
    /**
     * @brief Transfer one byte within an open startWriteReadByte()/endWriteReadByte() transaction.
     *
     * @param data Pointer to the byte to write.
     * @param readData Filled in with the byte read back (SPI: the module's simultaneous reply).
     */
    virtual void writeReadByte(const uint8_t *data, uint8_t *readData) = 0; // transfer
    /**
     * @brief Transfer one byte (by value) within an open startWriteReadByte()/endWriteReadByte() transaction.
     *
     * @param data The byte to write.
     * @param readData Filled in with the byte read back (SPI: the module's simultaneous reply).
     */
    virtual void writeReadByte(const uint8_t data, uint8_t *readData) = 0;  // transfer
    /**
     * @brief End a byte-at-a-time write/read transaction (SPI: de-assert CS + endTransaction()).
     */
    virtual void endWriteReadByte() = 0;                                    // endTransaction

    // For Serial, attempt Serial.read
    // For I2C, read from register 0xFF
    // For SPI, read the byte while writing 0xFF
    /**
     * @brief Read a block of bytes in from this bus.
     *
     * For Serial, this is Serial.read()/readBytes(). For I2C, this reads from register 0xFF.
     * For SPI, this clocks out 0xFF while capturing the bytes read back.
     *
     * @param data Buffer to receive the bytes read.
     * @param length Number of bytes to read.
     * @return The number of bytes actually read.
     */
    virtual uint8_t readBytes(uint8_t *data, uint8_t length) = 0;
  };

#if defined(SFE_ARDUINO)

  // The SfeI2C device defines behavior for I2C implementation based around the TwoWire class (Wire).
  // This is Arduino specific.
  class SfeI2C : public GNSSDeviceBus
  {
  public:
    /**
     * @brief Construct an SfeI2C bus object with no port/address set yet.
     */
    SfeI2C(void);

    /**
     * @brief Initialize this I2C bus using the default Wire port.
     *
     * @param address The 7-bit I2C address of the GNSS module.
     * @return true on success.
     */
    bool init(uint8_t address);

    /**
     * @brief Initialize this I2C bus using a caller-supplied Wire port.
     *
     * The address is always (re-)applied, even if this object was already initialized, so a
     * caller can change the module's I2C address without re-constructing the bus object.
     *
     * @param wirePort The TwoWire (Wire) instance to use.
     * @param address The 7-bit I2C address of the GNSS module.
     * @param bInit If true, also call wirePort.begin(). Defaults to false (caller already began it).
     * @return true on success.
     */
    bool init(TwoWire &wirePort, uint8_t address, bool bInit = false);

    /**
     * @brief Check whether the module ACKs on this I2C address.
     *
     * @return true if the module responded, false otherwise (or if not yet initialized).
     */
    bool ping();

    /**
     * @brief Read the module's byte-count registers (0xFD/0xFE) to see how many bytes are waiting.
     *
     * @return Number of bytes available to read, or 0 if not initialized or the module did not respond.
     */
    uint16_t available();

    /**
     * @brief Write a block of bytes to register 0xFF in a single I2C transmission.
     *
     * @param data Pointer to the bytes to write.
     * @param length Number of bytes to write.
     * @return The number of bytes actually written, or 0 on failure/not initialized/zero length.
     */
    uint8_t writeBytes(uint8_t *data, uint8_t length);

    /**
     * @brief Not applicable to I2C; always returns 0 without touching the bus.
     *
     * @param data Unused.
     * @param readData Unused.
     * @param length Unused.
     * @return Always 0.
     */
    uint8_t writeReadBytes(const uint8_t *data, uint8_t *readData, uint8_t length)
    { (void)data; (void)readData; (void)length; return 0; }

    /** @brief Not applicable to I2C; no-op. */
    void startWriteReadByte(){};
    /** @brief Not applicable to I2C; no-op. @param data Unused. @param readData Unused. */
    void writeReadByte(const uint8_t *data, uint8_t *readData){ (void)data; (void)readData; }
    /** @brief Not applicable to I2C; no-op. @param data Unused. @param readData Unused. */
    void writeReadByte(const uint8_t data, uint8_t *readData){ (void)data; (void)readData; }
    /** @brief Not applicable to I2C; no-op. */
    void endWriteReadByte(){};

    /**
     * @brief Read a block of bytes from register 0xFF via Wire.requestFrom().
     *
     * @param data Buffer to receive the bytes read.
     * @param length Number of bytes to read.
     * @return The number of bytes actually returned by the module, or 0 on failure.
     */
    uint8_t readBytes(uint8_t *data, uint8_t length);

  private:
    TwoWire *_i2cPort;
    uint8_t _address;
  };

  // The SfeSPI class defines behavior for SPI implementation based around the SPIClass class (SPI).
  // This is Arduino specific.
  // Note that writeBytes also reads bytes (into data)
  class SfeSPI : public GNSSDeviceBus
  {
  public:
    /**
     * @brief Construct an SfeSPI bus object with no port/chip-select set yet.
     */
    SfeSPI(void);

    /**
     * @brief Initialize this SPI bus using the default SPI port at 4 MHz, mode 0.
     *
     * @param cs The chip-select pin to drive for this module (required - platform/project-specific).
     * @return true on success, false if 'cs' is 0.
     */
    bool init(uint8_t cs);

    /**
     * @brief Initialize this SPI bus using a caller-supplied port and transaction settings.
     *
     * Configures 'cs' as an output, driven HIGH (deselected) once initialized.
     *
     * @param spiPort The SPIClass instance to use.
     * @param spiSettings SPISettings (clock, bit order, mode) applied on every transaction.
     * @param cs The chip-select pin to drive for this module (required).
     * @param bInit If true, also call spiPort.begin(). Defaults to false.
     * @return true on success, false if 'cs' is 0.
     */
    bool init(SPIClass &spiPort, SPISettings &spiSettings, uint8_t cs, bool bInit = false);

    /**
     * @brief Initialize this SPI bus using a caller-supplied port and clock speed (mode 0, MSB first).
     *
     * @param spiPort The SPIClass instance to use.
     * @param spiSpeed SPI clock speed in Hz.
     * @param cs The chip-select pin to drive for this module (required).
     * @param bInit If true, also call spiPort.begin(). Defaults to false.
     * @return true on success, false if 'cs' is 0.
     */
    bool init(SPIClass &spiPort, uint32_t spiSpeed, uint8_t cs, bool bInit = false);

    /**
     * @brief Not applicable to SPI; always returns false.
     *
     * @return Always false.
     */
    bool ping() { return false; }

    /**
     * @brief Not applicable to SPI; always returns 0.
     *
     * @return Always 0.
     */
    uint16_t available();

    /**
     * @brief Write a block of bytes over SPI (asserts CS, transfers, de-asserts CS). Read data is discarded.
     *
     * @param data Pointer to the bytes to write.
     * @param length Number of bytes to write.
     * @return The number of bytes transferred, or 0 on failure/not initialized/zero length.
     */
    uint8_t writeBytes(uint8_t *data, uint8_t length);

    /**
     * @brief Write a block of bytes over SPI while capturing the bytes read back simultaneously.
     *
     * @param data Pointer to the bytes to write.
     * @param readData Buffer to receive the bytes read back, same length as 'data'.
     * @param length Number of bytes to transfer.
     * @return The number of bytes transferred, or 0 on failure/not initialized/zero length.
     */
    uint8_t writeReadBytes(const uint8_t *data, uint8_t *readData, uint8_t length);

    /**
     * @brief Begin a byte-at-a-time SPI transaction: applies settings and asserts CS.
     */
    /**
     * @brief Transfer one byte over SPI within an open startWriteReadByte()/endWriteReadByte() transaction.
     *
     * @param data Pointer to the byte to write.
     * @param readData Filled in with the byte read back.
     */
    /**
     * @brief Transfer one byte (by value) over SPI within an open startWriteReadByte()/endWriteReadByte() transaction.
     *
     * @param data The byte to write.
     * @param readData Filled in with the byte read back.
     */
    /**
     * @brief End a byte-at-a-time SPI transaction: de-asserts CS and ends the transaction.
     */
    void startWriteReadByte();
    void writeReadByte(const uint8_t *data, uint8_t *readData);
    void writeReadByte(const uint8_t data, uint8_t *readData);
    void endWriteReadByte();

    /**
     * @brief Read a block of bytes over SPI, clocking out 0xFF for each byte read.
     *
     * @param data Buffer to receive the bytes read.
     * @param length Number of bytes to read.
     * @return The number of bytes actually read, or 0 on failure/not initialized/zero length.
     */
    uint8_t readBytes(uint8_t *data, uint8_t length);

  private:
    SPIClass *_spiPort;
    // Settings are used for every transaction.
    SPISettings _sfeSPISettings;
    uint8_t _cs;
  };

  // The sfeSerial device defines behavior for Serial (UART) implementation based around the Stream class.
  // This is Arduino specific.
  class SfeSerial : public GNSSDeviceBus
  {
  public:
    /**
     * @brief Construct an SfeSerial bus object with no port set yet.
     */
    SfeSerial(void);

    /**
     * @brief Initialize this Serial bus using a caller-supplied Stream, discarding any stale RX data.
     *
     * @param serialPort The Stream (e.g. a HardwareSerial/SoftwareSerial) to use.
     * @return Always true.
     */
    bool init(Stream &serialPort);

    /**
     * @brief Not applicable to Serial; always returns false.
     *
     * @return Always false.
     */
    bool ping() { return false; }

    /**
     * @brief Get the number of bytes currently waiting in the serial port's RX buffer.
     *
     * @return Number of bytes available, or 0 if not initialized.
     */
    uint16_t available();

    /**
     * @brief Write a block of bytes out the serial port.
     *
     * @param data Pointer to the bytes to write.
     * @param length Number of bytes to write.
     * @return The number of bytes actually written, or 0 on failure/not initialized/zero length.
     */
    uint8_t writeBytes(uint8_t *data, uint8_t length);

    /**
     * @brief Not applicable to Serial; always returns 0 without touching the port.
     *
     * @param data Unused.
     * @param readData Unused.
     * @param length Unused.
     * @return Always 0.
     */
    uint8_t writeReadBytes(const uint8_t *data, uint8_t *readData, uint8_t length)
    { (void)data; (void)readData; (void)length; return 0; }

    /** @brief Not applicable to Serial; no-op. */
    void startWriteReadByte(){};
    /** @brief Not applicable to Serial; no-op. @param data Unused. @param readData Unused. */
    void writeReadByte(const uint8_t *data, uint8_t *readData){ (void)data; (void)readData; }
    /** @brief Not applicable to Serial; no-op. @param data Unused. @param readData Unused. */
    void writeReadByte(const uint8_t data, uint8_t *readData){ (void)data; (void)readData; }
    /** @brief Not applicable to Serial; no-op. */
    void endWriteReadByte(){};

    /**
     * @brief Read a block of bytes from the serial port.
     *
     * @param data Buffer to receive the bytes read.
     * @param length Number of bytes to read.
     * @return The number of bytes actually read, or 0 on failure/not initialized/zero length.
     */
    uint8_t readBytes(uint8_t *data, uint8_t length);

  private:
    Stream *_serialPort;
  };

  // The sfePrint device defines behavior for Serial diagnostic prints based around the Stream class.
  // This is Arduino specific.
  class SfePrint
  {
  public:
    /**
     * @brief Construct an SfePrint with no output port set yet (prints are silently dropped).
     */
    SfePrint(void) { _outputPort = nullptr; }

    /**
     * @brief Set the Print-derived stream that debug/diagnostic output is sent to.
     *
     * @param outputPort The stream to print to (e.g. Serial).
     */
    /**
     * @brief Compare two SfePrint objects by their underlying output port pointer.
     *
     * @param other The SfePrint to compare against.
     * @return true if both wrap the same underlying Print pointer (including both null).
     */
    /**
     * @brief Inverse of operator==().
     *
     * @param other The SfePrint to compare against.
     * @return true if the two objects wrap different underlying Print pointers.
     */
    void init(Print &outputPort) { _outputPort = &outputPort; }
    inline bool operator==(SfePrint const &other) const { return _outputPort == other._outputPort; }
    inline bool operator!=(SfePrint const &other) const { return !(*this == other); }
    
    /**
     * @brief Write a single raw byte to the output port, if one is set.
     *
     * @param c The byte to write.
     */
    void write(uint8_t c)
    {
      if (_outputPort != nullptr)
        _outputPort->write(c);
    }
    /**
     * @brief Print a null-terminated string to the output port, if one is set.
     *
     * @param c The string to print.
     */
    void print(const char *c)
    {
      if (_outputPort != nullptr)
        _outputPort->print(c);
    }
    /**
     * @brief Print a PROGMEM (F()) string to the output port, if one is set.
     *
     * @param c The flash-stored string to print.
     */
    void print(const __FlashStringHelper *c)
    {
      if (_outputPort != nullptr)
        _outputPort->print(c);
    }
    /**
     * @brief Print an unsigned integer in a given base to the output port, if one is set.
     *
     * @param c The value to print.
     * @param f Number base to print in (e.g. HEX, DEC).
     */
    void print(unsigned int c, int f)
    {
      if (_outputPort != nullptr)
        _outputPort->print(c, f);
    }
    /**
     * @brief Print a uint16_t value in decimal to the output port, if one is set.
     *
     * @param c The value to print.
     */
    void print(uint16_t c)
    {
      if (_outputPort != nullptr)
        _outputPort->print(c);
    }
    /**
     * @brief Print a blank line to the output port, if one is set.
     */
    void println()
    {
      if (_outputPort != nullptr)
        _outputPort->println();
    }
    /**
     * @brief Print a null-terminated string followed by a newline, if an output port is set.
     *
     * @param c The string to print.
     */
    void println(const char *c)
    {
      if (_outputPort != nullptr)
        _outputPort->println(c);
    }
    /**
     * @brief Print a PROGMEM (F()) string followed by a newline, if an output port is set.
     *
     * @param c The flash-stored string to print.
     */
    void println(const __FlashStringHelper *c)
    {
      if (_outputPort != nullptr)
        _outputPort->println(c);
    }
    /**
     * @brief Print a size_t value in decimal followed by a newline, if an output port is set.
     *
     * @param c The value to print.
     */
    void println(size_t c)
    {
      if (_outputPort != nullptr)
        _outputPort->println(c);
    }
    /**
     * @brief Print a uint8_t value in a given base followed by a newline, if an output port is set.
     *
     * @param c The value to print.
     * @param f Number base to print in (e.g. HEX, DEC).
     */
    void println(uint8_t c, int f)
    {
      if (_outputPort != nullptr)
        _outputPort->println(c, f);
    }
  
  private: 
    Print *_outputPort;
  };

#endif // SFE_ARDUINO

};

#if defined(SFE_ARDUINO)
// The type used by the public API for debug and message output ports: enableDebugging(), setNMEAOutputPort(), etc.
typedef Print sfe_print_t;
#elif defined(SFE_ESP_IDF)
// ESP-IDF bus classes (SfeI2C, SfeSPI, SfeSerial), output ports (SfeOutput) and SfePrint
#include "sfe_bus_esp_idf.h"
#endif
