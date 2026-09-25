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
 * @file u-blox_GNSS.h
 * 
 */

#pragma once

#include "sfe_platform.h"
#include "u-blox_config_keys.h"
#include "u-blox_structs.h"
#include "u-blox_external_typedefs.h"
#include "u-blox_Class_and_ID.h"
#include "sfe_bus.h"
#include "sfe_debug.h" // v4 scaffolding - shared base for debugPrint()/debugPrintln(), see AGENTS.md
#include "ubxMessageVector.h" // v4 scaffolding - see AGENTS.md "Reference Scaffolding"
#include "nmeaMessageVector.h"

// Define a digital pin to aid debugging
// Leave set to -1 if not needed
const int debugPin = -1;

class DevUBLOXGNSS : public SparkFun_UBLOX_GNSS::SfeDebugPrint
{
public:
  /**
   * @brief Construct a new DevUBLOXGNSS object. Call begin() (via one of the SFE_UBLOX_GNSS_* subclasses) before use.
   */
  /**
   * @brief Destroy this DevUBLOXGNSS, freeing all RAM allocated for automatic messages, buffers and helper structs.
   */
  DevUBLOXGNSS(void);
  ~DevUBLOXGNSS(void);

  // New in v3.0: hardware interface is abstracted
  /**
   * @brief Check whether the GNSS module is present and responding.
   *
   * @param maxWait Timeout in milliseconds. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return true if the module responded, false otherwise.
   */
  bool isConnected(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

protected:
  enum commTypes
  {
    COMM_TYPE_I2C = 0,
    COMM_TYPE_SERIAL,
    COMM_TYPE_SPI
  } _commType = COMM_TYPE_I2C; // Controls which port we look to for incoming bytes
  /**
   * @brief Common initialization shared by every begin() overload: allocates buffers and waits for signs of life.
   *
   * @param maxWait Timeout in milliseconds to wait for signs of life.
   * @param assumeSuccess If true, skip waiting and assume the module is present.
   * @return true on success, false on startup failure.
   */
  /**
   * @brief Set which transport bus (I2C/SPI/Serial) this instance communicates over.
   *
   * @param theBus The bus object to use (an SfeI2C, SfeSPI or SfeSerial instance).
   */
  bool init(uint16_t maxWait, bool assumeSuccess);
  void setCommunicationBus(SparkFun_UBLOX_GNSS::GNSSDeviceBus &theBus);
  // For I2C, ping the _address
  // Not Applicable for SPI and Serial
  /**
   * @brief Check whether a module is present on the current bus (I2C only; always false for SPI/Serial).
   *
   * @return true if a device responded.
   */
  bool ping();
  // For Serial, return Serial.available()
  // For I2C, read registers 0xFD and 0xFE. Return bytes available as uint16_t
  // Not Applicable for SPI
  /**
   * @brief Get the number of bytes currently available to read from the current bus.
   *
   * @return Number of bytes available.
   */
  uint16_t available();
  // For Serial, do Serial.write
  // For I2C, push data to register 0xFF. Chunkify if necessary. Prevent single byte writes as these are illegal
  // For SPI, writing bytes will also read bytes simultaneously. Read data is _ignored_ here. Use writeReadBytes
  /**
   * @brief Write a block of bytes out over the current bus.
   *
   * @param data Pointer to the bytes to write.
   * @param length Number of bytes to write.
   * @return The number of bytes actually written.
   */
  uint8_t writeBytes(uint8_t *data, uint8_t length);
  // For SPI, writing bytes will also read bytes simultaneously. Read data is returned in readData
  /**
   * @brief Write a block of bytes while capturing the bytes read back simultaneously (SPI only).
   *
   * @param data Pointer to the bytes to write.
   * @param readData Buffer to receive the bytes read back.
   * @param length Number of bytes to transfer.
   * @return The number of bytes actually transferred.
   */
  /** @brief Begin a byte-at-a-time write/read transaction on the current bus (SPI only; no-op otherwise). */
  /**
   * @brief Transfer one byte within an open startWriteReadByte()/endWriteReadByte() transaction.
   *
   * @param data Pointer to the byte to write.
   * @param readData Filled in with the byte read back.
   */
  /**
   * @brief Transfer one byte (by value) within an open startWriteReadByte()/endWriteReadByte() transaction.
   *
   * @param data The byte to write.
   * @param readData Filled in with the byte read back.
   */
  /** @brief End a byte-at-a-time write/read transaction on the current bus (SPI only; no-op otherwise). */
  uint8_t writeReadBytes(const uint8_t *data, uint8_t *readData, uint8_t length);
  void startWriteReadByte();
  void writeReadByte(const uint8_t *data, uint8_t *readData);
  void writeReadByte(const uint8_t data, uint8_t *readData);
  void endWriteReadByte();
  // For Serial, attempt Serial.read
  // For I2C, read from register 0xFF
  // For SPI, read the byte while writing 0xFF
  /**
   * @brief Read a block of bytes in from the current bus.
   *
   * @param data Buffer to receive the bytes read.
   * @param length Number of bytes to read.
   * @return The number of bytes actually read.
   */
  uint8_t readBytes(uint8_t *data, uint8_t length);
  // Flag to indicate if we are connected to UART1 or UART2
  // Needed to select the correct config items when enabling a periodic message
  bool _UART2 = false; // Default to UART1

  // These lock / unlock functions can be used if you have multiple tasks writing to the bus.
  // The idea is that in a RTOS you override this class and the functions in which you take and give a mutex.
  /** @brief Hook for an RTOS-aware subclass to take a mutex before bus access. Default is a no-op returning true. @return true on success. */
  /** @brief Hook for an RTOS-aware subclass to acquire the mutex created by createLock(). Default is a no-op returning true. @return true on success. */
  /** @brief Hook for an RTOS-aware subclass to release the mutex acquired by lock(). Default is a no-op. */
  /** @brief Hook for an RTOS-aware subclass to destroy the mutex created by createLock(). Default is a no-op. */
  virtual bool createLock(void) { return true; }
  virtual bool lock(void) { return true; }
  virtual void unlock(void) {}
  virtual void deleteLock(void) {}

public:
  /**
   * @brief Tell this instance whether it is talking to the module's UART2 (rather than UART1).
   *
   * Needed so config-key helpers pick the correct UART1/UART2 key. @param connected true if using UART2. Defaults to true.
   */
  void connectedToUART2(bool connected = true) { _UART2 = connected; }

  // Depending on the sentence type the processor will load characters into different arrays
  enum sfe_ublox_sentence_types_e
  {
    SFE_UBLOX_SENTENCE_TYPE_NONE = 0,
    SFE_UBLOX_SENTENCE_TYPE_NMEA,
    SFE_UBLOX_SENTENCE_TYPE_UBX,
    SFE_UBLOX_SENTENCE_TYPE_RTCM
  } currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE;

  // New in v2.0: allow the payload size for packetCfg to be changed
  /**
   * @brief Change the allocated size of the packetCfg payload buffer.
   *
   * @param payloadSize The new payload buffer size, in bytes.
   * @return true on success, false if the allocation failed.
   */
  bool setPacketCfgPayloadSize(size_t payloadSize); // Set packetCfgPayloadSize
  /**
   * @brief Get how many free bytes remain in the packetCfg payload buffer.
   *
   * @return Number of free bytes remaining.
   */
  size_t getPacketCfgSpaceRemaining();              // Returns the number of free bytes remaining in packetCfgPayload

  /** @brief Stop all automatic message processing and free all RAM allocated for buffers/messages. */
  void end(void); // Stop all automatic message processing. Free all used RAM

  /**
   * @brief Change how often (in ms) checkUbloxI2C() re-polls the module for new data.
   *
   * @param newPollingWait_ms The new polling interval, in milliseconds.
   */
  void setI2CpollingWait(uint8_t newPollingWait_ms); // Allow the user to change the I2C polling wait if required
  /**
   * @brief Change how often (in ms) checkUbloxSpi() re-polls the module for new data.
   *
   * @param newPollingWait_ms The new polling interval, in milliseconds.
   */
  void setSPIpollingWait(uint8_t newPollingWait_ms); // Allow the user to change the SPI polling wait if required

  // Set the max number of bytes set in a given I2C transaction
  uint8_t i2cTransactionSize = 32; // Default to ATmega328 limit

  // Control the size of the internal I2C transaction amount
  /**
   * @brief Change the maximum number of bytes written in a single I2C transaction.
   *
   * @param transactionSize The new maximum transaction size, in bytes.
   */
  /**
   * @brief Get the maximum number of bytes written in a single I2C transaction.
   *
   * @return The current maximum transaction size, in bytes.
   */
  void setI2CTransactionSize(uint8_t transactionSize);
  uint8_t getI2CTransactionSize(void);

  // Control the size of the internal I2C transaction amount
  /**
   * @brief Change the maximum number of bytes transferred in a single SPI transaction.
   *
   * @param transactionSize The new maximum transaction size, in bytes.
   */
  /**
   * @brief Get the maximum number of bytes transferred in a single SPI transaction.
   *
   * @return The current maximum transaction size, in bytes.
   */
  void setSpiTransactionSize(uint8_t transactionSize);
  uint8_t getSpiTransactionSize(void);

  // Control the size of the SPI transfer buffer. If the buffer isn't big enough, we'll start to lose bytes
  /**
   * @brief Change the size of the internal buffer used to capture bytes read back during SPI writes.
   *
   * @param bufferSize The new buffer size, in bytes.
   */
  /**
   * @brief Get the size of the internal buffer used to capture bytes read back during SPI writes.
   *
   * @return The current buffer size, in bytes.
   */
  void setSpiBufferSize(size_t bufferSize);
  size_t getSpiBufferSize(void);

  // A dedicated buffer for RTCM data - separate to the logging buffer
  // RTCM data can be stored here and then extracted - avoiding processRTCM
  // This is useful on SPI systems, avoiding bus collisions between checkUblox/processRTCM
  // and pushing the RTCM data to (e.g.) Ethernet
  /**
   * @brief Set the size of the dedicated RTCM buffer. Must be called before .begin().
   *
   * @param bufferSize The new buffer size, in bytes.
   */
  void setRTCMBufferSize(uint16_t bufferSize);                             // Set the size of the RTCM buffer. This must be called _before_ .begin.
  /**
   * @brief Get the size of the dedicated RTCM buffer.
   *
   * @return The buffer size, in bytes.
   */
  uint16_t getRTCMBufferSize(void);                                        // Return the size of the RTCM buffer
  /**
   * @brief Copy bytes out of the dedicated RTCM buffer.
   *
   * @param destination Buffer to copy the extracted bytes into (must be at least 'numBytes' long).
   * @param numBytes Number of bytes to extract.
   * @return The number of bytes actually copied.
   */
  uint16_t extractRTCMBufferData(uint8_t *destination, uint16_t numBytes); // Extract numBytes of data from the RTCM buffer. Copy it to destination. It is the user's responsibility to ensure destination is large enough.
  /**
   * @brief Get how many bytes are waiting to be read from the dedicated RTCM buffer.
   *
   * @return Number of bytes available.
   */
  uint16_t rtcmBufferAvailable(void);                                      // Returns the number of bytes available in the RTCM buffer which are waiting to be read
  /** @brief Empty the dedicated RTCM buffer, discarding all contents. */
  void clearRTCMBuffer(void);                                              // Empty the RTCM buffer - discard all contents

  // Control the size of maxNMEAByteCount
  /**
   * @brief Change the maximum length an incoming NMEA sentence may reach before reception is aborted.
   *
   * @param newMax The new maximum byte count.
   */
  /**
   * @brief Get the maximum length an incoming NMEA sentence may reach before reception is aborted.
   *
   * @return The current maximum byte count.
   */
  void setMaxNMEAByteCount(int8_t newMax);
  int8_t getMaxNMEAByteCount(void);

// Enable debug messages using the chosen Serial port (Stream)
// Boards like the RedBoard Turbo use SerialUSB (not Serial).
// But other boards like the SAMD51 Thing Plus use Serial (not SerialUSB).
// These lines let the code compile cleanly on as many SAMD boards as possible.
  /**
   * @brief Enable debug message printing to the given stream.
   *
   * The default port (SerialUSB or Serial) is chosen at compile time for known SAMD boards by
   * the #if ladder below; every branch has the same signature and behavior.
   *
   * @param debugPort The stream to print debug messages to. Defaults to Serial (or SerialUSB on
   * some SAMD boards).
   * @param printLimitedDebug If true, only print messages flagged as "important". Defaults to false (print everything).
   */
#if defined(SFE_ESP_IDF)                                                              // Native ESP-IDF: default to stdout (the console)
  void enableDebugging(sfe_print_t &debugPort = SparkFun_UBLOX_GNSS::sfeStdout, bool printLimitedDebug = false); // Given a port to print to, enable debug messages. Default to all, not important-only.
#elif defined(ARDUINO_ARCH_SAMD)                                                      // Is this a SAMD board?
#if defined(USB_VID)                                                                  // Is the USB Vendor ID defined?
#if (USB_VID == 0x1B4F)                                                               // Is this a SparkFun board?
#if !defined(ARDUINO_SAMD51_THING_PLUS) & !defined(ARDUINO_SAMD51_MICROMOD)           // If it is not a SAMD51 Thing Plus or SAMD51 MicroMod
  void enableDebugging(Print &debugPort = SerialUSB, bool printLimitedDebug = false); // Given a port to print to, enable debug messages. Default to all, not important-only.
#else
  void enableDebugging(Print &debugPort = Serial, bool printLimitedDebug = false); // Given a port to print to, enable debug messages. Default to all, not important-only.
#endif
#else
  void enableDebugging(Print &debugPort = Serial, bool printLimitedDebug = false); // Given a port to print to, enable debug messages. Default to all, not important-only.
#endif
#else
  void enableDebugging(Print &debugPort = Serial, bool printLimitedDebug = false); // Given a port to print to, enable debug messages. Default to all, not important-only.
#endif
#else
  void enableDebugging(Print &debugPort = Serial, bool printLimitedDebug = false); // Given a port to print to, enable debug messages. Default to all, not important-only.
#endif

  /** @brief Turn off debug message printing (and propagate the change to ubxMessages/nmeaMessages). */
  void disableDebugging(void);                                    // Turn off debug statements
  // debugPrint()/debugPrintln() are inherited from SfeDebugPrint (see sfe_debug.h) - also
  // inherited by ubxMessageVector/nmeaMessageVector, so diagnostics deep inside those classes'
  // own methods can use them too, kept in sync by enableDebugging()/disableDebugging() below.
  /**
   * @brief Get a human-readable string for an sfe_ublox_status_e value.
   *
   * @param stat The status code to describe.
   * @return A short, null-terminated description string.
   */
  const char *statusString(sfe_ublox_status_e stat);              // Pretty print the return value

  // Check for the arrival of new I2C/Serial data
  // Changed in V1.8.1: provides backward compatibility for the examples that call checkUblox directly
  // Will default to using packetCfg to look for explicit autoPVT packets so they get processed correctly by processUBX
  /**
   * @brief Poll the module (over whichever bus was selected in begin()) for new I2C/SPI/Serial data and process it.
   *
   * Defaults to also looking for an explicit autoPVT packet in packetCfg, for backward compatibility.
   *
   * @param requestedClass If non-zero, also watch for a specific Class/ID response in packetCfg. Defaults to 0.
   * @param requestedID Message ID (within requestedClass) to watch for. Defaults to 0.
   * @return true if the requested Class/ID was seen, false otherwise.
   */
  bool checkUblox(uint8_t requestedClass = 0, uint8_t requestedID = 0); // Checks module with user selected commType

  /**
   * @brief Poll the module over I2C for new data and feed any bytes received into process().
   *
   * @param incomingUBX The packet structure to assemble a UBX response into (usually &packetCfg).
   * @param requestedClass If non-zero, watch for a specific Class/ID response.
   * @param requestedID Message ID (within requestedClass) to watch for.
   * @return true if the requested Class/ID was seen.
   */
  bool checkUbloxI2C(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID);    // Method for I2C polling of data, passing any new bytes to process()
  /**
   * @brief Poll the module over Serial for new data and feed any bytes received into process().
   *
   * @param incomingUBX The packet structure to assemble a UBX response into (usually &packetCfg).
   * @param requestedClass If non-zero, watch for a specific Class/ID response.
   * @param requestedID Message ID (within requestedClass) to watch for.
   * @return true if the requested Class/ID was seen.
   */
  bool checkUbloxSerial(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID); // Method for serial polling of data, passing any new bytes to process()
  /**
   * @brief Poll the module over SPI for new data and feed any bytes received into process().
   *
   * @param incomingUBX The packet structure to assemble a UBX response into (usually &packetCfg).
   * @param requestedClass If non-zero, watch for a specific Class/ID response.
   * @param requestedID Message ID (within requestedClass) to watch for.
   * @return true if the requested Class/ID was seen.
   */
  bool checkUbloxSpi(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID);    // Method for spi polling of data, passing any new bytes to process()
  /**
   * @brief Process any backlog of bytes already captured in spiBuffer (called by checkUbloxSpi()).
   *
   * @param incomingUBX The packet structure to assemble a UBX response into.
   * @param requestedClass If non-zero, watch for a specific Class/ID response.
   * @param requestedID Message ID (within requestedClass) to watch for.
   * @return true if the requested Class/ID was seen.
   */
  bool processSpiBuffer(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID); // Called by checkUbloxSpi to process any backlog data in the spiBuffer

  // Process the incoming data

  /**
   * @brief Feed one incoming byte into the NMEA/UBX/RTCM sentence parser.
   *
   * @param incoming The next raw byte received from the module.
   * @param incomingUBX The packet structure to assemble a UBX response into.
   * @param requestedClass If non-zero, watch for a specific Class/ID response.
   * @param requestedID Message ID (within requestedClass) to watch for.
   */
  void process(uint8_t incoming, ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID);             // Processes NMEA and UBX binary sentences one byte at a time
  /**
   * @brief Handle one character of an incoming NMEA sentence. Weak - override to pipe into tinyGPS/MicroNMEA/etc.
   *
   * @param incoming The next character of the NMEA sentence.
   */
  void processNMEA(char incoming) __attribute__((weak));                                                           // Given a NMEA character, do something with it. User can overwrite if desired to use something like tinyGPS or MicroNMEA libraries
  /**
   * @brief Watch incoming bytes for an RTCM3 frame's start/length so the byte count can be tracked. Weak - user-overridable.
   *
   * @param incoming The next raw byte received.
   * @param rtcmFrameCounter Running byte counter within the current RTCM frame, updated in place.
   * @return The sentence type detected (RTCM if a frame is in progress).
   */
  sfe_ublox_sentence_types_e processRTCMframe(uint8_t incoming, uint16_t *rtcmFrameCounter) __attribute__((weak)); // Monitor the incoming bytes for start and length bytes
  /**
   * @brief Handle one byte of an incoming RTCM3 frame. Weak, default is a no-op - override to pipe bytes to a radio/internet link.
   *
   * @param incoming The next raw byte of the RTCM3 frame.
   */
  void processRTCM(uint8_t incoming) __attribute__((weak));                                                        // Given rtcm byte, do something with it. User can overwrite if desired to pipe bytes to radio, internet, etc.
  /**
   * @brief Feed one incoming byte into the UBX frame assembler (sync/Class/ID/length/payload/checksum state machine).
   *
   * @param incoming The next raw byte received from the module.
   * @param incomingUBX The packet structure being assembled.
   * @param requestedClass If non-zero, watch for a specific Class/ID response.
   * @param requestedID Message ID (within requestedClass) to watch for.
   */
  void processUBX(uint8_t incoming, ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID);          // Given a character, file it away into the uxb packet structure
  /**
   * @brief Dispatch a fully-received, checksum-validated UBX packet by Class/ID and update internal state.
   *
   * Routes ACK/NACK, VALGET/VALSET responses, and every registered ubxMessage's storePayload(), and fires callbacks.
   *
   * @param msg The complete, validated packet.
   */
  void processUBXpacket(ubxPacket *msg);                                                                           // Once a packet has been received and validated, identify this packet's class/id and update internal flags
  /**
   * @brief Hook called for any UBX message enabled for logging with processMe=true via enableUBXlogging(). Default is a no-op.
   *
   * @param incomingUBX The complete, validated packet.
   */
  virtual void processLoggedUBX(ubxPacket *incomingUBX) {}                                                         // Process any UBX message with enableUBXlogging processMe set true

  // Send I2C/Serial/SPI commands to the module

  /**
   * @brief Compute and fill in a packet's checksumA/checksumB (UBX 8-bit Fletcher checksum) from its Class/ID/payload.
   *
   * @param msg The packet to checksum, modified in place.
   */
  void calcChecksum(ubxPacket *msg);                                                                                               // Sets the checksumA and checksumB of a given messages
  /**
   * @brief Send a UBX packet (over whichever bus is selected) and wait for its expected response.
   *
   * Computes the checksum, transmits sync+header+payload+checksum via sendI2cCommand()/sendSerialCommand()/
   * sendSpiCommand(), then waits via waitForACKResponse()/waitForNoACKResponse() as appropriate.
   *
   * @param outgoingUBX The packet to send.
   * @param maxWait Timeout in milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @param expectACKonly If true, only wait for an ACK/NACK, not a data response. Defaults to false.
   * @return SFE_UBLOX_STATUS_SUCCESS/_DATA_RECEIVED on success, or an error status.
   */
  sfe_ublox_status_e sendCommand(ubxPacket *outgoingUBX, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool expectACKonly = false); // Given a packet and payload, send everything including CRC bytes, return true if we got a response
  /**
   * @brief Transmit a packet's sync+header+payload+checksum over I2C.
   *
   * @param outgoingUBX The packet to send.
   * @return SFE_UBLOX_STATUS_SUCCESS on success, or an error status.
   */
  /**
   * @brief Transmit a packet's sync+header+payload+checksum over Serial.
   *
   * @param outgoingUBX The packet to send.
   */
  /**
   * @brief Transmit a packet's sync+header+payload+checksum over SPI.
   *
   * @param outgoingUBX The packet to send.
   * @return SFE_UBLOX_STATUS_SUCCESS on success, or an error status.
   */
  /**
   * @brief Transfer a single byte over SPI (write byteToTransfer, discard/route the byte read back per the SPI backlog logic).
   *
   * @param byteToTransfer The byte to write.
   */
  sfe_ublox_status_e sendI2cCommand(ubxPacket *outgoingUBX);
  void sendSerialCommand(ubxPacket *outgoingUBX);
  sfe_ublox_status_e sendSpiCommand(ubxPacket *outgoingUBX);
  void spiTransfer(const uint8_t byteToTransfer);

  /**
   * @brief Poll a single NMEA message using its GN Talker ID (the NMEA equivalent of sendCommand()).
   *
   * @param msgId The 3-character NMEA message identifier to poll (e.g. "GGA").
   * @param maxWait Timeout in milliseconds. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return SFE_UBLOX_STATUS_SUCCESS/_DATA_RECEIVED on success, or an error status.
   */
  sfe_ublox_status_e pollNMEA(const char *msgId, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // The equivalent of sendCommand but for NMEA. Poll a single NMEA message using the GN Talker ID

  /**
   * @brief Print a packet's Class/ID/length (and optionally payload) to the debug stream, if debugging is enabled.
   *
   * @param packet The packet to print.
   * @param alwaysPrintPayload If true, print the payload bytes even if not marked important. Defaults to false.
   */
  void printPacket(ubxPacket *packet, bool alwaysPrintPayload = false); // Useful for debugging

  // After sending a message to the module, wait for the expected response (data+ACK or just data)

  /**
   * @brief Poll the module until the requested Class/ID response and its ACK/NACK have both arrived (or timeout).
   *
   * @param outgoingUBX The packet just sent, whose response is expected.
   * @param requestedClass The Class of the expected data response.
   * @param requestedID The ID (within requestedClass) of the expected data response.
   * @param maxTime Timeout in milliseconds. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return SFE_UBLOX_STATUS_SUCCESS/_DATA_RECEIVED on success, or an error/timeout status.
   */
  sfe_ublox_status_e waitForACKResponse(ubxPacket *outgoingUBX, uint8_t requestedClass, uint8_t requestedID, uint16_t maxTime = kUBLOXGNSSDefaultMaxWait);   // Poll the module until a config packet and an ACK is received, or just an ACK
  /**
   * @brief Poll the module until the requested Class/ID response arrives (or timeout), for a message with no ACK/NACK.
   *
   * @param outgoingUBX The packet just sent, whose response is expected.
   * @param requestedClass The Class of the expected data response.
   * @param requestedID The ID (within requestedClass) of the expected data response.
   * @param maxTime Timeout in milliseconds. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return SFE_UBLOX_STATUS_SUCCESS/_DATA_RECEIVED on success, or an error/timeout status.
   */
  sfe_ublox_status_e waitForNoACKResponse(ubxPacket *outgoingUBX, uint8_t requestedClass, uint8_t requestedID, uint16_t maxTime = kUBLOXGNSSDefaultMaxWait); // Poll the module until a config packet is received

  // Check if any callbacks need to be called
  /**
   * @brief Fire any registered UBX/NMEA/RTCM callbacks for which fresh data is waiting.
   *
   * Reentrancy-guarded (checkCallbacksReentrant) so calling it from inside a callback is safe.
   */
  void checkCallbacks(void);

  // Push (e.g.) RTCM or Assist Now data directly to the module
  // Warning: this function does not check that the data is valid. It is the user's responsibility to ensure the data is valid before pushing.
  //
  // For SPI: callProcessBuffer defaults to true and forces pushRawData to call processSpiBuffer in between push transactions.
  // This is to try and prevent incoming data being 'lost' during large (bi-directional) pushes.
  // If you are only pushing limited amounts of data and/or will be calling checkUblox manually, it might be advantageous to set callProcessBuffer to false.
  //
  // Likewise for Serial: callProcessBuffer defaults to true and forces pushRawData to call checkUbloxSerial in between pushing data.
  // This is to try and prevent incoming data being 'lost' (overflowing the serial RX buffer) during a large push.
  // If you are only pushing limited amounts of data and/or will be calling checkUblox manually, it might be advantageous to set callProcessBuffer to false.
  /**
   * @brief Push arbitrary data (e.g. RTCM or AssistNow) directly to the module, unvalidated.
   *
   * @param dataBytes Pointer to the bytes to push.
   * @param numDataBytes Number of bytes to push.
   * @param callProcessBuffer For SPI/Serial, whether to interleave processSpiBuffer()/checkUbloxSerial() calls
   * between chunks so incoming data isn't lost during a large push. Defaults to true.
   * @return true if all the data was pushed successfully.
   */
  bool pushRawData(uint8_t *dataBytes, size_t numDataBytes, bool callProcessBuffer = true);
  // RTCM parsing - used inside pushRawData
protected:
  /**
   * @brief Look for and parse an RTCM 1005 message within data about to be pushed by pushRawData().
   *
   * @param dataBytes Pointer to the data being pushed.
   * @param numDataBytes Number of bytes in 'dataBytes'.
   */
  /**
   * @brief Look for and parse an RTCM 1006 message within data about to be pushed by pushRawData().
   *
   * @param dataBytes Pointer to the data being pushed.
   * @param numDataBytes Number of bytes in 'dataBytes'.
   */
  void parseRTCM1005(uint8_t *dataBytes, size_t numDataBytes);
  void parseRTCM1006(uint8_t *dataBytes, size_t numDataBytes);

public:

// Push MGA AssistNow data to the module.
// Check for UBX-MGA-ACK responses if required (if mgaAck is YES or ENQUIRE).
// Wait for maxWait millis after sending each packet (if mgaAck is NO).
// Return how many bytes were pushed successfully.
// If skipTime is true, any UBX-MGA-INI-TIME_UTC or UBX-MGA-INI-TIME_GNSS packets found in the data will be skipped,
// allowing the user to override with their own time data with setUTCTimeAssistance.
// offset allows a sub-set of the data to be sent - starting from offset.
#define defaultMGAdelay 7 // Default to waiting for 7ms between each MGA message
  /**
   * @brief Push AssistNow (MGA) data to the module.
   *
   * Sends each UBX-MGA-* message contained in dataBytes to the module in turn. If mgaAck is YES or
   * ENQUIRE, waits for and checks the UBX-MGA-ACK response after each message; if mgaAck is NO, simply
   * waits maxWait milliseconds between messages instead.
   *
   * @param dataBytes The AssistNow data, as a String.
   * @param numDataBytes Number of bytes in dataBytes to push.
   * @param mgaAck Whether/how to wait for a UBX-MGA-ACK after each message. Defaults to not checking.
   * @param maxWait Milliseconds to wait between messages when mgaAck is NO. Defaults to defaultMGAdelay.
   * @return The number of bytes successfully pushed.
   */
  size_t pushAssistNowData(const sfe_string_t &dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck = SFE_UBLOX_MGA_ASSIST_ACK_NO, uint16_t maxWait = defaultMGAdelay);
  /** @brief As pushAssistNowData(const String&, ...), but takes the AssistNow data as a raw byte buffer instead of a String. */
  size_t pushAssistNowData(const uint8_t *dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck = SFE_UBLOX_MGA_ASSIST_ACK_NO, uint16_t maxWait = defaultMGAdelay);
  /**
   * @brief As pushAssistNowData(const String&, ...), but can skip the time messages within the data.
   *
   * @param skipTime If true, any UBX-MGA-INI-TIME_UTC or UBX-MGA-INI-TIME_GNSS packets found in the data
   *                 are skipped, so the caller can supply its own time via setUTCTimeAssistance() instead.
   * @param dataBytes The AssistNow data, as a String.
   * @param numDataBytes Number of bytes in dataBytes to push.
   * @param mgaAck Whether/how to wait for a UBX-MGA-ACK after each message. Defaults to not checking.
   * @param maxWait Milliseconds to wait between messages when mgaAck is NO. Defaults to defaultMGAdelay.
   * @return The number of bytes successfully pushed.
   */
  size_t pushAssistNowData(bool skipTime, const sfe_string_t &dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck = SFE_UBLOX_MGA_ASSIST_ACK_NO, uint16_t maxWait = defaultMGAdelay);
  /** @brief As pushAssistNowData(bool, const String&, ...), but takes the AssistNow data as a raw byte buffer instead of a String. */
  size_t pushAssistNowData(bool skipTime, const uint8_t *dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck = SFE_UBLOX_MGA_ASSIST_ACK_NO, uint16_t maxWait = defaultMGAdelay);
  /**
   * @brief As pushAssistNowData(bool, const String&, ...), but starts partway through the data.
   *
   * @param offset Byte offset within dataBytes at which to start pushing.
   * @param skipTime If true, skips any UBX-MGA-INI-TIME_UTC/GNSS packets found in the data.
   * @param dataBytes The AssistNow data, as a String.
   * @param numDataBytes Number of bytes in dataBytes (from the start of the buffer, not from offset).
   * @param mgaAck Whether/how to wait for a UBX-MGA-ACK after each message. Defaults to not checking.
   * @param maxWait Milliseconds to wait between messages when mgaAck is NO. Defaults to defaultMGAdelay.
   * @return The number of bytes successfully pushed.
   */
  size_t pushAssistNowData(size_t offset, bool skipTime, const sfe_string_t &dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck = SFE_UBLOX_MGA_ASSIST_ACK_NO, uint16_t maxWait = defaultMGAdelay);
  /** @brief As pushAssistNowData(size_t, bool, const String&, ...), but takes the AssistNow data as a raw byte buffer instead of a String. */
  size_t pushAssistNowData(size_t offset, bool skipTime, const uint8_t *dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck = SFE_UBLOX_MGA_ASSIST_ACK_NO, uint16_t maxWait = defaultMGAdelay);

// Provide initial time assistance
#define defaultMGAINITIMEtAccS 2  // Default to setting the seconds time accuracy to 2 seconds
#define defaultMGAINITIMEtAccNs 0 // Default to setting the nanoseconds time accuracy to zero
#define defaultMGAINITIMEsource 0 // Set default source to none, i.e. on receipt of message (will be inaccurate!)
  /**
   * @brief Send UBX-MGA-INI-TIME_UTC to give the module an initial UTC time estimate for AssistNow.
   *
   * Typically used together with pushAssistNowData(..., skipTime=true, ...) so this caller-supplied time
   * is used instead of any time messages embedded in the AssistNow Offline/Online data.
   *
   * @param year Four-digit year.
   * @param month Month (1-12).
   * @param day Day of month (1-31).
   * @param hour Hour (0-23).
   * @param minute Minute (0-59).
   * @param second Second (0-59).
   * @param nanos Nanosecond fraction of the second. Defaults to 0.
   * @param tAccS Seconds part of the time accuracy estimate. Defaults to defaultMGAINITIMEtAccS.
   * @param tAccNs Nanoseconds part of the time accuracy estimate. Defaults to defaultMGAINITIMEtAccNs.
   * @param source Time source identifier (0 = none / on receipt of message). Defaults to defaultMGAINITIMEsource.
   * @param mgaAck Whether/how to wait for a UBX-MGA-ACK response. Defaults to not checking.
   * @param maxWait Milliseconds to wait for the response when mgaAck is not NO. Defaults to defaultMGAdelay.
   * @return True if the message was sent (and acknowledged, if requested) successfully.
   */
  bool setUTCTimeAssistance(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, uint32_t nanos = 0,
                            uint16_t tAccS = defaultMGAINITIMEtAccS, uint32_t tAccNs = defaultMGAINITIMEtAccNs, uint8_t source = defaultMGAINITIMEsource,
                            sfe_ublox_mga_assist_ack_e mgaAck = SFE_UBLOX_MGA_ASSIST_ACK_NO, uint16_t maxWait = defaultMGAdelay);

  // Provide initial position assistance
  // The units for ecefX/Y/Z and posAcc (stddev) are cm.
  /**
   * @brief Send UBX-MGA-INI-POS_XYZ to give the module an initial ECEF position estimate for AssistNow.
   *
   * @param ecefX ECEF X coordinate, cm.
   * @param ecefY ECEF Y coordinate, cm.
   * @param ecefZ ECEF Z coordinate, cm.
   * @param posAcc Position accuracy estimate (stddev), cm.
   * @param mgaAck Whether/how to wait for a UBX-MGA-ACK response. Defaults to not checking.
   * @param maxWait Milliseconds to wait for the response when mgaAck is not NO. Defaults to defaultMGAdelay.
   * @return True if the message was sent (and acknowledged, if requested) successfully.
   */
  bool setPositionAssistanceXYZ(int32_t ecefX, int32_t ecefY, int32_t ecefZ, uint32_t posAcc, sfe_ublox_mga_assist_ack_e mgaAck = SFE_UBLOX_MGA_ASSIST_ACK_NO, uint16_t maxWait = defaultMGAdelay);
  // The units for lat and lon are degrees * 1e-7 (WGS84)
  // The units for alt (WGS84) and posAcc (stddev) are cm.
  /**
   * @brief Send UBX-MGA-INI-POS_LLH to give the module an initial geodetic position estimate for AssistNow.
   *
   * @param lat Latitude, degrees * 1e-7 (WGS84).
   * @param lon Longitude, degrees * 1e-7 (WGS84).
   * @param alt Altitude above WGS84 ellipsoid, cm.
   * @param posAcc Position accuracy estimate (stddev), cm.
   * @param mgaAck Whether/how to wait for a UBX-MGA-ACK response. Defaults to not checking.
   * @param maxWait Milliseconds to wait for the response when mgaAck is not NO. Defaults to defaultMGAdelay.
   * @return True if the message was sent (and acknowledged, if requested) successfully.
   */
  bool setPositionAssistanceLLH(int32_t lat, int32_t lon, int32_t alt, uint32_t posAcc, sfe_ublox_mga_assist_ack_e mgaAck = SFE_UBLOX_MGA_ASSIST_ACK_NO, uint16_t maxWait = defaultMGAdelay);

  // Find the start of the AssistNow Offline (UBX_MGA_ANO) data for the chosen day
  // The daysIntoFture parameter makes it easy to get the data for (e.g.) tomorrow based on today's date
  // Returns numDataBytes if unsuccessful
  // TO DO: enhance this so it will find the nearest data for the chosen day - instead of an exact match
  /**
   * @brief Find the offset of the AssistNow Offline (UBX-MGA-ANO) entry for a given date within an AssistNow
   *        Offline data set.
   *
   * @param dataBytes The AssistNow Offline data, as a String.
   * @param numDataBytes Number of bytes in dataBytes.
   * @param year Four-digit year to search for.
   * @param month Month (1-12) to search for.
   * @param day Day of month (1-31) to search for.
   * @param daysIntoFuture Added to the requested date, so e.g. passing 1 finds tomorrow's data relative to
   *                       the year/month/day given. Defaults to 0.
   * @return The byte offset of the first matching UBX-MGA-ANO message, or numDataBytes if no exact match
   *         was found.
   */
  size_t findMGAANOForDate(const sfe_string_t &dataBytes, size_t numDataBytes, uint16_t year, uint8_t month, uint8_t day, uint8_t daysIntoFuture = 0);
  /** @brief As findMGAANOForDate(const String&, ...), but takes the AssistNow Offline data as a raw byte buffer instead of a String. */
  size_t findMGAANOForDate(const uint8_t *dataBytes, size_t numDataBytes, uint16_t year, uint8_t month, uint8_t day, uint8_t daysIntoFuture = 0);

// Read the whole navigation data base. The receiver will send all available data from its internal database.
// Data is written to dataBytes. Set maxNumDataBytes to the (maximum) size of dataBytes.
// If the database exceeds maxNumDataBytes, the excess bytes will be lost.
// The function returns the number of database bytes written to dataBytes.
// The return value will be equal to maxNumDataBytes if excess data was received.
// The function will timeout after maxWait milliseconds - in case the final UBX-MGA-ACK was missed.
#define defaultNavDBDMaxWait 3100
  /**
   * @brief Read the module's entire internal navigation database (UBX-MGA-DBD) into a buffer.
   *
   * @param dataBytes Destination buffer for the database bytes.
   * @param maxNumDataBytes Size of dataBytes; any database bytes beyond this are discarded.
   * @param maxWait Milliseconds to wait for the transfer to finish, in case the final UBX-MGA-ACK is missed.
   *                Defaults to defaultNavDBDMaxWait.
   * @return The number of database bytes written to dataBytes (equal to maxNumDataBytes if the database
   *         was larger than the buffer).
   */
  size_t readNavigationDatabase(uint8_t *dataBytes, size_t maxNumDataBytes, uint16_t maxWait = defaultNavDBDMaxWait);

  // Support for data logging
  /**
   * @brief Set the size (in bytes) of the internal file-logging buffer. Must be called before .begin().
   *
   * @param bufferSize The desired buffer size, in bytes.
   */
  void setFileBufferSize(uint16_t bufferSize);                             // Set the size of the file buffer. This must be called _before_ .begin.
  /**
   * @brief Get the size (in bytes) of the internal file-logging buffer.
   *
   * @return The buffer size, in bytes.
   */
  uint16_t getFileBufferSize(void);                                        // Return the size of the file buffer
  /**
   * @brief Copy bytes out of the file-logging buffer.
   *
   * @param destination Where to copy the bytes to. It is the caller's responsibility to ensure this is
   *                    large enough to hold numBytes.
   * @param numBytes Number of bytes to copy out of the buffer.
   * @return The number of bytes actually copied.
   */
  uint16_t extractFileBufferData(uint8_t *destination, uint16_t numBytes); // Extract numBytes of data from the file buffer. Copy it to destination. It is the user's responsibility to ensure destination is large enough.
  /**
   * @brief Get how many bytes are currently waiting to be read from the file-logging buffer.
   *
   * @return The number of bytes available.
   */
  uint16_t fileBufferAvailable(void);                                      // Returns the number of bytes available in file buffer which are waiting to be read
  /**
   * @brief Get the high-water mark of bytes ever waiting in the file-logging buffer at once.
   *
   * Handy for checking that the configured buffer size (see setFileBufferSize()) is large enough to
   * handle the incoming data rate.
   *
   * @return The largest number of bytes the file buffer has held at once since begin() or the last
   *         clearMaxFileBufferAvail() call.
   */
  uint16_t getMaxFileBufferAvail(void);                                    // Returns the maximum number of bytes which the file buffer has contained. Handy for checking the buffer is large enough to handle all the incoming data.
  /** @brief Empty the file-logging buffer, discarding all of its contents. */
  void clearFileBuffer(void);                                              // Empty the file buffer - discard all contents
  /** @brief Reset the high-water mark tracked by getMaxFileBufferAvail() back to zero. */
  void clearMaxFileBufferAvail(void);                                      // Reset fileBufferMaxAvail

  // Specific commands

  // Port configurations
  /**
   * @brief Change the I2C address the module responds on.
   *
   * @param deviceAddress The new 7-bit I2C address.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxTime Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the address was changed successfully.
   */
  bool setI2CAddress(uint8_t deviceAddress, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxTime = kUBLOXGNSSDefaultMaxWait);                                // Changes the I2C address of the u-blox module
  /**
   * @brief Change the baud rate of one of the module's UART ports.
   *
   * @param baudrate The new baud rate, in bits per second.
   * @param uartPort Which UART port to change: COM_PORT_UART1 or COM_PORT_UART2. Defaults to UART1.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxTime Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the baud rate was changed successfully.
   */
  bool setSerialRate(uint32_t baudrate, uint8_t uartPort = COM_PORT_UART1, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxTime = kUBLOXGNSSDefaultMaxWait); // Changes the serial baud rate of the u-blox module, uartPort should be COM_PORT_UART1/2

  /**
   * @brief Configure which protocol(s) the I2C port outputs.
   *
   * @param comSettings Bitfield of COM_TYPE_UBX / COM_TYPE_NMEA / COM_TYPE_RTCM3 / COM_TYPE_SPARTN.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the port was configured successfully.
   */
  bool setI2COutput(uint8_t comSettings, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);   // Configure I2C port to output UBX, NMEA, RTCM3, SPARTN or a combination thereof
  /** @brief As setI2COutput(), but configures the UART1 port's output protocol(s) instead. */
  bool setUART1Output(uint8_t comSettings, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Configure UART1 port to output UBX, NMEA, RTCM3, SPARTN or a combination thereof
  /** @brief As setI2COutput(), but configures the UART2 port's output protocol(s) instead. */
  bool setUART2Output(uint8_t comSettings, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Configure UART2 port to output UBX, NMEA, RTCM3, SPARTN or a combination thereof
  /** @brief As setI2COutput(), but configures the USB port's output protocol(s) instead. */
  bool setUSBOutput(uint8_t comSettings, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);   // Configure USB port to output UBX, NMEA, RTCM3, SPARTN or a combination thereof
  /** @brief As setI2COutput(), but configures the SPI port's output protocol(s) instead. */
  bool setSPIOutput(uint8_t comSettings, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);   // Configure SPI port to output UBX, NMEA, RTCM3, SPARTN or a combination thereof

  /**
   * @brief Configure which protocol(s) the I2C port accepts as input.
   *
   * @param comSettings Bitfield of COM_TYPE_UBX / COM_TYPE_NMEA / COM_TYPE_RTCM3 / COM_TYPE_SPARTN.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the port was configured successfully.
   */
  bool setI2CInput(uint8_t comSettings, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);   // Configure I2C port to output UBX, NMEA, RTCM3, SPARTN or a combination thereof
  /** @brief As setI2CInput(), but configures the UART1 port's accepted input protocol(s) instead. */
  bool setUART1Input(uint8_t comSettings, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Configure UART1 port to output UBX, NMEA, RTCM3, SPARTN or a combination thereof
  /** @brief As setI2CInput(), but configures the UART2 port's accepted input protocol(s) instead. */
  bool setUART2Input(uint8_t comSettings, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Configure UART2 port to output UBX, NMEA, RTCM3, SPARTN or a combination thereof
  /** @brief As setI2CInput(), but configures the USB port's accepted input protocol(s) instead. */
  bool setUSBInput(uint8_t comSettings, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);   // Configure USB port to output UBX, NMEA, RTCM3, SPARTN or a combination thereof
  /** @brief As setI2CInput(), but configures the SPI port's accepted input protocol(s) instead. */
  bool setSPIInput(uint8_t comSettings, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);   // Configure SPI port to output UBX, NMEA, RTCM3, SPARTN or a combination thereof

  /** @brief Direct only NMEA output characters to outputPort, in addition to any port set by setOutputPort(). */
  void setNMEAOutputPort(sfe_print_t &outputPort); // Sets the internal variable for the port to direct only NMEA characters to
  /** @brief Direct only RTCM output characters to outputPort, in addition to any port set by setOutputPort(). */
  void setRTCMOutputPort(sfe_print_t &outputPort); // Sets the internal variable for the port to direct only RTCM characters to
  /** @brief Direct only UBX output characters to outputPort, in addition to any port set by setOutputPort(). */
  void setUBXOutputPort(sfe_print_t &outputPort);  // Sets the internal variable for the port to direct only UBX characters to
  /** @brief Direct all output characters (UBX, NMEA and RTCM) to outputPort. */
  void setOutputPort(sfe_print_t &outputPort);     // Sets the internal variable for the port to direct ALL characters to

  // Reset to defaults

  /** @brief Send the factory-reset sequence: load the default configuration, then perform a hardReset(). */
  void factoryReset();                                              // Send factory reset sequence (i.e. load "default" configuration and perform hardReset)
  /**
   * @brief Reset the module's configuration to factory defaults (without a hard reset).
   *
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the command was acknowledged successfully.
   */
  bool factoryDefault(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Reset module to factory defaults
  /** @brief Perform a hardware reset leading to a cold start (zero info start-up). */
  void hardReset();                                                 // Perform a reset leading to a cold start (zero info start-up)
  /** @brief Controlled software reset of the GNSS tasks only; the rest of the receiver system and its stored configuration are left untouched. */
  void softwareResetGNSSOnly();                                     // Controlled Software Reset (GNSS only) only restarts the GNSS tasks, without reinitializing the full system or reloading any stored configuration.
  /**
   * @brief Controlled software start/stop of the GNSS tasks only.
   *
   * @param enable True to start the GNSS tasks, false to stop them.
   */
  void softwareEnableGNSS(bool enable);                             // Controlled Software Start / Stop (GNSS only)
  /**
   * @brief Send a raw UBX-CFG-RST message. Shared implementation used by factoryReset()/hardReset()/etc.
   *
   * @param data Pointer to the CFG-RST payload bytes.
   * @param len Number of payload bytes in data.
   */
  void cfgRst(uint8_t *data, uint8_t len);                          // Common method for CFG RST

  // Save configuration to BBR / Flash

  /**
   * @brief Save the current configuration to flash and BBR (battery-backed RAM).
   *
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the configuration was saved successfully.
   */
  bool saveConfiguration(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                        // Save current configuration to flash and BBR (battery backed RAM)
  /**
   * @brief Save only the selected configuration sub-sections to flash and BBR.
   *
   * @param configMask Bitmask of the VAL_CFG_SUBSEC_* sub-sections to save.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the configuration was saved successfully.
   */
  bool saveConfigSelective(uint32_t configMask, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Save the selected configuration sub-sections to flash and BBR (battery backed RAM)
  /**
   * @brief Send a raw UBX-CFG-CFG message. Shared implementation used by saveConfiguration()/saveConfigSelective()/factoryDefault().
   *
   * @param data Pointer to the CFG-CFG payload bytes.
   * @param len Number of payload bytes in data.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the command was acknowledged successfully.
   */
  bool cfgCfg(uint8_t *data, uint8_t len, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);       // Common method for CFG CFG

  // Functions used for RTK and base station setup
  /**
   * @brief Configure UBX-CFG-TMODE3 survey-in mode directly (16-bit observation time).
   *
   * @param mode SVIN_MODE_DISABLED or SVIN_MODE_ENABLE.
   * @param observationTime Minimum survey-in duration, seconds.
   * @param requiredAccuracy Required 3D position accuracy (stddev), metres.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the command was acknowledged successfully.
   */
  bool setSurveyMode(uint8_t mode, uint16_t observationTime, float requiredAccuracy, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);     // Control survey in mode
  /** @brief As setSurveyMode(), but observationTime is a full 32-bit value for longer survey-in durations. */
  bool setSurveyModeFull(uint8_t mode, uint32_t observationTime, float requiredAccuracy, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Control survey in mode
  /**
   * @brief Begin Survey-In (base station self-survey) on a NEO-M8P/ZED-F9x module.
   *
   * @param observationTime Minimum survey-in duration, seconds.
   * @param requiredAccuracy Required 3D position accuracy (stddev), metres.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if Survey-In was started successfully.
   */
  bool enableSurveyMode(uint16_t observationTime, float requiredAccuracy, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                // Begin Survey-In for NEO-M8P / ZED-F9x
  /** @brief As enableSurveyMode(), but observationTime is a full 32-bit value for longer survey-in durations. */
  bool enableSurveyModeFull(uint32_t observationTime, float requiredAccuracy, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);            // Begin Survey-In for NEO-M8P / ZED-F9x
  /**
   * @brief Stop Survey-In mode.
   *
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the command was acknowledged successfully.
   */
  bool disableSurveyMode(uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                                                                 // Stop Survey-In mode
  // Given coordinates, put receiver into static position. Set latlong to true to pass in lat/long values instead of ecef.
  // For ECEF the units are: cm, 0.1mm, cm, 0.1mm, cm, 0.1mm
  // For Lat/Lon/Alt the units are: degrees^-7, degrees^-9, degrees^-7, degrees^-9, cm, 0.1mm
  /**
   * @brief Put the receiver into a fixed (survey-in complete) static position, with high-precision fields.
   *
   * @param ecefXOrLat ECEF X (cm) if latLong is false, otherwise latitude (degrees * 1e-7).
   * @param ecefXOrLatHP High-precision extension of ecefXOrLat, 0.1 mm or degrees * 1e-9.
   * @param ecefYOrLon ECEF Y (cm) if latLong is false, otherwise longitude (degrees * 1e-7).
   * @param ecefYOrLonHP High-precision extension of ecefYOrLon, 0.1 mm or degrees * 1e-9.
   * @param ecefZOrAlt ECEF Z (cm) if latLong is false, otherwise altitude (cm).
   * @param ecefZOrAltHP High-precision extension of ecefZOrAlt, 0.1 mm.
   * @param latLong True to interpret the coordinates as lat/lon/alt instead of ECEF X/Y/Z.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the command was acknowledged successfully.
   */
  bool setStaticPosition(int32_t ecefXOrLat, int8_t ecefXOrLatHP, int32_t ecefYOrLon, int8_t ecefYOrLonHP, int32_t ecefZOrAlt, int8_t ecefZOrAltHP, bool latLong, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /** @brief As setStaticPosition(...HP overload...), but without the high-precision extension fields. */
  bool setStaticPosition(int32_t ecefXOrLat, int32_t ecefYOrLon, int32_t ecefZOrAlt, bool latLong, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /**
   * @brief Set the DGNSS differential correction mode (UBX-CFG-DGNSS).
   *
   * @param dgnssMode The differential mode. Defaults to SFE_UBLOX_DGNSS_MODE_FIXED.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the command was acknowledged successfully.
   */
  bool setDGNSSConfiguration(sfe_ublox_dgnss_mode_e dgnssMode = SFE_UBLOX_DGNSS_MODE_FIXED, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Set the DGNSS differential mode

  // Read the module's protocol version
  // For safety, call getProtocolVersion etc. inside an if(getModuleInfo())
  /**
   * @brief Get the major (integer) part of the module's protocol version. Call inside if(getModuleInfo()) for safety.
   *
   * @param maxWait Milliseconds to wait. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return The PROTVER major version number (XX in XX.00).
   */
  uint8_t getProtocolVersionHigh(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Returns the PROTVER XX.00 from UBX-MON-VER register
  /** @brief As getProtocolVersionHigh(), but returns the minor (fractional) part of the protocol version. */
  uint8_t getProtocolVersionLow(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);  // Returns the PROTVER 00.XX from UBX-MON-VER register
  /**
   * @brief Get the major (integer) part of the module's firmware version. Call inside if(getModuleInfo()) for safety.
   *
   * @param maxWait Milliseconds to wait. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return The FWVER major version number (XX in XX.00).
   */
  uint8_t getFirmwareVersionHigh(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Returns the FWVER XX.00 from UBX-MON-VER register
  /** @brief As getFirmwareVersionHigh(), but returns the minor (fractional) part of the firmware version. */
  uint8_t getFirmwareVersionLow(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);  // Returns the FWVER 00.XX from UBX-MON-VER register
  /**
   * @brief Get the module's firmware type string (e.g. "SPG", "HPG", "ADR"). Call inside if(getModuleInfo()) for safety.
   *
   * @param maxWait Milliseconds to wait. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return Pointer to the null-terminated firmware type string.
   */
  const char *getFirmwareType(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);    // Returns the firmware type (SPG, HPG, ADR, etc.) from UBX-MON-VER register
  /**
   * @brief Get the module's name string (e.g. "ZED-F9P", "ZED-F9R"). Call inside if(getModuleInfo()) for safety.
   *
   * @param maxWait Milliseconds to wait. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return Pointer to the null-terminated module name string.
   */
  const char *getModuleName(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);      // Returns the module name (ZED-F9P, ZED-F9R, etc.) from UBX-MON-VER register
  /**
   * @brief Deprecated. Use getModuleInfo() instead.
   *
   * @param maxWait Milliseconds to wait. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the (now-superseded) protocol-version query succeeded.
   */
  bool getProtocolVersion(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);        // Deprecated. Use getModuleInfo.
  /**
   * @brief Query UBX-MON-VER and populate the protocol/firmware/module-name fields it exposes.
   *
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if UBX-MON-VER was read and parsed successfully.
   */
  bool getModuleInfo(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);             // Queries module, extracts info. Returns true if MON-VER was read successfully
protected:
  /**
   * @brief Send the UBX-MON-VER poll and parse its response into moduleSWVersion. Shared implementation
   *        used by getModuleInfo() and the getProtocolVersion/getFirmwareVersion/getFirmwareType/getModuleName accessors.
   *
   * @param maxWait Milliseconds to wait for the response.
   * @return True if UBX-MON-VER was read and parsed successfully.
   */
  bool prepareModuleInfo(uint16_t maxWait);

public:
  moduleSWVersion_t *moduleSWVersion = nullptr; // Pointer to struct. RAM will be allocated for this if/when necessary

  // Support for geofences
  /**
   * @brief Add a new geofence (UBX-CFG-GEOFENCE), up to a maximum of 4.
   *
   * @param latitude Geofence centre latitude, degrees * 1e-7.
   * @param longitude Geofence centre longitude, degrees * 1e-7.
   * @param radius Geofence radius, metres.
   * @param confidence Confidence level for the transition detection (0-4). Defaults to 0.
   * @param pinPolarity Polarity of the pin used to indicate geofence state, if pin is used. Defaults to 0.
   * @param pin Pin number to use to indicate combined geofence state, or 0 for none. Defaults to 0.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the geofence was added successfully.
   */
  bool addGeofence(int32_t latitude, int32_t longitude, uint32_t radius, uint8_t confidence = 0, bool pinPolarity = 0, uint8_t pin = 0, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Add a new geofence
  /**
   * @brief Remove all configured geofences.
   *
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the geofences were cleared successfully.
   */
  bool clearGeofences(uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                                                                                                                   // Clears all geofences
  /**
   * @brief Query the combined state of all configured geofences (UBX-NAV-GEOFENCE).
   *
   * @param currentGeofenceState Filled in with the combined geofence status on success.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the geofence state was read successfully.
   */
  bool getGeofenceState(geofenceState &currentGeofenceState, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                                                                                                               // Returns the combined geofence state
  // Storage for the geofence parameters. RAM is allocated for this if/when required.
  geofenceParams_t *currentGeofenceParams = nullptr; // Pointer to struct. RAM will be allocated for this if/when necessary

  // Power save / off
  /**
   * @brief Put the module into backup mode (UBX-RXM-PMREQ) for a fixed duration.
   *
   * @param durationInMs How long to remain powered off, milliseconds.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the power-off command was sent successfully.
   */
  bool powerOff(uint32_t durationInMs, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /**
   * @brief As powerOff(), but the module can also be woken by one of the given interrupt sources.
   *
   * @param durationInMs How long to remain powered off, milliseconds.
   * @param wakeupSources Bitfield of VAL_RXM_PMREQ_WAKEUPSOURCE_* sources that can wake the module early.
   *                      Defaults to VAL_RXM_PMREQ_WAKEUPSOURCE_EXTINT0.
   * @param forceWhileUsb If true, powers off even while USB is connected. Defaults to true.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the power-off command was sent successfully.
   */
  bool powerOffWithInterrupt(uint32_t durationInMs, uint32_t wakeupSources = VAL_RXM_PMREQ_WAKEUPSOURCE_EXTINT0, bool forceWhileUsb = true, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  // Change the dynamic platform model using UBX-CFG-NAV5
  /**
   * @brief Set the dynamic platform model (UBX-CFG-NAV5) used by the module's navigation engine.
   *
   * @param newDynamicModel The dynamic model to use. Defaults to DYN_MODEL_PORTABLE.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the dynamic model was set successfully.
   */
  bool setDynamicModel(dynModel newDynamicModel = DYN_MODEL_PORTABLE, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /**
   * @brief Get the currently configured dynamic platform model.
   *
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return The current dynModel value, or 255 if the query failed.
   */
  uint8_t getDynamicModel(uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Get the dynamic model - returns 255 if the sendCommand fails

  // Reset / enable / configure the odometer
  /**
   * @brief Reset the module's odometer (UBX-NAV-RESETODO) back to zero.
   *
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the odometer was reset successfully.
   */
  bool resetOdometer(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                                                                                                                                                          // Reset the odometer
  /**
   * @brief Enable or disable the odometer/low-speed-course-over-ground filter (UBX-CFG-ODO).
   *
   * @param enable True to enable, false to disable. Defaults to true.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the setting was written successfully.
   */
  bool enableOdometer(bool enable = true, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                                                                                                  // Enable / disable the odometer
  /**
   * @brief Read the odometer/COG-filter configuration (UBX-CFG-ODO).
   *
   * @param flags Filled in with the ODO/COG enable flags.
   * @param odoCfg Filled in with the odometer profile configuration.
   * @param cogMaxSpeed Filled in with the COG filter's maximum speed threshold.
   * @param cogMaxPosAcc Filled in with the COG filter's maximum position accuracy threshold.
   * @param velLpGain Filled in with the velocity low-pass filter gain.
   * @param cogLpGain Filled in with the COG low-pass filter gain.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the configuration was read successfully.
   */
  bool getOdometerConfig(uint8_t *flags, uint8_t *odoCfg, uint8_t *cogMaxSpeed, uint8_t *cogMaxPosAcc, uint8_t *velLpGain, uint8_t *cogLpGain, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Read the odometer configuration
  /** @brief As getOdometerConfig(), but writes the given odometer/COG-filter configuration instead of reading it. */
  bool setOdometerConfig(uint8_t flags, uint8_t odoCfg, uint8_t cogMaxSpeed, uint8_t cogMaxPosAcc, uint8_t velLpGain, uint8_t cogLpGain, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);   // Configure the odometer

  // Enable/Disable individual GNSS systems using UBX-CFG-GNSS
  // Note: you must leave at least one major GNSS enabled! If in doubt, enable GPS before disabling the others
  /**
   * @brief Enable or disable one GNSS constellation (UBX-CFG-GNSS). At least one major GNSS must remain
   *        enabled at all times.
   *
   * @param enable True to enable the constellation, false to disable it.
   * @param id Which GNSS constellation to change.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the setting was written successfully.
   */
  bool enableGNSS(bool enable, sfe_ublox_gnss_ids_e id, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /**
   * @brief Check whether one GNSS constellation is currently enabled.
   *
   * @param id Which GNSS constellation to query.
   * @param enabled Filled in with true/false on success.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the query itself succeeded (see enabled for the actual state).
   */
  bool isGNSSenabled(sfe_ublox_gnss_ids_e id, bool *enabled, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /**
   * @brief Unsafe overload of isGNSSenabled(). Returns the enabled state directly with no way to
   *        distinguish "disabled" from "query failed".
   *
   * @param id Which GNSS constellation to query.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the constellation is enabled (also returned, ambiguously, if the query failed).
   */
  bool isGNSSenabled(sfe_ublox_gnss_ids_e id, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Unsafe overload
  /**
   * @brief Look up the UBX-CFG-VALSET/VALGET key that enables/disables a given GNSS constellation.
   *
   * @param id Which GNSS constellation to look up.
   * @return The corresponding configuration key ID.
   */
  uint32_t getEnableGNSSConfigKey(sfe_ublox_gnss_ids_e id);

  // Reset ESF automatic IMU-mount alignment
  /**
   * @brief Reset the ESF automatic IMU-mount alignment (UBX-ESF-RESETALG).
   *
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the alignment was reset successfully.
   */
  bool resetIMUalignment(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  // Enable/disable esfAutoAlignment
  /**
   * @brief Check whether ESF automatic IMU-mount alignment is enabled.
   *
   * @param enabled Filled in with true/false on success.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the query itself succeeded (see enabled for the actual state).
   */
  bool getESFAutoAlignment(bool *enabled, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /**
   * @brief Unsafe overload of getESFAutoAlignment(). Returns the enabled state directly with no way to
   *        distinguish "disabled" from "query failed".
   *
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if auto-alignment is enabled (also returned, ambiguously, if the query failed).
   */
  bool getESFAutoAlignment(uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Unsafe overload
  /**
   * @brief Enable or disable ESF automatic IMU-mount alignment.
   *
   * @param enable True to enable, false to disable.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the setting was written successfully.
   */
  bool setESFAutoAlignment(bool enable, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  // RF Information (including jamming) - ZED-F9 only. getRFinformation() has been replaced by
  // getMONRF() - see AGENTS.md "Adding the variable-length UBX messages" - declared alongside
  // getMONCOMMS() below, under "Receiver status (MON)".

  // UBX-CFG-NAVX5 - get/set the ackAiding byte. If ackAiding is 1, UBX-MGA-ACK messages will be sent by the module to acknowledge the MGA data
  /**
   * @brief Get the UBX-CFG-NAVX5 ackAiding byte. When set to 1, the module sends UBX-MGA-ACK to
   *        acknowledge AssistNow (MGA) data it receives.
   *
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return The ackAiding byte value, or 255 if the query failed.
   */
  uint8_t getAckAiding(uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                     // Get the ackAiding byte - returns 255 if the sendCommand fails
  /** @brief As getAckAiding(), but writes the given ackAiding byte instead of reading it. */
  bool setAckAiding(uint8_t ackAiding, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Set the ackAiding byte

  // AssistNow Autonomous support
  // UBX-CFG-NAVX5 - get/set the aopCfg byte and set the aopOrdMaxErr word. If aopOrbMaxErr is 0 (default), the max orbit error is reset to the firmware default.
  /**
   * @brief Get the UBX-CFG-NAVX5 aopCfg byte, i.e. whether AssistNow Autonomous is enabled.
   *
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return The aopCfg byte value, or 255 if the query failed.
   */
  uint8_t getAopCfg(uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                                             // Get the AssistNow Autonomous configuration (aopCfg) - returns 255 if the sendCommand fails
  /**
   * @brief Set the aopCfg byte (AssistNow Autonomous enable) and the aopOrbMaxErr word.
   *
   * @param aopCfg The new aopCfg byte value.
   * @param aopOrbMaxErr Maximum acceptable orbit error, or 0 to reset to the firmware default. Defaults to 0.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the setting was written successfully.
   */
  bool setAopCfg(uint8_t aopCfg, uint16_t aopOrbMaxErr = 0, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Set the aopCfg byte and the aopOrdMaxErr word

  // SPARTN dynamic keys
  //"When the receiver boots, the host should send 'current' and 'next' keys in one message." - Use setDynamicSPARTNKeys for this.
  //"Every time the 'current' key is expired, 'next' takes its place."
  //"Therefore the host should then retrieve the new 'next' key and send only that." - Use setDynamicSPARTNKey for this.
  // The key can be provided in binary (uint8_t) format or in ASCII Hex (char) format, but in both cases keyLengthBytes _must_ represent the binary key length in bytes.
  /**
   * @brief Send one SPARTN dynamic decryption key (UBX-RXM-SPARTNKEY) to the module.
   *
   * Typically used to send the new 'next' key once the previous 'current' key has expired; see
   * setDynamicSPARTNKeys() for sending both 'current' and 'next' together at boot.
   *
   * @param keyLengthBytes Length of the binary key, in bytes (even when key is given as ASCII hex).
   * @param validFromWno GNSS week number from which the key is valid.
   * @param validFromTow GNSS time of week (seconds) from which the key is valid.
   * @param key The key, as a null-terminated ASCII hex string.
   * @return True if the key was sent successfully.
   */
  bool setDynamicSPARTNKey(uint8_t keyLengthBytes, uint16_t validFromWno, uint32_t validFromTow, const char *key);
  /** @brief As setDynamicSPARTNKey(..., const char*), but key is a raw binary buffer instead of an ASCII hex string. */
  bool setDynamicSPARTNKey(uint8_t keyLengthBytes, uint16_t validFromWno, uint32_t validFromTow, const uint8_t *key);
  /**
   * @brief Send the 'current' and 'next' SPARTN dynamic decryption keys together in one message.
   *
   * Intended for use at boot, so the module has both keys before the 'current' one expires.
   *
   * @param keyLengthBytes1 Length of the binary 'current' key, in bytes.
   * @param validFromWno1 GNSS week number from which the 'current' key is valid.
   * @param validFromTow1 GNSS time of week (seconds) from which the 'current' key is valid.
   * @param key1 The 'current' key, as a null-terminated ASCII hex string.
   * @param keyLengthBytes2 Length of the binary 'next' key, in bytes.
   * @param validFromWno2 GNSS week number from which the 'next' key is valid.
   * @param validFromTow2 GNSS time of week (seconds) from which the 'next' key is valid.
   * @param key2 The 'next' key, as a null-terminated ASCII hex string.
   * @return True if both keys were sent successfully.
   */
  bool setDynamicSPARTNKeys(uint8_t keyLengthBytes1, uint16_t validFromWno1, uint32_t validFromTow1, const char *key1,
                            uint8_t keyLengthBytes2, uint16_t validFromWno2, uint32_t validFromTow2, const char *key2);
  /** @brief As setDynamicSPARTNKeys(..., const char*, ..., const char*), but the keys are raw binary buffers instead of ASCII hex strings. */
  bool setDynamicSPARTNKeys(uint8_t keyLengthBytes1, uint16_t validFromWno1, uint32_t validFromTow1, const uint8_t *key1,
                            uint8_t keyLengthBytes2, uint16_t validFromWno2, uint32_t validFromTow2, const uint8_t *key2);

  // Support for SPARTN parsing
  /**
   * @brief Compute the 4-bit CRC used by SPARTN frames with a 1-byte CRC field.
   *
   * @param pU8Msg Pointer to the message bytes to checksum.
   * @param size Number of bytes in pU8Msg.
   * @return The computed 4-bit CRC value.
   */
  uint8_t uSpartnCrc4(const uint8_t *pU8Msg, size_t size);
  /** @brief As uSpartnCrc4(), but computes the 8-bit SPARTN frame CRC. */
  uint8_t uSpartnCrc8(const uint8_t *pU8Msg, size_t size);
  /** @brief As uSpartnCrc4(), but computes the 16-bit SPARTN frame CRC. */
  uint16_t uSpartnCrc16(const uint8_t *pU8Msg, size_t size);
  /** @brief As uSpartnCrc4(), but computes the 24-bit SPARTN frame CRC. */
  uint32_t uSpartnCrc24(const uint8_t *pU8Msg, size_t size);
  /** @brief As uSpartnCrc4(), but computes the 32-bit SPARTN frame CRC. */
  uint32_t uSpartnCrc32(const uint8_t *pU8Msg, size_t size);
  /**
   * @brief Feed one incoming byte into the SPARTN frame parser's internal state machine.
   *
   * @param incoming The next byte of a SPARTN stream.
   * @param valid Set to true once a complete, CRC-validated SPARTN frame has been assembled.
   * @param len Set to the length of the assembled frame when valid is true.
   * @param header If non-null, filled in with the parsed SPARTN frame header when valid is true. Defaults to nullptr.
   * @return Pointer to the internal buffer holding the assembled frame once valid is true, otherwise nullptr.
   */
  uint8_t * parseSPARTN(uint8_t incoming, bool &valid, uint16_t &len, sfe_ublox_spartn_header_t *header = nullptr);

  // ubxSECUNIQID is now self-registered - see AGENTS.md "Adding the variable-length UBX messages".
  // getUniqueChipId()/getUniqueChipIdStr(UBX_SEC_UNIQID_data_t*, ...) have been replaced by
  // getSECUNIQID()/getUniqueChipIdStr() below. For safety, call getUniqueChipIdStr() inside an
  // if(getSECUNIQID()) or if(getUBX("SEC","UNIQID")).
  /**
   * @brief Query the module's unique chip ID (UBX-SEC-UNIQID). For safety, call inside if(getSECUNIQID())
   *        or if(getUBX("SEC","UNIQID")).
   *
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the chip ID was read successfully.
   */
  bool getSECUNIQID(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Get the unique chip ID using UBX_SEC_UNIQID
  /**
   * @brief Format the chip ID bytes read by getSECUNIQID() as a hex String, e.g. "0123456789AB".
   *
   * @return The chip ID as an uppercase hex String, or an empty String if getSECUNIQID() has not succeeded.
   */
  sfe_string_t getUniqueChipIdStr(void); // Returns the uniqueId bytes as a hex String, e.g. "0123456789AB" - see ubxSECUNIQID.h

  // General configuration (used only on protocol v27 and higher - ie, ZED-F9P)

  // VALGET

protected:                                                         // These use packetCfg - which is protected from the user
  /**
   * @brief Start building a new UBX-CFG-VALGET request in the internal packetCfg buffer. Add keys with
   *        addCfgValget() and send with sendCfgValget().
   *
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @return True if the request was initialized successfully.
   */
  bool newCfgValget(uint8_t layer = VAL_LAYER_RAM);                // Create a new, empty UBX-CFG-VALGET. Add entries with addCfgValget
  /**
   * @brief Add one more key to the in-progress internal UBX-CFG-VALGET request started by newCfgValget().
   *
   * @param key The configuration key to add; its value size is deduced automatically from the key ID.
   * @return True if the key was added successfully.
   */
  bool addCfgValget(uint32_t key);                                 // Add a new key to an existing UBX-CFG-VALGET ubxPacket - deduce the value size automatically
  /**
   * @brief Send the internal UBX-CFG-VALGET request built by newCfgValget()/addCfgValget() and wait for the response.
   *
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the request was sent and a response received successfully.
   */
  bool sendCfgValget(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Send the CfgValget (UBX-CFG-VALGET) construct

public:
  /**
   * @brief Start building a new UBX-CFG-VALGET request in a caller-supplied ubxPacket. Add keys with
   *        addCfgValget(ubxPacket*, uint32_t) and send with sendCfgValget(ubxPacket*, uint16_t).
   *
   * @param pkt The caller-owned ubxPacket to build the request in.
   * @param maxPayload Size of pkt's payload buffer, in bytes.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @return True if the request was initialized successfully.
   */
  bool newCfgValget(ubxPacket *pkt, uint16_t maxPayload, uint8_t layer = VAL_LAYER_RAM); // Create a new, empty UBX-CFG-VALGET. Add entries with addCfgValget8/16/32/64
  /**
   * @brief Add one more key to the in-progress UBX-CFG-VALGET request built in pkt by newCfgValget(ubxPacket*, ...).
   *
   * @param pkt The ubxPacket previously initialized by newCfgValget(ubxPacket*, ...).
   * @param key The configuration key to add; its value size is deduced automatically from the key ID.
   * @return True if the key was added successfully.
   */
  bool addCfgValget(ubxPacket *pkt, uint32_t key);                                       // Add a new key to an existing UBX-CFG-VALGET ubxPacket - deduce the value size automatically
  /**
   * @brief Send the UBX-CFG-VALGET request built in pkt and wait for the response.
   *
   * @param pkt The ubxPacket built by newCfgValget(ubxPacket*, ...)/addCfgValget(ubxPacket*, ...).
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the request was sent and a response received successfully.
   */
  bool sendCfgValget(ubxPacket *pkt, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);       // Send the CfgValget (UBX-CFG-VALGET) construct
  /** @brief Get the number of keys currently queued in the in-progress UBX-CFG-VALGET request. @return The key count. */
  uint8_t getNumGetCfgKeys() { return _numGetCfgKeys; }                                  // Return the number of keys in the VALGET packet
  /** @brief Get the expected payload length of the pending UBX-CFG-VALGET response. @return The expected length, in bytes. */
  uint16_t getLenCfgValGetResponse() { return _lenCfgValGetResponse; }                   // Return the expected length of the VALGET response
  /**
   * @brief Determine the value size (in bytes) encoded into a configuration key's ID.
   *
   * @param key The configuration key.
   * @return The value size, in bytes (1, 2, 4 or 8).
   */
  uint8_t getCfgValueSizeBytes(const uint32_t key);                                      // Returns the value size in bytes for the given key

  template <typename T>
  /**
   * @brief Find a key's value within a received UBX-CFG-VALGET response and decode it into *value,
   *        switching on the value-size bits encoded in the key.
   *
   * @tparam T Pointer type matching the key's expected value width (e.g. uint8_t*, uint32_t*, float*).
   * @param pkt The ubxPacket holding the UBX-CFG-VALGET response payload.
   * @param key The configuration key to look up.
   * @param value Destination to write the decoded value to.
   * @param maxWidth Size of the storage value points to, in bytes; used to prevent overruns.
   * @return True if the key was found and its value decoded successfully.
   */
  bool extractConfigValueByKey(ubxPacket *pkt, const uint32_t key, T value, size_t maxWidth) // Extract the config value by its key. maxWidth prevents writing beyond the end of value
  {
    if (cfgValgetValueSizes == nullptr) // Check the size list exists
      return false;
    uint8_t sizePtr = 0;

    if (pkt->len < 4)
      return false;

    uint32_t k1 = key & ~UBX_CFG_SIZE_MASK; // Convert key back into an actual key

    uint16_t ptr = 4;
    while (ptr < pkt->len)
    {
      uint32_t k2 = extractLong(pkt, ptr);
      if (k1 == k2)
      {
        ptr += 4; // Point to the value
        switch (key & UBX_CFG_SIZE_MASK)
        {
        case UBX_CFG_L:
          if (maxWidth < sizeof(bool))
            return false;
          *value = (bool)extractByte(pkt, ptr);
          return (true);
          break;
        case UBX_CFG_U1:
        case UBX_CFG_E1:
        case UBX_CFG_X1:
          if (maxWidth < sizeof(uint8_t))
            return false;
          *value = (uint8_t)extractByte(pkt, ptr);
          return (true);
          break;
        case UBX_CFG_I1:
          if (maxWidth < sizeof(int8_t))
            return false;
          *value = (int8_t)extractSignedChar(pkt, ptr);
          return (true);
          break;
        case UBX_CFG_U2:
        case UBX_CFG_E2:
        case UBX_CFG_X2:
          if (maxWidth < sizeof(uint16_t))
            return false;
          *value = (uint16_t)extractInt(pkt, ptr);
          return (true);
          break;
        case UBX_CFG_I2:
          if (maxWidth < sizeof(int16_t))
            return false;
          *value = (int16_t)extractSignedInt(pkt, ptr);
          return (true);
          break;
        case UBX_CFG_U4:
        case UBX_CFG_E4:
        case UBX_CFG_X4:
          if (maxWidth < sizeof(uint32_t))
            return false;
          *value = (uint32_t)extractLong(pkt, ptr);
          return (true);
          break;
        case UBX_CFG_I4:
          if (maxWidth < sizeof(int32_t))
            return false;
          *value = (int32_t)extractSignedLong(pkt, ptr);
          return (true);
          break;
        case UBX_CFG_R4:
          if (maxWidth < sizeof(float))
            return false;
          *value = (float)extractFloat(pkt, ptr);
          return (true);
          break;
        case UBX_CFG_U8:
        case UBX_CFG_X8:
          if (maxWidth < sizeof(uint64_t))
            return false;
          *value = (uint64_t)extractLongLong(pkt, ptr);
          return (true);
          break;
        case UBX_CFG_I8:
          if (maxWidth < sizeof(int64_t))
            return false;
          *value = (int64_t)extractSignedLongLong(pkt, ptr);
          return (true);
          break;
        case UBX_CFG_R8:
          if (maxWidth < sizeof(double))
            return false;
          *value = (double)extractDouble(pkt, ptr);
          return (true);
          break;
        default:
          return false;
          break;
        }
      }
      ptr += 4; // Update ptr
      ptr += cfgValgetValueSizes[sizePtr++];
    }
    return false;
  }

protected:
  /**
   * @brief Send a single-key UBX-CFG-VALGET and leave the raw response in packetCfg. Shared implementation
   *        used by the getVal8/16/32/64 and getValSigned8/16/32/64/getValFloat/getValDouble accessors.
   *
   * @param key The configuration key to query.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return An sfe_ublox_status_e status code for the request.
   */
  sfe_ublox_status_e getVal(uint32_t key, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Load payload with response
public:
  /**
   * @brief Read the current value of an 8-bit-wide configuration key.
   *
   * @param key The configuration key to query.
   * @param val Filled in with the key's current value on success.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the value was read successfully.
   */
  bool getVal8(uint32_t key, uint8_t *val, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);        // Returns the value at a given key location
  /**
   * @brief Unsafe overload of getVal8(), kept for backward compatibility. Returns the value directly with
   *        no way to distinguish a real 0 from a failed query.
   *
   * @param key The configuration key to query.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return The key's current value, or 0 if the query failed.
   */
  uint8_t getVal8(uint32_t key, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                   // Unsafe overload - for backward compatibility only
  /** @brief As getVal8(uint32_t, uint8_t*, ...), but for a 16-bit-wide configuration key. */
  bool getVal16(uint32_t key, uint16_t *val, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);      // Returns the value at a given key location
  /** @brief Unsafe overload of getVal16(), kept for backward compatibility. See getVal8(uint32_t, uint8_t, uint16_t)'s unsafe overload for the caveat. */
  uint16_t getVal16(uint32_t key, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                 // Unsafe overload - for backward compatibility only
  /** @brief As getVal8(uint32_t, uint8_t*, ...), but for a 32-bit-wide configuration key. */
  bool getVal32(uint32_t key, uint32_t *val, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);      // Returns the value at a given key location
  /** @brief Unsafe overload of getVal32(), kept for backward compatibility. See getVal8(uint32_t, uint8_t, uint16_t)'s unsafe overload for the caveat. */
  uint32_t getVal32(uint32_t key, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                 // Unsafe overload - for backward compatibility only
  /** @brief As getVal8(uint32_t, uint8_t*, ...), but for a 64-bit-wide configuration key. */
  bool getVal64(uint32_t key, uint64_t *val, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);      // Returns the value at a given key location
  /** @brief Unsafe overload of getVal64(), kept for backward compatibility. See getVal8(uint32_t, uint8_t, uint16_t)'s unsafe overload for the caveat. */
  uint64_t getVal64(uint32_t key, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                 // Unsafe overload - for backward compatibility only
  /** @brief As getVal8(uint32_t, uint8_t*, ...), but decodes the value as a signed 8-bit integer. */
  bool getValSigned8(uint32_t key, int8_t *val, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);   // Returns the value at a given key location
  /** @brief As getVal8(uint32_t, uint8_t*, ...), but decodes the value as a signed 16-bit integer. */
  bool getValSigned16(uint32_t key, int16_t *val, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Returns the value at a given key location
  /** @brief As getVal8(uint32_t, uint8_t*, ...), but decodes the value as a signed 32-bit integer. */
  bool getValSigned32(uint32_t key, int32_t *val, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Returns the value at a given key location
  /** @brief As getVal8(uint32_t, uint8_t*, ...), but decodes the value as a signed 64-bit integer. */
  bool getValSigned64(uint32_t key, int64_t *val, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Returns the value at a given key location
  /** @brief As getVal8(uint32_t, uint8_t*, ...), but decodes the value as a 32-bit IEEE-754 float. */
  bool getValFloat(uint32_t key, float *val, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);      // Returns the value at a given key location
  /** @brief As getVal8(uint32_t, uint8_t*, ...), but decodes the value as a 64-bit IEEE-754 double. */
  bool getValDouble(uint32_t key, double *val, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);    // Returns the value at a given key location

  // VALSET

  /**
   * @brief Write an N-byte configuration value via a single-key UBX-CFG-VALSET.
   *
   * @param key The configuration key to write.
   * @param value Pointer to the N raw bytes to write, little-endian.
   * @param N Number of bytes in value.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the value was written successfully.
   */
  bool setValN(uint32_t key, uint8_t *value, uint8_t N, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Sets the N-byte value at a given group/id/size location
  /**
   * @brief Write an 8-bit configuration value via a single-key UBX-CFG-VALSET.
   *
   * @param key The configuration key to write.
   * @param value The new value.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the value was written successfully.
   */
  bool setVal8(uint32_t key, uint8_t value, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);             // Sets the 8-bit value at a given group/id/size location
  /** @brief As setVal8(), but for a 16-bit-wide configuration key. */
  bool setVal16(uint32_t key, uint16_t value, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);           // Sets the 16-bit value at a given group/id/size location
  /** @brief As setVal8(), but for a 32-bit-wide configuration key. */
  bool setVal32(uint32_t key, uint32_t value, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);           // Sets the 32-bit value at a given group/id/size location
  /** @brief As setVal8(), but for a 64-bit-wide configuration key. */
  bool setVal64(uint32_t key, uint64_t value, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);           // Sets the 64-bit value at a given group/id/size location
  /** @brief As setVal8(), but value is a signed 8-bit integer. */
  bool setValSigned8(uint32_t key, int8_t value, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);        // Sets the 8-bit value at a given group/id/size location
  /** @brief As setVal8(), but value is a signed 16-bit integer. */
  bool setValSigned16(uint32_t key, int16_t value, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);      // Sets the 16-bit value at a given group/id/size location
  /** @brief As setVal8(), but value is a signed 32-bit integer. */
  bool setValSigned32(uint32_t key, int32_t value, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);      // Sets the 32-bit value at a given group/id/size location
  /** @brief As setVal8(), but value is a signed 64-bit integer. */
  bool setValSigned64(uint32_t key, int64_t value, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);      // Sets the 64-bit value at a given group/id/size location
  /** @brief As setVal8(), but value is a 32-bit IEEE-754 float. */
  bool setValFloat(uint32_t key, float value, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);           // Sets the 32-bit value at a given group/id/size location
  /** @brief As setVal8(), but value is a 64-bit IEEE-754 double. */
  bool setValDouble(uint32_t key, double value, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);         // Sets the 64-bit value at a given group/id/size location

  /**
   * @brief Start building a new UBX-CFG-VALSET request. Add keys with addCfgValsetN()/addCfgValset<T>() and
   *        send with sendCfgValset().
   *
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @return True if the request was initialized successfully.
   */
  bool newCfgValset(uint8_t layer = VAL_LAYER_RAM_BBR);                                                         // Create a new, empty UBX-CFG-VALSET. Add entries with addCfgValset8/16/32/64
  /**
   * @brief Add one more key/value pair to the in-progress UBX-CFG-VALSET request started by newCfgValset().
   *
   * @param key The configuration key to add.
   * @param value Pointer to the N raw value bytes, little-endian.
   * @param N Number of bytes in value.
   * @return True if the key/value pair was added successfully.
   */
  bool addCfgValsetN(uint32_t key, uint8_t *value, uint8_t N);                                                  // Add a new key and N-byte value to an existing UBX-CFG-VALSET ubxPacket
  /**
   * @brief Send the UBX-CFG-VALSET request built by newCfgValset()/addCfgValsetN()/addCfgValset<T>().
   *
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the request was sent and acknowledged successfully.
   */
  bool sendCfgValset(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                                              // Send the CfgValset (UBX-CFG-VALSET) construct
  /** @brief Get the number of key/value pairs currently queued in the in-progress UBX-CFG-VALSET request. @return The key count. */
  uint8_t getCfgValsetLen();                                                                                    // Returns the length of the current CfgValset construct as number-of-keys
  /** @brief Get how many free payload bytes remain in the in-progress UBX-CFG-VALSET request. @return The number of free bytes. */
  size_t getCfgValsetSpaceRemaining();                                                                          // Returns the number of free bytes remaining in packetCfg
  /**
   * @brief Automatically send the in-progress UBX-CFG-VALSET (and start a new one) once its remaining
   *        space drops below a threshold, so a long run of addCfgValset() calls never overflows the buffer.
   *
   * @param spaceRemaining Threshold, in bytes; 0 disables the automatic send.
   */
  void autoSendCfgValsetAtSpaceRemaining(size_t spaceRemaining) { _autoSendAtSpaceRemaining = spaceRemaining; } // Cause CFG_VALSET packets to be sent automatically when packetCfg has less than this many bytes available

  template <typename T>
  /**
   * @brief Add one more key/value pair to the in-progress UBX-CFG-VALSET request, encoding value's bytes
   *        according to the width/type bits encoded in key.
   *
   * @tparam T Type matching the key's expected value (e.g. bool, uint8_t, int32_t, float).
   * @param key The configuration key to add.
   * @param value The value to encode and add.
   * @return True if the key/value pair was added successfully.
   */
  bool addCfgValset(uint32_t key, T value) // Add a new key and value to an existing UBX-CFG-VALSET ubxPacket
  {
    uint8_t val[8];

    uint32_t k1 = key & ~UBX_CFG_SIZE_MASK; // Convert key back into an actual key

    switch (key & UBX_CFG_SIZE_MASK)
    {
    case UBX_CFG_L:
      val[0] = (bool)value;
      return (addCfgValsetN(k1, val, 1));
      break;
    case UBX_CFG_U1:
    case UBX_CFG_E1:
    case UBX_CFG_X1:
      val[0] = (uint8_t)value;
      return (addCfgValsetN(k1, val, 1));
      break;
    case UBX_CFG_I1:
      unsignedSigned8 usVal8;
      usVal8.signed8 = (int8_t)value;
      return (addCfgValsetN(k1, &usVal8.unsigned8, 1));
      break;
    case UBX_CFG_U2:
    case UBX_CFG_E2:
    case UBX_CFG_X2:
      for (uint8_t i = 0; i < 2; i++)
        val[i] = (uint8_t)(((uint16_t)value) >> (8 * i)); // Value
      return (addCfgValsetN(k1, val, 2));
      break;
    case UBX_CFG_I2:
      unsignedSigned16 usVal16;
      usVal16.signed16 = (int16_t)value;
      for (uint8_t i = 0; i < 2; i++)
        val[i] = (uint8_t)(usVal16.unsigned16 >> (8 * i)); // Value
      return (addCfgValsetN(k1, val, 2));
      break;
    case UBX_CFG_U4:
    case UBX_CFG_E4:
    case UBX_CFG_X4:
      for (uint8_t i = 0; i < 4; i++)
        val[i] = (uint8_t)(((uint32_t)value) >> (8 * i)); // Value
      return (addCfgValsetN(k1, val, 4));
      break;
    case UBX_CFG_I4:
      unsignedSigned32 usVal32;
      usVal32.signed32 = (int32_t)value;
      for (uint8_t i = 0; i < 4; i++)
        val[i] = (uint8_t)(usVal32.unsigned32 >> (8 * i)); // Value
      return (addCfgValsetN(k1, val, 4));
      break;
    case UBX_CFG_R4:
      unsigned32float us32flt;
      us32flt.flt = (float)value;
      for (uint8_t i = 0; i < 4; i++)
        val[i] = (uint8_t)(us32flt.unsigned32 >> (8 * i)); // Value
      return (addCfgValsetN(k1, val, 4));
      break;
    case UBX_CFG_U8:
    case UBX_CFG_X8:
      for (uint8_t i = 0; i < 8; i++)
        val[i] = (uint8_t)(((uint64_t)value) >> (8 * i)); // Value
      return (addCfgValsetN(k1, val, 8));
      break;
    case UBX_CFG_I8:
      unsignedSigned64 usVal64;
      usVal64.signed64 = (int64_t)value;
      for (uint8_t i = 0; i < 8; i++)
        val[i] = (uint8_t)(usVal64.unsigned64 >> (8 * i)); // Value
      return (addCfgValsetN(k1, val, 8));
      break;
    case UBX_CFG_R8:
      unsigned64double us64dbl;
      us64dbl.dbl = (float)value;
      for (uint8_t i = 0; i < 8; i++)
        val[i] = (uint8_t)(us64dbl.unsigned64 >> (8 * i)); // Value
      return (addCfgValsetN(k1, val, 8));
      break;
    default:
      return false;
      break;
    }
    return false;
  }

  // Convenience wrapper: set a single CFG key/value pair in one call, instead of the usual
  // newCfgValset() / addCfgValset() / sendCfgValset() three-step dance - see AGENTS.md
  // "setCfgValset" and CallbackExample1_NAVHPPOSLLH.ino (`myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSLLH_I2C, 1);`).
  template <typename T>
  /**
   * @brief Convenience wrapper: set a single configuration key/value pair in one call, instead of the
   *        usual newCfgValset()/addCfgValset()/sendCfgValset() three-step sequence.
   *
   * @tparam T Type matching the key's expected value (e.g. bool, uint8_t, int32_t, float).
   * @param key The configuration key to write.
   * @param value The value to write.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @return True if the value was written successfully.
   */
  bool setCfgValset(uint32_t key, T value, uint8_t layer = VAL_LAYER_RAM_BBR) // Set the single key to the given value using CFG-VALSET
  {
    newCfgValset(layer);
    addCfgValset(key, value);
    return sendCfgValset();
  }

  // get and set functions for all of the "automatic" message processing

  // getUBX will only return data once in each navigation cycle. By default, that is once per second.
  // Therefore we should set kUBLOXGNSSDefaultMaxWait to slightly longer than that.
  // If you change the navigation frequency to (e.g.) 4Hz using setNavigationFrequency(4)
  // then you should use a shorter maxWait. 300msec would be about right: getUBX(300)

  // ***** v4 scaffolding - generic (Class, ID)-keyed message access. See AGENTS.md "Reference Scaffolding" *****
  /**
   * @brief Recover the opaque per-message ubxMessage object that owns a given callback's data.
   *
   * @param theData The ubxCallbackDataCommon_t* passed into a v4 UBX message callback.
   * @return Pointer to the owning ubxMessage, for use with the other getUbxMessage*() accessors.
   */
  ubxMessage *getUbxMessagePtr(ubxCallbackDataCommon_t *theData); // Factory: hands back the opaque per-message object a callback's ubxCallbackDataCommon_t* points at
  /**
   * @brief Extract a named field from the message a callback just fired for, reading its callback-time
   *        snapshot (_callbackStorage) rather than the live, possibly-since-overwritten storage.
   *
   * @param theMessage The message object, as returned by getUbxMessagePtr().
   * @param fieldName The field's name, as declared in the message's field table.
   * @return The field's value, or an ubxAnyType indicating "field not found" if fieldName is unknown.
   */
  ubxAnyType getUbxMessageFieldCallback(ubxMessage *theMessage, const char *fieldName); // Factory: extracts a named field from the message a callback just fired for, reading from its _callbackStorage
  /** @brief As getUbxMessageFieldCallback(), but reads the message's live _storage instead of a callback-time snapshot. */
  ubxAnyType getUbxMessageField(ubxMessage *theMessage, const char *fieldName); // Factory: extracts a named field from the message, reading from its _storage
  /**
   * @brief Extract a named field from one repeated block of a variable-length message (e.g. one satellite
   *        entry of NAV-SAT), reading the callback-time snapshot. Bound blockIndex with
   *        getUbxMessageBlockCountCallback().
   *
   * @param theMessage The message object, as returned by getUbxMessagePtr().
   * @param blockIndex Which repeated block to read (0-based).
   * @param fieldName The block field's name, as declared in the message's block field table.
   * @return The field's value, or an ubxAnyType indicating "field not found" if fieldName is unknown.
   */
  ubxAnyType getUbxMessageBlockFieldCallback(ubxMessage *theMessage, uint16_t blockIndex, const char *fieldName); // Factory: extracts a named field from repeated block 'blockIndex' of a variable-length message (e.g. NAV-SAT), reading from its _callbackStorage
  /** @brief As getUbxMessageBlockFieldCallback(), but reads the message's live _storage instead of a callback-time snapshot. Bound blockIndex with getUbxMessageBlockCount(). */
  ubxAnyType getUbxMessageBlockField(ubxMessage *theMessage, uint16_t blockIndex, const char *fieldName); // Factory: extracts a named field from repeated block 'blockIndex' of a variable-length message (e.g. NAV-SAT), reading from its _storage
  // v4 scaffolding, added for ESF-MEAS - see AGENTS.md "Adding support for ESF-MEAS". General/reusable by any future message with the same shape, not ESF-MEAS-specific.
  /**
   * @brief Get the defensively-computed number of repeated blocks in a variable-length message's
   *        callback-time snapshot, for bounding a getUbxMessageBlockFieldCallback() loop.
   *
   * Cross-checks the header's own count field (for a message that declared one, e.g. ESF-MEAS's numMeas)
   * against the actual received length, or falls back to computing the count purely from the received
   * length for a message with no count field at all (e.g. ESF-RAW). Prefer this over reading the raw
   * header field directly.
   *
   * @param theMessage The message object, as returned by getUbxMessagePtr().
   * @return The number of repeated blocks actually present.
   */
  uint16_t getUbxMessageBlockCountCallback(ubxMessage *theMessage); // Factory: the DEFENSIVELY-computed real block count for any message with block support, reading from its _callbackStorage - cross-checks the header's own count field for a message that set _blockCountField (e.g. ESF-MEAS's numMeas), or falls back to computing it purely from the actual received length for a message with no such field at all (e.g. ESF-RAW) - use this, not the raw header field, to bound a getUbxMessageBlockFieldCallback() loop
  /** @brief As getUbxMessageBlockCountCallback(), but reads the message's live _storage instead of a callback-time snapshot. */
  uint16_t getUbxMessageBlockCount(ubxMessage *theMessage);         // Factory: same as above, reading from its live _storage
  /**
   * @brief Extract a named field from a variable-length message's optional trailing footer group (e.g.
   * ESF-MEAS's calibTtag), reading the callback-time snapshot.
   *
   * @param theMessage The message object, as returned by getUbxMessagePtr().
   * @param fieldName The footer field's name, as declared in the message's footer field table.
   * @return The field's value, or an ubxAnyType indicating "field not found" if fieldName is unknown or
   *         this particular message instance did not actually include the footer.
   */
  ubxAnyType getUbxMessageFooterFieldCallback(ubxMessage *theMessage, const char *fieldName); // Factory: extracts a named field from a variable-length message's OPTIONAL trailing footer group (e.g. ESF-MEAS's calibTtag), reading from its _callbackStorage - returns "field not found" if this particular message did not actually include the footer
  /** @brief As getUbxMessageFooterFieldCallback(), but reads the message's live _storage instead of a callback-time snapshot. */
  ubxAnyType getUbxMessageFooterField(ubxMessage *theMessage, const char *fieldName);         // Factory: same as above, reading from its live _storage
  /**
   * @brief Get the complete raw UBX frame length (6-byte header + payload + 2-byte checksum) captured for
   * the message a callback just fired for. Pair with getUbxMessageRawPtrCallback() to relay the frame
   * verbatim, e.g. to another device or UART.
   *
   * @param theMessage The message object, as returned by getUbxMessagePtr().
   * @return The raw frame length in bytes, or 0 if a raw copy was not captured for this message.
   */
  uint16_t getUbxMessageRawLengthCallback(ubxMessage *theMessage); // Factory: the COMPLETE raw UBX frame length (6-byte header + payload + 2-byte checksum) for the message a callback just fired for, or 0 if unavailable - pair with getUbxMessageRawPtrCallback() to relay the message verbatim (e.g. to another device/UART)
  /**
   * @brief Get a pointer to the start of the raw UBX frame captured for getUbxMessageRawLengthCallback().
   *
   * @param theMessage The message object, as returned by getUbxMessagePtr().
   * @return Pointer to the raw frame bytes, or nullptr if a raw copy was not captured for this message.
   */
  const uint8_t *getUbxMessageRawPtrCallback(ubxMessage *theMessage); // Factory: a pointer to the start of that complete raw UBX frame within _callbackRawFrame, or nullptr if unavailable
  /**
   * @brief Get fresh data for any registered UBX message, by Class/ID name. If the message is set to
   *        automatic, checks (non-blocking) whether new data has arrived; otherwise sends an explicit poll
   *        and waits for the response.
   *
   * @param Class The message's UBX class name, e.g. "NAV".
   * @param ID The message's UBX message name, e.g. "PVT".
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh data is now available.
   */
  bool getUBX(const char *Class, const char *ID, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Generic poll-or-check-automatic, by Class/ID
  /** @brief As getUBX(const char*, const char*, ...), but Class/ID are given as their raw UBX byte values instead of names. */
  bool getUBX(uint8_t Class, uint8_t ID, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Generic poll-or-check-automatic, by Class/ID
  /**
   * @brief Read one named field of a registered UBX message's live storage, by Class/ID/field name.
   *
   * @param Class The message's UBX class name, e.g. "NAV".
   * @param ID The message's UBX message name, e.g. "PVT".
   * @param field The field's name, as declared in the message's field table.
   * @param value Filled in with the field's value on success.
   * @return True if the message/field were found and read successfully.
   */
  bool getUBXfield(const char *Class, const char *ID, const char *field, ubxAnyType *value); // Generic field read, by Class/ID/name
  /** @brief As getUBXfield(const char*, const char*, ...), but Class/ID are given as their raw UBX byte values instead of names. */
  bool getUBXfield(uint8_t Class, uint8_t ID, const char *field, ubxAnyType *value); // Generic field read, by Class/ID/name

  /**
   * @brief Enable or disable automatic (unsolicited) output of a registered UBX message, by Class/ID name.
   *
   * @param Class The message's UBX class name, e.g. "NAV".
   * @param ID The message's UBX message name, e.g. "PVT".
   * @param enabled True to enable automatic output, false to disable it. Defaults to true.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the setting was written successfully.
   */
  bool setAutoUBX(const char *Class, const char *ID, bool enabled = true, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /** @brief As setAutoUBX(const char*, const char*, ...), but Class/ID are given as their raw UBX byte values instead of names. */
  bool setAutoUBX(uint8_t Class, uint8_t ID, bool enabled = true, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /**
   * @brief As setAutoUBX(const char*, const char*, bool, ...), but also controls whether the getter for
   *        this message implicitly re-polls when the auto data is stale.
   *
   * @param Class The message's UBX class name, e.g. "NAV".
   * @param ID The message's UBX message name, e.g. "PVT".
   * @param enabled True to enable automatic output, false to disable it.
   * @param implicitUpdate If true, the corresponding get*() call implicitly polls when no fresh automatic
   *                       data is available yet; if false, it returns false instead of blocking.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the setting was written successfully.
   */
  bool setAutoUBX(const char *Class, const char *ID, bool enabled, bool implicitUpdate, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /** @brief As setAutoUBX(const char*, const char*, bool, bool, ...), but Class/ID are given as their raw UBX byte values instead of names. */
  bool setAutoUBX(uint8_t Class, uint8_t ID, bool enabled, bool implicitUpdate, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /**
   * @brief Enable automatic output of a registered UBX message at a reduced rate (every rate navigation
   *        epochs), by Class/ID name.
   *
   * @param Class The message's UBX class name, e.g. "NAV".
   * @param ID The message's UBX message name, e.g. "PVT".
   * @param rate Output the message every rate navigation epochs (0 disables automatic output).
   * @param implicitUpdate If true, the corresponding get*() call implicitly polls when no fresh automatic
   *                       data is available yet; if false, it returns false instead of blocking.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the setting was written successfully.
   */
  bool setAutoUBXrate(const char *Class, const char *ID, uint8_t rate, bool implicitUpdate, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /** @brief As setAutoUBXrate(const char*, const char*, ...), but Class/ID are given as their raw UBX byte values instead of names. */
  bool setAutoUBXrate(uint8_t Class, uint8_t ID, uint8_t rate, bool implicitUpdate, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /**
   * @brief Tell the library to treat a message as automatic without configuring the module, for when the
   * module is already cyclically outputting it and config access is unavailable.
   *
   * @param Class The message's UBX class name, e.g. "NAV".
   * @param ID The message's UBX message name, e.g. "PVT".
   * @param enabled True to assume the message is being output automatically.
   * @param implicitUpdate If true, the corresponding get*() call implicitly polls when no fresh automatic
   *                       data is available yet. Defaults to true.
   * @return True if the internal state was updated successfully.
   */
  bool assumeAutoUBX(const char *Class, const char *ID, bool enabled, bool implicitUpdate = true);  // In case no config access to the GPS is possible and UBX is send cyclically already
  /** @brief As assumeAutoUBX(const char*, const char*, ...), but Class/ID are given as their raw UBX byte values instead of names. */
  bool assumeAutoUBX(uint8_t Class, uint8_t ID, bool enabled, bool implicitUpdate = true);  // In case no config access to the GPS is possible and UBX is send cyclically already
  /**
   * @brief Mark a registered UBX message's stored data as stale/already-read, by Class/ID name.
   *
   * @param Class The message's UBX class name, e.g. "NAV".
   * @param ID The message's UBX message name, e.g. "PVT".
   */
  void flushUBX(const char *Class, const char *ID); // Mark the UBX data as read/stale
  /** @brief As flushUBX(const char*, const char*), but Class/ID are given as their raw UBX byte values instead of names. */
  void flushUBX(uint8_t Class, uint8_t ID); // Mark the UBX data as read/stale
  /**
   * @brief Enable or disable logging a registered UBX message's raw bytes to the file buffer, by Class/ID name.
   *
   * @param Class The message's UBX class name, e.g. "NAV".
   * @param ID The message's UBX message name, e.g. "PVT".
   * @param enabled True to log this message, false to stop logging it. Defaults to true.
   */
  void logUBX(const char *Class, const char *ID, bool enabled = true); // Log data to file buffer
  /** @brief As logUBX(const char*, const char*, ...), but Class/ID are given as their raw UBX byte values instead of names. */
  void logUBX(uint8_t Class, uint8_t ID, bool enabled = true); // Log data to file buffer
  // Generic replacement for the old per-message setAuto<MSG>callbackPtr() functions
  /**
   * @brief Register a callback to be fired when fresh automatic data arrives for any registered UBX
   *        message, by Class/ID name. Generic replacement for the old per-message setAuto<MSG>callbackPtr() functions.
   *
   * @param classStr The message's UBX class name, e.g. "NAV".
   * @param idStr The message's UBX message name, e.g. "PVT".
   * @param callbackPointerPtr Function to call with a ubxCallbackDataCommon_t* when fresh data arrives.
   * @return True if the callback was registered successfully.
   */
  bool setAutoCallbackPtr(const char *classStr, const char *idStr, void (*callbackPointerPtr)(ubxCallbackDataCommon_t *));

  // UBX-NAV-SAT is now a registered v4 message (ubxNAVSAT) - see AGENTS.md "Adding the
  // variable-length UBX messages". setAutoNAVSAT/setAutoNAVSATrate/assumeAutoNAVSAT/
  // flushNAVSAT/logNAVSAT/setAutoNAVSATcallbackPtr are retired; use the generic
  // setAutoUBX/setAutoUBXrate/assumeAutoUBX/flushUBX/logUBX/setAutoCallbackPtr above instead
  // (by Class/ID = UBX_CLASS_NAV/UBX_NAV_SAT, or by name "NAV"/"SAT"). getNAVSAT() remains, as a
  // thin wrapper, since it is called directly rather than by name.
  /**
   * @brief Get fresh UBX-NAV-SAT (per-satellite signal/tracking status) data. Thin wrapper around the
   * generic getUBX("NAV", "SAT", ...), kept because it is called directly rather than by name.
   *
   * @param maxWait Milliseconds to wait for a polled response, if NAV-SAT is not automatic. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-SAT data is now available.
   */
  bool getNAVSAT(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Query module for latest NAVSAT data. If autoNAVSAT is disabled, performs an explicit poll and waits, if enabled does not block. Returns true if new NAVSAT is available.

  // UBX-NAV-SIG is now a registered v4 message (ubxNAVSIG) - see AGENTS.md "Adding the
  // variable-length UBX messages". setAutoNAVSIG/setAutoNAVSIGrate/assumeAutoNAVSIG/
  // flushNAVSIG/logNAVSIG/setAutoNAVSIGcallbackPtr are retired; use the generic
  // setAutoUBX/setAutoUBXrate/assumeAutoUBX/flushUBX/logUBX/setAutoCallbackPtr above instead
  // (by Class/ID = UBX_CLASS_NAV/UBX_NAV_SIG, or by name "NAV"/"SIG"). getNAVSIG() remains, as a
  // thin wrapper, since it is called directly rather than by name.
  /**
   * @brief Get fresh UBX-NAV-SIG (per-signal tracking status) data. Thin wrapper around the generic
   * getUBX("NAV", "SIG", ...), kept because it is called directly rather than by name.
   *
   * @param maxWait Milliseconds to wait for a polled response, if NAV-SIG is not automatic. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-SIG data is now available.
   */
  bool getNAVSIG(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Query module for latest NAVSIG data. If autoNAVSIG is disabled, performs an explicit poll and waits, if enabled does not block. Returns true if new NAVSIG is available.

  // Receiver Manager Messages (RXM)

  // UBX-RXM-PMP is now a registered v4 message (ubxRXMPMP) - see AGENTS.md "Adding support for
  // RXM-PMP". setRXMPMPcallbackPtr/setRXMPMPmessageCallbackPtr are retired; use the generic
  // setAutoCallbackPtr above instead (by name "RXM"/"PMP"), then getUbxMessageFieldCallback()/
  // getUbxMessageBlockFieldCallback()/getUbxMessageBlockCountCallback() to read the fields/userData
  // bytes. There is no getRXMPMP() - PMP cannot be polled, it is "Output" only, same as ESF-RAW.
  // setRXMPMPmessageCallbackPtr's "push the whole message" use case is now covered generically, for
  // ANY message with a callback registered, by getUbxMessageRawLengthCallback()/
  // getUbxMessageRawPtrCallback() (added in Phase 30, for ESF-MEAS) - not reimplemented here, per
  // explicit instruction. Note: on the NEO-D9S, UBX-RXM-PMP is enabled by default on all ports; you
  // can disable it by calling (e.g.) setVal8(UBLOX_CFG_MSGOUT_UBX_RXM_PMP_I2C, 0) - the NEO-D9S
  // does not support UBX-CFG-MSG, but does support UBX-CFG-VALSET, which is what setVal8() uses.

  // UBX-RXM-QZSSL6 is now a registered v4 message (ubxRXMQZSSL6) - see AGENTS.md "Adding
  // support for RXM-QZSSL6". setRXMQZSSL6messageCallbackPtr is retired; use the generic
  // setAutoCallbackPtr above instead (by name "RXM"/"QZSSL6"), then
  // getUbxMessageFieldCallback()/getUbxMessageBlockFieldCallback()/
  // getUbxMessageBlockCountCallback() to read the fields/msgBytes bytes - remember
  // numCallbackCopies is 2 for this message (UBX_RXM_QZSSL6_NUM_CHANNELS), not 1, since
  // QZSSL6 is output two at a time (one per L6 reception channel). There is no
  // getRXMQZSSL6() - QZSSL6 cannot be polled, it is "Output" only, same as RXM-PMP/ESF-RAW.
  // setRXMQZSSL6messageCallbackPtr's "push the whole message" use case is now covered
  // generically, for ANY message with a callback registered, by
  // getUbxMessageRawLengthCallback()/getUbxMessageRawPtrCallback() (added in Phase 30, for
  // ESF-MEAS) - not reimplemented here, same treatment as RXM-PMP (Phase 32). Note: on the
  // NEO-D9C, UBX-RXM-QZSSL6 is enabled by default on all ports; you can disable it by calling
  // (e.g.) setVal8(UBLOX_CFG_MSGOUT_UBX_RXM_QZSSL6_I2C, 0) - the NEO-D9C does not support
  // UBX-CFG-MSG, but does support UBX-CFG-VALSET, which is what setVal8() uses.


  // UBX-RXM-SFRBX is now a registered v4 message (ubxRXMSFRBX) - see AGENTS.md "Adding support
  // for RXM-SFRBX". setAutoRXMSFRBX/setAutoRXMSFRBXrate/setAutoRXMSFRBXcallbackPtr/
  // setAutoRXMSFRBXmessageCallbackPtr/assumeAutoRXMSFRBX/flushRXMSFRBX/logRXMSFRBX are retired -
  // see the comment above getRXMSFRBX()'s definition in u-blox_GNSS.cpp. getRXMSFRBX is kept as a
  // thin wrapper for backward compatibility, though it should strictly be deprecated (SFRBX is
  // output-only and cannot be polled - see issue #167).
  /**
   * @brief Get fresh UBX-RXM-SFRBX (broadcast navigation data subframe) data. Thin wrapper kept for
   * backward compatibility; RXM-SFRBX is output-only and cannot actually be polled (see issue #167).
   *
   * @param maxWait Milliseconds to wait. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh RXM-SFRBX data is now available.
   */
  bool getRXMSFRBX(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // RXM SFRBX

  // UBX-RXM-RAWX and UBX-RXM-MEASX are now registered v4 messages (ubxRXMRAWX/ubxRXMMEASX) -
  // see AGENTS.md "Adding the variable-length UBX messages". setAutoRXMRAWX/setAutoRXMRAWXrate/
  // assumeAutoRXMRAWX/flushRXMRAWX/logRXMRAWX/setAutoRXMRAWXcallbackPtr and their RXM-MEASX
  // equivalents are retired; use the generic setAutoUBX/setAutoUBXrate/assumeAutoUBX/flushUBX/
  // logUBX/setAutoCallbackPtr above instead (by Class/ID = UBX_CLASS_RXM/UBX_RXM_RAWX or
  // UBX_CLASS_RXM/UBX_RXM_MEASX, or by name "RXM"/"RAWX" or "RXM"/"MEASX"). getRXMRAWX()/
  // getRXMMEASX() remain, as thin wrappers, since they are called directly rather than by name.
  /**
   * @brief Get fresh UBX-RXM-RAWX (multi-GNSS raw measurement) data. Thin wrapper around the generic
   * getUBX("RXM", "RAWX", ...), kept because it is called directly rather than by name.
   *
   * @param maxWait Milliseconds to wait for a polled response, if RXM-RAWX is not automatic. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh RXM-RAWX data is now available.
   */
  bool getRXMRAWX(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);   // Query module for latest RXM RAWX data. If autoRXMRAWX is disabled, performs an explicit poll and waits, if enabled does not block. Returns true if new RXM RAWX is available.
  /**
   * @brief Get fresh UBX-RXM-MEASX (satellite measurements for RRLP) data. Thin wrapper around the
   * generic getUBX("RXM", "MEASX", ...), kept because it is called directly rather than by name.
   *
   * @param maxWait Milliseconds to wait for a polled response, if RXM-MEASX is not automatic. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh RXM-MEASX data is now available.
   */
  bool getRXMMEASX(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Query module for latest RXM MEASX data. If autoRXMMEASX is disabled, performs an explicit poll and waits, if enabled does not block. Returns true if new RXM MEASX is available.

  // Receiver status (MON)

  // ubxMONCOMMS is now self-registered - see AGENTS.md "Adding the variable-length UBX messages".
  // getMONCOMMS() remains, as a thin wrapper, since it is called directly rather than by name.
  /**
   * @brief Get fresh UBX-MON-COMMS (communication port status) data. Thin wrapper around the generic
   * getUBX("MON", "COMMS", ...), kept because it is called directly rather than by name.
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh MON-COMMS data is now available.
   */
  bool getMONCOMMS(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // MON COMMS

  // ubxMONRF is now self-registered - see AGENTS.md "Adding the variable-length UBX messages".
  // getMONRF() remains, as a thin wrapper, since it is called directly rather than by name.
  // Replaces the old getRFinformation(UBX_MON_RF_data_t*, ...) - read fields via
  // getUBXfield()/getUbxMessageBlockField() (with the "nBlocks" header field to bound the loop),
  // same as every other migrated variable-length message.
  /**
   * @brief Get fresh UBX-MON-RF (RF/jamming information, ZED-F9 only) data. Thin wrapper around the
   * generic getUBX("MON", "RF", ...), kept because it is called directly rather than by name. Read
   * fields via getUBXfield()/getUbxMessageBlockField(), bounding block loops with the "nBlocks" header field.
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh MON-RF data is now available.
   */
  bool getMONRF(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // MON RF (RF information, including jamming)

  // Sensor fusion (dead reckoning) (ESF)

  /**
   * @brief Get fresh UBX-ESF-STATUS data. Historical alias for getESFSTATUS(), kept unchanged for
   * backward compatibility.
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh ESF-STATUS data is now available.
   */
  bool getEsfInfo(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // ESF STATUS Helper - thin wrapper, unchanged

  // ubxESFSTATUS is now self-registered - see AGENTS.md "Adding support for ESF-RAW and
  // ESF-STATUS". setAutoESFSTATUS/setAutoESFSTATUSrate/setAutoESFSTATUScallbackPtr/
  // assumeAutoESFSTATUS/initPacketUBXESFSTATUS/flushESFSTATUS/logESFSTATUS are retired; the
  // generic setAutoUBX/setAutoUBXrate/setAutoCallbackPtr/assumeAutoUBX/flushUBX/logUBX (by
  // Class/ID UBX_CLASS_ESF/UBX_ESF_STATUS, or by name "ESF"/"STATUS") do the same job, with no
  // per-message code required. getESFSTATUS() remains, as a thin wrapper, since it is called
  // directly rather than by name (including from getEsfInfo() above).
  /**
   * @brief Get fresh UBX-ESF-STATUS (sensor fusion status) data. Thin wrapper around the generic
   * getUBX("ESF", "STATUS", ...), kept because it is called directly rather than by name (including
   * from getEsfInfo()).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh ESF-STATUS data is now available.
   */
  bool getESFSTATUS(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // ESF STATUS

  // ubxESFMEAS is now self-registered - see AGENTS.md "Adding support for ESF-MEAS". The old
  // setAutoESFMEAS/setAutoESFMEASrate/setAutoESFMEAScallbackPtr/assumeAutoESFMEAS/logESFMEAS
  // declarations here had no definitions anywhere in u-blox_GNSS.cpp (dead declarations, never
  // callable) - the generic setAutoUBX/setAutoUBXrate/setAutoCallbackPtr/assumeAutoUBX/flushUBX/
  // logUBX (by Class/ID UBX_CLASS_ESF/UBX_ESF_MEAS, or by name "ESF"/"MEAS") do the same job.
  // getESFMEAS() remains, as a thin wrapper, since it is called directly rather than by name.
  /**
   * @brief Get fresh UBX-ESF-MEAS (external sensor fusion measurement) data. Thin wrapper around the
   * generic getUBX("ESF", "MEAS", ...), kept because it is called directly rather than by name.
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh ESF-MEAS data is now available.
   */
  bool getESFMEAS(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Query module for latest ESF MEAS data

  // ubxESFRAW is now self-registered - see AGENTS.md "Adding support for ESF-RAW and
  // ESF-STATUS". The old setAutoESFRAW/setAutoESFRAW(implicitUpdate)/setAutoESFRAWrate/
  // setAutoESFRAWcallbackPtr/assumeAutoESFRAW/logESFRAW/initPacketUBXESFRAW declarations here had
  // NO definitions anywhere in u-blox_GNSS.cpp - dead declarations, never callable, predating this
  // migration - so there is nothing to retire beyond removing them from this header. The generic
  // setAutoUBX/setAutoUBXrate/setAutoCallbackPtr/assumeAutoUBX/flushUBX/logUBX (by Class/ID
  // UBX_CLASS_ESF/UBX_ESF_RAW, or by name "ESF"/"RAW") do the same job, with no per-message code
  // required. There is no getESFRAW() - ESF RAW data cannot be polled, it is "Output" only (see
  // the comment above the real ESF-RAW section in u-blox_structs.h), and no such wrapper existed
  // in the old API either.

  // ubxSECSIG (Version 3 - see ubxSECSIG.h) is now self-registered - see AGENTS.md "Adding the
  // variable-length UBX messages". getSECSIG() remains, as a thin wrapper, since it is called
  // directly rather than by name. The UBX_SEC_SIG_data_t* overload is redacted.
  /**
   * @brief Get fresh UBX-SEC-SIG (signal security/spoofing information) data. Thin wrapper around the
   * generic getUBX("SEC", "SIG", ...), kept because it is called directly rather than by name.
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh SEC-SIG data is now available.
   */
  bool getSECSIG(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Query module for latest data

// Helper functions for CFG RATE

  /**
   * @brief Set the number of navigation solutions output per second (UBX-CFG-RATE).
   *
   * @param navFreq Navigation solutions per second.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the rate was set successfully.
   */
  bool setNavigationFrequency(uint8_t navFreq, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Set the number of nav solutions sent per second
  /**
   * @brief Get the number of navigation solutions currently output per second.
   *
   * @param navFreq Filled in with the current rate on success.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the rate was read successfully.
   */
  bool getNavigationFrequency(uint8_t *navFreq, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);    // Get the number of nav solutions sent per second currently being output by module
  /** @brief Unsafe overload of getNavigationFrequency(). Returns the rate directly with no way to distinguish failure from a real value. */
  uint8_t getNavigationFrequency(uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                   // Unsafe overload
  /**
   * @brief Set the elapsed time between GNSS measurements (UBX-CFG-RATE measRate).
   *
   * @param rate Time between measurements, milliseconds.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the rate was set successfully.
   */
  bool setMeasurementRate(uint16_t rate, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);       // Set the elapsed time between GNSS measurements in milliseconds, which defines the rate
  /**
   * @brief Get the elapsed time between GNSS measurements, in milliseconds.
   *
   * @param measRate Filled in with the current measurement period on success.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the rate was read successfully.
   */
  bool getMeasurementRate(uint16_t *measRate, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);      // Return the elapsed time between GNSS measurements in milliseconds
  /** @brief Unsafe overload of getMeasurementRate(). Returns the period directly with no way to distinguish failure from a real value. */
  uint16_t getMeasurementRate(uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                      // Unsafe overload
  /**
   * @brief Set the ratio between the number of measurements and the number of navigation solutions
   *        (UBX-CFG-RATE navRate).
   *
   * @param rate Ratio, in measurement cycles. Maximum 127.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the rate was set successfully.
   */
  bool setNavigationRate(uint16_t rate, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);        // Set the ratio between the number of measurements and the number of navigation solutions. Unit is cycles. Max is 127
  /**
   * @brief Get the ratio between the number of measurements and the number of navigation solutions, in cycles.
   *
   * @param navRate Filled in with the current ratio on success.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the ratio was read successfully.
   */
  bool getNavigationRate(uint16_t *navRate, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);        // Return the ratio between the number of measurements and the number of navigation solutions. Unit is cycles
  /** @brief Unsafe overload of getNavigationRate(). Returns the ratio directly with no way to distinguish failure from a real value. */
  uint16_t getNavigationRate(uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                       // Unsafe overload

  // Helper functions for DOP
  // For safety, call these inside an if(getNAVDOP()) or if(getUBX("NAV","DOP"))

  /**
   * @brief Get fresh UBX-NAV-DOP (dilution of precision) data. Call the getXxxDOP() field accessors below
   *        only after this returns true, or inside if(getUBX("NAV","DOP")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-DOP data is now available.
   */
  bool getNAVDOP(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get the geometric dilution of precision from the last getNAVDOP(), * 10^-2 (dimensionless). */
  uint16_t getGeometricDOP();
  /** @brief Get the positional dilution of precision from the last getNAVDOP(), * 10^-2 (dimensionless). */
  uint16_t getPositionDOP();
  /** @brief Get the time dilution of precision from the last getNAVDOP(), * 10^-2 (dimensionless). */
  uint16_t getTimeDOP();
  /** @brief Get the vertical dilution of precision from the last getNAVDOP(), * 10^-2 (dimensionless). */
  uint16_t getVerticalDOP();
  /** @brief Get the horizontal dilution of precision from the last getNAVDOP(), * 10^-2 (dimensionless). */
  uint16_t getHorizontalDOP();
  /** @brief Get the northing dilution of precision from the last getNAVDOP(), * 10^-2 (dimensionless). */
  uint16_t getNorthingDOP();
  /** @brief Get the easting dilution of precision from the last getNAVDOP(), * 10^-2 (dimensionless). */
  uint16_t getEastingDOP();

  // Helper functions for ATT
  // For safety, call these inside an if(getNAVATT()) or if(getUBX("NAV","ATT"))

  /**
   * @brief Get fresh UBX-NAV-ATT (vehicle attitude solution) data. Call the getATTxxx() field accessors
   *        below only after this returns true, or inside if(getUBX("NAV","ATT")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-ATT data is now available.
   */
  bool getNAVATT(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get vehicle roll from the last getNAVATT(), degrees. */
  float getATTroll();    // Returned as degrees
  /** @brief Get vehicle pitch from the last getNAVATT(), degrees. */
  float getATTpitch();   // Returned as degrees
  /** @brief Get vehicle heading from the last getNAVATT(), degrees. */
  float getATTheading(); // Returned as degrees

  // Helper functions for PVT
  // For safety, call these inside an if (getNAVPVT()) or if(getUBX("NAV","PVT"))

  /**
   * @brief Get fresh UBX-NAV-PVT (navigation position/velocity/time solution) data. Call the field
   *        accessors below only after this returns true, or inside if(getUBX("NAV","PVT")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-PVT data is now available.
   */
  bool getNAVPVT(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get the GNSS time of week from the last getNAVPVT(), milliseconds. */
  uint32_t getTimeOfWeek();
  /** @brief Get the UTC year from the last getNAVPVT(). */
  uint16_t getYear();
  /** @brief Get the UTC month (1-12) from the last getNAVPVT(). */
  uint8_t getMonth();
  /** @brief Get the UTC day of month (1-31) from the last getNAVPVT(). */
  uint8_t getDay();
  /** @brief Get the UTC hour (0-23) from the last getNAVPVT(). */
  uint8_t getHour();
  /** @brief Get the UTC minute (0-59) from the last getNAVPVT(). */
  uint8_t getMinute();
  /** @brief Get the UTC second (0-60) from the last getNAVPVT(). */
  uint8_t getSecond();
  /** @brief Get the UTC millisecond fraction of the second from the last getNAVPVT(), rounded from getNanosecond(). */
  uint16_t getMillisecond();
  /** @brief Get the UTC nanosecond fraction of the second from the last getNAVPVT() (may be negative). */
  int32_t getNanosecond();
  /** @brief Convert the last getNAVPVT()'s UTC date/time to a Unix epoch timestamp, seconds. */
  uint32_t getUnixEpoch();
  /**
   * @brief As getUnixEpoch(), but also returns the sub-second remainder.
   *
   * @param microsecond Filled in with the microsecond fraction of the second.
   * @return The Unix epoch timestamp, seconds.
   */
  uint32_t getUnixEpoch(uint32_t &microsecond);

  /** @brief Get whether the UTC date from the last getNAVPVT() is valid. */
  bool getDateValid();
  /** @brief Get whether the UTC time of day from the last getNAVPVT() is valid. */
  bool getTimeValid();
  /** @brief Get whether the UTC time from the last getNAVPVT() has been fully resolved (no seconds ambiguity). */
  bool getTimeFullyResolved();
  /** @brief Get whether the UTC date from the last getNAVPVT() has been independently confirmed. */
  bool getConfirmedDate();
  /** @brief Get whether the UTC time from the last getNAVPVT() has been independently confirmed. */
  bool getConfirmedTime();

  /** @brief Get the fix type from the last getNAVPVT(): 0 = no fix, 3 = 3D, 4 = GNSS + dead reckoning, etc. */
  uint8_t getFixType(); // Returns the type of fix: 0=no, 3=3D, 4=GNSS+Deadreckoning

  /** @brief Get whether the last getNAVPVT() fix is valid (within the configured DOP and accuracy masks). */
  bool getGnssFixOk(); // Get whether we have a valid fix (i.e within DOP & accuracy masks)
  /** @brief Get whether differential (DGNSS) corrections were applied to the last getNAVPVT() fix. */
  bool getDiffSoln();  // Get whether differential corrections were applied
  /** @brief Get whether the vehicle heading (getHeadVeh()) from the last getNAVPVT() is valid. */
  bool getHeadVehValid();
  /** @brief Get the RTK carrier-phase solution type from the last getNAVPVT(): 0 = none, 1 = float, 2 = fixed. */
  uint8_t getCarrierSolutionType(); // Returns RTK solution: 0=no, 1=float solution, 2=fixed solution

  /** @brief Get the number of satellites used in the last getNAVPVT() fix. */
  uint8_t getSIV();         // Returns number of sats used in fix
  /** @brief Get longitude from the last getNAVPVT(), degrees * 1e-7. Uses the high-precision fields automatically when the module supports them. */
  int32_t getLongitude();   // Returns the current longitude in degrees * 10-7. Auto selects between HighPrecision and Regular depending on ability of module.
  /** @brief Get latitude from the last getNAVPVT(), degrees * 1e-7. Uses the high-precision fields automatically when the module supports them. */
  int32_t getLatitude();    // Returns the current latitude in degrees * 10^-7. Auto selects between HighPrecision and Regular depending on ability of module.
  /** @brief Get altitude above the WGS84 ellipsoid from the last getNAVPVT(), mm. */
  int32_t getAltitude();    // Returns the current altitude in mm above ellipsoid
  /** @brief Get altitude above mean sea level from the last getNAVPVT(), mm. */
  int32_t getAltitudeMSL(); // Returns the current altitude in mm above mean sea level
  /** @brief Get the horizontal accuracy estimate from the last getNAVPVT(), mm. */
  uint32_t getHorizontalAccEst();
  /** @brief Get the vertical accuracy estimate from the last getNAVPVT(), mm. */
  uint32_t getVerticalAccEst();
  /** @brief Get the NED-frame north velocity from the last getNAVPVT(), mm/s. */
  int32_t getNedNorthVel();
  /** @brief Get the NED-frame east velocity from the last getNAVPVT(), mm/s. */
  int32_t getNedEastVel();
  /** @brief Get the NED-frame down velocity from the last getNAVPVT(), mm/s. */
  int32_t getNedDownVel();
  /** @brief Get 2D ground speed from the last getNAVPVT(), mm/s. */
  int32_t getGroundSpeed(); // Returns speed in mm/s
  /** @brief Get heading of motion (2D) from the last getNAVPVT(), degrees * 1e-5. */
  int32_t getHeading();     // Returns heading in degrees * 10^-5
  /** @brief Get the speed accuracy estimate from the last getNAVPVT(), mm/s. */
  uint32_t getSpeedAccEst();
  /** @brief Get the heading accuracy estimate from the last getNAVPVT(), degrees * 1e-5. */
  uint32_t getHeadingAccEst();
  /** @brief Get positional dilution of precision from the last getNAVPVT(), * 10^-2 (dimensionless). */
  uint16_t getPDOP(); // Returns positional dillution of precision * 10^-2 (dimensionless)

  /** @brief Get whether the lat/lon/height in the last getNAVPVT() is flagged invalid. */
  bool getInvalidLlh();

  /** @brief Get vehicle heading (2D) from the last getNAVPVT(), degrees * 1e-5; valid only when getHeadVehValid() is true. */
  int32_t getHeadVeh();
  /** @brief Get the magnetic declination from the last getNAVPVT(), degrees * 1e-2. */
  int16_t getMagDec();
  /** @brief Get the magnetic declination accuracy from the last getNAVPVT(), degrees * 1e-2. */
  uint16_t getMagAcc();

  /** @brief Get the geoid separation (difference between ellipsoid and mean-sea-level height) from the last getNAVPVT(), mm. */
  int32_t getGeoidSeparation();

  // Helper functions for POSECEF
  // For safety, call these inside an if(getNAVPOSECEF()) or if(getUBX("NAV","POSECEF"))

  /**
   * @brief Get fresh UBX-NAV-POSECEF (ECEF position solution) data. Call the field accessors below only
   *        after this returns true, or inside if(getUBX("NAV","POSECEF")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-POSECEF data is now available.
   */
  bool getNAVPOSECEF(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get the position accuracy estimate from the last getNAVPOSECEF(), mm. */
  uint32_t getPositionAccuracyPOSECEF(); // Returns the position accuracy estimate of the current POSECEF solution, in mm (not cm)

  // Helper functions for HPPOSECEF
  // For safety, call these inside an if(getNAVHPPOSECEF()) or if(getUBX("NAV","HPPOSECEF"))

  /**
   * @brief Get fresh UBX-NAV-HPPOSECEF (high-precision ECEF position solution) data. Call the field
   *        accessors below only after this returns true, or inside if(getUBX("NAV","HPPOSECEF")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-HPPOSECEF data is now available.
   */
  bool getNAVHPPOSECEF(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get the 3D position accuracy estimate from the last getNAVHPPOSECEF(), mm. Supported on NEO-M8P, ZED-F9P, etc. */
  uint32_t getPositionAccuracy(); // Returns the 3D accuracy of the current high-precision fix, in mm. Supported on NEO-M8P, ZED-F9P,
  /** @brief Get the ECEF X coordinate from the last getNAVHPPOSECEF(), cm. Combine with getHighResECEFXHp() for full precision. */
  int32_t getHighResECEFX();      // Returns the ECEF X coordinate (cm)
  /** @brief Get the ECEF Y coordinate from the last getNAVHPPOSECEF(), cm. Combine with getHighResECEFYHp() for full precision. */
  int32_t getHighResECEFY();      // Returns the ECEF Y coordinate (cm)
  /** @brief Get the ECEF Z coordinate from the last getNAVHPPOSECEF(), cm. Combine with getHighResECEFZHp() for full precision. */
  int32_t getHighResECEFZ();      // Returns the ECEF Z coordinate (cm)
  /** @brief Get the high-precision extension of getHighResECEFX() from the last getNAVHPPOSECEF(), 0.1 mm. */
  int8_t getHighResECEFXHp();     // Returns the ECEF X coordinate High Precision Component (0.1 mm)
  /** @brief Get the high-precision extension of getHighResECEFY() from the last getNAVHPPOSECEF(), 0.1 mm. */
  int8_t getHighResECEFYHp();     // Returns the ECEF Y coordinate High Precision Component (0.1 mm)
  /** @brief Get the high-precision extension of getHighResECEFZ() from the last getNAVHPPOSECEF(), 0.1 mm. */
  int8_t getHighResECEFZHp();     // Returns the ECEF Z coordinate High Precision Component (0.1 mm)

  // Helper functions for HPPOSLLH
  // For safety, call these inside an if(getNAVHPPOSLLH()) or if(getUBX("NAV","HPPOSLLH"))

  /**
   * @brief Get fresh UBX-NAV-HPPOSLLH (high-precision geodetic position solution) data. Call the field
   *        accessors below only after this returns true, or inside if(getUBX("NAV","HPPOSLLH")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-HPPOSLLH data is now available.
   */
  bool getNAVHPPOSLLH(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get the GNSS time of week from the last getNAVHPPOSLLH(), milliseconds. */
  uint32_t getTimeOfWeekFromHPPOSLLH();
  /** @brief Get longitude from the last getNAVHPPOSLLH(), degrees * 1e-7. Combine with getHighResLongitudeHp() for full precision. */
  int32_t getHighResLongitude();
  /** @brief Get latitude from the last getNAVHPPOSLLH(), degrees * 1e-7. Combine with getHighResLatitudeHp() for full precision. */
  int32_t getHighResLatitude();
  /** @brief Get altitude above the WGS84 ellipsoid from the last getNAVHPPOSLLH(), mm. Combine with getElipsoidHp() for full precision. */
  int32_t getElipsoid();
  /** @brief Get altitude above mean sea level from the last getNAVHPPOSLLH(), mm. Combine with getMeanSeaLevelHp() for full precision. */
  int32_t getMeanSeaLevel();
  /** @brief Get the high-precision extension of getHighResLongitude() from the last getNAVHPPOSLLH(), degrees * 1e-9. */
  int8_t getHighResLongitudeHp();
  /** @brief Get the high-precision extension of getHighResLatitude() from the last getNAVHPPOSLLH(), degrees * 1e-9. */
  int8_t getHighResLatitudeHp();
  /** @brief Get the high-precision extension of getElipsoid() from the last getNAVHPPOSLLH(), 0.1 mm. */
  int8_t getElipsoidHp();
  /** @brief Get the high-precision extension of getMeanSeaLevel() from the last getNAVHPPOSLLH(), 0.1 mm. */
  int8_t getMeanSeaLevelHp();
  /** @brief Get the horizontal accuracy estimate from the last getNAVHPPOSLLH(), 0.1 mm. */
  uint32_t getHorizontalAccuracy();
  /** @brief Get the vertical accuracy estimate from the last getNAVHPPOSLLH(), 0.1 mm. */
  uint32_t getVerticalAccuracy();

  // Helper functions for PVAT
  // For safety, call these inside an if(getNAVPVAT()) or if(getUBX("NAV","PVAT"))

  /**
   * @brief Get fresh UBX-NAV-PVAT (navigation position/velocity/attitude/time solution, ZED-F9R only)
   *        data. Call the field accessors below only after this returns true, or inside
   *        if(getUBX("NAV","PVAT")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-PVAT data is now available.
   */
  bool getNAVPVAT(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get vehicle roll from the last getNAVPVAT(), degrees * 1e-5. */
  int32_t getVehicleRoll();    // Returns vehicle roll in degrees * 10^-5
  /** @brief Get vehicle pitch from the last getNAVPVAT(), degrees * 1e-5. */
  int32_t getVehiclePitch();   // Returns vehicle pitch in degrees * 10^-5
  /** @brief Get vehicle heading from the last getNAVPVAT(), degrees * 1e-5. */
  int32_t getVehicleHeading(); // Returns vehicle heading in degrees * 10^-5
  /** @brief Get heading of motion from the last getNAVPVAT(), degrees * 1e-5. */
  int32_t getMotionHeading();  // Returns the motion heading in degrees * 10^-5

  // Helper functions for SVIN
  // For safety, call these inside an if(getNAVSVIN()) or if(getUBX("NAV","SVIN"))

  /**
   * @brief Get fresh UBX-NAV-SVIN (survey-in status) data. Call the field accessors below only after
   *        this returns true, or inside if(getUBX("NAV","SVIN")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-SVIN data is now available.
   */
  bool getNAVSVIN(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get whether Survey-In is currently active, from the last getNAVSVIN(). */
  bool getSurveyInActive();
  /** @brief Get whether the Survey-In result is valid (accuracy/time requirements met), from the last getNAVSVIN(). */
  bool getSurveyInValid();
  /** @brief Get elapsed Survey-In observation time from the last getNAVSVIN(), seconds, truncated to 65535. */
  uint16_t getSurveyInObservationTime();     // Truncated to 65535 seconds
  /** @brief As getSurveyInObservationTime(), but returns the full untruncated 32-bit value. */
  uint32_t getSurveyInObservationTimeFull(); // Return the full uint32_t
  /** @brief Get the Survey-In mean position accuracy from the last getNAVSVIN(), metres. */
  float getSurveyInMeanAccuracy();           // Returned as m

  // Helper functions for TIMELS
  // For safety, call these inside an if(getNAVTIMELS()) or if(getUBX("NAV","TIMELS"))

  /**
   * @brief Get fresh UBX-NAV-TIMELS (leap second event information) data. Call the field accessors below
   *        only after this returns true, or inside if(getUBX("NAV","TIMELS")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-TIMELS data is now available.
   */
  bool getNAVTIMELS(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get the time to the next/last leap second event from the last getNAVTIMELS(), seconds. */
  int32_t getTimeToLsEvent();
  /** @brief Get the current GPS-UTC leap second offset from the last getNAVTIMELS(). */
  int8_t getCurrentLeapSeconds();

  // Helper functions for RELPOSNED
  // For safety, call these inside an if(getNAVRELPOSNED()) or if(getUBX("NAV","RELPOSNED"))

  /**
   * @brief Get fresh UBX-NAV-RELPOSNED (relative positioning in the NED frame) data. Call the field
   *        accessors below only after this returns true, or inside if(getUBX("NAV","RELPOSNED")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-RELPOSNED data is now available.
   */
  bool getNAVRELPOSNED(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get the relative-position north component from the last getNAVRELPOSNED(), metres. */
  float getRelPosN();    // Returned as m
  /** @brief Get the relative-position east component from the last getNAVRELPOSNED(), metres. */
  float getRelPosE();    // Returned as m
  /** @brief Get the relative-position down component from the last getNAVRELPOSNED(), metres. */
  float getRelPosD();    // Returned as m
  /** @brief Get the accuracy estimate of getRelPosN() from the last getNAVRELPOSNED(), metres. */
  float getRelPosAccN(); // Returned as m
  /** @brief Get the accuracy estimate of getRelPosE() from the last getNAVRELPOSNED(), metres. */
  float getRelPosAccE(); // Returned as m
  /** @brief Get the accuracy estimate of getRelPosD() from the last getNAVRELPOSNED(), metres. */
  float getRelPosAccD(); // Returned as m

  // Helper functions for DAHEADING
  // For safety, call these inside an if(getNAVDAHEADING()) or if(getUBX("NAV","DAHEADING"))

  /**
   * @brief Get fresh UBX-NAV-RELPOSNED dual-antenna heading data (relative NED position between antenna 1
   *        and antenna 2). Call the field accessors below only after this returns true, or inside
   *        if(getUBX("NAV","DAHEADING")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh dual-antenna heading data is now available.
   */
  bool getNAVDAHEADING(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get the antenna-1-to-antenna-2 relative-position north component from the last getNAVDAHEADING(), metres. */
  float getDAHeadingRelPosN();    // Returned as m
  /** @brief Get the antenna-1-to-antenna-2 relative-position east component from the last getNAVDAHEADING(), metres. */
  float getDAHeadingRelPosE();    // Returned as m
  /** @brief Get the antenna-1-to-antenna-2 relative-position down component from the last getNAVDAHEADING(), metres. */
  float getDAHeadingRelPosD();    // Returned as m
  /** @brief Get the accuracy estimate of getDAHeadingRelPosN() from the last getNAVDAHEADING(), metres. */
  float getDAHeadingRelPosAccN(); // Returned as m
  /** @brief Get the accuracy estimate of getDAHeadingRelPosE() from the last getNAVDAHEADING(), metres. */
  float getDAHeadingRelPosAccE(); // Returned as m
  /** @brief Get the accuracy estimate of getDAHeadingRelPosD() from the last getNAVDAHEADING(), metres. */
  float getDAHeadingRelPosAccD(); // Returned as m

  // Helper functions for AOPSTATUS
  // For safety, call these inside an if(getNAVAOPSTATUS()) or if(getUBX("NAV","AOPSTATUS"))

  /**
   * @brief Get fresh UBX-NAV-AOPSTATUS (AssistNow Autonomous status) data. Call the field accessors below
   *        only after this returns true, or inside if(getUBX("NAV","AOPSTATUS")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh NAV-AOPSTATUS data is now available.
   */
  bool getNAVAOPSTATUS(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get the useAOP flag from the last getNAVAOPSTATUS(). Not to be confused with getAopCfg(), which reads UBX-CFG-NAVX5's aopCfg byte instead. */
  uint8_t getAOPSTATUSuseAOP(); // Returns the UBX-NAV-AOPSTATUS useAOP flag. Don't confuse this with getAopCfg - which returns the aopCfg byte from UBX-CFG-NAVX5
  /** @brief Get the status field from the last getNAVAOPSTATUS(). A host can pick the optimal shutdown moment by watching for this to settle at a steady 0. */
  uint8_t getAOPSTATUSstatus(); // Returns the UBX-NAV-AOPSTATUS status field. A host application can determine the optimal time to shut down the receiver by monitoring the status field for a steady 0.

  // Helper functions for TIM TP
  // For safety, call these inside an if(getTIMTP()) or if(getUBX("TIM","TP"))

  /**
   * @brief Get fresh UBX-TIM-TP (time pulse timing) data. Call the field accessors below only after this
   *        returns true, or inside if(getUBX("TIM","TP")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh TIM-TP data is now available.
   */
  bool getTIMTP(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get the time pulse's time of week from the last getTIMTP(), milliseconds. */
  uint32_t getTIMTPtowMS();                          // Returns the UBX-TIM-TP towMS time pulse of week (ms)
  /** @brief Get the sub-millisecond fraction of getTIMTPtowMS() from the last getTIMTP(), ms * 2^-32. */
  uint32_t getTIMTPtowSubMS();                       // Returns the UBX-TIM-TP submillisecond part of towMS (ms * 2^-32)
  /** @brief Get the time pulse's week number from the last getTIMTP(), per the configured time base. */
  uint16_t getTIMTPweek();                           // Returns the UBX-TIM-TP time pulse week according to time base
  /**
   * @brief Convert the last getTIMTP()'s time pulse timestamp to a Unix epoch timestamp. Caution: assumes
   *        the time base is UTC and the week number is GPS.
   *
   * @param microsecond Filled in with the microsecond fraction of the second.
   * @return The Unix epoch timestamp, seconds.
   */
  uint32_t getTIMTPAsEpoch(uint32_t &microsecond); // Convert TIM TP to Unix Epoch - CAUTION! Assumes the time base is UTC and the week number is GPS

  // Helper function for hardware status (including jamming)
  // For safety, call getAntennaStatus inside an if(getMONHW()) or if(getUBX("MON","HW"))

  /**
   * @brief Get fresh UBX-MON-HW (hardware status) data. For safety, call getAntennaStatus() only after
   *        this returns true, or inside if(getUBX("MON","HW")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh MON-HW data is now available.
   */
  bool getMONHW(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get the antenna status. Reads UBX-MON-RF's aStatus field internally; UBX-MON-HW's own antenna status is mostly deprecated. */
  sfe_ublox_antenna_status_e getAntennaStatus(); // Get the antenna status (aStatus) using UBX_MON_RF. MON-HW is mostly deprecated.

  // Helper functions for ESF
  // For safety, call getESFroll/pitch/yaw inside an if(getESFALG()) or if(getUBX("ESF","ALG"))

  /**
   * @brief Get fresh UBX-ESF-ALG (IMU alignment) data. Call getESFroll()/getESFpitch()/getESFyaw() only
   *        after this returns true, or inside if(getUBX("ESF","ALG")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh ESF-ALG data is now available.
   */
  bool getESFALG(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /** @brief Get the IMU alignment roll angle from the last getESFALG(), degrees. */
  float getESFroll();  // Returned as degrees
  /** @brief Get the IMU alignment pitch angle from the last getESFALG(), degrees. */
  float getESFpitch(); // Returned as degrees
  /** @brief Get the IMU alignment yaw angle from the last getESFALG(), degrees. */
  float getESFyaw();   // Returned as degrees
  // getSensorFusionMeasurement() is redacted, per explicit instruction - see AGENTS.md "Adding
  // support for ESF-MEAS". It took its data struct by value from the caller (the old v3-style
  // UBX_ESF_MEAS_data_t), but nothing constructs one of those to pass it any more now that
  // ESF-MEAS's storage path is the generic registry (ubxESFMEAS). The generic
  // getUbxMessageBlockField()/getUbxMessageBlockFieldCallback() (with getUbxMessageBlockCount()/
  // ...Callback() to bound the loop) cover the same ground.

  // getRawSensorMeasurement() and both overloads of getSensorFusionStatus() are redacted, per
  // explicit instruction - see AGENTS.md "Adding support for ESF-RAW and ESF-STATUS". They took
  // their data structs by value/pointer from the caller (the old v3-style UBX_ESF_RAW_data_t/
  // UBX_ESF_STATUS_data_t), but nothing constructs one of those to pass any more now that
  // ESF-RAW's/ESF-STATUS's storage paths are the generic registry (ubxESFRAW/ubxESFSTATUS). The
  // generic getUbxMessageBlockField()/getUbxMessageBlockFieldCallback() (with
  // getUbxMessageBlockCount()/...Callback() to bound the loop) cover the same ground.

  // Helper functions for HNR
  // For safety, call getHNRroll/pitch/yaw inside an if(getHNRATT()) or if(getUBX("HNR","ATT"))

  /**
   * @brief Get fresh UBX-HNR-ATT (high rate attitude) data. Call getHNRroll()/getHNRpitch()/
   *        getHNRheading() only after this returns true, or inside if(getUBX("HNR","ATT")).
   *
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh HNR-ATT data is now available.
   */
  bool getHNRATT(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  /**
   * @brief Set the High Navigation Rate (HNR) output rate (UBX-CFG-HNR).
   *
   * @param rate HNR solutions per second.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the rate was set successfully.
   */
  bool setHNRNavigationRate(uint8_t rate, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Returns true if the setHNRNavigationRate is successful
  /**
   * @brief Get the currently configured HNR output rate.
   *
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return The HNR rate, or 0 if the query failed.
   */
  uint8_t getHNRNavigationRate(uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);                // Returns 0 if the getHNRNavigationRate fails
  /** @brief Get vehicle roll from the last getHNRATT(), degrees. */
  float getHNRroll();                                                           // Returned as degrees
  /** @brief Get vehicle pitch from the last getHNRATT(), degrees. */
  float getHNRpitch();                                                          // Returned as degrees
  /** @brief Get vehicle heading from the last getHNRATT(), degrees. */
  float getHNRheading();                                                        // Returned as degrees

  // Helper functions for the remaining registered ubxMessages (thin wrappers only - see
  // AGENTS.md "getUBX()"). None of these has per-field convenience getters yet; call
  // getUBXfield(Class, ID, "fieldName", &value) directly, or use getUBX("Class","ID") /
  // findByName(), to read individual fields.

  /** @brief Get fresh UBX-NAV-POSLLH (geodetic position solution) data. Thin wrapper around getUBX("NAV","POSLLH",...); read fields with getUBXfield(). */
  bool getNAVPOSLLH(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);  // NAV-POSLLH: Geodetic position solution
  /** @brief Get fresh UBX-NAV-STATUS (receiver navigation status) data. Thin wrapper around getUBX("NAV","STATUS",...); read fields with getUBXfield(). */
  bool getNAVSTATUS(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);  // NAV-STATUS: Receiver navigation status
  /** @brief Get fresh UBX-NAV-ODO (odometer solution) data. Thin wrapper around getUBX("NAV","ODO",...); read fields with getUBXfield(). */
  bool getNAVODO(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);     // NAV-ODO: Odometer solution
  /** @brief Get fresh UBX-NAV-VELECEF (velocity solution in ECEF) data. Thin wrapper around getUBX("NAV","VELECEF",...); read fields with getUBXfield(). */
  bool getNAVVELECEF(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // NAV-VELECEF: Velocity solution in ECEF
  /** @brief Get fresh UBX-NAV-VELNED (velocity solution in NED frame) data. Thin wrapper around getUBX("NAV","VELNED",...); read fields with getUBXfield(). */
  bool getNAVVELNED(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);  // NAV-VELNED: Velocity solution in NED frame
  /** @brief Get fresh UBX-NAV-TIMEUTC (UTC time solution) data. Thin wrapper around getUBX("NAV","TIMEUTC",...); read fields with getUBXfield(). */
  bool getNAVTIMEUTC(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // NAV-TIMEUTC: UTC time solution
  /** @brief Get fresh UBX-NAV-CLOCK (clock solution) data. Thin wrapper around getUBX("NAV","CLOCK",...); read fields with getUBXfield(). */
  bool getNAVCLOCK(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);   // NAV-CLOCK: Clock solution
  /** @brief Get fresh UBX-NAV-EOE (end-of-epoch marker) data. Thin wrapper around getUBX("NAV","EOE",...); read fields with getUBXfield(). */
  bool getNAVEOE(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);     // NAV-EOE: End of epoch marker
  /** @brief Get fresh UBX-RXM-COR (differential correction input status) data. Thin wrapper around getUBX("RXM","COR",...); read fields with getUBXfield(). */
  bool getRXMCOR(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);     // RXM-COR: Differential correction input status
  /** @brief Get fresh UBX-MON-HW2 (extended hardware status) data. Thin wrapper around getUBX("MON","HW2",...); read fields with getUBXfield(). */
  bool getMONHW2(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);     // MON-HW2: Extended hardware status
  /** @brief Get fresh UBX-TIM-TM2 (time mark data) data. Thin wrapper around getUBX("TIM","TM2",...); read fields with getUBXfield(). */
  bool getTIMTM2(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);     // TIM-TM2: Time mark data
  /** @brief Get fresh UBX-ESF-INS (vehicle dynamics information) data. Thin wrapper around getUBX("ESF","INS",...); read fields with getUBXfield(). */
  bool getESFINS(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);     // ESF-INS: Vehicle dynamics information
  /** @brief Get fresh UBX-HNR-PVT (high rate PVT solution) data. Thin wrapper around getUBX("HNR","PVT",...); read fields with getUBXfield(). */
  bool getHNRPVT(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);     // HNR-PVT: High rate output of PVT solution
  /** @brief Get fresh UBX-HNR-INS (high rate inertial solution) data. Thin wrapper around getUBX("HNR","INS",...); read fields with getUBXfield(). */
  bool getHNRINS(uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);     // HNR-INS: High rate inertial solution

  // Helper functions for the NEO-F10N
  /**
   * @brief Get the LNA (low-noise amplifier) operating mode (NEO-F10N).
   *
   * @param mode Filled in with the current LNA mode on success.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the mode was read successfully.
   */
  bool getLNAMode(sfe_ublox_lna_mode_e *mode, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Get the LNA mode
  /** @brief As getLNAMode(), but writes the given LNA mode instead of reading it. */
  bool setLNAMode(sfe_ublox_lna_mode_e mode, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Set the LNA mode
  /**
   * @brief Get whether the GPS L5 signal health override is enabled (NEO-F10N).
   *
   * @param override Filled in with true/false on success.
   * @param layer Which configuration layer to read from. Defaults to VAL_LAYER_RAM.
   * @param maxWait Milliseconds to wait for the response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the query itself succeeded (see override for the actual state).
   */
  bool getGPSL5HealthOverride(bool *override, uint8_t layer = VAL_LAYER_RAM, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Get the GPS L5 health override status
  /** @brief As getGPSL5HealthOverride(), but writes the given override state instead of reading it. */
  bool setGPSL5HealthOverride(bool override, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Set the GPS L5 health override status

  // Set the mainTalkerId used by NMEA messages - allows all NMEA messages except GSV to be prefixed with GP instead of GN
  /**
   * @brief Set the main NMEA talker ID. Lets all NMEA sentences except GSV be prefixed with GP instead
   *        of the default GN.
   *
   * @param id The talker ID to use. Defaults to SFE_UBLOX_MAIN_TALKER_ID_DEFAULT.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the setting was written successfully.
   */
  bool setMainTalkerID(sfe_ublox_talker_ids_e id = SFE_UBLOX_MAIN_TALKER_ID_DEFAULT, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  // Enable/Disable NMEA High Precision Mode - include extra decimal places in the Lat and Lon
  /**
   * @brief Enable or disable NMEA High Precision Mode, which adds extra decimal places to the lat/lon
   *        fields of NMEA position sentences.
   *
   * @param enable True to enable, false to disable. Defaults to true.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the setting was written successfully.
   */
  bool setHighPrecisionMode(bool enable = true, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);

  // NMEA

  // Helper functions for NMEA logging
  /**
   * @brief Select which NMEA sentence types are written to the file buffer (when file logging is enabled).
   *
   * @param messages Bitmask of SFE_UBLOX_FILTER_NMEA_* sentence types. Defaults to SFE_UBLOX_FILTER_NMEA_ALL.
   */
  void setNMEALoggingMask(uint32_t messages = SFE_UBLOX_FILTER_NMEA_ALL); // Add selected NMEA messages to file buffer - if enabled. Default to adding ALL messages to the file buffer
  /** @brief Get the bitmask of NMEA sentence types currently selected for file-buffer logging. */
  uint32_t getNMEALoggingMask();                                          // Return which NMEA messages are selected for logging to the file buffer - if enabled

  // Helper functions to control which NMEA messages are passed to processNMEA
  /**
   * @brief Select which NMEA sentence types are passed to processNMEA() for parsing.
   *
   * @param messages Bitmask of SFE_UBLOX_FILTER_NMEA_* sentence types. Defaults to SFE_UBLOX_FILTER_NMEA_ALL.
   */
  void setProcessNMEAMask(uint32_t messages = SFE_UBLOX_FILTER_NMEA_ALL); // Control which NMEA messages are passed to processNMEA. Default to passing ALL messages
  /** @brief Get the bitmask of NMEA sentence types currently passed to processNMEA(). */
  uint32_t getProcessNMEAMask();                                          // Return which NMEA messages are passed to processNMEA

  // ***** v4 scaffolding - generic (Class, ID)-keyed message access. See AGENTS.md "Reference Scaffolding" *****
  /**
   * @brief Recover the opaque per-message nmeaMessage object that owns a given callback's data.
   *
   * @param theData The nmeaCallbackDataCommon_t* passed into a v4 NMEA message callback.
   * @return Pointer to the owning nmeaMessage, for use with the other getNmeaMessage*() accessors.
   */
  nmeaMessage *getNmeaMessagePtr(nmeaCallbackDataCommon_t *theData); // Factory: hands back the opaque per-message object a callback's nmeaCallbackDataCommon_t* points at
  /**
   * @brief Extract a named field from the NMEA message a callback just fired for, reading its
   *        callback-time snapshot (_callbackStorage).
   *
   * @param theMessage The message object, as returned by getNmeaMessagePtr().
   * @param fieldName The field's name, as declared in the message's field table.
   * @return The field's value as a String, or an empty String if fieldName is unknown.
   */
  sfe_string_t getNmeaMessageFieldCallback(nmeaMessage *theMessage, const char *fieldName); // Factory: extracts a named field from the message a callback just fired for, reading from its _callbackStorage
  /** @brief As getNmeaMessageFieldCallback(), but reads the message's live _storage instead of a callback-time snapshot. */
  sfe_string_t getNmeaMessageField(nmeaMessage *theMessage, const char *fieldName); // Factory: extracts a named field from the message, reading from its _storage
  /**
   * @brief Extract a named field from one repeated block of a variable-length NMEA message (e.g. one
   *        satellite entry of GSV), reading the callback-time snapshot.
   *
   * @param theMessage The message object, as returned by getNmeaMessagePtr().
   * @param blockIndex Which repeated block to read (0-based).
   * @param fieldName The block field's name, as declared in the message's block field table.
   * @return The field's value as a String, or an empty String if fieldName is unknown.
   */
  sfe_string_t getNmeaMessageBlockFieldCallback(nmeaMessage *theMessage, uint16_t blockIndex, const char *fieldName); // Factory: extracts a named field from repeated block 'blockIndex' of a variable-length message (e.g. GSV), reading from its _callbackStorage
  /** @brief As getNmeaMessageBlockFieldCallback(), but reads the message's live _storage instead of a callback-time snapshot. */
  sfe_string_t getNmeaMessageBlockField(nmeaMessage *theMessage, uint16_t blockIndex, const char *fieldName); // Factory: extracts a named field from repeated block 'blockIndex' of a variable-length message (e.g. GSV), reading from its _storage

  nmeaMessageVector nmeaMessages; // v4 scaffolding - the registry of per-message objects

  /**
   * @brief Get fresh data for any registered NMEA message, by sentence ID. If the message is set to
   *        automatic, checks (non-blocking) whether new data has arrived; otherwise sends an explicit poll
   *        and waits for the response.
   *
   * @param msgId The NMEA sentence ID, e.g. "GGA".
   * @param maxWait Milliseconds to wait for a polled response. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if fresh data is now available.
   */
  bool getNMEA(const char *msgId, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait); // Generic poll-or-check-automatic, by message name
  /**
   * @brief Read one named field of a registered NMEA message's live storage, by sentence ID/field name.
   *
   * @param msgId The NMEA sentence ID, e.g. "GGA".
   * @param field The field's name, as declared in the message's field table.
   * @param value Filled in with the field's value on success.
   * @return True if the message/field were found and read successfully.
   */
  bool getNMEAfield(const char *msgId, const char *field, sfe_string_t &value); // Generic field read, by message ID

  /**
   * @brief Enable or disable automatic (unsolicited) output of a registered NMEA message, by sentence ID.
   *
   * @param msgId The NMEA sentence ID, e.g. "GGA".
   * @param enabled True to enable automatic output, false to disable it. Defaults to true.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the setting was written successfully.
   */
  bool setAutoNMEA(const char *msgId, bool enabled = true, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /**
   * @brief As setAutoNMEA(const char*, bool, ...), but also controls whether the getter for this message
   *        implicitly re-polls when the auto data is stale.
   *
   * @param msgId The NMEA sentence ID, e.g. "GGA".
   * @param enabled True to enable automatic output, false to disable it.
   * @param implicitUpdate If true, the corresponding get*() call implicitly polls when no fresh automatic
   *                       data is available yet; if false, it returns false instead of blocking.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the setting was written successfully.
   */
  bool setAutoNMEA(const char *msgId, bool enabled, bool implicitUpdate, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /**
   * @brief Enable automatic output of a registered NMEA message at a reduced rate (every rate navigation
   *        epochs), by sentence ID.
   *
   * @param msgId The NMEA sentence ID, e.g. "GGA".
   * @param rate Output the sentence every rate navigation epochs (0 disables automatic output).
   * @param implicitUpdate If true, the corresponding get*() call implicitly polls when no fresh automatic
   *                       data is available yet; if false, it returns false instead of blocking.
   * @param layer Which configuration layer(s) to write to. Defaults to VAL_LAYER_RAM_BBR.
   * @param maxWait Milliseconds to wait for acknowledgement. Defaults to kUBLOXGNSSDefaultMaxWait.
   * @return True if the setting was written successfully.
   */
  bool setAutoNMEArate(const char *msgId, uint8_t rate, bool implicitUpdate, uint8_t layer = VAL_LAYER_RAM_BBR, uint16_t maxWait = kUBLOXGNSSDefaultMaxWait);
  /**
   * @brief Tell the library to treat an NMEA sentence as automatic without configuring the module, for
   *        when the module is already cyclically outputting it and config access is unavailable.
   *
   * @param msgId The NMEA sentence ID, e.g. "GGA".
   * @param enabled True to assume the sentence is being output automatically.
   * @param implicitUpdate If true, the corresponding get*() call implicitly polls when no fresh automatic
   *                       data is available yet. Defaults to true.
   * @return True if the internal state was updated successfully.
   */
  bool assumeAutoNMEA(const char *msgId, bool enabled, bool implicitUpdate = true);  // In case no config access to the GPS is possible and NMEA is send cyclically already
  /**
   * @brief Mark a registered NMEA message's stored data as stale/already-read, by sentence ID.
   *
   * @param msgId The NMEA sentence ID, e.g. "GGA".
   */
  void flushNMEA(const char *msgId); // Mark the NMEA data as read/stale
  /**
   * @brief Enable or disable logging a registered NMEA sentence's raw bytes to the file buffer, by sentence ID.
   *
   * @param msgId The NMEA sentence ID, e.g. "GGA".
   * @param enabled True to log this sentence, false to stop logging it. Defaults to true.
   */
  void logNMEA(const char *msgId, bool enabled = true); // Log data to file buffer
  // Generic replacement for the old per-message setNMEA<MSG>callbackPtr() functions
  /**
   * @brief Register a callback to be fired when fresh automatic data arrives for any registered NMEA
   *        message, by sentence ID. Generic replacement for the old per-message setNMEA<MSG>callbackPtr() functions.
   *
   * @param msgId The NMEA sentence ID, e.g. "GGA".
   * @param callbackPointerPtr Function to call with a nmeaCallbackDataCommon_t* when fresh data arrives.
   * @return True if the callback was registered successfully.
   */
  bool setNmeaCallbackPtr(const char *msgId, void (*callbackPointerPtr)(nmeaCallbackDataCommon_t *));

  // RTCM

  /**
   * @brief Get the most recently parsed outgoing RTCM 1005 (base station antenna reference point) message.
   *
   * @param data Filled in with the parsed RTCM 1005 fields.
   * @return 0 = no data yet, 1 = stale (already read), 2 = fresh data.
   */
  uint8_t getLatestRTCM1005(RTCM_1005_data_t *data);                           // Return the most recent RTCM 1005: 0 = no data, 1 = stale data, 2 = fresh data
  /**
   * @brief Register a callback to be fired when a fresh outgoing RTCM 1005 message is parsed.
   *
   * @param callbackPointerPtr Function to call with the parsed RTCM_1005_data_t.
   * @return True if the callback was registered successfully.
   */
  bool setRTCM1005callbackPtr(void (*callbackPointerPtr)(RTCM_1005_data_t *)); // Configure a callback for the RTCM 1005 Message

  /**
   * @brief As getLatestRTCM1005(), but for an RTCM 1005 message seen incoming via pushRawData() (e.g. from
   *        a base station) rather than one this module output itself.
   *
   * @param data Filled in with the parsed RTCM 1005 fields.
   * @return 0 = no data yet, 1 = stale (already read), 2 = fresh data.
   */
  uint8_t getLatestRTCM1005Input(RTCM_1005_data_t *data);                                // Return the most recent RTCM 1005 Input, extracted from pushRawData: 0 = no data, 1 = stale data, 2 = fresh data
  /**
   * @brief Register a callback to be fired when a fresh incoming RTCM 1005 message is parsed from pushRawData().
   *
   * @param rtcm1005CallbackPointer Function to call with the parsed RTCM_1005_data_t.
   */
  void setRTCM1005InputcallbackPtr(void (*rtcm1005CallbackPointer)(RTCM_1005_data_t *)); // Configure a callback for RTCM 1005 Input - from pushRawData
  /**
   * @brief As getLatestRTCM1005Input(), but for an incoming RTCM 1006 message (antenna reference point
   *        plus antenna height).
   *
   * @param data Filled in with the parsed RTCM 1006 fields.
   * @return 0 = no data yet, 1 = stale (already read), 2 = fresh data.
   */
  uint8_t getLatestRTCM1006Input(RTCM_1006_data_t *data);                                // Return the most recent RTCM 1006 Input, extracted from pushRawData: 0 = no data, 1 = stale data, 2 = fresh data
  /**
   * @brief Register a callback to be fired when a fresh incoming RTCM 1006 message is parsed from pushRawData().
   *
   * @param rtcm1006CallbackPointer Function to call with the parsed RTCM_1006_data_t.
   */
  void setRTCM1006InputcallbackPtr(void (*rtcm1006CallbackPointer)(RTCM_1006_data_t *)); // Configure a callback for RTCM 1006 Input - from pushRawData

  /**
   * @brief Parse a raw RTCM 1005 message body into its individual fields.
   *
   * @param destination Filled in with the parsed fields.
   * @param source Pointer to the raw RTCM 1005 message bytes.
   */
  void extractRTCM1005(RTCM_1005_data_t *destination, uint8_t *source); // Extract RTCM 1005 from source into destination
  /** @brief As extractRTCM1005(), but parses an RTCM 1006 message body instead. */
  void extractRTCM1006(RTCM_1006_data_t *destination, uint8_t *source); // Extract RTCM 1006 from source into destination

  // Helper functions for RTCM logging
  /**
   * @brief Select which RTCM message types are written to the file buffer (when file logging is enabled).
   *
   * @param messages Bitmask of SFE_UBLOX_FILTER_RTCM_* message types. Defaults to SFE_UBLOX_FILTER_RTCM_ALL.
   * @return True if the mask was set successfully.
   */
  bool setRTCMLoggingMask(uint32_t messages = SFE_UBLOX_FILTER_RTCM_ALL); // Add selected RTCM messages to file buffer - if enabled. Default to adding ALL messages to the file buffer
  /** @brief Get the bitmask of RTCM message types currently selected for file-buffer logging. */
  uint32_t getRTCMLoggingMask();                                          // Return which RTCM messages are selected for logging to the file buffer - if enabled

  // UBX Logging - log any UBX message using packetAuto and avoiding having to have and use "Auto" (setAutonnn and lognnn) methods
  /**
   * @brief Enable (or disable) logging/processing of any UBX message to the file buffer, by raw Class/ID,
   *        without needing per-message setAutoXxx()/logXxx() calls.
   *
   * @param UBX_CLASS The message's UBX class byte.
   * @param UBX_ID The message's UBX ID byte.
   * @param logMe True to write this message's bytes to the file buffer. Defaults to true.
   * @param processMe True to also invoke processUBX()'s user hook for this message. Defaults to false.
   */
  void enableUBXlogging(uint8_t UBX_CLASS, uint8_t UBX_ID, bool logMe = true, bool processMe = false);

  // Functions to extract signed and unsigned 8/16/32-bit data from a ubxPacket
  // From v2.0: These are public. The user can call these to extract data from custom packets
  /**
   * @brief Combine 8 little-endian bytes from a UBX packet's payload into a uint64_t.
   *
   * @param msg The packet to read from.
   * @param spotToStart Byte offset within msg->payload of the first (least-significant) byte.
   * @return The combined 64-bit unsigned value.
   */
  uint64_t extractLongLong(ubxPacket *msg, uint16_t spotToStart);      // Combine eight bytes from payload into uint64_t
  /** @brief As extractLongLong(), but reinterprets the combined bytes as a signed int64_t. */
  int64_t extractSignedLongLong(ubxPacket *msg, uint16_t spotToStart); // Combine eight bytes from payload into uint64_t
  /**
   * @brief Combine 4 little-endian bytes from a UBX packet's payload into a uint32_t.
   *
   * @param msg The packet to read from.
   * @param spotToStart Byte offset within msg->payload of the first (least-significant) byte.
   * @return The combined 32-bit unsigned value.
   */
  uint32_t extractLong(ubxPacket *msg, uint16_t spotToStart);          // Combine four bytes from payload into long
  /** @brief As extractLong(), but reinterprets the combined bytes as a signed int32_t (avoiding the ambiguity of a plain cast). */
  int32_t extractSignedLong(ubxPacket *msg, uint16_t spotToStart);     // Combine four bytes from payload into signed long (avoiding any ambiguity caused by casting)
  /**
   * @brief Combine 2 little-endian bytes from a UBX packet's payload into a uint16_t.
   *
   * @param msg The packet to read from.
   * @param spotToStart Byte offset within msg->payload of the first (least-significant) byte.
   * @return The combined 16-bit unsigned value.
   */
  uint16_t extractInt(ubxPacket *msg, uint16_t spotToStart);           // Combine two bytes from payload into int
  /** @brief As extractInt(), but reinterprets the combined bytes as a signed int16_t. */
  int16_t extractSignedInt(ubxPacket *msg, uint16_t spotToStart);
  /**
   * @brief Read a single byte from a UBX packet's payload.
   *
   * @param msg The packet to read from.
   * @param spotToStart Byte offset within msg->payload.
   * @return The byte value.
   */
  uint8_t extractByte(ubxPacket *msg, uint16_t spotToStart);      // Get byte from payload
  /** @brief As extractByte(), but reinterprets the byte as a signed int8_t. */
  int8_t extractSignedChar(ubxPacket *msg, uint16_t spotToStart); // Get signed 8-bit value from payload
  /**
   * @brief Combine 4 little-endian bytes from a UBX packet's payload into a 32-bit IEEE-754 float (UBX R4).
   *
   * @param msg The packet to read from.
   * @param spotToStart Byte offset within msg->payload of the first byte.
   * @return The decoded float value.
   */
  float extractFloat(ubxPacket *msg, uint16_t spotToStart);       // Get signed 32-bit float (R4) from payload
  /** @brief As extractFloat(), but combines 8 bytes into a 64-bit IEEE-754 double (UBX R8). */
  double extractDouble(ubxPacket *msg, uint16_t spotToStart);     // Get signed 64-bit double (R8) from payload

  // Functions to help extract RTCM bit fields
  /**
   * @brief Extract an arbitrary-width, big-endian, bit-packed field from an RTCM message body.
   *
   * @param ptr Pointer to the start of the RTCM message body.
   * @param start Bit offset of the field, counted from the start of the message body.
   * @param width Field width, in bits (up to 64).
   * @return The extracted value, zero-extended to 64 bits.
   */
  uint64_t extractUnsignedBits(uint8_t *ptr, uint16_t start, uint16_t width);
  /** @brief As extractUnsignedBits(), but sign-extends the extracted field instead of zero-extending it. */
  int64_t extractSignedBits(uint8_t *ptr, uint16_t start, uint16_t width);

  // Pointers to storage for the "automatic" messages
  // RAM is allocated for these if/when required.

  ubxMessageVector ubxMessages; // v4 scaffolding - the registry of per-message objects. See AGENTS.md "Reference Scaffolding"

  // packetUBXRXMQZSSL6message no longer exists - ubxRXMQZSSL6 is now self-registered and
  // destroyed by ubxMessageVector's own destructor - see AGENTS.md "Adding support for
  // RXM-QZSSL6".
  // packetUBXRXMSFRBX no longer exists - ubxRXMSFRBX is now self-registered - see AGENTS.md
  // "Adding support for RXM-SFRBX".
  // packetUBXRXMRAWX/packetUBXRXMMEASX no longer exist - ubxRXMRAWX/ubxRXMMEASX are now
  // self-registered - see AGENTS.md "Adding the variable-length UBX messages".
  // packetUBXRXMPMP/packetUBXRXMPMPmessage no longer exist - ubxRXMPMP is now self-registered -
  // see AGENTS.md "Adding support for RXM-PMP".

  // packetUBXMONCOMMS no longer exists - ubxMONCOMMS is now self-registered - see AGENTS.md
  // "Adding the variable-length UBX messages".

  // packetUBXESFMEAS no longer exists - ubxESFMEAS is now self-registered - see AGENTS.md "Adding
  // support for ESF-MEAS".
  // packetUBXESFRAW/packetUBXESFSTATUS no longer exist - ubxESFRAW/ubxESFSTATUS are now
  // self-registered - see AGENTS.md "Adding support for ESF-RAW and ESF-STATUS". (packetUBXESFRAW
  // itself could never actually have been allocated anyway - initPacketUBXESFRAW() was declared
  // but had no definition, so every "if (packetUBXESFRAW != nullptr)" branch that used to exist
  // in this file was dead, unreachable code even before this migration.)

  // packetUBXSECSIG no longer exists - ubxSECSIG is now self-registered - see AGENTS.md
  // "Adding the variable-length UBX messages".

  UBX_MGA_ACK_DATA0_t *packetUBXMGAACK = nullptr; // Pointer to struct. RAM will be allocated for this if/when necessary
  UBX_MGA_DBD_t *packetUBXMGADBD = nullptr;       // Pointer to struct. RAM will be allocated for this if/when necessary

  RTCM_1005_t *storageRTCM1005 = nullptr; // Pointer to struct. RAM will be allocated for this if/when necessary

  struct
  {
    union
    {
      uint8_t all;
      struct
      {
        uint8_t dataValid1005 : 1;
        uint8_t dataRead1005 : 1;
        uint8_t dataValid1006 : 1;
        uint8_t dataRead1006 : 1;
      } bits;
    } flags;
    RTCM_1005_data_t rtcm1005; // Latest RTCM 1005 parsed from pushRawData
    RTCM_1006_data_t rtcm1006; // Latest RTCM 1006 parsed from pushRawData
    void (*rtcm1005CallbackPointer)(RTCM_1005_data_t *);
    void (*rtcm1006CallbackPointer)(RTCM_1006_data_t *);
    /** @brief Reset this rtcmInputStorage struct's flags and clear its registered RTCM 1005/1006 input callbacks. */
    void init(void) // Initializer / constructor
    {
      flags.all = 0;                     // Clear the RTCM Input flags
      rtcm1005CallbackPointer = nullptr; // Clear the callback pointers
      rtcm1006CallbackPointer = nullptr;
    }
  } rtcmInputStorage; // Latest RTCM parsed from pushRawData

  uint16_t rtcmFrameCounter = 0; // Tracks the type of incoming byte inside RTCM frame

protected:
  // Depending on the ubx binary response class, store binary responses into different places
  enum classTypes
  {
    CLASS_NONE = 0,
    CLASS_ACK,
    CLASS_NOT_AN_ACK
  } ubxFrameClass = CLASS_NONE;

  // Functions

  /**
   * @brief Shared implementation for the setAutoUBXrate()/setAutoNMEArate() family: writes a message's
   *        output-rate configuration key via VALSET and updates its automatic-message bookkeeping.
   *
   * @param key The output-rate configuration key to write.
   * @param rate The new output rate.
   * @param implicitUpdate New value for the message's implicitUpdate bookkeeping flag.
   * @param flags The message's ubxAutomaticFlags to update.
   * @param layer Which configuration layer(s) to write to.
   * @param maxWait Milliseconds to wait for acknowledgement.
   * @return True if the setting was written successfully.
   */
  bool setAutoMsgRateVal(uint32_t key, uint8_t rate, bool implicitUpdate, ubxAutomaticFlags &flags, uint8_t layer, uint16_t maxWait); // Helper for setAuto*rate functions using VALSET

  /**
   * @brief Shared implementation behind checkUbloxI2C()/checkUbloxSerial()/checkUbloxSpi(): polls the
   *        currently selected bus for new bytes and feeds them into process().
   *
   * @param incomingUBX The ubxPacket to divert a matching response into.
   * @param requestedClass If non-zero, the UBX class being waited for. Defaults to 0.
   * @param requestedID If non-zero, the UBX ID being waited for. Defaults to 0.
   * @return True if new data was checked for successfully (not necessarily that a match was found).
   */
  bool checkUbloxInternal(ubxPacket *incomingUBX, uint8_t requestedClass = 0, uint8_t requestedID = 0); // Checks module with user selected commType
  /**
   * @brief Fold one incoming byte into the running UBX 8-bit Fletcher checksum (rollingChecksumA/B).
   *
   * @param incoming The next payload byte of the frame being received.
   */
  void addToChecksum(uint8_t incoming);                                                                 // Given an incoming byte, adjust rollingChecksumA/B
  /**
   * @brief Shared implementation behind all six pushAssistNowData() overloads.
   *
   * @param offset Byte offset within dataBytes at which to start pushing.
   * @param skipTime If true, skips any UBX-MGA-INI-TIME_UTC/GNSS packets found in the data.
   * @param dataBytes The AssistNow data, as a raw byte buffer.
   * @param numDataBytes Number of bytes in dataBytes.
   * @param mgaAck Whether/how to wait for a UBX-MGA-ACK after each message.
   * @param maxWait Milliseconds to wait between messages when mgaAck is NO.
   * @return The number of bytes successfully pushed.
   */
  size_t pushAssistNowDataInternal(size_t offset, bool skipTime, const uint8_t *dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck, uint16_t maxWait);
  /**
   * @brief Shared implementation behind both findMGAANOForDate() overloads.
   *
   * @param dataBytes The AssistNow Offline data, as a raw byte buffer.
   * @param numDataBytes Number of bytes in dataBytes.
   * @param year Four-digit year to search for.
   * @param month Month (1-12) to search for.
   * @param day Day of month (1-31) to search for.
   * @param daysIntoFuture Added to the requested date before searching.
   * @return The byte offset of the first matching UBX-MGA-ANO message, or numDataBytes if no exact match was found.
   */
  size_t findMGAANOForDateInternal(const uint8_t *dataBytes, size_t numDataBytes, uint16_t year, uint8_t month, uint8_t day, uint8_t daysIntoFuture);

  // Return true if this "automatic" message has storage allocated for it. Also provide the associated max payload size
  /**
   * @brief Check whether a legacy "automatic" message (Class/ID) has dedicated storage allocated for it,
   *        and optionally report its maximum payload size.
   *
   * @param Class The message's UBX class byte.
   * @param ID The message's UBX ID byte.
   * @param maxSize If non-null, filled in with the message's maximum payload size. Defaults to nullptr.
   * @return True if this message has automatic storage allocated.
   */
  bool autoLookup(uint8_t Class, uint8_t ID, uint16_t *maxSize = nullptr);

  /** @brief Allocate and zero-initialize currentGeofenceParams, if not already allocated. @return True on success. */
  bool initGeofenceParams();  // Allocate RAM for currentGeofenceParams and initialize it
  /** @brief Allocate and zero-initialize moduleSWVersion, if not already allocated. @return True on success. */
  bool initModuleSWVersion(); // Allocate RAM for moduleSWVersion and initialize it

  // initPacketUBXRXMQZSSL6message() no longer exists - ubxRXMQZSSL6 is now self-registered -
  // see AGENTS.md "Adding support for RXM-QZSSL6".
  // initPacketUBXRXMPMP()/initPacketUBXRXMPMPmessage() no longer exist - ubxRXMPMP is now
  // self-registered - see AGENTS.md "Adding support for RXM-PMP".
  // initPacketUBXESFSTATUS() no longer exists - ubxESFSTATUS is now self-registered - see
  // AGENTS.md "Adding support for ESF-RAW and ESF-STATUS".
  // initPacketUBXESFMEAS() no longer exists - ubxESFMEAS is now self-registered - see AGENTS.md
  // "Adding support for ESF-MEAS". (It had no definition anywhere in u-blox_GNSS.cpp either - a
  // dead declaration, never callable.)
  // initPacketUBXESFRAW() no longer exists - ubxESFRAW is now self-registered - see AGENTS.md
  // "Adding support for ESF-RAW and ESF-STATUS". (It had no definition anywhere in
  // u-blox_GNSS.cpp either - a dead declaration, never callable - see the comment above
  // packetUBXESFRAW's old declaration.)
  /** @brief Allocate and zero-initialize packetUBXMGAACK, if not already allocated. @return True on success. */
  bool initPacketUBXMGAACK();           // Allocate RAM for packetUBXMGAACK and initialize it
  /** @brief Allocate and zero-initialize packetUBXMGADBD, if not already allocated. @return True on success. */
  bool initPacketUBXMGADBD();           // Allocate RAM for packetUBXMGADBD and initialize it

  /** @brief Allocate and zero-initialize the incoming-RTCM-message storage (_storageRTCM), if not already allocated. @return True on success. */
  bool initStorageRTCM(); // Allocate RAM for incoming RTCM messages and initialize it
  /** @brief Allocate and zero-initialize the non-automatic incoming-NMEA storage (_storageNMEA), if not already allocated. @return True on success. */
  bool initStorageNMEA(); // Allocate RAM for incoming non-Auto NMEA messages and initialize it

  /** @brief Allocate and zero-initialize the incoming-RTCM-1005 storage (storageRTCM1005), if not already allocated. @return True on success. */
  bool initStorageRTCM1005(); // Allocate RAM for incoming RTCM 1005 messages and initialize it

  // Variables
  SparkFun_UBLOX_GNSS::GNSSDeviceBus *_sfeBus;

  SparkFun_UBLOX_GNSS::SfePrint _nmeaOutputPort; // The user can assign an output port to print NMEA sentences if they wish
  SparkFun_UBLOX_GNSS::SfePrint _rtcmOutputPort; // The user can assign an output port to print RTCM sentences if they wish
  SparkFun_UBLOX_GNSS::SfePrint _ubxOutputPort;  // The user can assign an output port to print UBX sentences if they wish
  SparkFun_UBLOX_GNSS::SfePrint _outputPort;     // The user can assign an output port to print ALL characters to if they wish
  // _debugSerial/_printDebug/_printLimitedDebug are inherited from SfeDebugPrint (see sfe_debug.h)

  // The packet buffers
  // These are pointed at from within the ubxPacket
  uint8_t payloadAck[2];           // Holds the requested ACK/NACK
  uint8_t payloadBuf[2];           // Temporary buffer used to screen incoming packets or dump unrequested packets
  size_t packetCfgPayloadSize = 0; // Size for the packetCfg payload. .begin will set this to MAX_PAYLOAD_SIZE if necessary. User can change with setPacketCfgPayloadSize
  uint8_t *payloadCfg = nullptr;
  uint8_t *payloadAuto = nullptr;

  uint8_t *spiBuffer = nullptr;                                // A buffer to store any bytes being recieved back from the device while we are sending via SPI
  size_t spiBufferIndex = 0;                                   // Index into the SPI buffer
  size_t spiBufferSize = SFE_UBLOX_SPI_BUFFER_DEFAULT_SIZE;    // Default size of the SPI buffer
  uint8_t spiTransactionSize = SFE_UBLOX_SPI_TRANSACTION_SIZE; // Default size of SPI transactions

  // Init the packet structures and init them with pointers to the payloadAck, payloadCfg, payloadBuf and payloadAuto arrays
  ubxPacket packetAck = {0, 0, 0, 0, 0, payloadAck, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
  ubxPacket packetBuf = {0, 0, 0, 0, 0, payloadBuf, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
  ubxPacket packetCfg = {0, 0, 0, 0, 0, payloadCfg, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
  ubxPacket packetAuto = {0, 0, 0, 0, 0, payloadAuto, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  // Flag if this packet is unrequested (and so should be ignored and not copied into packetCfg or packetAck)
  bool ignoreThisPayload = false;

  // Identify which buffer is in use
  // Data is stored in packetBuf until the requested class and ID can be validated
  // If a match is seen, data is diverted into packetAck or packetCfg
  //"Automatic" messages which have RAM allocated for them are diverted into packetAuto
  sfe_ublox_packet_buffer_e activePacketBuffer = SFE_UBLOX_PACKET_PACKETBUF;

  // Limit checking of new data to every X ms
  // If we are expecting an update every X Hz then we should check every quarter that amount of time
  // Otherwise we may block ourselves from seeing new data
  uint8_t i2cPollingWait = 100;    // Default to 100ms. Adjusted when user calls setNavigationFrequency() or setHNRNavigationRate() or setMeasurementRate()
  uint8_t i2cPollingWaitNAV = 100; // We need to record the desired polling rate for standard nav messages
  uint8_t i2cPollingWaitHNR = 100; // and for HNR too so we can set i2cPollingWait to the lower of the two

  // The SPI polling wait is a little different. checkUbloxSpi will delay for this amount before returning if
  // there is no data waiting to be read. This prevents waitForACKResponse from pounding the SPI bus too hard.
  uint8_t spiPollingWait = 9; // Default to 9ms; waitForACKResponse delays for 1ms on top of this. User can adjust with setSPIPollingWait.

  unsigned long lastCheck = 0;

  uint16_t ubxFrameCounter; // Count all UBX frame bytes. [Fixed header(2bytes), CLS(1byte), ID(1byte), length(2bytes), payload(x bytes), checksums(2bytes)]
  uint8_t rollingChecksumA; // Rolls forward as we receive incoming bytes. Checked against the last two A/B checksum bytes
  uint8_t rollingChecksumB; // Rolls forward as we receive incoming bytes. Checked against the last two A/B checksum bytes

  // NMEA logging / Auto support
  sfe_ublox_nmea_filtering_t _logNMEA;     // Flags to indicate which NMEA messages should be added to the file buffer for logging
  sfe_ublox_nmea_filtering_t _processNMEA; // Flags to indicate which NMEA messages should be passed to processNMEA

  int8_t nmeaByteCounter; // Count all NMEA message bytes.
  // Abort NMEA message reception if nmeaByteCounter exceeds maxNMEAByteCount.
  // The user can adjust maxNMEAByteCount by calling setMaxNMEAByteCount
  int8_t maxNMEAByteCount = SFE_UBLOX_MAX_NMEA_BYTE_COUNT;
  uint8_t nmeaAddressField[6]; // NMEA Address Field - includes the start character (*)
  /**
   * @brief Check the NMEA logging mask (setNMEALoggingMask()) to see whether msgId should be written to
   *        the file buffer.
   *
   * @param msgId The NMEA sentence ID.
   * @return True if this sentence should be logged.
   */
  bool logThisNMEA(const char *msgId); // Return true if we should log this NMEA message
  /** @brief As logThisNMEA(), but checks the processNMEA mask (setProcessNMEAMask()) instead of the logging mask. */
  bool processThisNMEA(const char *msgId); // Return true if we should pass this NMEA message to processNMEA
  /**
   * @brief Sanity-check a 6-byte NMEA sentence header (talker ID + sentence ID). Used to set _signsOfLife.
   *
   * @param msgId The 6-byte NMEA header to check.
   * @return True if the header looks like valid NMEA.
   */
  bool isNMEAHeaderValid(const char *msgId); // Return true if the six byte NMEA header appears valid. Used to set _signsOfLife
  /**
   * @brief Check whether an NMEA sentence is currently known to be output automatically/periodically.
   *
   * @param msgId The NMEA sentence ID.
   * @return True if this sentence is treated as automatic.
   */
  bool isThisNMEAauto(const char *msgId); // Return true if msgId is known to be automatic / periodic
  /**
   * @brief Check whether an NMEA sentence has automatic-message storage allocated for it. Note: having
   *        storage allocated does not by itself mean the message is currently automatic.
   *
   * @param msgId The NMEA sentence ID.
   * @return True if storage is allocated.
   */
  bool doesThisNMEAHaveStorage(const char *msgId); // Return true if this msgId has "Auto" storage allocated - BUT it may not actually be "Auto"
  /**
   * @brief Check whether an NMEA sentence has a callback registered via setNmeaCallbackPtr().
   *
   * @param msgId The NMEA sentence ID.
   * @return True if a callback is registered.
   */
  bool doesThisNMEAHaveCallback(const char *msgId); // Return true if this msgId has a callback

  NMEA_STORAGE_t *_storageNMEA = nullptr; // Pointer to struct. RAM will be allocated for this if/when necessary

  // RTCM logging
  sfe_ublox_rtcm_filtering_t _logRTCM; // Flags to indicate which NMEA messages should be added to the file buffer for logging

  RTCM_FRAME_t *_storageRTCM = nullptr;              // Pointer to struct. RAM will be allocated for this if/when necessary
  /**
   * @brief Fold one incoming byte into a running CRC-24Q checksum, as used by RTCM3 frames.
   *
   * @param incoming The next byte of the frame being received.
   * @param checksum The running checksum to update in place.
   */
  void crc24q(uint8_t incoming, uint32_t *checksum); // Add incoming to checksum as per CRC-24Q

  // Define the maximum possible message length for packetAuto and enableUBXlogging
  // On the ZED-X20P, we see:
  //   RXM-RAWX messages containing 3920 bytes (122 blocks)
  //   NAV-SAT messages containing 644 bytes (53 blocks)
  //   NAV-SIG messages containing 2248 bytes (140 blocks)
  const uint16_t SFE_UBX_MAX_LENGTH = UBX_NAV_SAT_MAX_LEN;

  // UBX logging
  sfe_ublox_ubx_logging_list_t *sfe_ublox_ubx_logging_list_head = nullptr; // Linked list of which messages to log
  /**
   * @brief Check whether a UBX message (by raw Class/ID) should be written to the file buffer, per
   *        enableUBXlogging()'s logMe flag.
   *
   * @param UBX_CLASS The message's UBX class byte.
   * @param UBX_ID The message's UBX ID byte.
   * @return True if this message should be logged.
   */
  bool logThisUBX(uint8_t UBX_CLASS, uint8_t UBX_ID);                      // Returns true if this UBX should be added to the logging buffer - for logging
  /** @brief As logThisUBX(), but checks enableUBXlogging()'s processMe flag instead of its logMe flag. */
  bool processThisUBX(uint8_t UBX_CLASS, uint8_t UBX_ID);                  // Returns true if this UBX should be added to the logging buffer - for processing
  /**
   * @brief Shared implementation behind logThisUBX()/processThisUBX(): walks the enableUBXlogging()
   *        linked list looking for a matching Class/ID entry.
   *
   * @param UBX_CLASS The message's UBX class byte.
   * @param UBX_ID The message's UBX ID byte.
   * @param log True to check the logMe flag, false to check the processMe flag.
   * @return True if a matching, enabled entry was found.
   */
  bool logOrProcessThisUBX(uint8_t UBX_CLASS, uint8_t UBX_ID, bool log);   // Called by logThisUBX and processThisUBX

  // Flag to prevent reentry into checkCallbacks
  // Prevent badness if the user accidentally calls checkCallbacks from inside a callback
  volatile bool checkCallbacksReentrant = false;

  // Support for data logging
  uint8_t *ubxFileBuffer = nullptr;                             // Pointer to the file buffer. RAM is allocated for this if required in .begin
  uint16_t fileBufferSize = 0;                                  // The size of the file buffer. This can be changed by calling setFileBufferSize _before_ .begin
  uint16_t fileBufferHead = 0;                                  // The incoming byte is written into the file buffer at this location
  uint16_t fileBufferTail = 0;                                  // The next byte to be read from the buffer will be read from this location
  uint16_t fileBufferMaxAvail = 0;                              // The maximum number of bytes the file buffer has contained. Handy for checking the buffer is large enough to handle all the incoming data.
  /** @brief Allocate the file-logging buffer at its configured size (setFileBufferSize()). Called by begin(). @return True on success. */
  bool createFileBuffer(void);                                  // Create the file buffer. Called by .begin
  /** @brief Get how many free bytes remain in the file-logging buffer. */
  uint16_t fileBufferSpaceAvailable(void);                      // Check how much space is available in the buffer
  /** @brief Get how many bytes are currently occupied in the file-logging buffer. */
  uint16_t fileBufferSpaceUsed(void);                           // Check how much space is used in the buffer
  /**
   * @brief Serialize a complete UBX packet (header/Class/ID/length/payload/checksum) and append it to
   *        the file-logging buffer.
   *
   * @param msg The packet to log.
   * @return True if the packet was stored successfully.
   */
  bool storePacket(ubxPacket *msg);                             // Add a UBX packet to the file buffer
  /**
   * @brief Append raw bytes to the file-logging buffer, tracking the high-water mark (fileBufferMaxAvail).
   *
   * @param theBytes Pointer to the bytes to add.
   * @param numBytes Number of bytes to add.
   * @return True if the bytes were stored successfully.
   */
  bool storeFileBytes(uint8_t *theBytes, uint16_t numBytes);    // Add theBytes to the file buffer
  /**
   * @brief Low-level circular-buffer write used by storeFileBytes()/storePacket().
   *
   * @param theBytes Pointer to the bytes to write.
   * @param numBytes Number of bytes to write.
   */
  void writeToFileBuffer(uint8_t *theBytes, uint16_t numBytes); // Write theBytes to the file buffer

  // Support for RTCM buffering
  uint8_t *rtcmBuffer = nullptr;                                // Pointer to the RTCM buffer. RAM is allocated for this if required in .begin
  uint16_t rtcmBufferSize = 0;                                  // The size of the RTCM buffer. This can be changed by calling setRTCMBufferSize _before_ .begin
  uint16_t rtcmBufferHead = 0;                                  // The incoming byte is written into the buffer at this location
  uint16_t rtcmBufferTail = 0;                                  // The next byte to be read from the buffer will be read from this location
  /** @brief Allocate the dedicated RTCM buffer at its configured size (setRTCMBufferSize()). Called by begin(). @return True on success. */
  bool createRTCMBuffer(void);                                  // Create the RTCM buffer. Called by .begin
  /** @brief Get how many free bytes remain in the dedicated RTCM buffer. */
  uint16_t rtcmBufferSpaceAvailable(void);                      // Check how much space is available in the buffer
  /** @brief Get how many bytes are currently occupied in the dedicated RTCM buffer. */
  uint16_t rtcmBufferSpaceUsed(void);                           // Check how much space is used in the buffer
  /**
   * @brief Append raw bytes to the dedicated RTCM buffer.
   *
   * @param theBytes Pointer to the bytes to add.
   * @param numBytes Number of bytes to add.
   * @return True if the bytes were stored successfully.
   */
  bool storeRTCMBytes(uint8_t *theBytes, uint16_t numBytes);    // Add theBytes to the buffer
  /**
   * @brief Low-level circular-buffer write used by storeRTCMBytes().
   *
   * @param theBytes Pointer to the bytes to write.
   * @param numBytes Number of bytes to write.
   */
  void writeToRTCMBuffer(uint8_t *theBytes, uint16_t numBytes); // Write theBytes to the buffer

  // .begin will return true if the assumeSuccess parameter is true and if _signsOfLife is true
  // _signsOfLife is set to true when: a valid UBX message is seen; a valig NMEA header is seen.
  bool _signsOfLife;

  // Keep track of how many keys have been added to CfgValset
  uint8_t _numCfgKeys = 0;

  // Keep track of how many keys have been added to CfgValget and what size the response will be
  uint8_t _numGetCfgKeys = 0;
  uint16_t _lenCfgValGetResponse = 0;
  uint8_t *cfgValgetValueSizes = nullptr; // A pointer to a list of the value sizes for each key in the cfgValget
  uint16_t _cfgValgetMaxPayload = 0;

  // Send the current CFG_VALSET message when packetCfg has less than this many bytes available
  size_t _autoSendAtSpaceRemaining = 0;

public:
  // Flag to indicate if currentSentence should be reset on a (I2C) bus error
  bool _resetCurrentSentenceOnBusError = true;

  typedef union
  {
    uint64_t unsigned64;
    int64_t signed64;
  } unsignedSigned64;

  typedef union
  {
    uint32_t unsigned32;
    int32_t signed32;
  } unsignedSigned32;

  typedef union
  {
    uint16_t unsigned16;
    int16_t signed16;
  } unsignedSigned16;

  typedef union
  {
    uint8_t unsigned8;
    int8_t signed8;
  } unsignedSigned8;

  typedef union
  {
    uint32_t unsigned32;
    float flt;
  } unsigned32float;

  typedef union
  {
    uint64_t unsigned64;
    double dbl;
  } unsigned64double;
};
