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
 * @file u-blox_GNSS.cpp
 * 
 */

#include "sfe_platform.h"
#include "u-blox_GNSS.h"

DevUBLOXGNSS::DevUBLOXGNSS(void)
{
  // Constructor
  if (debugPin >= 0)
  {
    sfe_pin_output((uint8_t)debugPin);
    sfe_pin_write((uint8_t)debugPin, true);
  }

  _logNMEA.all = 0;                             // Default to passing no NMEA messages to the file buffer
  _processNMEA.all = SFE_UBLOX_FILTER_NMEA_ALL; // Default to passing all NMEA messages to processNMEA
  _logRTCM.all = 0;                             // Default to passing no RTCM messages to the file buffer

  rtcmInputStorage.init();
}

DevUBLOXGNSS::~DevUBLOXGNSS(void)
{
  // Destructor

  end(); // Delete all allocated memory - excluding payloadCfg, payloadAuto and spiBuffer

  if (payloadCfg != nullptr)
  {
    delete[] payloadCfg; // Created with new[]
    payloadCfg = nullptr;
  }

  if (payloadAuto != nullptr)
  {
    delete[] payloadAuto; // Created with new[]
    payloadAuto = nullptr;
  }

  if (spiBuffer != nullptr)
  {
    delete[] spiBuffer; // Created with new[]
    spiBuffer = nullptr;
  }
}

// Stop all automatic message processing. Free all used RAM
void DevUBLOXGNSS::end(void)
{
  // Note: payloadCfg is not deleted

  // Note: payloadAuto is not deleted

  // Note: spiBuffer is not deleted

  if (ubxFileBuffer != nullptr) // Check if RAM has been allocated for the file buffer
  {
    debugPrintln("end: the file buffer has been deleted. You will need to call setFileBufferSize before .begin to create a new one.");
    delete[] ubxFileBuffer; // Created with new[]
    ubxFileBuffer = nullptr;
    fileBufferSize = 0; // Reset file buffer size. User will have to call setFileBufferSize again
    fileBufferMaxAvail = 0;
  }

  if (rtcmBuffer != nullptr) // Check if RAM has been allocated for the RTCM buffer
  {
    delete[] rtcmBuffer; // Created with new[]
    rtcmBuffer = nullptr;
    rtcmBufferSize = 0; // Reset file buffer size. User will have to call setFileBufferSize again
  }

  if (cfgValgetValueSizes != nullptr)
  {
    delete[] cfgValgetValueSizes;
    cfgValgetValueSizes = nullptr;
  }

  if (moduleSWVersion != nullptr)
  {
    delete moduleSWVersion; // Created with new moduleSWVersion_t
    moduleSWVersion = nullptr;
  }

  if (currentGeofenceParams != nullptr)
  {
    delete currentGeofenceParams; // Created with new geofenceParams_t
    currentGeofenceParams = nullptr;
  }

  // packetUBXNAVSAT/packetUBXNAVSIG no longer exist - ubxNAVSAT/ubxNAVSIG are now
  // self-registered and destroyed by ubxMessageVector's own destructor. See AGENTS.md
  // "Adding the variable-length UBX messages".

  // packetUBXRXMPMP/packetUBXRXMPMPmessage no longer exist - ubxRXMPMP is now self-registered
  // and destroyed by ubxMessageVector's own destructor. See AGENTS.md "Adding support for
  // RXM-PMP".

  // packetUBXRXMQZSSL6message no longer exists - ubxRXMQZSSL6 is now self-registered and
  // destroyed by ubxMessageVector's own destructor. See AGENTS.md "Adding support for
  // RXM-QZSSL6".

  // packetUBXRXMSFRBX no longer exists - ubxRXMSFRBX is now self-registered and destroyed by
  // ubxMessageVector's own destructor. See AGENTS.md "Adding support for RXM-SFRBX".

  // packetUBXRXMRAWX/packetUBXRXMMEASX no longer exist - ubxRXMRAWX/ubxRXMMEASX are now
  // self-registered and destroyed by ubxMessageVector's own destructor. See AGENTS.md
  // "Adding the variable-length UBX messages".

  // packetUBXMONCOMMS no longer exists - ubxMONCOMMS is now self-registered and destroyed by
  // ubxMessageVector's own destructor. See AGENTS.md "Adding the variable-length UBX messages".

  // packetUBXESFSTATUS no longer exists - ubxESFSTATUS is now self-registered and destroyed by
  // ubxMessageVector's own destructor. See AGENTS.md "Adding support for ESF-RAW and
  // ESF-STATUS".

  // packetUBXESFMEAS no longer exists - ubxESFMEAS is now self-registered and destroyed by
  // ubxMessageVector's own destructor. See AGENTS.md "Adding support for ESF-MEAS".

  // packetUBXESFRAW no longer exists - ubxESFRAW is now self-registered and destroyed by
  // ubxMessageVector's own destructor. See AGENTS.md "Adding support for ESF-RAW and
  // ESF-STATUS". (This cleanup block was already unreachable dead code before this migration -
  // packetUBXESFRAW could never actually be non-nullptr, since initPacketUBXESFRAW() was
  // declared but never defined.)

  if (packetUBXMGAACK != nullptr)
  {
    delete packetUBXMGAACK;
    packetUBXMGAACK = nullptr;
  }

  if (packetUBXMGADBD != nullptr)
  {
    delete packetUBXMGADBD;
    packetUBXMGADBD = nullptr;
  }

  // packetUBXSECSIG no longer exists - ubxSECSIG is now self-registered and destroyed by
  // ubxMessageVector's own destructor. See AGENTS.md "Adding the variable-length UBX messages".

  if (_storageNMEA != nullptr)
  {
    if (_storageNMEA->data != nullptr)
    {
      delete[] _storageNMEA->data;
    }
    delete _storageNMEA;
    _storageNMEA = nullptr;
  }

  if (_storageRTCM != nullptr)
  {
    delete _storageRTCM;
    _storageRTCM = nullptr;
  }

  if (storageRTCM1005 != nullptr)
  {
    if (storageRTCM1005->callbackData != nullptr)
    {
      delete storageRTCM1005->callbackData;
    }
    delete storageRTCM1005;
    storageRTCM1005 = nullptr;
  }

  if (sfe_ublox_ubx_logging_list_head != nullptr)
  {
    while (sfe_ublox_ubx_logging_list_head->next != nullptr)
    {
      // Step through the list, find the tail
      sfe_ublox_ubx_logging_list_t *sfe_ublox_ubx_logging_list_ptr_previous = sfe_ublox_ubx_logging_list_head;
      sfe_ublox_ubx_logging_list_t *sfe_ublox_ubx_logging_list_ptr = sfe_ublox_ubx_logging_list_head->next;
      while (sfe_ublox_ubx_logging_list_ptr->next != nullptr)
      {
        sfe_ublox_ubx_logging_list_ptr_previous = sfe_ublox_ubx_logging_list_ptr;
        sfe_ublox_ubx_logging_list_ptr = sfe_ublox_ubx_logging_list_ptr->next;
      }
      // Delete the tail
      delete sfe_ublox_ubx_logging_list_ptr;
      sfe_ublox_ubx_logging_list_ptr_previous->next = nullptr;
    }
    // Finally, delete the head
    delete sfe_ublox_ubx_logging_list_head;
    sfe_ublox_ubx_logging_list_head = nullptr;
  }

  deleteLock(); // Delete the lock semaphore - if required
}

// Allow the user to change packetCfgPayloadSize. Handy if you want to process big messages like RAWX
// This can be called before .begin if required / desired
bool DevUBLOXGNSS::setPacketCfgPayloadSize(size_t payloadSize)
{
  bool success = true;

  if ((payloadSize == 0) && (payloadCfg != nullptr))
  {
    // Zero payloadSize? Dangerous! But we'll free the memory anyway...
    delete[] payloadCfg; // Created with new[]
    payloadCfg = nullptr;
    packetCfg.payload = payloadCfg;
    packetCfgPayloadSize = payloadSize;
    debugPrintln("setPacketCfgPayloadSize: Zero payloadSize!", true); // Important
  }

  else if (payloadCfg == nullptr) // Memory has not yet been allocated - so use new
  {
    payloadCfg = new uint8_t[payloadSize];
    packetCfg.payload = payloadCfg;

    if (payloadCfg == nullptr)
    {
      success = false;
      packetCfgPayloadSize = 0;
      debugPrintln("setPacketCfgPayloadSize: RAM alloc failed!", true); // Important
    }
    else
      packetCfgPayloadSize = payloadSize;

    if ((packetCfgPayloadSize + 8) > spiBufferSize) // Warn the user if spiBuffer is now smaller than the packetCfg payload. Could result in lost data
    {
      debugPrintln("setPacketCfgPayloadSize: packetCfgPayloadSize > spiBufferSize!", true); // Important
    }
  }

  else // Memory has already been allocated - so resize
  {
    uint8_t *newPayload = new uint8_t[payloadSize];

    if (newPayload == nullptr) // Check if the alloc was successful
    {
      success = false;                                           // Report failure. Don't change payloadCfg, packetCfg.payload or packetCfgPayloadSize
      debugPrintln("setPacketCfgPayloadSize: RAM resize failed!", true); // Important
    }
    else
    {
      memcpy(newPayload, payloadCfg, payloadSize <= packetCfgPayloadSize ? payloadSize : packetCfgPayloadSize); // Copy as much existing data as we can
      delete[] payloadCfg;                                                                                      // Free payloadCfg. Created with new[]
      payloadCfg = newPayload;                                                                                  // Point to the newPayload
      packetCfg.payload = payloadCfg;                                                                           // Update the packet pointer
      packetCfgPayloadSize = payloadSize;                                                                       // Update the packet payload size
    }

    if ((packetCfgPayloadSize + 8) > spiBufferSize) // Warn the user if spiBuffer is now smaller than the packetCfg payload. Could result in lost data
    {
      debugPrintln("setPacketCfgPayloadSize: packetCfgPayloadSize > spiBufferSize!", true); // Important
    }
  }

  return (success);
}

// Return the number of free bytes remaining in packetCfgPayload
size_t DevUBLOXGNSS::getPacketCfgSpaceRemaining()
{
  return (packetCfgPayloadSize - packetCfg.len);
}

// New in v3.0: hardware interface is abstracted
void DevUBLOXGNSS::setCommunicationBus(SparkFun_UBLOX_GNSS::GNSSDeviceBus &theBus)
{
  _sfeBus = &theBus;
}
// For Serial, return Serial.available()
// For I2C, read registers 0xFD and 0xFE. Return bytes available as uint16_t
// Not Applicable for SPI
uint16_t DevUBLOXGNSS::available()
{
  return _sfeBus->available();
}
// For I2C, ping the _address
// Not Applicable for SPI and Serial
bool DevUBLOXGNSS::ping()
{
  if (!lock())
    return false;
  bool ok = _sfeBus->ping();
  unlock();
  return ok;
}
// For Serial, do Serial.write
// For I2C, push data to register 0xFF. Chunkify if necessary. Prevent single byte writes as these are illegal
// For SPI, writing bytes will also read bytes simultaneously. Read data is _ignored_ here. Use writeReadBytes
uint8_t DevUBLOXGNSS::writeBytes(uint8_t *data, uint8_t length)
{
  return _sfeBus->writeBytes(data, length);
}
// For SPI, writing bytes will also read bytes simultaneously. Read data is returned in readData
uint8_t DevUBLOXGNSS::writeReadBytes(const uint8_t *data, uint8_t *readData, uint8_t length)
{
  return _sfeBus->writeReadBytes(data, readData, length);
}
void DevUBLOXGNSS::startWriteReadByte()
{
  _sfeBus->startWriteReadByte();
}
void DevUBLOXGNSS::writeReadByte(const uint8_t *data, uint8_t *readData)
{
  _sfeBus->writeReadByte(data, readData);
}
void DevUBLOXGNSS::writeReadByte(const uint8_t data, uint8_t *readData)
{
  _sfeBus->writeReadByte(data, readData);
}
void DevUBLOXGNSS::endWriteReadByte()
{
  _sfeBus->endWriteReadByte();
}
// For Serial, attempt Serial.read
// For I2C, read from register 0xFF
// For SPI, read the byte while writing 0xFF
uint8_t DevUBLOXGNSS::readBytes(uint8_t *data, uint8_t length)
{
  return _sfeBus->readBytes(data, length);
}

bool DevUBLOXGNSS::init(uint16_t maxWait, bool assumeSuccess)
{
  createLock(); // Create the lock semaphore - if needed

  _signsOfLife = false; // Clear the _signsOfLife flag. It will be set true if valid traffic is seen.

  // New in v2.0: allocate memory for the packetCfg payload here - if required. (The user may have called setPacketCfgPayloadSize already)
  if (packetCfgPayloadSize == 0)
    setPacketCfgPayloadSize(MAX_PAYLOAD_SIZE);

  // New in v2.0: allocate memory for the file buffer - if required. (The user should have called setFileBufferSize already)
  createFileBuffer();

  // Create storage for RTCM data - only useful on systems where the GNSS is interfaced via SPI and you may want to write
  // data to another SPI device (e.g. Ethernet) in a safe way, avoiding processRTCM (which would be called _during_ checkUblox).
  createRTCMBuffer();

  if (_commType == COMM_TYPE_SPI)
  {
    // Create the SPI buffer
    if (spiBuffer == nullptr) // Memory has not yet been allocated - so use new
    {
      spiBuffer = new uint8_t[spiBufferSize];
    }

    if (spiBuffer == nullptr)
    {
      debugPrintln("begin (SPI): memory allocation failed for SPI Buffer!", true); // Important
      return (false);
    }
    else
    {
      // Initialize/clear the SPI buffer - fill it with 0xFF as this is what is received from the UBLOX module if there's no data to be processed
      for (size_t i = 0; i < spiBufferSize; i++)
      {
        spiBuffer[i] = 0xFF;
      }
      debugPrint("begin (SPI): spiBuffer size is ", true); // Important
      debugPrintln(spiBufferSize, true);
      if ((packetCfgPayloadSize + 8) > spiBufferSize) // Warn the user if spiBuffer is now smaller than the packetCfg payload. Could result in lost data
      {
        debugPrintln("begin (SPI): packetCfgPayloadSize > spiBufferSize!", true); // Important
      }
    }
  }

  // Call isConnected up to three times - tests on the NEO-M8U show the CFG RATE poll occasionally being ignored
  bool connected = isConnected(maxWait);

  if (!connected)
  {
    debugPrintln("begin: isConnected - second attempt", true); // Important
    connected = isConnected(maxWait);
  }

  if (!connected)
  {
    debugPrintln("begin: isConnected - third attempt", true); // Important
    connected = isConnected(maxWait);
  }

  if ((!connected) && assumeSuccess && _signsOfLife) // Advanced users can assume success if required. Useful if the port is outputting messages at high navigation rate.
  {
    debugPrintln("begin: third attempt failed. Assuming success...", true); // Important
    return (true);
  }

  return (connected);
}

// Allow the user to change I2C polling wait (the minimum interval between I2C data requests - to avoid pounding the bus)
// i2cPollingWait defaults to 100ms and is adjusted automatically when setNavigationFrequency()
// or setHNRNavigationRate() are called. But if the user is using callbacks, it might be advantageous
// to be able to set the polling wait manually.
void DevUBLOXGNSS::setI2CpollingWait(uint8_t newPollingWait_ms)
{
  i2cPollingWait = newPollingWait_ms;
}

// Allow the user to change SPI polling wait
// (the minimum interval between SPI data requests when no data is available - to avoid pounding the bus)
void DevUBLOXGNSS::setSPIpollingWait(uint8_t newPollingWait_ms)
{
  spiPollingWait = newPollingWait_ms;
}

// Sets the global size for I2C transactions
// Most platforms use 32 bytes (the default) but this allows users to increase the transaction
// size if the platform supports it
// Note: If the transaction size is set larger than the platforms buffer size, bad things will happen.
void DevUBLOXGNSS::setI2CTransactionSize(uint8_t transactionSize)
{
  if (transactionSize < 8)
    transactionSize = 8; // Ensure transactionSize is at least 8 bytes otherwise sendI2cCommand will have problems!

  i2cTransactionSize = transactionSize;
}
uint8_t DevUBLOXGNSS::getI2CTransactionSize(void)
{
  return (i2cTransactionSize);
}

// Sets the global size for SPI transactions.
// Call this **before** begin()!
void DevUBLOXGNSS::setSpiTransactionSize(uint8_t transactionSize)
{
  if (spiBuffer == nullptr)
  {
    if (transactionSize < spiTransactionSize)
      transactionSize = spiTransactionSize;
    spiTransactionSize = transactionSize;

    if (spiBufferSize < spiTransactionSize) // Ensure the buffer is at least spiTransactionSize
      spiBufferSize = spiTransactionSize;
  }
  else
  {
    debugPrintln("setSpiTransactionSize: you need to call setSpiTransactionSize _before_ begin!"); // Not important
  }
}
uint8_t DevUBLOXGNSS::getSpiTransactionSize(void)
{
  return (spiTransactionSize);
}

// Sets the global size for the SPI buffer.
// Call this **before** begin()!
// Note: if the buffer size is too small, incoming characters may be lost if the message sent
// is larger than this buffer. If too big, you may run out of SRAM on constrained architectures!
void DevUBLOXGNSS::setSpiBufferSize(size_t bufferSize)
{
  if (spiBuffer == nullptr)
  {
    if (bufferSize < spiTransactionSize) // Ensure the buffer is at least spiTransactionSize
      bufferSize = spiTransactionSize;
    spiBufferSize = bufferSize;
  }
  else
  {
    debugPrintln("setSpiBufferSize: you need to call setSpiBufferSize _before_ begin!"); // Not important
  }
}
size_t DevUBLOXGNSS::getSpiBufferSize(void)
{
  return (spiBufferSize);
}

// Sets the size of maxNMEAByteCount
// Call this before .begin to avoid badness with _storageNMEA
void DevUBLOXGNSS::setMaxNMEAByteCount(int8_t newMax)
{
  maxNMEAByteCount = newMax;
}
int8_t DevUBLOXGNSS::getMaxNMEAByteCount(void)
{
  return (maxNMEAByteCount);
}

// Returns true if I2C device ack's
bool DevUBLOXGNSS::isConnected(uint16_t maxWait)
{
  if (_commType == COMM_TYPE_I2C)
  {
    if (!ping())
      return false; // Sensor did not ack
  }

  // Query port configuration to see whether we get a meaningful response
  // We could simply request the config for any port but, just for giggles, let's request the config for most appropriate port
  uint8_t en;
  if (_commType == COMM_TYPE_I2C)
    return getVal8(UBLOX_CFG_I2CINPROT_UBX, &en, VAL_LAYER_RAM, maxWait);
  else if ((_commType == COMM_TYPE_SERIAL) && (!_UART2))
    return getVal8(UBLOX_CFG_UART1INPROT_UBX, &en, VAL_LAYER_RAM, maxWait);
  else if ((_commType == COMM_TYPE_SERIAL) && (_UART2))
    return getVal8(UBLOX_CFG_UART2INPROT_UBX, &en, VAL_LAYER_RAM, maxWait);
  else if (_commType == COMM_TYPE_SPI)
    return getVal8(UBLOX_CFG_SPIINPROT_UBX, &en, VAL_LAYER_RAM, maxWait);
  else
    return false;
}

// Enable or disable the printing of sent/response HEX values.
// Use this in conjunction with 'Transport Logging' from the Universal Reader Assistant to see what they're doing that we're not
void DevUBLOXGNSS::enableDebugging(sfe_print_t &debugPort, bool printLimitedDebug)
{
  _debugSerial.init(debugPort); // Grab which port the user wants us to use for debugging
  _printDebug = true; // Should we print the commands we send? Good for debugging
  _printLimitedDebug = printLimitedDebug; // Should we print limited debug messages? Good for debugging high navigation rates

  // v4 scaffolding: ubxMessages/nmeaMessages inherit debugPrint()/debugPrintln() from the same
  // SfeDebugPrint base we do (see sfe_debug.h), but as separate objects they have their own
  // separate debug state, so they don't see the enableDebugging() call above automatically -
  // push it to them explicitly instead.
  ubxMessages.copyDebugStateFrom(*this);
  nmeaMessages.copyDebugStateFrom(*this);
}
void DevUBLOXGNSS::disableDebugging(void)
{
  _printDebug = false; // Turn off extra print statements
  _printLimitedDebug = false;

  ubxMessages.copyDebugStateFrom(*this);
  nmeaMessages.copyDebugStateFrom(*this);
}

// debugPrint()/debugPrintln() are now inherited from SfeDebugPrint - see sfe_debug.h/.cpp.

const char *DevUBLOXGNSS::statusString(sfe_ublox_status_e stat)
{
  switch (stat)
  {
  case SFE_UBLOX_STATUS_SUCCESS:
    return "Success";
    break;
  case SFE_UBLOX_STATUS_FAIL:
    return "General Failure";
    break;
  case SFE_UBLOX_STATUS_CRC_FAIL:
    return "CRC Fail";
    break;
  case SFE_UBLOX_STATUS_TIMEOUT:
    return "Timeout";
    break;
  case SFE_UBLOX_STATUS_COMMAND_NACK:
    return "Command not acknowledged (NACK)";
    break;
  case SFE_UBLOX_STATUS_OUT_OF_RANGE:
    return "Out of range";
    break;
  case SFE_UBLOX_STATUS_INVALID_ARG:
    return "Invalid Arg";
    break;
  case SFE_UBLOX_STATUS_INVALID_OPERATION:
    return "Invalid operation";
    break;
  case SFE_UBLOX_STATUS_MEM_ERR:
    return "Memory Error";
    break;
  case SFE_UBLOX_STATUS_HW_ERR:
    return "Hardware Error";
    break;
  case SFE_UBLOX_STATUS_DATA_SENT:
    return "Data Sent";
    break;
  case SFE_UBLOX_STATUS_DATA_RECEIVED:
    return "Data Received";
    break;
  case SFE_UBLOX_STATUS_I2C_COMM_FAILURE:
    return "I2C Comm Failure";
    break;
  case SFE_UBLOX_STATUS_DATA_OVERWRITTEN:
    return "Data Packet Overwritten";
    break;
  default:
    return "Unknown Status";
    break;
  }
  return "None";
}

// Check for the arrival of new I2C/Serial/SPI data
// Called regularly to check for available bytes on the user' specified port

bool DevUBLOXGNSS::checkUblox(uint8_t requestedClass, uint8_t requestedID)
{
  return checkUbloxInternal(&packetCfg, requestedClass, requestedID);
}

// PRIVATE: Called regularly to check for available bytes on the user' specified port
bool DevUBLOXGNSS::checkUbloxInternal(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID)
{
  if (!lock())
    return false;

  bool ok = false;
  if (_commType == COMM_TYPE_I2C)
    ok = (checkUbloxI2C(incomingUBX, requestedClass, requestedID));
  else if (_commType == COMM_TYPE_SERIAL)
    ok = (checkUbloxSerial(incomingUBX, requestedClass, requestedID));
  else if (_commType == COMM_TYPE_SPI)
    ok = (checkUbloxSpi(incomingUBX, requestedClass, requestedID));

  unlock();

  return ok;
}

// Polls I2C for data, passing any new bytes to process()
// Returns true if new bytes are available
bool DevUBLOXGNSS::checkUbloxI2C(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID)
{
  if (sfe_millis() - lastCheck >= i2cPollingWait)
  {
    // Get the number of bytes available from the module
    // From the u-blox integration manual:
    // "There are two forms of DDC read transfer. The "random access" form includes a peripheral register
    //  address and thus allows any register to be read. The second "current address" form omits the
    //  register address. If this second form is used, then an address pointer in the receiver is used to
    //  determine which register to read. This address pointer will increment after each read unless it
    //  is already pointing at register 0xFF, the highest addressable register, in which case it remains
    //  unaltered."
    uint16_t bytesAvailable = available();

    if (bytesAvailable == 0)
    {
      lastCheck = sfe_millis(); // Put off checking to avoid I2C bus traffic
      return (false);
    }

    // Check for undocumented bit error. We found this doing logic scans.
    // This error is rare but if we incorrectly interpret the first bit of the two 'data available' bytes as 1
    // then we have far too many bytes to check. May be related to I2C setup time violations: https://github.com/sparkfun/SparkFun_Ublox_Arduino_Library/issues/40
    if (bytesAvailable & ((uint16_t)1 << 15))
    {
      // Clear the MSbit
      bytesAvailable &= ~((uint16_t)1 << 15);
    }

    debugPrint("checkUbloxI2C: ");
    debugPrint(bytesAvailable);
    debugPrintln(" bytes available");

    while (bytesAvailable)
    {
      // Limit to 32 bytes or whatever the buffer limit is for given platform
      uint16_t bytesToRead = bytesAvailable; // 16-bit
      if (bytesToRead > i2cTransactionSize)  // Limit for i2cTransactionSize is 8-bit
        bytesToRead = i2cTransactionSize;

      // Here it would be desireable to use a restart where possible / supported, but only if there will be multiple reads.
      // However, if an individual requestFrom fails, we could end up leaving the bus hanging.
      // On balance, it is probably safest to not use restarts here.
      uint8_t buf[i2cTransactionSize];
      uint8_t bytesReturned = readBytes(buf, (uint8_t)bytesToRead);
      if ((uint16_t)bytesReturned == bytesToRead)
      {
        for (uint16_t x = 0; x < bytesToRead; x++)
        {
          process(buf[x], incomingUBX, requestedClass, requestedID); // Process this valid character
        }
      }
      else
      {
        // Something has gone very wrong. Sensor did not respond - or a bus error happened...
        if (_resetCurrentSentenceOnBusError)
          currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // Reset the sentence to being looking for a new start char
        debugPrintln("checkUbloxI2C: bus error? bytesReturned != bytesToRead", true); // Important
        return (false);
      }

      bytesAvailable -= bytesToRead;
    }
  }

  return (true);

} // end checkUbloxI2C()

// Checks Serial for data, passing any new bytes to process()
bool DevUBLOXGNSS::checkUbloxSerial(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID)
{
  if (available())
  {
    while (available())
    {
      uint8_t chr;
      readBytes(&chr, 1);
      process(chr, incomingUBX, requestedClass, requestedID);
    }
    return (true);
  }
  return (false);
} // end checkUbloxSerial()

bool DevUBLOXGNSS::processSpiBuffer(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID)
{
  bool retVal = false;
  // Process the contents of the SPI buffer if not empty!
  if (spiBuffer != nullptr)
  {
    if (spiBufferIndex > 0)
    {
      retVal = true;
      for (size_t i = 0; i < spiBufferIndex; i++)
      {
        process(spiBuffer[i], incomingUBX, requestedClass, requestedID);
      }
      spiBufferIndex = 0;
    }
  }

  return retVal;
}

// Checks SPI for data, passing any new bytes to process()
//
// SPI has no available(). When the module has no data for us, it clocks out 0xFF.
// We read the module in blocks of up to spiTransactionSize bytes (writeReadBytes) - not one byte at a time -
// as the per-transaction overhead can be significant (e.g. ESP-IDF spi_device_polling_transmit). This matters
// at high data rates, e.g. RAWX at 20Hz. Reading past the end of the module's data is harmless: the extra bytes
// are 0xFF "no data" filler, which process() ignores when currentSentence is NONE. A 0xFF within a message
// (e.g. in RAWX data) is still processed correctly because currentSentence is not NONE.
// We stop when a block ends with 0xFF and no sentence is in progress (the module has no more data), or after
// kMaxSpiBytesPerCheck bytes so that checkUblox() always returns - letting the caller write to SD, process
// callbacks and let other RTOS tasks run - even if the module's data never pauses. process() keeps its state
// between calls, so stopping part-way through a message is safe.
bool DevUBLOXGNSS::checkUbloxSpi(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID)
{
  bool retVal = processSpiBuffer(incomingUBX, requestedClass, requestedID);

  const size_t kMaxSpiBlockSize = 128;       // Maximum bytes per SPI transaction (limits the stack used here)
  const size_t kMaxSpiBytesPerCheck = 16384; // Maximum bytes to read per call of checkUbloxSpi

  size_t blockSize = spiTransactionSize;
  if (blockSize > kMaxSpiBlockSize)
    blockSize = kMaxSpiBlockSize;
  if (blockSize == 0)
    blockSize = 1;

  uint8_t txBytes[kMaxSpiBlockSize];
  uint8_t rxBytes[kMaxSpiBlockSize];
  memset(txBytes, 0xFF, blockSize); // Write 0xFF while reading - the module ignores 0xFF

  size_t bytesRead = 0;
  bool dataReceived = false;

  while (bytesRead < kMaxSpiBytesPerCheck)
  {
    if (writeReadBytes(txBytes, rxBytes, (uint8_t)blockSize) != blockSize)
      break; // Bus error

    bytesRead += blockSize;

    for (size_t i = 0; i < blockSize; i++)
    {
      // Skip 0xFF filler between messages. Everything else - including 0xFF within a message - is processed
      if ((rxBytes[i] != 0xFF) || (currentSentence != SFE_UBLOX_SENTENCE_TYPE_NONE))
      {
        process(rxBytes[i], incomingUBX, requestedClass, requestedID);
        dataReceived = true;
      }
    }

    // If the block ended with 0xFF and we are not part-way through a sentence, the module has no more data for us
    if ((rxBytes[blockSize - 1] == 0xFF) && (currentSentence == SFE_UBLOX_SENTENCE_TYPE_NONE))
      break;
  }

  if (!dataReceived)
  {
    // The module has no data for us. Delay and return
    sfe_delay(spiPollingWait);
    return (retVal);
  }

  return (true);

} // end checkUbloxSpi()

// PRIVATE: Check if we have storage allocated for an incoming "automatic" message
// Also calculate how much RAM is needed to store the payload for a given automatic message
bool DevUBLOXGNSS::autoLookup(uint8_t Class, uint8_t ID, uint16_t *maxSize)
{
  if (maxSize != nullptr)
    *maxSize = 0;

  ubxMessage *ubxMessagePtr = ubxMessages.find(Class, ID);
  if (ubxMessagePtr)
  {
    if (maxSize != nullptr)
        *maxSize = ubxMessagePtr->_messageLength;
    return (ubxMessagePtr->_storage != nullptr);
  }

  switch (Class)
  {
  case UBX_CLASS_NAV:
    // UBX_NAV_SAT and UBX_NAV_SIG are both handled above via the registry (ubxNAVSAT/ubxNAVSIG
    // are now self-registered) - see AGENTS.md "Adding the variable-length UBX messages".
    break;
  case UBX_CLASS_RXM:
    // UBX_RXM_SFRBX, UBX_RXM_RAWX, UBX_RXM_MEASX, UBX_RXM_PMP and UBX_RXM_QZSSL6 are all
    // handled above via the registry (ubxRXMSFRBX/ubxRXMRAWX/ubxRXMMEASX/ubxRXMPMP/
    // ubxRXMQZSSL6 are now self-registered) - see AGENTS.md "Adding the variable-length UBX
    // messages", "Adding support for RXM-SFRBX", "Adding support for RXM-PMP" and "Adding
    // support for RXM-QZSSL6".
    break;
  case UBX_CLASS_MON:
    // UBX_MON_COMMS is handled above via the registry (ubxMONCOMMS is now self-registered) -
    // see AGENTS.md "Adding the variable-length UBX messages".
    break;
  case UBX_CLASS_ESF:
    // UBX_ESF_MEAS, UBX_ESF_RAW and UBX_ESF_STATUS are all handled above via the registry
    // (ubxESFMEAS/ubxESFRAW/ubxESFSTATUS are now self-registered) - see AGENTS.md "Adding
    // support for ESF-MEAS" and "Adding support for ESF-RAW and ESF-STATUS".
    break;
  case UBX_CLASS_MGA:
    if (ID == UBX_MGA_ACK_DATA0)
    {
      if (maxSize != nullptr)
        *maxSize = UBX_MGA_ACK_DATA0_LEN;
      return (packetUBXMGAACK != nullptr);
    }
    else if (ID == UBX_MGA_DBD)
    {
      if (maxSize != nullptr)
        *maxSize = UBX_MGA_DBD_LEN;
      return (packetUBXMGADBD != nullptr);
    }
    break;
  case UBX_CLASS_SEC:
    // UBX_SEC_SIG (Version 3 - see ubxSECSIG.h) is handled above via the registry (ubxSECSIG is
    // now self-registered) - see AGENTS.md "Adding the variable-length UBX messages".
    break;
  default:
    return false;
    break;
  }
  return false;
}

// Processes NMEA, RTCM and UBX binary sentences one byte at a time
// Take a given byte and file it into the proper array
void DevUBLOXGNSS::process(uint8_t incoming, ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID)
{
  // Update storedClass and storedID if either requestedClass or requestedID is non-zero,
  // otherwise leave unchanged. This allows calls of checkUblox() (which defaults to checkUblox(0,0))
  // by other threads without overwriting the requested / expected Class and ID.
  volatile static uint8_t storedClass = 0;
  volatile static uint8_t storedID = 0;
  static size_t payloadAutoBytes;
  if (requestedClass || requestedID) // If either is non-zero, store the requested Class and ID
  {
    storedClass = requestedClass;
    storedID = requestedID;
  }

  static char storedNMEAID[4] = { 0,0,0,0 }; // Store the NMEA message ID: e.g. GGA

  _outputPort.write(incoming); // Echo this byte to the serial port

  if ((currentSentence == SFE_UBLOX_SENTENCE_TYPE_NONE) || (currentSentence == SFE_UBLOX_SENTENCE_TYPE_NMEA))
  {
    if (incoming == UBX_SYNCH_1) // UBX binary frames start with 0xB5, aka μ
    {
      // This is the start of a binary sentence. Reset flags.
      // We still don't know the response class
      ubxFrameCounter = 0;
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_UBX;
      // Reset the packetBuf.counter even though we will need to reset it again when ubxFrameCounter == 2
      packetBuf.counter = 0;
      ignoreThisPayload = false; // We should not ignore this payload - yet
      // Store data in packetBuf until we know if we have a stored class and ID match
      activePacketBuffer = SFE_UBLOX_PACKET_PACKETBUF;
    }
    else if (incoming == '$')
    {
      nmeaByteCounter = 0; // Reset the NMEA byte counter
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_NMEA;
    }
    else if (incoming == 0xD3) // RTCM frames start with 0xD3
    {
      rtcmFrameCounter = 0;
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_RTCM;
    }
    else
    {
      // This character is unknown or we missed the previous start of a sentence
      // Or it could be a 0xFF from a SPI transaction
    }
  }

  uint16_t maxPayload = 0;

  // Depending on the sentence, pass the character to the individual processor
  if (currentSentence == SFE_UBLOX_SENTENCE_TYPE_UBX)
  {
    // Decide what type of response this is
    if ((ubxFrameCounter == 0) && (incoming != UBX_SYNCH_1))      // ISO 'μ'
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE;             // Something went wrong. Reset.
    else if ((ubxFrameCounter == 1) && (incoming != UBX_SYNCH_2)) // ASCII 'b'
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE;             // Something went wrong. Reset.
    // Note to future self:
    // There may be some duplication / redundancy in the next few lines as processUBX will also
    // load information into packetBuf, but we'll do it here too for clarity
    else if (ubxFrameCounter == 2) // Class
    {
      // Record the class in packetBuf until we know what to do with it
      packetBuf.cls = incoming; // (Duplication)
      rollingChecksumA = 0;     // Reset our rolling checksums here (not when we receive the 0xB5)
      rollingChecksumB = 0;
      packetBuf.counter = 0;                                   // Reset the packetBuf.counter (again)
      packetBuf.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // Reset the packet validity (redundant?)
      packetBuf.startingSpot = incomingUBX->startingSpot;      // Copy the startingSpot
    }
    else if (ubxFrameCounter == 3) // ID
    {
      // Record the ID in packetBuf until we know what to do with it
      packetBuf.id = incoming; // (Duplication)
      // We can now identify the type of response
      // If the packet we are receiving is not an ACK then check for a class and ID match
      if (packetBuf.cls != UBX_CLASS_ACK)
      {
        bool logBecauseAuto = autoLookup(packetBuf.cls, packetBuf.id, &maxPayload);
        bool logBecauseEnabled = logThisUBX(packetBuf.cls, packetBuf.id) || processThisUBX(packetBuf.cls, packetBuf.id);

        // This is not an ACK so check for a class and ID match
        if ((packetBuf.cls == storedClass) && (packetBuf.id == storedID))
        {
          // This is not an ACK and we have a class and ID match
          // So start diverting data into incomingUBX (usually packetCfg)
          activePacketBuffer = SFE_UBLOX_PACKET_PACKETCFG;
          incomingUBX->cls = packetBuf.cls; // Copy the class and ID into incomingUBX (usually packetCfg)
          incomingUBX->id = packetBuf.id;
          incomingUBX->counter = packetBuf.counter; // Copy over the .counter too
        }
        // This is not an ACK and we do not have a complete class and ID match
        // So let's check if this is an "automatic" message which has its own storage defined
        else if (logBecauseAuto || logBecauseEnabled)
        {
          // This is not the message we were expecting but it has its own storage and so we should process it anyway.
          // We'll try to use packetAuto to buffer the message (so it can't overwrite anything in packetCfg).
          // We need to allocate memory for the packetAuto payload (payloadAuto) - and delete it once
          // reception is complete.
          if (logBecauseAuto && (maxPayload == 0))
          {
            debugPrint("process: autoLookup returned ZERO maxPayload!! Class: 0x", true); // Important
            debugPrint(packetBuf.cls, HEX, true);
            debugPrint(" ID: 0x", true);
            debugPrintln(packetBuf.id, HEX, true);
          }

          // Determine the payload length
          if ((!logBecauseAuto) && (logBecauseEnabled))
            maxPayload = SFE_UBX_MAX_LENGTH;

          // Increase the payloadAuto buffer size if necessary, by removing
          // the previous buffer
          if (payloadAuto && (payloadAutoBytes < maxPayload))
          {
            delete[] payloadAuto; // Created with new[] below
            payloadAuto = nullptr;
            payloadAutoBytes = 0;
          }

          // Allocate the payloadAuto buffer if necessary
          if (payloadAuto == nullptr)
          {
            payloadAuto = new uint8_t[maxPayload];
            if (payloadAuto)
              payloadAutoBytes = maxPayload;
          }

          packetAuto.payload = payloadAuto;
          
          if (payloadAuto == nullptr) // Check if the alloc failed
          {
            debugPrint("process: memory allocation failed for \"automatic\" message: Class: 0x", true); // Important
            debugPrint(packetBuf.cls, HEX, true);
            debugPrint(" ID: 0x", true);
            debugPrintln(packetBuf.id, HEX, true);
            debugPrintln("process: \"automatic\" message could overwrite data", true);
            // The RAM allocation failed so fall back to using incomingUBX (usually packetCfg) even though we risk overwriting data
            activePacketBuffer = SFE_UBLOX_PACKET_PACKETCFG;
            incomingUBX->cls = packetBuf.cls; // Copy the class and ID into incomingUBX (usually packetCfg)
            incomingUBX->id = packetBuf.id;
            incomingUBX->counter = packetBuf.counter; // Copy over the .counter too
          }
          else
          {
            // The RAM allocation was successful so we start diverting data into packetAuto and process it
            activePacketBuffer = SFE_UBLOX_PACKET_PACKETAUTO;
            packetAuto.cls = packetBuf.cls; // Copy the class and ID into packetAuto
            packetAuto.id = packetBuf.id;
            packetAuto.counter = packetBuf.counter;           // Copy over the .counter too
            packetAuto.startingSpot = packetBuf.startingSpot; // And the starting spot? (Probably redundant)
            debugPrint("process: incoming \"automatic\" message: Class: 0x");
            debugPrint(packetBuf.cls, HEX);
            debugPrint(" ID: 0x");
            debugPrint(packetBuf.id, HEX);
            debugPrint(" logBecauseAuto:");
            debugPrint(logBecauseAuto);
            debugPrint(" logBecauseEnabled:");
            debugPrintln(logBecauseEnabled);
          }
        }
        else
        {
          // This is not an ACK and we do not have a class and ID match
          // so we should keep diverting data into packetBuf and ignore the payload
          ignoreThisPayload = true;
        }
      }
      else
      {
        // This is an ACK so it is to early to do anything with it
        // We need to wait until we have received the length and data bytes
        // So we should keep diverting data into packetBuf
      }
    }
    else if (ubxFrameCounter == 4) // Length LSB
    {
      // We should save the length in packetBuf even if activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG
      packetBuf.len = incoming; // (Duplication)
    }
    else if (ubxFrameCounter == 5) // Length MSB
    {
      // We should save the length in packetBuf even if activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG
      packetBuf.len |= incoming << 8; // (Duplication)
    }
    else if (ubxFrameCounter == 6) // This should be the first byte of the payload unless .len is zero
    {
      if (packetBuf.len == 0) // Check if length is zero (hopefully this is impossible!)
      {
        debugPrint("process: ZERO LENGTH packet received: Class: 0x", true); // Important
        debugPrint(packetBuf.cls, HEX, true);
        debugPrint(" ID: 0x", true);
        debugPrintln(packetBuf.id, HEX, true);
        // If length is zero (!) this will be the first byte of the checksum so record it
        packetBuf.checksumA = incoming;
      }
      else
      {
        // The length is not zero so record this byte in the payload
        packetBuf.payload[0] = incoming;
      }
    }
    else if (ubxFrameCounter == 7) // This should be the second byte of the payload unless .len is zero or one
    {
      if (packetBuf.len == 0) // Check if length is zero (hopefully this is impossible!)
      {
        // If length is zero (!) this will be the second byte of the checksum so record it
        packetBuf.checksumB = incoming;
      }
      else if (packetBuf.len == 1) // Check if length is one
      {
        // The length is one so this is the first byte of the checksum
        packetBuf.checksumA = incoming;
      }
      else // Length is >= 2 so this must be a payload byte
      {
        packetBuf.payload[1] = incoming;
      }
      // Now that we have received two payload bytes, we can check for a matching ACK/NACK
      if ((activePacketBuffer == SFE_UBLOX_PACKET_PACKETBUF) // If we are not already processing a data packet
          && (packetBuf.cls == UBX_CLASS_ACK)                // and if this is an ACK/NACK
          && (packetBuf.payload[0] == storedClass)           // and if the class matches
          && (packetBuf.payload[1] == storedID))             // and if the ID matches
      {
        if (packetBuf.len == 2) // Check if .len is 2
        {
          // Then this is a matching ACK so copy it into packetAck
          activePacketBuffer = SFE_UBLOX_PACKET_PACKETACK;
          packetAck.cls = packetBuf.cls;
          packetAck.id = packetBuf.id;
          packetAck.len = packetBuf.len;
          packetAck.counter = packetBuf.counter;
          packetAck.payload[0] = packetBuf.payload[0];
          packetAck.payload[1] = packetBuf.payload[1];
        }
        else // Length is not 2 (hopefully this is impossible!)
        {
          debugPrint("process: ACK received with .len != 2: Class: 0x", true); // Important
          debugPrint(packetBuf.payload[0], HEX, true);
          debugPrint(" ID: 0x", true);
          debugPrint(packetBuf.payload[1], HEX, true);
          debugPrint(" len: ", true);
          debugPrintln(packetBuf.len, true);
        }
      }
    }

    // Divert incoming into the correct buffer
    if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETACK)
      processUBX(incoming, &packetAck, storedClass, storedID);
    else if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG)
      processUBX(incoming, incomingUBX, storedClass, storedID);
    else if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETBUF)
      processUBX(incoming, &packetBuf, storedClass, storedID);
    else // if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETAUTO)
      processUBX(incoming, &packetAuto, storedClass, storedID);

    // If user has assigned an output port then pipe the characters there,
    // but only if the port is different (otherwise we'll output each character twice!)
    if (_outputPort != _ubxOutputPort)
      _ubxOutputPort.write(incoming); // Echo this byte to the serial port

    // Finally, increment the frame counter
    ubxFrameCounter++;
  }
  else if (currentSentence == SFE_UBLOX_SENTENCE_TYPE_NMEA) // Process incoming NMEA mesages. Selectively log if desired.
  {
    if ((nmeaByteCounter == 0) && (incoming != '$'))
    {
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // Something went wrong. Reset. (Almost certainly redundant!)
    }
    else if ((nmeaByteCounter == 1) && (incoming != 'G'))
    {
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // Something went wrong. Reset.
    }
    else if ((nmeaByteCounter >= 0) && (nmeaByteCounter <= 5))
    {
      nmeaAddressField[nmeaByteCounter] = incoming; // Store the start character and NMEA address field
      if (nmeaByteCounter >= 3)
        storedNMEAID[nmeaByteCounter - 3] = incoming; // Store just the ID ("GGA" etc.) for quick reference
    }

    if (nmeaByteCounter == 5)
    {
      if (!_signsOfLife) // If _signsOfLife is not already true, set _signsOfLife to true if the NMEA header is valid
      {
        _signsOfLife = isNMEAHeaderValid(storedNMEAID);
      }

      // Check if we have automatic storage for this message.
      // We will only copy complete, CRC-checked messages into automatic storage.
      // In the interim, store the message in the non-Auto storage _storageNMEA.
      // Note: a polled NMEA message - like ZDA - can have storage but not be periodic.
      // If it **has storage**, it gets recorded.
      if (doesThisNMEAHaveStorage(storedNMEAID) || logThisNMEA(storedNMEAID))
      {
        if (initStorageNMEA()) // Check we have (non-Auto) storage for it
        {
          _storageNMEA->length = 6; // Set the working copy length
          memset(_storageNMEA->data, 0, maxNMEAByteCount);     // Clear the working copy
          memcpy(_storageNMEA->data, &nmeaAddressField[0], 6); // Copy the start character and address field into the working copy
        }
      }
      else
      {
        // debugPrintln("process: non-auto NMEA message", true); // Important
      }

      // Check if it should be passed to processNMEA
      if (processThisNMEA(storedNMEAID))
      {
        for (uint8_t i = 0; i < 6; i++)
        {
          processNMEA(nmeaAddressField[i]); // Process the start character and address field
          // If user has assigned an output port then pipe the characters there,
          // but only if the port is different (otherwise we'll output each character twice!)
          if (_outputPort != _nmeaOutputPort)
            _nmeaOutputPort.write(nmeaAddressField[i]); // Echo this byte to the serial port
        }
      }
    }

    if ((nmeaByteCounter > 5) || (nmeaByteCounter < 0)) // Should we add incoming to the file buffer and/or pass it to processNMEA?
    {
      if (doesThisNMEAHaveStorage(storedNMEAID) || logThisNMEA(storedNMEAID))
      {
        // This check is probably redundant.
        // currentSentence is set to SFE_UBLOX_SENTENCE_TYPE_NONE below if nmeaByteCounter == maxNMEAByteCount
        if (_storageNMEA->length < maxNMEAByteCount) // Check we have room for it
        {
          _storageNMEA->data[_storageNMEA->length] = incoming; // Store the byte
          _storageNMEA->length = _storageNMEA->length + 1;
        }
        else
        {
          debugPrintln("process: NMEA buffer is full!", true); // Important
        }
      }
      if (processThisNMEA(storedNMEAID))
      {
        processNMEA(incoming); // Pass incoming to processNMEA
        // If user has assigned an output port then pipe the characters there,
        // but only if the port is different (otherwise we'll output each character twice!)
        if (_outputPort != _nmeaOutputPort)
          _nmeaOutputPort.write(incoming); // Echo this byte to the serial port
      }
    }

    if (incoming == '*')
      nmeaByteCounter = -5; // We are expecting * plus two checksum bytes plus CR and LF

    nmeaByteCounter++; // Increment the byte counter

    if (nmeaByteCounter == maxNMEAByteCount)          // Check if we have processed too many bytes
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // Something went wrong. Reset.

    if (nmeaByteCounter == 0) // Check if we are done
    {
      // If this NMEA has Auto storage or is being logged, check the checksum
      if (doesThisNMEAHaveStorage(storedNMEAID) || logThisNMEA(storedNMEAID))
      {
        // Check the checksum: the checksum is the exclusive-OR of all characters between the $ and the *
        uint8_t nmeaChecksum = 0;
        int8_t charsChecked = 1; // Start after the $
        uint8_t thisChar = '\0';
        while ((charsChecked < maxNMEAByteCount) && (charsChecked < (_storageNMEA->length - 4)) && (thisChar != '*'))
        {
          thisChar = _storageNMEA->data[charsChecked]; // Get a char from the storage
          if (thisChar != '*')                         // Ex-or the char into the checksum - but not if it is the '*'
            nmeaChecksum ^= thisChar;
          charsChecked++; // Increment the counter
        }
        if (thisChar == '*') // Make sure we found the *
        {
          uint8_t expectedChecksum1 = (nmeaChecksum >> 4) + '0';
          if (expectedChecksum1 >= ':') // Handle Hex correctly
            expectedChecksum1 += 'A' - ':';
          uint8_t expectedChecksum2 = (nmeaChecksum & 0x0F) + '0';
          if (expectedChecksum2 >= ':') // Handle Hex correctly
            expectedChecksum2 += 'A' - ':';
          if ((expectedChecksum1 == _storageNMEA->data[charsChecked]) && (expectedChecksum2 == _storageNMEA->data[charsChecked + 1]))
          {
            // Message is valid. The checksum was OK.
            // For Auto messages, copy into the Auto _storage
            if (doesThisNMEAHaveStorage(storedNMEAID))
            {
              nmeaMessage *nmeaMessagePtr = nmeaMessages.find(storedNMEAID);
              if (nmeaMessagePtr)
              {
                uint8_t bytesToCopy = _storageNMEA->length;
                if (bytesToCopy > maxNMEAByteCount) // Just for safety. Should be impossible
                  bytesToCopy = maxNMEAByteCount;
                if (bytesToCopy > nmeaMessagePtr->_messageLength) // Could be possible?
                {
                  debugPrint("process: NMEA message length is > nmeaMessagePtr->_messageLength!");
                  bytesToCopy = nmeaMessagePtr->_messageLength;
                }
                memcpy(nmeaMessagePtr->_storage, _storageNMEA->data, bytesToCopy);
                // We don't need to NULL-terminate. _storageNMEA->data was memset to 0 above.
                nmeaMessagePtr->_moduleQueried = true; // Mark the data as fresh

                // Callback - write into the next ring-buffer slot. See AGENTS.md "Adding
                // support for NMEA GSV messages" and ubxMessageVector::storePayload(), which this
                // mirrors. For _numCallbackCopies <= 1 (every message except GSV) this reproduces
                // today's single-slot overwrite-always behavior exactly; for GSV
                // (_numCallbackCopies == 54) a full ring drops the new sentence and keeps what's
                // already buffered, so an unread sentence is never silently replaced.
                if (doesThisNMEAHaveCallback(storedNMEAID)) // Do we need to copy the data into the callback copy?
                {
                  if (nmeaMessagePtr->_numCallbackCopies <= 1)
                  {
                    memcpy(nmeaMessagePtr->_callbackStorage, _storageNMEA->data, bytesToCopy);
                    nmeaMessagePtr->_callbackStorage[bytesToCopy] = 0; // NULL-terminate
                    nmeaMessagePtr->_callbackCount = 1; // head/tail stay at 0 - a degenerate 1-slot ring
                  }
                  else if (nmeaMessagePtr->_callbackCount < nmeaMessagePtr->_numCallbackCopies) // Ring has a free slot
                  {
                    uint8_t *slot = nmeaMessagePtr->_callbackStorage + ((uint32_t)nmeaMessagePtr->_callbackHead * nmeaMessagePtr->_messageLength);
                    memcpy(slot, _storageNMEA->data, bytesToCopy);
                    slot[bytesToCopy] = 0; // NULL-terminate this slot
                    nmeaMessagePtr->_callbackHead = (uint8_t)((nmeaMessagePtr->_callbackHead + 1) % nmeaMessagePtr->_numCallbackCopies);
                    nmeaMessagePtr->_callbackCount++;
                  }
                  // else: ring is full - drop this sentence, keep what's already buffered
                }
              }
            }
            if (logThisNMEA(storedNMEAID))
              storeFileBytes(_storageNMEA->data, _storageNMEA->length); // Add NMEA to the file buffer
          }
          else
          {
            debugPrint("process: NMEA checksum fail! Expected ", true); // Important
            char checkHex[3];
            sprintf(checkHex, "%c%c", expectedChecksum1, expectedChecksum2);
            debugPrint(checkHex, true);
            debugPrint(" Got ", true);
            sprintf(checkHex, "%c%c", _storageNMEA->data[charsChecked], _storageNMEA->data[charsChecked + 1]);
            debugPrint(checkHex, true);
            debugPrint("\r\n", true);
          }
        }
      }
      currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // All done!
    }
  }
  else if (currentSentence == SFE_UBLOX_SENTENCE_TYPE_RTCM)
  {

    // RTCM Logging
    if (_storageRTCM != nullptr) // Check if RTCM logging storage exists
    {
      if (rtcmFrameCounter == 0)
      {
        _storageRTCM->dataMessage[0] = incoming;
        _storageRTCM->rollingChecksum = 0; // Initialize the checksum. Seed is 0x000000
      }
      else if (rtcmFrameCounter == 1)
      {
        _storageRTCM->dataMessage[1] = incoming;
        _storageRTCM->messageLength = (uint16_t)(incoming & 0x03) << 8;
      }
      else if (rtcmFrameCounter == 2)
      {
        _storageRTCM->dataMessage[2] = incoming;
        _storageRTCM->messageLength |= incoming;
      }

      // Store the mesage data (and CRC) - now that the message length is known
      if ((rtcmFrameCounter >= 3) && (rtcmFrameCounter < (_storageRTCM->messageLength + 6)) && (rtcmFrameCounter < (3 + SFE_UBLOX_MAX_RTCM_MSG_LEN + 3)))
        _storageRTCM->dataMessage[rtcmFrameCounter] = incoming;

      // Add incoming header and data bytes to the checksum
      if ((rtcmFrameCounter < 3) || ((rtcmFrameCounter >= 3) && (rtcmFrameCounter < (_storageRTCM->messageLength + 3))))
        crc24q(incoming, &_storageRTCM->rollingChecksum);

      // Check if all bytes have been received
      if ((rtcmFrameCounter >= 3) && (rtcmFrameCounter == _storageRTCM->messageLength + 5))
      {
        uint32_t expectedChecksum = _storageRTCM->dataMessage[_storageRTCM->messageLength + 3];
        expectedChecksum <<= 8;
        expectedChecksum |= _storageRTCM->dataMessage[_storageRTCM->messageLength + 4];
        expectedChecksum <<= 8;
        expectedChecksum |= _storageRTCM->dataMessage[_storageRTCM->messageLength + 5];

        if (expectedChecksum == _storageRTCM->rollingChecksum) // Does the checksum match?
        {
          // Extract the message type and check if it should be logged

          // Extract the message number from the first 12 bits
          uint16_t messageType = ((uint16_t)_storageRTCM->dataMessage[3]) << 4;
          messageType |= _storageRTCM->dataMessage[4] >> 4;
          uint16_t messageSubType = ((uint16_t)_storageRTCM->dataMessage[4] & 0x0F) << 8;
          messageSubType |= _storageRTCM->dataMessage[5];
          bool logThisRTCM = false;

          debugPrint("process: valid RTCM message type: ");
          debugPrint(messageType);
          if (messageType == 4072)
          {
            debugPrint("_");
            debugPrint(messageSubType);
          }
          debugPrintln();

          if (!logThisRTCM)
            logThisRTCM = (messageType == 1001) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1001 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1002) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1002 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1003) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1003 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1004) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1004 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1005) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1005 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1006) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1006 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1007) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1007 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1009) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1009 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1010) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1010 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1011) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1011 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1012) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1012 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1033) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1033 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1074) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1074 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1075) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1075 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1077) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1077 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1084) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1084 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1085) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1085 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1087) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1087 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1094) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1094 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1095) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1095 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1097) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1097 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1124) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1124 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1125) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1125 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1127) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1127 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 1230) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE1230 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 4072) && (messageSubType == 0) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE4072_0 == 1));
          if (!logThisRTCM)
            logThisRTCM = (messageType == 4072) && (messageSubType == 1) && ((_logRTCM.bits.all == 1) || (_logRTCM.bits.UBX_RTCM_TYPE4072_1 == 1));

          if (logThisRTCM) // Should we log this message?
          {
            storeFileBytes(_storageRTCM->dataMessage, _storageRTCM->messageLength + 6);
          }

          // If there is space in the RTCM buffer, store the data there too
          if (rtcmBufferSpaceAvailable() >= _storageRTCM->messageLength + 6)
            storeRTCMBytes(_storageRTCM->dataMessage, _storageRTCM->messageLength + 6);

          // Check "Auto" RTCM
          if ((messageType == 1005) && (_storageRTCM->messageLength == RTCM_1005_MSG_LEN_BYTES) && (storageRTCM1005 != nullptr))
          {
            extractRTCM1005(&storageRTCM1005->data, &_storageRTCM->dataMessage[3]);

            storageRTCM1005->automaticFlags.flags.bits.dataValid = 1; // Mark the data as valid and unread
            storageRTCM1005->automaticFlags.flags.bits.dataRead = 0;

            if (storageRTCM1005->callbackData != nullptr)                              // Should we copy the data for the callback?
              if (storageRTCM1005->callbackPointerPtr != nullptr)                      // Has the callback been defined?
                if (storageRTCM1005->automaticFlags.flags.bits.callbackDataValid == 0) // Only overwrite the callback copy if it has been read
                {
                  memcpy(storageRTCM1005->callbackData, &storageRTCM1005->data, sizeof(RTCM_1005_data_t));
                  storageRTCM1005->automaticFlags.flags.bits.callbackDataValid = 1;
                }
          }
        }
        else
        {
          debugPrintln("process: RTCM checksum fail!", true); // Important
        }
      }
    }

    currentSentence = processRTCMframe(incoming, &rtcmFrameCounter); // Deal with RTCM bytes

    // If user has assigned an output port then pipe the characters there,
    // but only if the port is different (otherwise we'll output each character twice!)
    if (_outputPort != _rtcmOutputPort)
      _rtcmOutputPort.write(incoming); // Echo this byte to the serial port
  }
}

// PRIVATE: Return true if we should add this NMEA message to the file buffer for logging
bool DevUBLOXGNSS::logThisNMEA(const char *msgId)
{
  bool logMe = false;
  if (_logNMEA.bits.all == 1)
    logMe = true;
  if ((msgId[0] == 'D') && (msgId[1] == 'T') && (msgId[2] == 'M') && (_logNMEA.bits.UBX_NMEA_DTM == 1))
    logMe = true;
  if (msgId[0] == 'G')
  {
    if ((msgId[1] == 'A') && (msgId[2] == 'Q') && (_logNMEA.bits.UBX_NMEA_GAQ == 1))
      logMe = true;
    if ((msgId[1] == 'B') && (msgId[2] == 'Q') && (_logNMEA.bits.UBX_NMEA_GBQ == 1))
      logMe = true;
    if ((msgId[1] == 'B') && (msgId[2] == 'S') && (_logNMEA.bits.UBX_NMEA_GBS == 1))
      logMe = true;
    if ((msgId[1] == 'G') && (msgId[2] == 'A') && (_logNMEA.bits.UBX_NMEA_GGA == 1))
      logMe = true;
    if ((msgId[1] == 'L') && (msgId[2] == 'L') && (_logNMEA.bits.UBX_NMEA_GLL == 1))
      logMe = true;
    if ((msgId[1] == 'L') && (msgId[2] == 'Q') && (_logNMEA.bits.UBX_NMEA_GLQ == 1))
      logMe = true;
    if ((msgId[1] == 'N') && (msgId[2] == 'Q') && (_logNMEA.bits.UBX_NMEA_GNQ == 1))
      logMe = true;
    if ((msgId[1] == 'N') && (msgId[2] == 'S') && (_logNMEA.bits.UBX_NMEA_GNS == 1))
      logMe = true;
    if ((msgId[1] == 'P') && (msgId[2] == 'Q') && (_logNMEA.bits.UBX_NMEA_GPQ == 1))
      logMe = true;
    if ((msgId[1] == 'Q') && (msgId[2] == 'Q') && (_logNMEA.bits.UBX_NMEA_GQQ == 1))
      logMe = true;
    if ((msgId[1] == 'R') && (msgId[2] == 'S') && (_logNMEA.bits.UBX_NMEA_GRS == 1))
      logMe = true;
    if ((msgId[1] == 'S') && (msgId[2] == 'A') && (_logNMEA.bits.UBX_NMEA_GSA == 1))
      logMe = true;
    if ((msgId[1] == 'S') && (msgId[2] == 'T') && (_logNMEA.bits.UBX_NMEA_GST == 1))
      logMe = true;
    if ((msgId[1] == 'S') && (msgId[2] == 'V') && (_logNMEA.bits.UBX_NMEA_GSV == 1))
      logMe = true;
  }
  if ((msgId[0] == 'R') && (msgId[1] == 'L') && (msgId[2] == 'M') && (_logNMEA.bits.UBX_NMEA_RLM == 1))
    logMe = true;
  if ((msgId[0] == 'R') && (msgId[1] == 'M') && (msgId[2] == 'C') && (_logNMEA.bits.UBX_NMEA_RMC == 1))
    logMe = true;
  if ((msgId[0] == 'T') && (msgId[1] == 'H') && (msgId[2] == 'S') && (_logNMEA.bits.UBX_NMEA_THS == 1))
    logMe = true;
  if ((msgId[0] == 'T') && (msgId[1] == 'X') && (msgId[2] == 'T') && (_logNMEA.bits.UBX_NMEA_TXT == 1))
    logMe = true;
  if ((msgId[0] == 'V') && (msgId[1] == 'L') && (msgId[2] == 'W') && (_logNMEA.bits.UBX_NMEA_VLW == 1))
    logMe = true;
  if ((msgId[0] == 'V') && (msgId[1] == 'T') && (msgId[2] == 'G') && (_logNMEA.bits.UBX_NMEA_VTG == 1))
    logMe = true;
  if ((msgId[0] == 'Z') && (msgId[1] == 'D') && (msgId[2] == 'A') && (_logNMEA.bits.UBX_NMEA_ZDA == 1))
    logMe = true;

  if (logMe)                   // Message should be logged.
    logMe = initStorageNMEA(); // Check we have non-Auto storage for it
  return (logMe);
}

// PRIVATE: Return true if the NMEA header is valid
bool DevUBLOXGNSS::isNMEAHeaderValid(const char *msgId)
{
  if ((msgId[0] == 'D') && (msgId[1] == 'T') && (msgId[2] == 'M'))
    return (true);
  if (msgId[0] == 'G')
  {
    if ((msgId[1] == 'A') && (msgId[2] == 'Q'))
      return (true);
    if ((msgId[1] == 'B') && (msgId[2] == 'Q'))
      return (true);
    if ((msgId[1] == 'B') && (msgId[2] == 'S'))
      return (true);
    if ((msgId[1] == 'G') && (msgId[2] == 'A'))
      return (true);
    if ((msgId[1] == 'L') && (msgId[2] == 'L'))
      return (true);
    if ((msgId[1] == 'L') && (msgId[2] == 'Q'))
      return (true);
    if ((msgId[1] == 'N') && (msgId[2] == 'Q'))
      return (true);
    if ((msgId[1] == 'N') && (msgId[2] == 'S'))
      return (true);
    if ((msgId[1] == 'P') && (msgId[2] == 'Q'))
      return (true);
    if ((msgId[1] == 'Q') && (msgId[2] == 'Q'))
      return (true);
    if ((msgId[1] == 'R') && (msgId[2] == 'S'))
      return (true);
    if ((msgId[1] == 'S') && (msgId[2] == 'A'))
      return (true);
    if ((msgId[1] == 'S') && (msgId[2] == 'T'))
      return (true);
    if ((msgId[1] == 'S') && (msgId[2] == 'V'))
      return (true);
  }
  if ((msgId[0] == 'R') && (msgId[1] == 'L') && (msgId[2] == 'M'))
    return (true);
  if ((msgId[0] == 'R') && (msgId[1] == 'M') && (msgId[2] == 'C'))
    return (true);
  if ((msgId[0] == 'T') && (msgId[1] == 'H') && (msgId[2] == 'S'))
    return (true);
  if ((msgId[0] == 'T') && (msgId[1] == 'X') && (msgId[2] == 'T'))
    return (true);
  if ((msgId[0] == 'V') && (msgId[1] == 'L') && (msgId[2] == 'W'))
    return (true);
  if ((msgId[0] == 'V') && (msgId[1] == 'T') && (msgId[2] == 'G'))
    return (true);
  if ((msgId[0] == 'Z') && (msgId[1] == 'D') && (msgId[2] == 'A'))
    return (true);
  return (false);
}

// PRIVATE: Return true if we should pass this NMEA message to processNMEA
bool DevUBLOXGNSS::processThisNMEA(const char *msgId)
{
  if (_processNMEA.bits.all == 1)
    return (true);
  if ((msgId[0] == 'D') && (msgId[1] == 'T') && (msgId[2] == 'M') && (_processNMEA.bits.UBX_NMEA_DTM == 1))
    return (true);
  if (msgId[0] == 'G')
  {
    if ((msgId[1] == 'A') && (msgId[2] == 'Q') && (_processNMEA.bits.UBX_NMEA_GAQ == 1))
      return (true);
    if ((msgId[1] == 'B') && (msgId[2] == 'Q') && (_processNMEA.bits.UBX_NMEA_GBQ == 1))
      return (true);
    if ((msgId[1] == 'B') && (msgId[2] == 'S') && (_processNMEA.bits.UBX_NMEA_GBS == 1))
      return (true);
    if ((msgId[1] == 'G') && (msgId[2] == 'A') && (_processNMEA.bits.UBX_NMEA_GGA == 1))
      return (true);
    if ((msgId[1] == 'L') && (msgId[2] == 'L') && (_processNMEA.bits.UBX_NMEA_GLL == 1))
      return (true);
    if ((msgId[1] == 'L') && (msgId[2] == 'Q') && (_processNMEA.bits.UBX_NMEA_GLQ == 1))
      return (true);
    if ((msgId[1] == 'N') && (msgId[2] == 'Q') && (_processNMEA.bits.UBX_NMEA_GNQ == 1))
      return (true);
    if ((msgId[1] == 'N') && (msgId[2] == 'S') && (_processNMEA.bits.UBX_NMEA_GNS == 1))
      return (true);
    if ((msgId[1] == 'P') && (msgId[2] == 'Q') && (_processNMEA.bits.UBX_NMEA_GPQ == 1))
      return (true);
    if ((msgId[1] == 'Q') && (msgId[2] == 'Q') && (_processNMEA.bits.UBX_NMEA_GQQ == 1))
      return (true);
    if ((msgId[1] == 'R') && (msgId[2] == 'S') && (_processNMEA.bits.UBX_NMEA_GRS == 1))
      return (true);
    if ((msgId[1] == 'S') && (msgId[2] == 'A') && (_processNMEA.bits.UBX_NMEA_GSA == 1))
      return (true);
    if ((msgId[1] == 'S') && (msgId[2] == 'T') && (_processNMEA.bits.UBX_NMEA_GST == 1))
      return (true);
    if ((msgId[1] == 'S') && (msgId[2] == 'V') && (_processNMEA.bits.UBX_NMEA_GSV == 1))
      return (true);
  }
  if ((msgId[0] == 'R') && (msgId[1] == 'L') && (msgId[2] == 'M') && (_processNMEA.bits.UBX_NMEA_RLM == 1))
    return (true);
  if ((msgId[0] == 'R') && (msgId[1] == 'M') && (msgId[2] == 'C') && (_processNMEA.bits.UBX_NMEA_RMC == 1))
    return (true);
  if ((msgId[0] == 'T') && (msgId[1] == 'H') && (msgId[2] == 'S') && (_processNMEA.bits.UBX_NMEA_THS == 1))
    return (true);
  if ((msgId[0] == 'T') && (msgId[1] == 'X') && (msgId[2] == 'T') && (_processNMEA.bits.UBX_NMEA_TXT == 1))
    return (true);
  if ((msgId[0] == 'V') && (msgId[1] == 'L') && (msgId[2] == 'W') && (_processNMEA.bits.UBX_NMEA_VLW == 1))
    return (true);
  if ((msgId[0] == 'V') && (msgId[1] == 'T') && (msgId[2] == 'G') && (_processNMEA.bits.UBX_NMEA_VTG == 1))
    return (true);
  if ((msgId[0] == 'Z') && (msgId[1] == 'D') && (msgId[2] == 'A') && (_processNMEA.bits.UBX_NMEA_ZDA == 1))
    return (true);
  return (false);
}

// This is the default or generic NMEA processor. We're only going to pipe the data to serial port so we can see it.
// User could overwrite this function to pipe characters to nmea.process(c) of tinyGPS or MicroNMEA
// Or user could pipe each character to a buffer, radio, etc.
void DevUBLOXGNSS::processNMEA(char incoming)
{
  (void)incoming;
}

// Check if the NMEA message (in nmeaAddressField) is "auto" (i.e. its isAutomatic flag is set)
bool DevUBLOXGNSS::isThisNMEAauto(const char *msgId)
{
  bool automatic = false;
  if (nmeaMessages.isAutomatic(msgId, &automatic) != SFE_UBLOX_STATUS_SUCCESS)
    return false;
  return automatic;
}

// Check if the NMEA message (in nmeaAddressField) has dedicated RAM allocated for it
bool DevUBLOXGNSS::doesThisNMEAHaveStorage(const char *msgId)
{
    nmeaMessage *msg = nmeaMessages.find(msgId);
    if (msg == nullptr)
        return false;
    return msg->_storage != nullptr;
}

// Do we need to copy the data into the callback copy?
bool DevUBLOXGNSS::doesThisNMEAHaveCallback(const char *msgId)
{
    nmeaMessage *msg = nmeaMessages.find(msgId);
    if (msg == nullptr)
        return false;
    return msg->_callbackPtr != nullptr;
}

// We need to be able to identify an RTCM packet and then the length
// so that we know when the RTCM message is completely received and we then start
// listening for other sentences (like NMEA or UBX)
// RTCM packet structure is very odd. I never found RTCM STANDARD 10403.2 but
// http://d1.amobbs.com/bbs_upload782111/files_39/ourdev_635123CK0HJT.pdf is good
// https://dspace.cvut.cz/bitstream/handle/10467/65205/F3-BP-2016-Shkalikava-Anastasiya-Prenos%20polohove%20informace%20prostrednictvim%20datove%20site.pdf?sequence=-1
// Lead me to: https://forum.u-blox.com/index.php/4348/how-to-read-rtcm-messages-from-neo-m8p
// RTCM 3.2 bytes look like this:
// Byte 0: Always 0xD3
// Byte 1: 6-bits of zero
// Byte 2: 10-bits of length of this packet including the first two-ish header bytes, + 6.
// byte 3 + 4 bits: Msg type 12 bits
// Example: D3 00 7C 43 F0 ... / 0x7C = 124+6 = 130 bytes in this packet, 0x43F = Msg type 1087
DevUBLOXGNSS::sfe_ublox_sentence_types_e DevUBLOXGNSS::processRTCMframe(uint8_t incoming, uint16_t *rtcmFrameCounter)
{
  static uint16_t rtcmLen = 0; // Static - length is retained between calls

  if (*rtcmFrameCounter == 1)
  {
    rtcmLen = (incoming & 0x03) << 8; // Get the last two bits of this byte. Bits 8&9 of 10-bit length
  }
  else if (*rtcmFrameCounter == 2)
  {
    rtcmLen |= incoming; // Bits 0-7 of packet length
    rtcmLen += 6;        // There are 6 additional bytes of what we presume is header, msgType, CRC, and stuff
  }
  /*else if (rtcmFrameCounter == 3)
  {
    rtcmMsgType = incoming << 4; //Message Type, MS 4 bits
  }
  else if (rtcmFrameCounter == 4)
  {
    rtcmMsgType |= (incoming >> 4); //Message Type, bits 0-7
  }*/

  *rtcmFrameCounter = *rtcmFrameCounter + 1; // Increment rtcmFrameCounter

  processRTCM(incoming); // Here is where we expose this byte to the user

  // If rtcmLen is not yet known, return SFE_UBLOX_SENTENCE_TYPE_RTCM
  if (*rtcmFrameCounter <= 2) // If this is header byte 0 or 1 (rtcmFrameCounter has been incremented)
    return SFE_UBLOX_SENTENCE_TYPE_RTCM;

  // Reset and start looking for next sentence type when done
  return (*rtcmFrameCounter == rtcmLen) ? SFE_UBLOX_SENTENCE_TYPE_NONE : SFE_UBLOX_SENTENCE_TYPE_RTCM;
}

// This function is called for each byte of an RTCM frame
// Ths user can overwrite this function and process the RTCM frame as they please
// Bytes can be piped to Serial or other interface. The consumer could be a radio or the internet (Ntrip broadcaster)
void DevUBLOXGNSS::processRTCM(uint8_t incoming)
{
  (void)incoming;
}

// Given a character, file it away into the uxb packet structure
// Set valid to VALID or NOT_VALID once sentence is completely received and passes or fails CRC
// The payload portion of the packet can be 100s of bytes but the max array size is packetCfgPayloadSize bytes.
// startingSpot can be set so we only record a subset of bytes within a larger packet.
void DevUBLOXGNSS::processUBX(uint8_t incoming, ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID)
{
  // If incomingUBX is a user-defined custom packet, then the payload size could be different to packetCfgPayloadSize.
  // TO DO: update this to prevent an overrun when receiving an automatic message
  //        and the incomingUBX payload size is smaller than packetCfgPayloadSize.
  uint16_t maximum_payload_size;
  if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG)
    maximum_payload_size = packetCfgPayloadSize;
  else if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETAUTO)
  {
    // Calculate maximum payload size once Class and ID have been received
    // (This check is probably redundant as activePacketBuffer can only be SFE_UBLOX_PACKET_PACKETAUTO
    //  when ubxFrameCounter >= 3)
    // if (incomingUBX->counter >= 2)
    //{

    bool logBecauseAuto = autoLookup(incomingUBX->cls, incomingUBX->id, &maximum_payload_size);
    bool logBecauseEnabled = logThisUBX(incomingUBX->cls, incomingUBX->id) || processThisUBX(incomingUBX->cls, incomingUBX->id);
    if ((!logBecauseAuto) && (logBecauseEnabled))
      maximum_payload_size = SFE_UBX_MAX_LENGTH;
    if (maximum_payload_size == 0)
    {
      debugPrint("processUBX: autoLookup returned ZERO maxPayload!! Class: 0x", true); // Important
      debugPrint(incomingUBX->cls, HEX, true);
      debugPrint(" ID: 0x", true);
      debugPrintln(incomingUBX->id, HEX, true);
    }
    //}
    // else
    //  maximum_payload_size = 2;
  }
  else
    maximum_payload_size = 2;

  bool overrun = false;

  // Add all incoming bytes to the rolling checksum
  // Stop at len+4 as this is the checksum bytes to that should not be added to the rolling checksum
  if (incomingUBX->counter < (incomingUBX->len + 4))
    addToChecksum(incoming);

  if (incomingUBX->counter == 0)
  {
    incomingUBX->cls = incoming;
  }
  else if (incomingUBX->counter == 1)
  {
    incomingUBX->id = incoming;
  }
  else if (incomingUBX->counter == 2) // Len LSB
  {
    incomingUBX->len = incoming;
  }
  else if (incomingUBX->counter == 3) // Len MSB
  {
    incomingUBX->len |= incoming << 8;
  }
  else if (incomingUBX->counter == incomingUBX->len + 4) // ChecksumA
  {
    incomingUBX->checksumA = incoming;
  }
  else if (incomingUBX->counter == incomingUBX->len + 5) // ChecksumB
  {
    incomingUBX->checksumB = incoming;

    currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // We're done! Reset the sentence to being looking for a new start char

    // Validate this sentence
    if ((incomingUBX->checksumA == rollingChecksumA) && (incomingUBX->checksumB == rollingChecksumB))
    {
      incomingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_VALID; // Flag the packet as valid
      _signsOfLife = true;                                  // The checksum is valid, so set the _signsOfLife flag

      // Let's check if the class and ID match the requestedClass and requestedID
      // Remember - this could be a data packet or an ACK packet
      if ((incomingUBX->cls == requestedClass) && (incomingUBX->id == requestedID))
      {
        incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_VALID; // If we have a match, set the classAndIDmatch flag to valid
      }

      // If this is an ACK then let's check if the class and ID match the requestedClass and requestedID
      else if ((incomingUBX->cls == UBX_CLASS_ACK) && (incomingUBX->id == UBX_ACK_ACK) && (incomingUBX->payload[0] == requestedClass) && (incomingUBX->payload[1] == requestedID))
      {
        incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_VALID; // If we have a match, set the classAndIDmatch flag to valid
      }

      // If this is a NACK then let's check if the class and ID match the requestedClass and requestedID
      else if ((incomingUBX->cls == UBX_CLASS_ACK) && (incomingUBX->id == UBX_ACK_NACK) && (incomingUBX->payload[0] == requestedClass) && (incomingUBX->payload[1] == requestedID))
      {
        incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_NOTACKNOWLEDGED; // If we have a match, set the classAndIDmatch flag to NOTACKNOWLEDGED
        debugPrint("processUBX: NACK received: Requested Class: 0x");
        debugPrint(incomingUBX->payload[0], HEX);
        debugPrint(" Requested ID: 0x");
        debugPrintln(incomingUBX->payload[1], HEX);
      }

      // This is not an ACK and we do not have a complete class and ID match
      // So let's check for an "automatic" message arriving
      else if ((autoLookup(incomingUBX->cls, incomingUBX->id)) || (logThisUBX(incomingUBX->cls, incomingUBX->id)) || (processThisUBX(incomingUBX->cls, incomingUBX->id)))
      {
        // This isn't the message we are looking for...
        // Let's say so and leave incomingUBX->classAndIDmatch _unchanged_
        debugPrint("processUBX: incoming \"automatic\" message: Class: 0x");
        debugPrint(incomingUBX->cls, HEX);
        debugPrint(" ID: 0x");
        debugPrintln(incomingUBX->id, HEX);
      }

      debugPrint("Incoming: Size: ");
      debugPrint(incomingUBX->len);
      debugPrint(" Received: ");
      printPacket(incomingUBX);

      if (incomingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID)
      {
        debugPrintln("packetCfg now valid");
      }
      if (packetAck.valid == SFE_UBLOX_PACKET_VALIDITY_VALID)
      {
        debugPrintln("packetAck now valid");
      }
      if (incomingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID)
      {
        debugPrintln("packetCfg classAndIDmatch");
      }
      if (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID)
      {
        debugPrintln("packetAck classAndIDmatch");
      }

      // We've got a valid packet, now do something with it but only if ignoreThisPayload is false
      if (ignoreThisPayload == false)
      {
        processUBXpacket(incomingUBX);
      }
    }
    else // Checksum failure
    {
      incomingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_NOT_VALID;

      // Let's check if the class and ID match the requestedClass and requestedID.
      // This is potentially risky as we are saying that we saw the requested Class and ID
      // but that the packet checksum failed. Potentially it could be the class or ID bytes
      // that caused the checksum error!
      if ((incomingUBX->cls == requestedClass) && (incomingUBX->id == requestedID))
      {
        incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_VALID; // If we have a match, set the classAndIDmatch flag to not valid
      }
      // If this is an ACK then let's check if the class and ID match the requestedClass and requestedID
      else if ((incomingUBX->cls == UBX_CLASS_ACK) && (incomingUBX->payload[0] == requestedClass) && (incomingUBX->payload[1] == requestedID))
      {
        incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_VALID; // If we have a match, set the classAndIDmatch flag to not valid
      }

      // Drive an external pin to allow for easier logic analyzation
      if (debugPin >= 0)
      {
        sfe_pin_write((uint8_t)debugPin, false);
        sfe_delay(10);
        sfe_pin_write((uint8_t)debugPin, true);
      }

      debugPrint("Checksum failed:", true); // Important
      debugPrint(" checksumA: ", true);
      debugPrint(incomingUBX->checksumA, true);
      debugPrint(" checksumB: ", true);
      debugPrint(incomingUBX->checksumB, true);

      debugPrint(" rollingChecksumA: ", true);
      debugPrint(rollingChecksumA, true);
      debugPrint(" rollingChecksumB: ", true);
      debugPrint(rollingChecksumB, true);
      debugPrint("\r\n", true);
    }

    // Now that the packet is complete and has been processed, 'free' the memory for packetAuto
    // but leave payloadAuto allocated (See #75)
    if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETAUTO)
      packetAuto.payload = nullptr;
  }
  else // Load this byte into the payload array
  {
    // If an automatic packet comes in asynchronously, we need to fudge the startingSpot
    uint16_t startingSpot = incomingUBX->startingSpot;
    if (autoLookup(incomingUBX->cls, incomingUBX->id))
      startingSpot = 0;
    // Check if this is payload data which should be ignored
    if (ignoreThisPayload == false)
    {
      // Begin recording if counter goes past startingSpot
      if ((incomingUBX->counter - 4) >= startingSpot)
      {
        // Check to see if we have room for this byte
        if (((incomingUBX->counter - 4) - startingSpot) < maximum_payload_size) // If counter = 208, starting spot = 200, we're good to record.
        {
          incomingUBX->payload[(incomingUBX->counter - 4) - startingSpot] = incoming; // Store this byte into payload array
        }
        else
        {
          overrun = true;
        }
      }
    }
  }

  // incomingUBX->counter should never reach maximum_payload_size + class + id + len[2] + checksum[2]
  if (overrun || ((incomingUBX->counter == maximum_payload_size + 6) && (ignoreThisPayload == false)))
  {
    // Something has gone very wrong
    currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // Reset the sentence to being looking for a new start char
    if (overrun)
      debugPrint("processUBX: buffer overrun detected!", true); // Important
    else
      debugPrint("processUBX: counter hit maximum_payload_size + 6!", true);
    debugPrint(" activePacketBuffer: ", true);
    debugPrint(activePacketBuffer, true);
    switch (activePacketBuffer)
    {
      case SFE_UBLOX_PACKET_PACKETCFG:
        debugPrint(" (SFE_UBLOX_PACKET_PACKETCFG)");
        break;
      case SFE_UBLOX_PACKET_PACKETACK:
        debugPrint(" (SFE_UBLOX_PACKET_PACKETACK)");
        break;
      case SFE_UBLOX_PACKET_PACKETBUF:
        debugPrint(" (SFE_UBLOX_PACKET_PACKETBUF)");
        break;
      case SFE_UBLOX_PACKET_PACKETAUTO:
        debugPrint(" (SFE_UBLOX_PACKET_PACKETAUTO)");
        break;
    }
    debugPrint(" maximum_payload_size: ", true);
    debugPrint(maximum_payload_size, true);
    debugPrint(" Class: 0x", true);
    debugPrint(incomingUBX->cls, HEX, true);
    debugPrint(" ID: 0x", true);
    debugPrint(incomingUBX->id, HEX, true);
    debugPrint(" Len: ", true);
    debugPrintln(incomingUBX->len, true);
  }

  // Increment the counter
  incomingUBX->counter++;
}

// Once a packet has been received and validated, identify this packet's class/id and update internal flags
void DevUBLOXGNSS::processUBXpacket(ubxPacket *msg)
{
  bool addedToFileBuffer = false;

  ubxMessage *ubxMessagePtr = ubxMessages.find(msg->cls, msg->id);
  if (ubxMessagePtr)
  {
    // storePayload:
    //  finds the message
    //  calls its initStorage method
    //  copies the payload into storage - correctly truncating the length if needed
    //  marks the data as fresh (_moduleQueried = true)
    //  if the _callbackPtr is not nullptr:
    //   it also copies the payload into a free slot of _callbackStorage's ring buffer, advancing
    //   _callbackHead and _callbackCount - see AGENTS.md "Adding support for RXM-SFRBX"
    //  also synthesizes the complete raw UBX frame (using msg->checksumA/checksumB) into
    //   _callbackRawFrame, for messages that want to relay it verbatim - see AGENTS.md
    //   "Adding support for ESF-MEAS"
    if (ubxMessages.storePayload(msg->cls, msg->id, msg->payload, msg->len, msg->checksumA, msg->checksumB) != SFE_UBLOX_STATUS_SUCCESS)
    {
      debugPrint("processUBXpacket: storePayload failed for msg Class ");
      debugPrint(msg->cls);
      debugPrint(" ID ");
      debugPrint(msg->id);
      debugPrint(" Len ");
      debugPrintln(msg->len);
    }

    // The only thing storePayload doesn't do is copy the message into the file buffer.
    // Check if we need to copy the data into the file buffer
    bool adding = false;
    ubxMessages.getAddToFileBuffer(msg->cls, msg->id, &adding);
    if (adding)
      addedToFileBuffer = storePacket(msg);
  }
  else
  {
    switch (msg->cls)
    {
    case UBX_CLASS_NAV:
      // UBX_NAV_SAT and UBX_NAV_SIG are both handled above via the registry (ubxNAVSAT/ubxNAVSIG
      // are now self-registered) - see AGENTS.md "Adding the variable-length UBX messages".
      break;
    case UBX_CLASS_RXM:
      // UBX_RXM_PMP and UBX_RXM_QZSSL6 are now registered v4 messages (ubxRXMPMP/ubxRXMQZSSL6) -
      // see AGENTS.md "Adding support for RXM-PMP" and "Adding support for RXM-QZSSL6".
      // processUBXpacket() no longer parses either here; the registry-first branch at the top
      // of this function (ubxMessages.storePayload()) does that generically for both, including
      // writing into _callbackStorage/_callbackActualLength and _callbackRawFrame - and, for
      // QZSSL6 specifically, into the correct one of its 2 ring-buffer slots
      // (numCallbackCopies = UBX_RXM_QZSSL6_NUM_CHANNELS), same ring-buffer write logic already
      // used for RXM-SFRBX/ESF-MEAS.
      // UBX_RXM_SFRBX is now a registered v4 message (ubxRXMSFRBX) - see AGENTS.md "Adding
      // support for RXM-SFRBX". processUBXpacket() no longer parses it here; the registry-first
      // branch at the top of this function (ubxMessages.storePayload()) does that generically,
      // including writing into the ring-buffered _callbackStorage.
      // UBX_RXM_RAWX and UBX_RXM_MEASX are both handled above via the registry (ubxRXMRAWX/
      // ubxRXMMEASX are now self-registered) - see AGENTS.md "Adding the variable-length UBX
      // messages".
      break;
    case UBX_CLASS_MON:
      // UBX_MON_COMMS is now a registered v4 message (ubxMONCOMMS) - see AGENTS.md "Adding
      // the variable-length UBX messages". processUBXpacket() no longer parses it here; the
      // registry-first branch at the top of this function (ubxMessages.storePayload()) does
      // that generically.
      break;
    case UBX_CLASS_ESF:
      // UBX_ESF_MEAS, UBX_ESF_RAW and UBX_ESF_STATUS are all now registered v4 messages
      // (ubxESFMEAS/ubxESFRAW/ubxESFSTATUS) - see AGENTS.md "Adding support for ESF-MEAS" and
      // "Adding support for ESF-RAW and ESF-STATUS". processUBXpacket() no longer parses any of
      // them here; the registry-first branch at the top of this function
      // (ubxMessages.storePayload()) does that generically, including writing into the
      // ring-buffered _callbackStorage/_callbackActualLength/_callbackRawFrame. (The ESF_RAW
      // branch that used to be here was already unreachable dead code before this migration -
      // packetUBXESFRAW could never actually be non-nullptr, since initPacketUBXESFRAW() was
      // declared but never defined.)
      break;
    case UBX_CLASS_MGA:
      if (msg->id == UBX_MGA_ACK_DATA0 && msg->len == UBX_MGA_ACK_DATA0_LEN)
      {
        // Parse various byte fields into storage - but only if we have memory allocated for it
        if (packetUBXMGAACK != nullptr)
        {
          // Calculate how many ACKs are already stored in the ring buffer
          uint8_t ackBufferContains;
          if (packetUBXMGAACK->head >= packetUBXMGAACK->tail) // Check if wrap-around has occurred
          {
            // Wrap-around has not occurred so do a simple subtraction
            ackBufferContains = packetUBXMGAACK->head - packetUBXMGAACK->tail;
          }
          else
          {
            // Wrap-around has occurred so do a simple subtraction but add in the buffer length (UBX_MGA_ACK_RINGBUFFER_LEN)
            ackBufferContains = ((uint8_t)(((uint16_t)packetUBXMGAACK->head + (uint16_t)UBX_MGA_ACK_DATA0_RINGBUFFER_LEN) - (uint16_t)packetUBXMGAACK->tail));
          }
          // Have we got space to store this ACK?
          if (ackBufferContains < (UBX_MGA_ACK_DATA0_RINGBUFFER_LEN - 1))
          {
            // Yes, we have, so store it
            packetUBXMGAACK->data[packetUBXMGAACK->head].type = extractByte(msg, 0);
            packetUBXMGAACK->data[packetUBXMGAACK->head].version = extractByte(msg, 1);
            packetUBXMGAACK->data[packetUBXMGAACK->head].infoCode = extractByte(msg, 2);
            packetUBXMGAACK->data[packetUBXMGAACK->head].msgId = extractByte(msg, 3);
            packetUBXMGAACK->data[packetUBXMGAACK->head].msgPayloadStart[0] = extractByte(msg, 4);
            packetUBXMGAACK->data[packetUBXMGAACK->head].msgPayloadStart[1] = extractByte(msg, 5);
            packetUBXMGAACK->data[packetUBXMGAACK->head].msgPayloadStart[2] = extractByte(msg, 6);
            packetUBXMGAACK->data[packetUBXMGAACK->head].msgPayloadStart[3] = extractByte(msg, 7);
            // Increment the head
            packetUBXMGAACK->head++;
            if (packetUBXMGAACK->head == UBX_MGA_ACK_DATA0_RINGBUFFER_LEN)
              packetUBXMGAACK->head = 0;
          }
          else
          {
              debugPrintln("processUBXpacket: packetUBXMGAACK is full. ACK will be lost!", true); // Important
          }
        }
      }
      else if (msg->id == UBX_MGA_DBD && msg->len <= UBX_MGA_DBD_LEN) // Message length may be less than UBX_MGA_DBD_LEN. UBX_MGA_DBD_LEN is the maximum it will be.
      {
        // Parse various byte fields into storage - but only if we have memory allocated for it
        if (packetUBXMGADBD != nullptr)
        {
          // Calculate how many DBDs are already stored in the ring buffer
          uint8_t dbdBufferContains;
          if (packetUBXMGADBD->head >= packetUBXMGADBD->tail) // Check if wrap-around has occurred
          {
            // Wrap-around has not occurred so do a simple subtraction
            dbdBufferContains = packetUBXMGADBD->head - packetUBXMGADBD->tail;
          }
          else
          {
            // Wrap-around has occurred so do a simple subtraction but add in the buffer length (UBX_MGA_DBD_RINGBUFFER_LEN)
            dbdBufferContains = ((uint8_t)(((uint16_t)packetUBXMGADBD->head + (uint16_t)UBX_MGA_DBD_RINGBUFFER_LEN) - (uint16_t)packetUBXMGADBD->tail));
          }
          // Have we got space to store this DBD?
          if (dbdBufferContains < (UBX_MGA_DBD_RINGBUFFER_LEN - 1))
          {
            // Yes, we have, so store it
            // We need to save the entire message - header, payload and checksum
            packetUBXMGADBD->data[packetUBXMGADBD->head].dbdEntryHeader1 = UBX_SYNCH_1;
            packetUBXMGADBD->data[packetUBXMGADBD->head].dbdEntryHeader2 = UBX_SYNCH_2;
            packetUBXMGADBD->data[packetUBXMGADBD->head].dbdEntryClass = UBX_CLASS_MGA;
            packetUBXMGADBD->data[packetUBXMGADBD->head].dbdEntryID = UBX_MGA_DBD;
            packetUBXMGADBD->data[packetUBXMGADBD->head].dbdEntryLenLSB = (uint8_t)(msg->len & 0xFF); // We need to store the length of the DBD entry. The entry itself does not contain a length...
            packetUBXMGADBD->data[packetUBXMGADBD->head].dbdEntryLenMSB = (uint8_t)((msg->len >> 8) & 0xFF);
            for (uint16_t i = 0; i < msg->len; i++)
            {
              packetUBXMGADBD->data[packetUBXMGADBD->head].dbdEntry[i] = extractByte(msg, i);
            }
            packetUBXMGADBD->data[packetUBXMGADBD->head].dbdEntryChecksumA = msg->checksumA;
            packetUBXMGADBD->data[packetUBXMGADBD->head].dbdEntryChecksumB = msg->checksumB;
            // Increment the head
            packetUBXMGADBD->head++;
            if (packetUBXMGADBD->head == UBX_MGA_DBD_RINGBUFFER_LEN)
              packetUBXMGADBD->head = 0;
          }
          else
          {
            debugPrintln("processUBXpacket: packetUBXMGADBD is full. DBD data will be lost!", true); // Important
          }
        }
      }
      break;
    case UBX_CLASS_SEC:
      // UBX_SEC_SIG (Version 3 - see ubxSECSIG.h) is now a registered v4 message (ubxSECSIG) - see
      // AGENTS.md "Adding the variable-length UBX messages". processUBXpacket() no longer parses
      // it here; the registry-first branch at the top of this function (ubxMessages.storePayload())
      // does that generically. Version 1 was never modelled by ubxSECSIG and is not handled at all.
      break;
    }
  }

  // Check if this UBX message should be added to the file buffer - if it has not been added already
  if ((!addedToFileBuffer) && (logThisUBX(msg->cls, msg->id)))
    storePacket(msg);

  // Check if UBX message should be processed
  if (processThisUBX(msg->cls, msg->id))
    processLoggedUBX(msg);
}

// UBX Logging - without needing to have or use "Auto" methods
void DevUBLOXGNSS::enableUBXlogging(uint8_t UBX_CLASS, uint8_t UBX_ID, bool logMe, bool processMe)
{
  // If the list is empty
  if (sfe_ublox_ubx_logging_list_head == nullptr)
  {
    // Start the list with this CLASS + ID
    sfe_ublox_ubx_logging_list_head = new sfe_ublox_ubx_logging_list_t;
    sfe_ublox_ubx_logging_list_head->UBX_CLASS = UBX_CLASS;
    sfe_ublox_ubx_logging_list_head->UBX_ID = UBX_ID;
    sfe_ublox_ubx_logging_list_head->logMe = logMe;
    sfe_ublox_ubx_logging_list_head->processMe = processMe;
    sfe_ublox_ubx_logging_list_head->next = nullptr;
    return;
  }

  // Check if this CLASS + ID is already registered in the linked list
  sfe_ublox_ubx_logging_list_t *sfe_ublox_ubx_logging_list_ptr = sfe_ublox_ubx_logging_list_head;

  // Step through the list, check for CLASS + ID
  bool keepGoing = true;
  while (keepGoing)
  {
    if ((sfe_ublox_ubx_logging_list_ptr->UBX_CLASS == UBX_CLASS) // Check for a match
        && (sfe_ublox_ubx_logging_list_ptr->UBX_ID == UBX_ID))
    {
      sfe_ublox_ubx_logging_list_ptr->logMe = logMe; // Update logMe
      sfe_ublox_ubx_logging_list_ptr->logMe = processMe; // Update processMe
      return;
    }

    if (sfe_ublox_ubx_logging_list_ptr->next == nullptr)
      keepGoing = false;
    else
      sfe_ublox_ubx_logging_list_ptr = sfe_ublox_ubx_logging_list_ptr->next;
  }

  // CLASS + ID not found. Add them.
  sfe_ublox_ubx_logging_list_ptr->next = new sfe_ublox_ubx_logging_list_t;
  sfe_ublox_ubx_logging_list_ptr = sfe_ublox_ubx_logging_list_ptr->next;
  sfe_ublox_ubx_logging_list_ptr->UBX_CLASS = UBX_CLASS;
  sfe_ublox_ubx_logging_list_ptr->UBX_ID = UBX_ID;
  sfe_ublox_ubx_logging_list_ptr->logMe = logMe;
  sfe_ublox_ubx_logging_list_ptr->processMe = processMe;
  sfe_ublox_ubx_logging_list_ptr->next = nullptr;
}

// PRIVATE: Returns true if this UBX should be added to the logging buffer
bool DevUBLOXGNSS::logThisUBX(uint8_t UBX_CLASS, uint8_t UBX_ID)
{
  return logOrProcessThisUBX(UBX_CLASS, UBX_ID, true);
}
bool DevUBLOXGNSS::processThisUBX(uint8_t UBX_CLASS, uint8_t UBX_ID)
{
  return logOrProcessThisUBX(UBX_CLASS, UBX_ID, false);
}
bool DevUBLOXGNSS::logOrProcessThisUBX(uint8_t UBX_CLASS, uint8_t UBX_ID, bool log)
{
  // If the list is empty
  if (sfe_ublox_ubx_logging_list_head == nullptr)
    return false;

  // Step through the list, check for CLASS + ID
  sfe_ublox_ubx_logging_list_t *sfe_ublox_ubx_logging_list_ptr = sfe_ublox_ubx_logging_list_head;
  bool keepGoing = true;
  while (keepGoing)
  {
    if ((sfe_ublox_ubx_logging_list_ptr->UBX_CLASS == UBX_CLASS) // Check for a match
        && (sfe_ublox_ubx_logging_list_ptr->UBX_ID == UBX_ID))
    {
      if (log)
        return (sfe_ublox_ubx_logging_list_ptr->logMe);
      else
        return (sfe_ublox_ubx_logging_list_ptr->processMe);
    }

    if (sfe_ublox_ubx_logging_list_ptr->next == nullptr)
      keepGoing = false;
    else
      sfe_ublox_ubx_logging_list_ptr = sfe_ublox_ubx_logging_list_ptr->next;
  }

  return false;
}

// Given a message, calc and store the two byte "8-Bit Fletcher" checksum over the entirety of the message
// This is called before we send a command message
void DevUBLOXGNSS::calcChecksum(ubxPacket *msg)
{
  msg->checksumA = 0;
  msg->checksumB = 0;

  msg->checksumA += msg->cls;
  msg->checksumB += msg->checksumA;

  msg->checksumA += msg->id;
  msg->checksumB += msg->checksumA;

  msg->checksumA += (msg->len & 0xFF);
  msg->checksumB += msg->checksumA;

  msg->checksumA += (msg->len >> 8);
  msg->checksumB += msg->checksumA;

  for (uint16_t i = 0; i < msg->len; i++)
  {
    msg->checksumA += msg->payload[i];
    msg->checksumB += msg->checksumA;
  }
}

// Given a message and a byte, add to rolling "8-Bit Fletcher" checksum
// This is used when receiving messages from module
void DevUBLOXGNSS::addToChecksum(uint8_t incoming)
{
  rollingChecksumA += incoming;
  rollingChecksumB += rollingChecksumA;
}

// Given a packet and payload, send everything including CRC bytes via I2C port
sfe_ublox_status_e DevUBLOXGNSS::sendCommand(ubxPacket *outgoingUBX, uint16_t maxWait, bool expectACKonly)
{
  if (!lock())
    return SFE_UBLOX_STATUS_FAIL;

  sfe_ublox_status_e retVal = SFE_UBLOX_STATUS_SUCCESS;

  calcChecksum(outgoingUBX); // Sets checksum A and B bytes of the packet

  debugPrint("\nSending: ");
  printPacket(outgoingUBX, true); // Always print payload

  if (_commType == COMM_TYPE_I2C)
  {
    retVal = sendI2cCommand(outgoingUBX);
    if (retVal != SFE_UBLOX_STATUS_SUCCESS)
    {
      debugPrintln("Send I2C Command failed"); // Not important
      unlock();
      return retVal;
    }
  }
  else if (_commType == COMM_TYPE_SERIAL)
  {
    sendSerialCommand(outgoingUBX);
  }
  else if (_commType == COMM_TYPE_SPI)
  {
    sendSpiCommand(outgoingUBX);
  }

  unlock();

  if (maxWait > 0)
  {
    // Depending on what we just sent, either we need to look for an ACK or not
    if ((outgoingUBX->cls == UBX_CLASS_CFG) || (expectACKonly == true))
    {
      debugPrintln("sendCommand: Waiting for ACK response"); // Not important
      retVal = waitForACKResponse(outgoingUBX, outgoingUBX->cls, outgoingUBX->id, maxWait); // Wait for Ack response
    }
    else
    {
      debugPrintln("sendCommand: Waiting for No ACK response"); // Not important
      retVal = waitForNoACKResponse(outgoingUBX, outgoingUBX->cls, outgoingUBX->id, maxWait); // Wait for Ack response
    }
  }
  else
  {
    processSpiBuffer(&packetCfg, 0, 0); // Process any SPI data received during the sendSpiCommand - but only if not checking for a response
  }

  return retVal;
}

// Poll a single NMEA message on the current interface using the GN Talker ID
sfe_ublox_status_e DevUBLOXGNSS::pollNMEA(const char *msgId, uint16_t maxWait)
{
  if (!lock())
    return SFE_UBLOX_STATUS_FAIL;

  unsigned long startTime = sfe_millis();

  sfe_ublox_status_e retVal = SFE_UBLOX_STATUS_SUCCESS;

  // Prepare the EIGNQ request, add the checksum and \r\n
  char pollRequest[strlen("$EIGNQ,RMC*3A\r\n") + 1];
  snprintf(pollRequest, sizeof(pollRequest), "$EIGNQ,%s*", msgId);
  uint8_t checksum = 0;
  for (int x = 1; (pollRequest[x] != '*') && (x < sizeof(pollRequest)); x++)
    checksum ^= pollRequest[x];
  char checksumStr[strlen("3A\r\n") + 1];
  snprintf(checksumStr, sizeof(checksumStr), "%02X\r\n", checksum);
  strncat(pollRequest, checksumStr, sizeof(pollRequest) - strlen(pollRequest));

  debugPrint("\nSending: ");
  debugPrint(pollRequest);

  if (_commType == COMM_TYPE_I2C)
  {
    if (writeBytes((uint8_t *)pollRequest, strlen(pollRequest)) != strlen(pollRequest))
    {
      unlock();
      return SFE_UBLOX_STATUS_I2C_COMM_FAILURE;
    }
  }
  else if (_commType == COMM_TYPE_SERIAL)
  {
    writeBytes((uint8_t *)pollRequest, strlen(pollRequest));
  }
  else if (_commType == COMM_TYPE_SPI)
  {
    startWriteReadByte();
    for (int x = 0; x < strlen(pollRequest); x++)
      spiTransfer(pollRequest[x]);
    endWriteReadByte();
  }

  unlock();

  if (maxWait > 0)
  {
    // Poll request sent. Wait for the NMEA to arrive
    bool queried = false;
    while ((!queried) &&((sfe_millis() - startTime) < maxWait))
    {
      checkUbloxInternal(&packetCfg, 0, 0); // Hijack packetCfg
      retVal = nmeaMessages.moduleQueried(msgId, &queried);
      if (retVal != SFE_UBLOX_STATUS_SUCCESS)
        return retVal;
    }

    if (!queried) // Did we time out?
      retVal = SFE_UBLOX_STATUS_TIMEOUT;
    else
      retVal = SFE_UBLOX_STATUS_DATA_RECEIVED;
  }
  else
  {
    processSpiBuffer(&packetCfg, 0, 0); // Process any SPI data received during the sendSpiCommand - but only if not checking for a response
  }

  return retVal;
}

// Returns false if sensor fails to respond to I2C traffic
sfe_ublox_status_e DevUBLOXGNSS::sendI2cCommand(ubxPacket *outgoingUBX)
{
  // From the integration guide:
  // "The receiver does not provide any write access except for writing UBX and NMEA messages to the
  //  receiver, such as configuration or aiding data. Therefore, the register set mentioned in section Read
  //  Access is not writeable. Following the start condition from the master, the 7-bit device address and
  //  the RW bit (which is a logic low for write access) are clocked onto the bus by the master transmitter.
  //  The receiver answers with an acknowledge (logic low) to indicate that it is responsible for the given
  //  address. Now, the master can write 2 to N bytes to the receiver, generating a stop condition after the
  //  last byte being written. The number of data bytes must be at least 2 to properly distinguish from
  //  the write access to set the address counter in random read accesses."
  // I take two things from this:
  // 1) We do not need to write 0xFF to point at register 0xFF. We're already pointing at it.
  // 2) We must always write at least 2 bytes, otherwise it looks like we are starting to do a read.
  // Point 2 is important. It means:
  // * In this function:
  //     if we do multiple writes (because we're trying to write more than i2cTransactionSize),
  //     we may need to write one byte less in the penultimate write to ensure we always have two bytes left for the final write.
  // * In pushRawData:
  //     if there is one byte to write, or one byte left to write, we need to do the same thing and may need to store a single
  //     byte until pushRawData is called again.

  // The total number of bytes to be written is: payload len + 8
  // UBX_SYNCH_1
  // UBX_SYNCH_2
  // cls
  // id
  // len (MSB)
  // len (LSB)
  // < payload >
  // checksumA
  // checksumB

  // i2cTransactionSize will be at least 8. We don't need to check for smaller values than that.

  uint16_t bytesLeftToSend = outgoingUBX->len; // How many bytes remain to be sent
  uint16_t startSpot = 0;                      // Payload pointer

  // Check if we can send all the data in one transfer?
  if (bytesLeftToSend + 8 <= i2cTransactionSize)
  {
    uint8_t buf[i2cTransactionSize];
    buf[0] = UBX_SYNCH_1; // μ - oh ublox, you're funny. I will call you micro-blox from now on.
    buf[1] = UBX_SYNCH_2; // b
    buf[2] = outgoingUBX->cls;
    buf[3] = outgoingUBX->id;
    buf[4] = outgoingUBX->len & 0xFF; // LSB
    buf[5] = outgoingUBX->len >> 8;   // MSB
    uint16_t i = 0;
    for (; i < outgoingUBX->len; i++)
      buf[i + 6] = outgoingUBX->payload[startSpot + i];
    buf[i + 6] = outgoingUBX->checksumA;
    buf[i + 7] = outgoingUBX->checksumB;

    if (writeBytes(buf, bytesLeftToSend + 8) != bytesLeftToSend + 8)
      return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); // Sensor did not ACK
  }

  else
  {
    uint8_t buf[6];
    buf[0] = UBX_SYNCH_1; // μ - oh ublox, you're funny. I will call you micro-blox from now on.
    buf[1] = UBX_SYNCH_2; // b
    buf[2] = outgoingUBX->cls;
    buf[3] = outgoingUBX->id;
    buf[4] = outgoingUBX->len & 0xFF; // LSB
    buf[5] = outgoingUBX->len >> 8;   // MSB

    if (writeBytes(buf, 6) != 6)
      return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); // Sensor did not ACK

    // If bytesLeftToSend is zero, that's OK.
    // If bytesLeftToSend is >= 2, that's OK.
    // But if bytesLeftToSend is 1, we need to carry that byte over and send it with the checksum bytes
    while (bytesLeftToSend > 1)
    {
      uint16_t len = bytesLeftToSend; // How many bytes should we actually write?
      if (len > i2cTransactionSize)   // Limit len to i2cTransactionSize
        len = i2cTransactionSize;

      bytesLeftToSend -= len; // Calculate how many bytes will be left after we do this write

      // Write a portion of the payload to the bus.
      // Keep going until we've sent as many bytes as we can in this transmission (x == len)
      // or until we reach the end of the payload ((startSpot + x) == (outgoingUBX->len))
      uint16_t x = len;
      if ((startSpot + x) >= (outgoingUBX->len))
        x = outgoingUBX->len - startSpot;

      if (writeBytes(&outgoingUBX->payload[startSpot], x) != x)
        return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); // Sensor did not ACK

      startSpot += x;
    }

    // Finally, write any left-over bytes plus the checksum
    if (bytesLeftToSend == 1)
    {
      buf[0] = outgoingUBX->payload[startSpot];
      buf[1] = outgoingUBX->checksumA;
      buf[2] = outgoingUBX->checksumB;

      if (writeBytes(buf, 3) != 3)
        return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); // Sensor did not ACK
    }
    else
    {
      buf[0] = outgoingUBX->checksumA;
      buf[1] = outgoingUBX->checksumB;

      if (writeBytes(buf, 2) != 2)
        return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); // Sensor did not ACK
    }
  }

  return (SFE_UBLOX_STATUS_SUCCESS);
}

// Given a packet and payload, send everything including CRC bytesA via Serial port
void DevUBLOXGNSS::sendSerialCommand(ubxPacket *outgoingUBX)
{
  uint8_t buf[6];
  buf[0] = UBX_SYNCH_1; // μ - oh ublox, you're funny. I will call you micro-blox from now on.
  buf[1] = UBX_SYNCH_2; // b
  buf[2] = outgoingUBX->cls;
  buf[3] = outgoingUBX->id;
  buf[4] = outgoingUBX->len & 0xFF; // LSB
  buf[5] = outgoingUBX->len >> 8;   // MSB
  writeBytes(buf, 6);

  // Write payload
  writeBytes(outgoingUBX->payload, outgoingUBX->len);

  buf[0] = outgoingUBX->checksumA;
  buf[1] = outgoingUBX->checksumB;
  writeBytes(buf, 2);
}

// Transfer a byte to SPI. Also capture any bytes received from the UBLOX device during sending and capture them in a small buffer so that
// they can be processed later with process
void DevUBLOXGNSS::spiTransfer(const uint8_t byteToTransfer)
{
  static bool printOnce = false;
  if (spiBufferIndex == 0)
    printOnce = false;

  // If we start to receive something, we need to keep receiving and buffering
  // otherwise 0xFF bytes will be ignored if currentSentence == SFE_UBLOX_SENTENCE_TYPE_NONE
  static bool receivedSomething = false;
  if (spiBufferIndex == 0)
    receivedSomething = false;

  uint8_t returnedByte = 0xFF;

  writeReadByte(byteToTransfer, &returnedByte);

  if ((returnedByte != 0xFF) || (currentSentence != SFE_UBLOX_SENTENCE_TYPE_NONE) || receivedSomething)
  {
    if (spiBufferIndex < spiBufferSize)
    {
      spiBuffer[spiBufferIndex] = returnedByte;
      spiBufferIndex++;
      receivedSomething = true;
    }
    else
    {
      if (!printOnce)
      {
        debugPrintln("spiTransfer: spiBuffer is full!", true); // Important
        printOnce = true;
      }
    }
  }
}

// Send a command via SPI
sfe_ublox_status_e DevUBLOXGNSS::sendSpiCommand(ubxPacket *outgoingUBX)
{
  if (spiBuffer == nullptr)
  {
    debugPrint("sendSpiCommand: no memory allocation for SPI Buffer!", true); // Important
    return (SFE_UBLOX_STATUS_MEM_ERR);
  }

  // Start at the beginning of the SPI buffer
  // spiBufferIndex = 0;

  uint16_t bytesLeftToSend = outgoingUBX->len; // How many bytes remain to be sent

  startWriteReadByte();

  spiTransfer(UBX_SYNCH_1); // μ - oh ublox, you're funny. I will call you micro-blox from now on.
  spiTransfer(UBX_SYNCH_2); // b
  spiTransfer(outgoingUBX->cls);
  spiTransfer(outgoingUBX->id);
  spiTransfer(outgoingUBX->len & 0xFF); // LSB
  spiTransfer(outgoingUBX->len >> 8);   // MSB

  // Check if we can send all the data in one transfer?
  if ((bytesLeftToSend + 8) <= spiTransactionSize)
  {
    for (uint16_t i = 0; i < bytesLeftToSend; i++)
      spiTransfer(outgoingUBX->payload[i]);
  }

  else
  {
    endWriteReadByte();

    uint16_t bytesSent = 0;

    while (bytesLeftToSend > 0)
    {
      uint16_t len = bytesLeftToSend; // How many bytes should we actually write?
      if (len > spiTransactionSize)   // Limit len to spiTransactionSize
        len = spiTransactionSize;

      bytesLeftToSend -= len; // Calculate how many bytes will be left after we do this write

      // Write a portion of the payload to the bus.

      startWriteReadByte();

      for (uint16_t i = 0; i < len; i++)
        spiTransfer(outgoingUBX->payload[bytesSent + i]);

      bytesSent += len;

      endWriteReadByte();
    }

    startWriteReadByte();
  }

  // Finally, write the checksum
  spiTransfer(outgoingUBX->checksumA);
  spiTransfer(outgoingUBX->checksumB);

  endWriteReadByte();

  return (SFE_UBLOX_STATUS_SUCCESS);
}

// Pretty prints the current ubxPacket
void DevUBLOXGNSS::printPacket(ubxPacket *packet, bool alwaysPrintPayload)
{
  // Only print the payload is ignoreThisPayload is false otherwise
  // we could be printing gibberish from beyond the end of packetBuf
  // (These two lines get rid of a pesky compiler warning)
  bool printPayload = (ignoreThisPayload == false);
  printPayload |= (alwaysPrintPayload == true);

  debugPrint("CLS:");
  if (packet->cls == UBX_CLASS_NAV) // 1
    debugPrint("NAV");
  else if (packet->cls == UBX_CLASS_ACK) // 5
    debugPrint("ACK");
  else if (packet->cls == UBX_CLASS_CFG) // 6
    debugPrint("CFG");
  else if (packet->cls == UBX_CLASS_MON) // 0x0A
    debugPrint("MON");
  else
  {
    debugPrint("0x");
    debugPrint(packet->cls, HEX);
  }

  debugPrint(" ID:");
  if (packet->cls == UBX_CLASS_NAV && packet->id == UBX_NAV_PVT)
    debugPrint("PVT");
  else if (packet->cls == UBX_CLASS_CFG && packet->id == UBX_CFG_CFG)
    debugPrint("SAVE");
  else
  {
    debugPrint("0x");
    debugPrint(packet->id, HEX);
  }

  debugPrint(" Len: 0x");
  debugPrint(packet->len, HEX);

  if (printPayload)
  {
    debugPrint(" Payload:");

    for (uint16_t x = 0; x < packet->len - packet->startingSpot; x++)
    {
      debugPrint(" ");
      debugPrint(packet->payload[x], HEX);
    }
  }
  else
  {
    debugPrint(" Payload: IGNORED");
  }
  debugPrintln();
}

// When messages from the class CFG are sent to the receiver, the receiver will send an "acknowledge"(UBX - ACK - ACK) or a
//"not acknowledge"(UBX-ACK-NAK) message back to the sender, depending on whether or not the message was processed correctly.
// Some messages from other classes also use the same acknowledgement mechanism.

// When we poll or get a setting, we will receive _both_ a config packet and an ACK
// If the poll or get request is not valid, we will receive _only_ a NACK

// If we are trying to get or poll a setting, then packetCfg.len will be 0 or 1 when the packetCfg is _sent_.
// If we poll the setting for a particular port using UBX-CFG-PRT then .len will be 1 initially
// For all other gets or polls, .len will be 0 initially
//(It would be possible for .len to be 2 _if_ we were using UBX-CFG-MSG to poll the settings for a particular message - but we don't use that (currently))

// If the get or poll _fails_, i.e. is NACK'd, then packetCfg.len could still be 0 or 1 after the NACK is received
// But if the get or poll is ACK'd, then packetCfg.len will have been updated by the incoming data and will always be at least 2

// If we are going to set the value for a setting, then packetCfg.len will be at least 3 when the packetCfg is _sent_.
//(UBX-CFG-MSG appears to have the shortest set length of 3 bytes)

// We need to think carefully about how interleaved PVT packets affect things.
// It is entirely possible that our packetCfg and packetAck were received successfully
// but while we are still in the "if (checkUblox() == true)" loop a PVT packet is processed
// or _starts_ to arrive (remember that Serial data can arrive very slowly).

// Returns SFE_UBLOX_STATUS_DATA_RECEIVED if we got an ACK and a valid packetCfg (module is responding with register content)
// Returns SFE_UBLOX_STATUS_DATA_SENT if we got an ACK and no packetCfg (no valid packetCfg needed, module absorbs new register data)
// Returns SFE_UBLOX_STATUS_FAIL if something very bad happens (e.g. a double checksum failure)
// Returns SFE_UBLOX_STATUS_COMMAND_NACK if the packet was not-acknowledged (NACK)
// Returns SFE_UBLOX_STATUS_CRC_FAIL if we had a checksum failure
// Returns SFE_UBLOX_STATUS_TIMEOUT if we timed out
// Returns SFE_UBLOX_STATUS_DATA_OVERWRITTEN if we got an ACK and a valid packetCfg but that the packetCfg has been
//  or is currently being overwritten (remember that Serial data can arrive very slowly)
sfe_ublox_status_e DevUBLOXGNSS::waitForACKResponse(ubxPacket *outgoingUBX, uint8_t requestedClass, uint8_t requestedID, uint16_t maxTime)
{
  outgoingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // This will go VALID (or NOT_VALID) when we receive a response to the packet we sent
  packetAck.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetBuf.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetAuto.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  outgoingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // This will go VALID (or NOT_VALID) when we receive a packet that matches the requested class and ID
  packetAck.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetBuf.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetAuto.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;

  unsigned long startTime = sfe_millis();
  while ((sfe_millis() - startTime) < (unsigned long)maxTime)
  {
    if (checkUbloxInternal(outgoingUBX, requestedClass, requestedID) == true) // See if new data is available. Process bytes as they come in.
    {
      // If both the outgoingUBX->classAndIDmatch and packetAck.classAndIDmatch are VALID
      // and outgoingUBX->valid is _still_ VALID and the class and ID _still_ match
      // then we can be confident that the data in outgoingUBX is valid
      if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->cls == requestedClass) && (outgoingUBX->id == requestedID))
      {
        debugPrint("waitForACKResponse: valid data and valid ACK received after ");
        debugPrint(sfe_millis() - startTime);
        debugPrintln(" msec");
        return (SFE_UBLOX_STATUS_DATA_RECEIVED); // We received valid data and a correct ACK!
      }

      // We can be confident that the data packet (if we are going to get one) will always arrive
      // before the matching ACK. So if we sent a config packet which only produces an ACK
      // then outgoingUBX->classAndIDmatch will be NOT_DEFINED and the packetAck.classAndIDmatch will VALID.
      // We should not check outgoingUBX->valid, outgoingUBX->cls or outgoingUBX->id
      // as these may have been changed by an automatic packet.
      else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID))
      {
        debugPrint("waitForACKResponse: no data and valid ACK after ");
        debugPrint(sfe_millis() - startTime);
        debugPrintln(" msec");
        return (SFE_UBLOX_STATUS_DATA_SENT); // We got an ACK but no data...
      }

      // If both the outgoingUBX->classAndIDmatch and packetAck.classAndIDmatch are VALID
      // but the outgoingUBX->cls or ID no longer match then we can be confident that we had
      // valid data but it has been or is currently being overwritten by an automatic packet (e.g. PVT).
      // If (e.g.) a PVT packet is _being_ received: outgoingUBX->valid will be NOT_DEFINED
      // If (e.g.) a PVT packet _has been_ received: outgoingUBX->valid will be VALID (or just possibly NOT_VALID)
      // So we cannot use outgoingUBX->valid as part of this check.
      // Note: the addition of packetBuf should make this check redundant!
      else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && ((outgoingUBX->cls != requestedClass) || (outgoingUBX->id != requestedID)))
      {
        debugPrint("waitForACKResponse: data being OVERWRITTEN after ");
        debugPrint(sfe_millis() - startTime);
        debugPrintln(" msec");
        return (SFE_UBLOX_STATUS_DATA_OVERWRITTEN); // Data was valid but has been or is being overwritten
      }

      // If packetAck.classAndIDmatch is VALID but both outgoingUBX->valid and outgoingUBX->classAndIDmatch
      // are NOT_VALID then we can be confident we have had a checksum failure on the data packet
      else if ((packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID))
      {
        debugPrint("waitForACKResponse: CRC failed after ");
        debugPrint(sfe_millis() - startTime);
        debugPrintln(" msec");
        return (SFE_UBLOX_STATUS_CRC_FAIL); // Checksum fail
      }

      // If our packet was not-acknowledged (NACK) we do not receive a data packet - we only get the NACK.
      // So you would expect outgoingUBX->valid and outgoingUBX->classAndIDmatch to still be NOT_DEFINED
      // But if a full PVT packet arrives afterwards outgoingUBX->valid could be VALID (or just possibly NOT_VALID)
      // but outgoingUBX->cls and outgoingUBX->id would not match...
      // So I think this is telling us we need a special state for packetAck.classAndIDmatch to tell us
      // the packet was definitely NACK'd otherwise we are possibly just guessing...
      // Note: the addition of packetBuf changes the logic of this, but we'll leave the code as is for now.
      else if (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_NOTACKNOWLEDGED)
      {
        debugPrint("waitForACKResponse: data was NOTACKNOWLEDGED (NACK) after ");
        debugPrint(sfe_millis() - startTime);
        debugPrintln(" msec");
        return (SFE_UBLOX_STATUS_COMMAND_NACK); // We received a NACK!
      }

      // If the outgoingUBX->classAndIDmatch is VALID but the packetAck.classAndIDmatch is NOT_VALID
      // then the ack probably had a checksum error. We will take a gamble and return DATA_RECEIVED.
      // If we were playing safe, we should return FAIL instead
      else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->cls == requestedClass) && (outgoingUBX->id == requestedID))
      {
        debugPrint("waitForACKResponse: VALID data and INVALID ACK received after ");
        debugPrint(sfe_millis() - startTime);
        debugPrintln(" msec");
        return (SFE_UBLOX_STATUS_DATA_RECEIVED); // We received valid data and an invalid ACK!
      }

      // If the outgoingUBX->classAndIDmatch is NOT_VALID and the packetAck.classAndIDmatch is NOT_VALID
      // then we return a FAIL. This must be a double checksum failure?
      else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID))
      {
        debugPrint("waitForACKResponse: INVALID data and INVALID ACK received after ");
        debugPrint(sfe_millis() - startTime);
        debugPrintln(" msec");
        return (SFE_UBLOX_STATUS_FAIL); // We received invalid data and an invalid ACK!
      }

      // If the outgoingUBX->classAndIDmatch is VALID and the packetAck.classAndIDmatch is NOT_DEFINED
      // then the ACK has not yet been received and we should keep waiting for it
      else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED))
      {
        //   debugPrint("waitForACKResponse: valid data after ");
        //   debugPrint(millis() - startTime);
        //   debugPrintln(" msec. Waiting for ACK.");
      }

    } // checkUbloxInternal == true

    sfe_delay(1); // Allow an RTOS to get an elbow in (#11)
  }           // while ((millis() - startTime) < (unsigned long)maxTime)

  // We have timed out...
  // If the outgoingUBX->classAndIDmatch is VALID then we can take a gamble and return DATA_RECEIVED
  // even though we did not get an ACK
  if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->cls == requestedClass) && (outgoingUBX->id == requestedID))
  {
    debugPrint("waitForACKResponse: TIMEOUT with valid data after ");
    debugPrint(sfe_millis() - startTime);
    debugPrintln(" msec. ");
    return (SFE_UBLOX_STATUS_DATA_RECEIVED); // We received valid data... But no ACK!
  }

  debugPrint("waitForACKResponse: TIMEOUT after ");
  debugPrint(sfe_millis() - startTime);
  debugPrintln(" msec.");

  return (SFE_UBLOX_STATUS_TIMEOUT);
}

// For non-CFG queries no ACK is sent so we use this function
// Returns SFE_UBLOX_STATUS_DATA_RECEIVED if we got a config packet full of response data that has CLS/ID match to our query packet
// Returns SFE_UBLOX_STATUS_CRC_FAIL if we got a corrupt config packet that has CLS/ID match to our query packet
// Returns SFE_UBLOX_STATUS_TIMEOUT if we timed out
// Returns SFE_UBLOX_STATUS_DATA_OVERWRITTEN if we got an a valid packetCfg but that the packetCfg has been
//  or is currently being overwritten (remember that Serial data can arrive very slowly)
sfe_ublox_status_e DevUBLOXGNSS::waitForNoACKResponse(ubxPacket *outgoingUBX, uint8_t requestedClass, uint8_t requestedID, uint16_t maxTime)
{
  outgoingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // This will go VALID (or NOT_VALID) when we receive a response to the packet we sent
  packetAck.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetBuf.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetAuto.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  outgoingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // This will go VALID (or NOT_VALID) when we receive a packet that matches the requested class and ID
  packetAck.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetBuf.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
  packetAuto.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;

  unsigned long startTime = sfe_millis();
  while (sfe_millis() - startTime < maxTime)
  {
    if (checkUbloxInternal(outgoingUBX, requestedClass, requestedID) == true) // See if new data is available. Process bytes as they come in.
    {

      // If outgoingUBX->classAndIDmatch is VALID
      // and outgoingUBX->valid is _still_ VALID and the class and ID _still_ match
      // then we can be confident that the data in outgoingUBX is valid
      if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->cls == requestedClass) && (outgoingUBX->id == requestedID))
      {
        debugPrint("waitForNoACKResponse: valid data with CLS/ID match after ");
        debugPrint(sfe_millis() - startTime);
        debugPrintln(" msec");
        return (SFE_UBLOX_STATUS_DATA_RECEIVED); // We received valid data!
      }

      // If the outgoingUBX->classAndIDmatch is VALID
      // but the outgoingUBX->cls or ID no longer match then we can be confident that we had
      // valid data but it has been or is currently being overwritten by another packet (e.g. PVT).
      // If (e.g.) a PVT packet is _being_ received: outgoingUBX->valid will be NOT_DEFINED
      // If (e.g.) a PVT packet _has been_ received: outgoingUBX->valid will be VALID (or just possibly NOT_VALID)
      // So we cannot use outgoingUBX->valid as part of this check.
      // Note: the addition of packetBuf should make this check redundant!
      else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && ((outgoingUBX->cls != requestedClass) || (outgoingUBX->id != requestedID)))
      {
        debugPrint("waitForNoACKResponse: data being OVERWRITTEN after ");
        debugPrint(sfe_millis() - startTime);
        debugPrintln(" msec");
        return (SFE_UBLOX_STATUS_DATA_OVERWRITTEN); // Data was valid but has been or is being overwritten
      }

      // If outgoingUBX->classAndIDmatch is NOT_DEFINED
      // and outgoingUBX->valid is VALID then this must be (e.g.) a PVT packet
      else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID))
      {
        //   debugPrint("waitForNoACKResponse: valid but UNWANTED data after ");
        //   debugPrint(millis() - startTime);
        //   debugPrint(" msec. Class: 0x");
        //   debugPrint(outgoingUBX->cls, HEX);
        //   debugPrint(" ID: 0x");
        //   debugPrintln(outgoingUBX->id, HEX);
      }

      // If the outgoingUBX->classAndIDmatch is NOT_VALID then we return CRC failure
      else if (outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID)
      {
        debugPrint("waitForNoACKResponse: CLS/ID match but failed CRC after ");
        debugPrint(sfe_millis() - startTime);
        debugPrintln(" msec");
        return (SFE_UBLOX_STATUS_CRC_FAIL); // We received invalid data
      }
    }

    sfe_delay(1); // Allow an RTOS to get an elbow in (#11)
  }

  debugPrint("waitForNoACKResponse: TIMEOUT after ");
  debugPrint(sfe_millis() - startTime);
  debugPrintln(" msec. No packet received.");

  return (SFE_UBLOX_STATUS_TIMEOUT);
}

// v4 scaffolding: 
// Factory: hands back the opaque per-message object a callback's ubxCallbackDataCommon_t* points
// at, so getUbxMessageField() can navigate its field table and extract a named field's
// value - see AGENTS.md "getUbxMessagePtr Factory design pattern".
ubxMessage *DevUBLOXGNSS::getUbxMessagePtr(ubxCallbackDataCommon_t *theData)
{
    if (theData == nullptr)
        return nullptr;
    return theData->messagePtr;
}

// v4 scaffolding: 
// Factory: extracts a named field from the message a callback just fired for, reading from its
// _callbackStorage (the copy storePayload() froze when the callback was queued) rather than its
// live _storage (which may already have been overwritten by newer data by the time the callback
// actually runs). See AGENTS.md "getUbxMessageField will also need to use a Factory
// method / design pattern to handle the different return types. If this is not possible, identify
// the nearest alternative strategy which is possible" - see ubxAnyType::operator double() above for
// why this returns ubxAnyType rather than a genuinely per-field C++ type.
// _callbackStorage may hold several buffered slots (see AGENTS.md "Adding support for
// RXM-SFRBX"); checkCallbacks() sets _callbackReadIndex to the slot this firing is for immediately
// before calling the callback, so this reads that slot rather than always offset 0. For
// _numCallbackCopies <= 1, _callbackReadIndex is always 0, so this is unchanged for every other
// message.
ubxAnyType DevUBLOXGNSS::getUbxMessageFieldCallback(ubxMessage *theMessage, const char *fieldName)
{
    ubxAnyType value;
    value.ubxDataType = 0xFF; // Sentinel - ubxDataType8bit() can never produce this value; operator double() returns 0.0 for it
    value.U8 = 0;
    if ((theMessage != nullptr) && (theMessage->_callbackStorage != nullptr))
    {
        const uint8_t *slot = theMessage->_callbackStorage + ((uint32_t)theMessage->_callbackReadIndex * theMessage->_messageLength);
        theMessage->extractFieldFrom(slot, fieldName, &value);
    }
    return value;
}

// v4 scaffolding: 
// Factory: extracts a named field from the message, reading from its live _storage
ubxAnyType DevUBLOXGNSS::getUbxMessageField(ubxMessage *theMessage, const char *fieldName)
{
    ubxAnyType value;
    value.ubxDataType = 0xFF; // Sentinel - ubxDataType8bit() can never produce this value; operator double() returns 0.0 for it
    value.U8 = 0;
    if (theMessage != nullptr)
        theMessage->extractFieldFrom(theMessage->_storage, fieldName, &value);
    return value;
}

// v4 scaffolding: variable-length/repeated-block support (e.g. UBX-NAV-SAT's per-SV blocks) -
// see AGENTS.md "Adding the variable-length UBX messages". Mirrors getUbxMessageFieldCallback()
// above, but reads field 'fieldName' from repeated block 'blockIndex' (0..numSvs-1, where
// numSvs itself is a HEADER field, read the ordinary way via getUbxMessageFieldCallback()) rather
// than from the message's header. blockIndex is not bounds-checked here against the message's
// _maxBlocks/actual block count - the caller is expected to loop only up to the numSvs (or
// equivalent) value it already read, exactly as in the NAV-SAT callback example in AGENTS.md.
ubxAnyType DevUBLOXGNSS::getUbxMessageBlockFieldCallback(ubxMessage *theMessage, uint16_t blockIndex, const char *fieldName)
{
    ubxAnyType value;
    value.ubxDataType = 0xFF; // Sentinel - ubxDataType8bit() can never produce this value; operator double() returns 0.0 for it
    value.U8 = 0;
    if ((theMessage != nullptr) && (theMessage->_blockFields != nullptr) && (theMessage->_callbackStorage != nullptr))
    {
        // See getUbxMessageFieldCallback() above - _callbackReadIndex selects which buffered slot
        // this callback firing is for (always 0 for a _numCallbackCopies <= 1 message).
        const uint8_t *slotBase = theMessage->_callbackStorage + ((uint32_t)theMessage->_callbackReadIndex * theMessage->_messageLength);
        const uint8_t *blockBuffer = slotBase + theMessage->_blockHeaderLength
                                      + ((uint32_t)blockIndex * theMessage->_blockLength);
        theMessage->extractFieldFrom(blockBuffer, fieldName, &value, (const ubxMessage::ubxField *)theMessage->_blockFields,
                                     theMessage->_numBlockFields);
    }
    return value;
}

// v4 scaffolding: variable-length/repeated-block support - see getUbxMessageBlockFieldCallback()
// above. Reads from the message's live _storage rather than its frozen _callbackStorage - see
// AGENTS.md "getUbxMessagePtr Factory design pattern" for the same live-vs-callback distinction
// on getUbxMessageField()/getUbxMessageFieldCallback().
ubxAnyType DevUBLOXGNSS::getUbxMessageBlockField(ubxMessage *theMessage, uint16_t blockIndex, const char *fieldName)
{
    ubxAnyType value;
    value.ubxDataType = 0xFF; // Sentinel - ubxDataType8bit() can never produce this value; operator double() returns 0.0 for it
    value.U8 = 0;
    if ((theMessage != nullptr) && (theMessage->_blockFields != nullptr) && (theMessage->_storage != nullptr))
    {
        const uint8_t *blockBuffer = theMessage->_storage + theMessage->_blockHeaderLength
                                      + ((uint32_t)blockIndex * theMessage->_blockLength);
        theMessage->extractFieldFrom(blockBuffer, fieldName, &value, (const ubxMessage::ubxField *)theMessage->_blockFields,
                                     theMessage->_numBlockFields);
    }
    return value;
}

// v4 scaffolding, added for ESF-MEAS, extended for ESF-RAW - see AGENTS.md "Adding support for
// ESF-MEAS" and "Adding support for ESF-RAW and ESF-STATUS". General/reusable by any future
// message with the same shape.
// Factory: the DEFENSIVELY-computed real block count (ubxMessage::getBlockCount()) for any
// message with block support, reading from its _callbackStorage - use this, not the message's own
// (possibly-unreliable, or entirely absent) header count field, to bound a
// getUbxMessageBlockFieldCallback() loop. For a message that set _blockCountField (e.g. ESF-MEAS),
// this cross-checks that header field against the actual received length; for a message with no
// block-count field at all (e.g. ESF-RAW), this computes the count purely from the actual received
// length. 0 if this message has no block support at all, or if no callback data is available yet.
uint16_t DevUBLOXGNSS::getUbxMessageBlockCountCallback(ubxMessage *theMessage)
{
    if ((theMessage == nullptr) || (theMessage->_callbackStorage == nullptr) || (theMessage->_callbackActualLength == nullptr))
        return 0;
    const uint8_t *slot = theMessage->_callbackStorage + ((uint32_t)theMessage->_callbackReadIndex * theMessage->_messageLength);
    return theMessage->getBlockCount(slot, theMessage->_callbackActualLength[theMessage->_callbackReadIndex]);
}

// Factory: same as getUbxMessageBlockCountCallback() above, reading from the message's live
// _storage instead.
uint16_t DevUBLOXGNSS::getUbxMessageBlockCount(ubxMessage *theMessage)
{
    if ((theMessage == nullptr) || (theMessage->_storage == nullptr))
        return 0;
    return theMessage->getBlockCount(theMessage->_storage, theMessage->_actualLength);
}

// Factory: extracts field 'fieldName' from a variable-length message's OPTIONAL trailing footer
// group (e.g. ESF-MEAS's calibTtag - see ubxMessage::extractFooterFieldFrom()), reading from the
// message a callback just fired for's _callbackStorage. Returns the usual "field not found"
// sentinel (see getUbxMessageFieldCallback() above) both when this message has no footer at all,
// and when THIS PARTICULAR message's actual received length was too short for the footer to have
// actually been present - the caller cannot tell those two cases apart from the return value alone
// (by design - see the file header comment in ubxESFMEAS.h), only that no footer data is
// available.
ubxAnyType DevUBLOXGNSS::getUbxMessageFooterFieldCallback(ubxMessage *theMessage, const char *fieldName)
{
    ubxAnyType value;
    value.ubxDataType = 0xFF; // Sentinel - ubxDataType8bit() can never produce this value; operator double() returns 0.0 for it
    value.U8 = 0;
    if ((theMessage != nullptr) && (theMessage->_callbackStorage != nullptr) && (theMessage->_callbackActualLength != nullptr))
    {
        const uint8_t *slot = theMessage->_callbackStorage + ((uint32_t)theMessage->_callbackReadIndex * theMessage->_messageLength);
        uint16_t actualLength = theMessage->_callbackActualLength[theMessage->_callbackReadIndex];
        uint16_t blockCount = theMessage->getBlockCount(slot, actualLength);
        theMessage->extractFooterFieldFrom(slot, actualLength, blockCount, fieldName, &value);
    }
    return value;
}

// Factory: same as getUbxMessageFooterFieldCallback() above, reading from the message's live
// _storage/_actualLength instead.
ubxAnyType DevUBLOXGNSS::getUbxMessageFooterField(ubxMessage *theMessage, const char *fieldName)
{
    ubxAnyType value;
    value.ubxDataType = 0xFF; // Sentinel - ubxDataType8bit() can never produce this value; operator double() returns 0.0 for it
    value.U8 = 0;
    if ((theMessage != nullptr) && (theMessage->_storage != nullptr))
    {
        uint16_t blockCount = theMessage->getBlockCount(theMessage->_storage, theMessage->_actualLength);
        theMessage->extractFooterFieldFrom(theMessage->_storage, theMessage->_actualLength, blockCount, fieldName, &value);
    }
    return value;
}

// Factory: the COMPLETE raw UBX frame length (6-byte header + payload + 2-byte checksum) for the
// message a callback just fired for - see ubxMessage::writeCallbackRawFrame() and AGENTS.md
// "Adding support for ESF-MEAS". Pair with getUbxMessageRawPtrCallback() to relay the message
// verbatim (e.g. Serial2.write(ptr, len)) from inside a callback. 0 if unavailable (no callback
// registered, or no data has arrived yet).
uint16_t DevUBLOXGNSS::getUbxMessageRawLengthCallback(ubxMessage *theMessage)
{
    if ((theMessage == nullptr) || (theMessage->_callbackRawFrame == nullptr) || (theMessage->_callbackActualLength == nullptr))
        return 0;
    return (uint16_t)(8 + theMessage->_callbackActualLength[theMessage->_callbackReadIndex]);
}

// Factory: a pointer to the start of the complete raw UBX frame within _callbackRawFrame, for the
// message a callback just fired for - see getUbxMessageRawLengthCallback() above. nullptr if
// unavailable.
const uint8_t *DevUBLOXGNSS::getUbxMessageRawPtrCallback(ubxMessage *theMessage)
{
    if ((theMessage == nullptr) || (theMessage->_callbackRawFrame == nullptr))
        return nullptr;
    return theMessage->_callbackRawFrame + ((uint32_t)theMessage->_callbackReadIndex * ((uint32_t)theMessage->_messageLength + 8));
}

// v4 scaffolding:
// Factory: hands back the opaque per-message object a callback's nmeaCallbackDataCommon_t* points
// at, so getNmeaMessageField() can navigate its field table and extract a named field's
// value
nmeaMessage *DevUBLOXGNSS::getNmeaMessagePtr(nmeaCallbackDataCommon_t *theData)
{
    if (theData == nullptr)
        return nullptr;
    return theData->messagePtr;
}

// v4 scaffolding: 
// Factory: extracts a named field from the message a callback just fired for, reading from its
// _callbackStorage (the copy the NMEA dispatch block in process() froze when the callback was
// queued) rather than its live _storage (which may already have been overwritten by newer data by
// the time the callback actually runs).
// _callbackStorage may hold several buffered slots for GSV (see AGENTS.md "Adding support for
// NMEA GSV messages"); checkCallbacks() sets _callbackReadIndex to the slot this firing is for
// immediately before calling the callback, so this reads that slot rather than always offset 0.
// For _numCallbackCopies <= 1 (every other message), _callbackReadIndex is always 0, so this is
// unchanged from before.
sfe_string_t DevUBLOXGNSS::getNmeaMessageFieldCallback(nmeaMessage *theMessage, const char *fieldName)
{
    sfe_string_t value = "";
    if ((theMessage != nullptr) && (theMessage->_callbackStorage != nullptr))
    {
        const uint8_t *slot = theMessage->_callbackStorage + ((uint32_t)theMessage->_callbackReadIndex * theMessage->_messageLength);
        theMessage->extractFieldFrom(slot, fieldName, value);
    }
    return value;
}

// v4 scaffolding: 
// Factory: extracts a named field from the message, reading from its live _storage
sfe_string_t DevUBLOXGNSS::getNmeaMessageField(nmeaMessage *theMessage, const char *fieldName)
{
    sfe_string_t value = "";
    if (theMessage != nullptr)
        theMessage->extractFieldFrom(theMessage->_storage, fieldName, value);
    return value;
}

// v4 scaffolding: variable-length/repeated-block support (e.g. NMEA GSV's per-satellite blocks) -
// see AGENTS.md "Adding support for NMEA GSV messages". Mirrors getNmeaMessageFieldCallback()
// above, but reads field 'fieldName' from repeated block 'blockIndex' (0..numSV-1, where numSV
// itself is a HEADER field, read the ordinary way via getNmeaMessageFieldCallback()) rather than
// from the message's header/footer. Unlike the UBX block accessors, this - and
// nmeaMessage::extractFieldFrom() underneath it - explicitly bounds-checks blockIndex against the
// message's maxNumBlocks and returns an empty String if it's out of range, per AGENTS.md: NMEA
// fields are ASCII of unknown extent, not a fixed-size binary block, so an out-of-range block
// can't just be treated as unused-but-allocated memory the way the UBX side does.
sfe_string_t DevUBLOXGNSS::getNmeaMessageBlockFieldCallback(nmeaMessage *theMessage, uint16_t blockIndex, const char *fieldName)
{
    sfe_string_t value = "";
    if ((theMessage != nullptr) && (theMessage->_blockFields != nullptr) && (theMessage->_callbackStorage != nullptr))
    {
        // See getNmeaMessageFieldCallback() above - _callbackReadIndex selects which buffered
        // slot this callback firing is for (always 0 for a _numCallbackCopies <= 1 message).
        const uint8_t *slot = theMessage->_callbackStorage + ((uint32_t)theMessage->_callbackReadIndex * theMessage->_messageLength);
        theMessage->extractFieldFrom(slot, fieldName, value, theMessage->_blockFields, theMessage->_numBlockFields, blockIndex);
    }
    return value;
}

// v4 scaffolding: variable-length/repeated-block support - see getNmeaMessageBlockFieldCallback()
// above. Reads from the message's live _storage rather than its frozen _callbackStorage.
sfe_string_t DevUBLOXGNSS::getNmeaMessageBlockField(nmeaMessage *theMessage, uint16_t blockIndex, const char *fieldName)
{
    sfe_string_t value = "";
    if ((theMessage != nullptr) && (theMessage->_blockFields != nullptr) && (theMessage->_storage != nullptr))
        theMessage->extractFieldFrom(theMessage->_storage, fieldName, value, theMessage->_blockFields, theMessage->_numBlockFields, blockIndex);
    return value;
}

// v4 scaffolding: generic replacement for the removed per-message setAuto<MSG>callbackPtr()
// functions - see AGENTS.md "setAutoCallbackPtr". Finds the registered message by name and wires
// up the callback; does not itself touch the module's message-output rate (see the declaration's
// comment in u-blox_GNSS.h and CallbackExample1_NAVHPPOSLLH.ino).
bool DevUBLOXGNSS::setAutoCallbackPtr(const char *classStr, const char *idStr, void (*callbackPointerPtr)(ubxCallbackDataCommon_t *))
{
  ubxMessage *msg = ubxMessages.findByName(classStr, idStr);
  if (msg == nullptr) // No message registered under that classStr/idStr
    return false;

  if (!msg->initStorage()) // Check that RAM has been allocated for the message data
    return false;

  if (!msg->initCallbackStorage()) // Check that RAM has been allocated for the callback copy
    return false;

  return (ubxMessages.setCallback(msg->_Class, msg->_ID, callbackPointerPtr) == SFE_UBLOX_STATUS_SUCCESS);
}

// v4 scaffolding: generic replacement for the removed per-message setNMEA<MSG>callbackPtr()
// functions. Finds the registered message by name and wires up the callback;
// does not itself touch the module's message-output rate.
bool DevUBLOXGNSS::setNmeaCallbackPtr(const char *msgId, void (*callbackPointerPtr)(nmeaCallbackDataCommon_t *))
{
  nmeaMessage *msg = nmeaMessages.findByName(msgId);
  if (msg == nullptr) // No message registered under that classStr/idStr
    return false;

  if (!msg->initStorage()) // Check that RAM has been allocated for the message data
    return false;

  if (!msg->initCallbackStorage()) // Check that RAM has been allocated for the callback copy
    return false;

  return (nmeaMessages.setCallback(msg->_msgId, callbackPointerPtr) == SFE_UBLOX_STATUS_SUCCESS);
}

// Check if any callbacks are waiting to be processed
void DevUBLOXGNSS::checkCallbacks(void)
{
  if (checkCallbacksReentrant == true) // Check for reentry (i.e. checkCallbacks has been called from inside a callback)
    return;

  checkCallbacksReentrant = true;

  // v4 scaffolding: generic callback dispatch for every message registered in the new registry -
  // replaces the old per-message blocks below for the messages that have been migrated to it (see
  // AGENTS.md "setAutoCallbackPtr" and "Future work"). Messages not yet migrated to the registry
  // keep using their own dedicated block further down.
  for (auto msg : ubxMessages.ubxMessageVectors)
  {
    // Drain every buffered slot for this message, oldest first (FIFO). Most messages have only
    // one slot (_numCallbackCopies == 1), so this runs at most once, exactly as before. RXM-SFRBX/
    // ESF-MEAS can have several buffered messages waiting after a burst arrived in a single
    // checkUblox() call - see AGENTS.md "Adding support for RXM-SFRBX" and
    // ubxMessageVector::storePayload() (the write side of this same ring buffer).
    while ((msg->_callbackPtr != nullptr) && (msg->_callbackCount > 0))
    {
      msg->_callbackReadIndex = msg->_callbackTail; // Tell the getters which slot this firing reads
      ubxCallbackDataCommon_t commonData;
      commonData.Class = msg->_Class;
      commonData.ID = msg->_ID;
      commonData.messagePtr = msg;
      msg->_callbackPtr(&commonData); // Call the callback
      msg->_callbackTail = (uint8_t)((msg->_callbackTail + 1) % msg->_numCallbackCopies);
      msg->_callbackCount--; // One fewer fresh slot waiting
    }
  }

  // v4 scaffolding: generic callback dispatch for every NMEA message registered in the new
  // registry. Drains every buffered slot for this message, oldest first (FIFO) - most messages
  // have only one slot (_numCallbackCopies == 1), so this runs at most once, exactly as before.
  // GSV can have several buffered sentences waiting after a burst arrived in a single
  // checkUblox() call - see AGENTS.md "Adding support for NMEA GSV messages" and the ring-buffer
  // write side in the NMEA dispatch block of process(), above.
  for (auto msg : nmeaMessages.nmeaMessageVectors)
  {
    while ((msg->_callbackPtr != nullptr) && (msg->_callbackCount > 0))
    {
      msg->_callbackReadIndex = msg->_callbackTail; // Tell the getters which slot this firing reads
      nmeaCallbackDataCommon_t commonData;
      memcpy(commonData.msgId, msg->_msgId, 4); // Copy the three char ID plus the NULL
      commonData.messagePtr = msg;
      msg->_callbackPtr(&commonData); // Call the callback
      msg->_callbackTail = (uint8_t)((msg->_callbackTail + 1) % msg->_numCallbackCopies);
      msg->_callbackCount--; // One fewer fresh slot waiting
    }
  }

  // UBX_NAV_SAT's and UBX_NAV_SIG's callbacks are both dispatched by the generic registry walk
  // above (ubxNAVSAT/ubxNAVSIG are now self-registered) - see AGENTS.md "Adding the
  // variable-length UBX messages".

  // UBX_RXM_PMP's and UBX_RXM_QZSSL6's callbacks are now dispatched by the generic registry
  // walk above (ubxRXMPMP/ubxRXMQZSSL6 are now self-registered) - see AGENTS.md "Adding support
  // for RXM-PMP" and "Adding support for RXM-QZSSL6". QZSSL6's 2-slot ring buffer
  // (numCallbackCopies = UBX_RXM_QZSSL6_NUM_CHANNELS) is drained the same draining-while-loop
  // way as RXM-SFRBX/ESF-MEAS, oldest slot first - see the generic ring-buffer walk above.

  // UBX_RXM_SFRBX's callback is now dispatched by the generic registry walk above
  // (ubxRXMSFRBX is now self-registered, with its own ring-buffered _callbackStorage) - see
  // AGENTS.md "Adding support for RXM-SFRBX". The old raw-full-message callback
  // (setAutoRXMSFRBXmessageCallbackPtr) is retired - see the comment above getRXMSFRBX().
  // UBX_RXM_RAWX's and UBX_RXM_MEASX's callbacks are both dispatched by the generic registry
  // walk above (ubxRXMRAWX/ubxRXMMEASX are now self-registered) - see AGENTS.md "Adding the
  // variable-length UBX messages".

  // UBX_MON_COMMS's callback is now dispatched by the generic registry walk above
  // (ubxMONCOMMS is now self-registered) - see AGENTS.md "Adding the variable-length UBX
  // messages".

  // UBX_ESF_MEAS's callback is now dispatched by the generic registry walk above (ubxESFMEAS is
  // now self-registered, with its own ring-buffered _callbackStorage) - see AGENTS.md "Adding
  // support for ESF-MEAS".

  // UBX_ESF_RAW's and UBX_ESF_STATUS's callbacks are both dispatched by the generic registry walk
  // above (ubxESFRAW/ubxESFSTATUS are now self-registered) - see AGENTS.md "Adding support for
  // ESF-RAW and ESF-STATUS". (The ESF_RAW block that used to be here was already unreachable dead
  // code before this migration - packetUBXESFRAW could never actually be non-nullptr, since
  // initPacketUBXESFRAW() was declared but never defined.)

  // UBX_SEC_SIG's callback is now dispatched by the generic registry walk above (ubxSECSIG is
  // now self-registered) - see AGENTS.md "Adding the variable-length UBX messages".

  if (storageRTCM1005 != nullptr)                                            // If RAM has been allocated for message storage
    if (storageRTCM1005->callbackData != nullptr)                            // If RAM has been allocated for the copy of the data
      if (storageRTCM1005->automaticFlags.flags.bits.callbackDataValid == 1) // If the copy of the data is valid
      {
        if (storageRTCM1005->callbackPointerPtr != nullptr)                   // If the pointer to the callback has been defined
          storageRTCM1005->callbackPointerPtr(storageRTCM1005->callbackData); // Call the callback
        storageRTCM1005->automaticFlags.flags.bits.callbackDataValid = 0;     // Mark the data as stale
      }

  if (rtcmInputStorage.rtcm1005CallbackPointer != nullptr) // If the pointer to the callback has been defined
    if (rtcmInputStorage.flags.bits.dataValid1005 == 1)    // If the copy of the data is valid
      if (rtcmInputStorage.flags.bits.dataRead1005 == 0)   // If the data has not been read
      {
        rtcmInputStorage.rtcm1005CallbackPointer(&rtcmInputStorage.rtcm1005); // Call the callback
        rtcmInputStorage.flags.bits.dataRead1005 = 1;                         // Mark the data as read
      }

  if (rtcmInputStorage.rtcm1006CallbackPointer != nullptr) // If the pointer to the callback has been defined
    if (rtcmInputStorage.flags.bits.dataValid1006 == 1)    // If the copy of the data is valid
      if (rtcmInputStorage.flags.bits.dataRead1006 == 0)   // If the data has not been read
      {
        rtcmInputStorage.rtcm1006CallbackPointer(&rtcmInputStorage.rtcm1006); // Call the callback
        rtcmInputStorage.flags.bits.dataRead1006 = 1;                         // Mark the data as read
      }

  checkCallbacksReentrant = false;
}

// Push (e.g.) RTCM data directly to the module
// Returns true if all numDataBytes were pushed successfully
// Warning: this function does not check that the data is valid. It is the user's responsibility to ensure the data is valid before pushing.
bool DevUBLOXGNSS::pushRawData(uint8_t *dataBytes, size_t numDataBytes, bool callProcessBuffer)
{
  // Return now if numDataBytes is zero
  if (numDataBytes == 0)
    return false; // Indicate to the user that there was no data to push

  parseRTCM1005(dataBytes, numDataBytes);
  parseRTCM1006(dataBytes, numDataBytes);

  if (!lock())
    return false;

  bool ok = false;
  if (_commType == COMM_TYPE_SERIAL)
  {
    // Serial: divide pushes up into 16 byte chunks and call checkUbloxSerial between pushes
    // (if callProcessBuffer is true) to try and avoid data loss
    ok = true;
    size_t bytesLeftToWrite = numDataBytes;
    while (bytesLeftToWrite > 0)
    {
      uint8_t bytesToWrite;

      if (bytesLeftToWrite < 16)
        bytesToWrite = (uint8_t)bytesLeftToWrite;
      else
        bytesToWrite = 16;

      ok &= (writeBytes(dataBytes, bytesToWrite) == bytesToWrite); // Will set ok to false if any one write fails

      bytesLeftToWrite -= (size_t)bytesToWrite;
      dataBytes += bytesToWrite;

      if (callProcessBuffer)                // Try and prevent data loss during large pushes by calling checkUbloxSerial between chunks
        checkUbloxSerial(&packetCfg, 0, 0); // Don't call checkUbloxInternal as we already have the lock!
    }
  }
  else if (_commType == COMM_TYPE_I2C)
  {
    // We can not write a single data byte to I2C as it would look like the address of a random read.
    // If numDataBytes is 1, we should probably just reject the data and return false.
    // But we'll be nice and store the byte until the next time pushRawData is called.

    // Storage just in case the user tries to push a single byte using pushRawBytes
    static bool _pushSingleByte = false;
    static uint8_t _pushThisSingleByte = 0;

    if ((numDataBytes == 1) && (_pushSingleByte == false))
    {
      _pushThisSingleByte = *dataBytes;
      _pushSingleByte = true;
      ok = false; // Indicate to the user that their data has not been pushed yet
    }
    else
    {

      // I2C: split the data up into packets of i2cTransactionSize
      size_t bytesLeftToWrite = numDataBytes;
      size_t bytesWrittenTotal = 0;

      if (_pushSingleByte == true) // Increment bytesLeftToWrite if we have a single byte waiting to be pushed
        bytesLeftToWrite++;

      while (bytesLeftToWrite > 0)
      {
        size_t bytesToWrite; // Limit bytesToWrite to i2cTransactionSize

        if (bytesLeftToWrite > i2cTransactionSize)
          bytesToWrite = i2cTransactionSize;
        else
          bytesToWrite = bytesLeftToWrite;

        // If there would be one byte left to be written next time, send one byte less now
        if ((bytesLeftToWrite - bytesToWrite) == 1)
          bytesToWrite--;

        size_t bytesWritten = 0;

        if (_pushSingleByte == true)
        {
          uint8_t buf[i2cTransactionSize];

          buf[0] = _pushThisSingleByte;

          for (uint16_t x = 1; x < bytesToWrite; x++)
            buf[x] = dataBytes[x - 1];

          bytesWritten += writeBytes(buf, bytesToWrite); // Write the bytes
          dataBytes += bytesToWrite - 1;                 // Point to fresh data
          _pushSingleByte = false;                       // Clear the flag
        }
        else
        {
          bytesWritten += writeBytes(dataBytes, bytesToWrite); // Write the bytes
          dataBytes += bytesToWrite;                           // Point to fresh data
        }

        bytesWrittenTotal += bytesWritten; // Update the totals
        bytesLeftToWrite -= bytesToWrite;
      }

      ok = (bytesWrittenTotal == numDataBytes); // Return true if the correct number of bytes were written
    }
  }
  else if (_commType == COMM_TYPE_SPI)
  {
    // We've got to be careful here...
    // We could be pushing a lot of data.
    // And we're supposed to be reading the same amount of data at the same time.
    // If numDataBytes is > spiBuffer, we'll lose data
    // We'll call processSpiBuffer between transactions to try and prevent lost data
    size_t bytesLeftToWrite = numDataBytes;

    while (bytesLeftToWrite > 0)
    {
      size_t bytesToWrite; // Limit bytesToWrite to spiTransactionSize

      if (bytesLeftToWrite > spiTransactionSize)
        bytesToWrite = spiTransactionSize;
      else
        bytesToWrite = bytesLeftToWrite;

      startWriteReadByte();

      for (size_t i = 0; i < bytesToWrite; i++)
      {
        spiTransfer(*dataBytes);
        dataBytes++;
      }

      endWriteReadByte();

      bytesLeftToWrite -= bytesToWrite; // Update the totals

      if (callProcessBuffer)
        processSpiBuffer(&packetCfg, 0, 0); // This will hopefully prevent any lost data?
    }

    ok = true;
  }

  unlock();

  return ok;
}

// Push MGA AssistNow data to the module.
// Check for UBX-MGA-ACK responses if required (if mgaAck is YES or ENQUIRE).
// Wait for maxWait millis after sending each packet (if mgaAck is NO).
// Return how many bytes were pushed successfully.
// If skipTime is true, any UBX-MGA-INI-TIME_UTC or UBX-MGA-INI-TIME_GNSS packets found in the data will be skipped,
// allowing the user to override with their own time data with setUTCTimeAssistance.
size_t DevUBLOXGNSS::pushAssistNowData(const sfe_string_t &dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck, uint16_t maxWait)
{
  return (pushAssistNowDataInternal(0, false, (const uint8_t *)dataBytes.c_str(), numDataBytes, mgaAck, maxWait));
}
size_t DevUBLOXGNSS::pushAssistNowData(const uint8_t *dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck, uint16_t maxWait)
{
  return (pushAssistNowDataInternal(0, false, dataBytes, numDataBytes, mgaAck, maxWait));
}
size_t DevUBLOXGNSS::pushAssistNowData(bool skipTime, const sfe_string_t &dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck, uint16_t maxWait)
{
  return (pushAssistNowDataInternal(0, skipTime, (const uint8_t *)dataBytes.c_str(), numDataBytes, mgaAck, maxWait));
}
size_t DevUBLOXGNSS::pushAssistNowData(bool skipTime, const uint8_t *dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck, uint16_t maxWait)
{
  return (pushAssistNowDataInternal(0, skipTime, dataBytes, numDataBytes, mgaAck, maxWait));
}
size_t DevUBLOXGNSS::pushAssistNowData(size_t offset, bool skipTime, const sfe_string_t &dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck, uint16_t maxWait)
{
  return (pushAssistNowDataInternal(offset, skipTime, (const uint8_t *)dataBytes.c_str(), numDataBytes, mgaAck, maxWait));
}
size_t DevUBLOXGNSS::pushAssistNowData(size_t offset, bool skipTime, const uint8_t *dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck, uint16_t maxWait)
{
  return (pushAssistNowDataInternal(offset, skipTime, dataBytes, numDataBytes, mgaAck, maxWait));
}
size_t DevUBLOXGNSS::pushAssistNowDataInternal(size_t offset, bool skipTime, const uint8_t *dataBytes, size_t numDataBytes, sfe_ublox_mga_assist_ack_e mgaAck, uint16_t maxWait)
{
  size_t dataPtr = offset;     // Pointer into dataBytes
  size_t packetsProcessed = 0; // Keep count of how many packets have been processed
  size_t bytesPushed = 0;      // Keep count

  bool checkForAcks = (mgaAck == SFE_UBLOX_MGA_ASSIST_ACK_YES); // If mgaAck is YES, always check for Acks

  // If mgaAck is ENQUIRE, we need to check UBX-CFG-NAVX5 ackAiding to determine if UBX-MGA-ACK's are expected
  if (mgaAck == SFE_UBLOX_MGA_ASSIST_ACK_ENQUIRE)
  {
    uint8_t ackAiding = getAckAiding(maxWait); // Enquire if we should expect Acks
    if (ackAiding == 1)
      checkForAcks = true;

    debugPrint("pushAssistNowData: mgaAck is ENQUIRE. getAckAiding returned ", true); // Important
    debugPrintln(ackAiding, true);
  }

  // If checkForAcks is true, then we need to set up storage for the UBX-MGA-ACK-DATA0 messages
  if (checkForAcks)
  {
    if (packetUBXMGAACK == nullptr)
      initPacketUBXMGAACK();        // Check that RAM has been allocated for the MGA_ACK data
    if (packetUBXMGAACK == nullptr) // Bail if the RAM allocation failed
      return (0);
  }

  while (dataPtr < (offset + numDataBytes)) // Keep going until we have processed all the bytes
  {
    // Start by checking the validity of the packet being pointed to
    bool dataIsOK = true;

    dataIsOK &= (*(dataBytes + dataPtr + 0) == UBX_SYNCH_1);   // Check for 0xB5
    dataIsOK &= (*(dataBytes + dataPtr + 1) == UBX_SYNCH_2);   // Check for 0x62
    dataIsOK &= (*(dataBytes + dataPtr + 2) == UBX_CLASS_MGA); // Check for class UBX-MGA

    size_t packetLength = ((size_t) * (dataBytes + dataPtr + 4)) | (((size_t) * (dataBytes + dataPtr + 5)) << 8); // Extract the length

    uint8_t checksumA = 0;
    uint8_t checksumB = 0;
    // Calculate the checksum bytes
    // Keep going until the end of the packet is reached (payloadPtr == (dataPtr + packetLength))
    // or we reach the end of the AssistNow data (payloadPtr == offset + numDataBytes)
    for (size_t payloadPtr = dataPtr + ((size_t)2); (payloadPtr < (dataPtr + packetLength + ((size_t)6))) && (payloadPtr < (offset + numDataBytes)); payloadPtr++)
    {
      checksumA += *(dataBytes + payloadPtr);
      checksumB += checksumA;
    }
    // Check the checksum bytes
    dataIsOK &= (checksumA == *(dataBytes + dataPtr + packetLength + ((size_t)6)));
    dataIsOK &= (checksumB == *(dataBytes + dataPtr + packetLength + ((size_t)7)));

    dataIsOK &= ((dataPtr + packetLength + ((size_t)8)) <= (offset + numDataBytes)); // Check we haven't overrun

    // If the data is valid, push it
    if (dataIsOK)
    {
      // Check if this is time assistance data which should be skipped
      if ((skipTime) && ((*(dataBytes + dataPtr + 3) == UBX_MGA_INI_TIME_UTC) || (*(dataBytes + dataPtr + 3) == UBX_MGA_INI_TIME_GNSS)))
      {
        debugPrint("pushAssistNowData: skipped INI_TIME ID 0x", true); // Important
        if (*(dataBytes + dataPtr + 3) < 0x10)
          debugPrint("0", true);
        debugPrintln(*(dataBytes + dataPtr + 3), HEX, true);
      }
      else
      {
        bool pushResult = pushRawData((uint8_t *)(dataBytes + dataPtr), packetLength + ((size_t)8), checkForAcks ? false : true); // Push the data. Don't call processSpiBuffer / checkUbloxSerial when using ACKs

        if (pushResult)
          bytesPushed += packetLength + ((size_t)8); // Increment bytesPushed if the push was successful

        debugPrint("pushAssistNowData: packet ID 0x", true); // Important
        if (*(dataBytes + dataPtr + 3) < 0x10)
          debugPrint("0", true);
        debugPrint(*(dataBytes + dataPtr + 3), HEX, true);
        debugPrint(" length ", true);
        debugPrintln(packetLength, true);

        if (checkForAcks)
        {
          unsigned long startTime = sfe_millis();
          bool keepGoing = true;
          while (keepGoing && ((sfe_millis() - startTime) < maxWait)) // Keep checking for the ACK until we time out
          {
            checkUbloxInternal(&packetCfg, 0, 0);               // Call checkUbloxInternal to parse any incoming data. Don't overwrite the requested Class and ID. We could be pushing this from another thread...
            if (packetUBXMGAACK->head != packetUBXMGAACK->tail) // Does the MGA ACK ringbuffer contain any ACK's?
            {
              bool dataAckd = true;                                                                                        // Check if we've received the correct ACK
              dataAckd &= (packetUBXMGAACK->data[packetUBXMGAACK->tail].msgId == *(dataBytes + dataPtr + 3));              // Check if the message ID matches
              dataAckd &= (packetUBXMGAACK->data[packetUBXMGAACK->tail].msgPayloadStart[0] == *(dataBytes + dataPtr + 6)); // Check if the first four data bytes match
              dataAckd &= (packetUBXMGAACK->data[packetUBXMGAACK->tail].msgPayloadStart[1] == *(dataBytes + dataPtr + 7));
              dataAckd &= (packetUBXMGAACK->data[packetUBXMGAACK->tail].msgPayloadStart[2] == *(dataBytes + dataPtr + 8));
              dataAckd &= (packetUBXMGAACK->data[packetUBXMGAACK->tail].msgPayloadStart[3] == *(dataBytes + dataPtr + 9));

              if (dataAckd) // Is this the ACK we are looking for?
              {
                if ((packetUBXMGAACK->data[packetUBXMGAACK->tail].type == (uint8_t)1) && (packetUBXMGAACK->data[packetUBXMGAACK->tail].infoCode == (uint8_t)SFE_UBLOX_MGA_ACK_INFOCODE_ACCEPTED))
                {
                  debugPrint("pushAssistNowData: packet was accepted after ", true); // Important
                  debugPrint(sfe_millis() - startTime, true);
                  debugPrintln(" ms", true);
                  packetsProcessed++;
                }
                else
                {
                  debugPrint("pushAssistNowData: packet was _not_ accepted. infoCode is ", true); // Important
                  debugPrintln(packetUBXMGAACK->data[packetUBXMGAACK->tail].infoCode, true);
                }
                keepGoing = false;
              }
              // Increment the tail
              packetUBXMGAACK->tail++;
              if (packetUBXMGAACK->tail == UBX_MGA_ACK_DATA0_RINGBUFFER_LEN)
                packetUBXMGAACK->tail = 0;
            }
          }
          if (keepGoing) // If keepGoing is still true, we must have timed out
          {
            debugPrintln("pushAssistNowData: packet ack timed out!", true); // Important
          }
        }
        else
        {
          // We are not checking for Acks, so let's assume the send was successful?
          packetsProcessed++;
          // We are not checking for Acks, so delay for maxWait millis unless we've reached the end of the data
          if ((dataPtr + packetLength + ((size_t)8)) < (offset + numDataBytes))
          {
            sfe_delay(maxWait);
          }
        }
      }

      dataPtr += packetLength + ((size_t)8); // Point to the next message
    }
    else
    {

      // The data was invalid. Send a debug message and then try to find the next 0xB5
      debugPrint("pushAssistNowData: bad data - ignored! dataPtr is ", true); // Important
      debugPrintln(dataPtr, true);

      while ((dataPtr < (offset + numDataBytes)) && (*(dataBytes + ++dataPtr) != UBX_SYNCH_1))
      {
        ; // Increment dataPtr until we are pointing at the next 0xB5 - or we reach the end of the data
      }
    }
  }

  debugPrint("pushAssistNowData: packetsProcessed: ", true); // Important
  debugPrintln(packetsProcessed, true);

  return (bytesPushed); // Return the number of valid bytes successfully pushed
}

// PRIVATE: Allocate RAM for packetUBXMGAACK and initialize it
bool DevUBLOXGNSS::initPacketUBXMGAACK()
{
  packetUBXMGAACK = new UBX_MGA_ACK_DATA0_t; // Allocate RAM for the main struct
  if (packetUBXMGAACK == nullptr)
  {
    debugPrintln("initPacketUBXMGAACK: RAM alloc failed!", true); // Important
    return (false);
  }
  packetUBXMGAACK->head = 0; // Initialize the ring buffer pointers
  packetUBXMGAACK->tail = 0;
  return (true);
}

// Provide initial time assistance
bool DevUBLOXGNSS::setUTCTimeAssistance(uint16_t year, uint8_t month, uint8_t day,
                                        uint8_t hour, uint8_t minute, uint8_t second, uint32_t nanos,
                                        uint16_t tAccS, uint32_t tAccNs, uint8_t source,
                                        sfe_ublox_mga_assist_ack_e mgaAck, uint16_t maxWait)
{
  uint8_t iniTimeUTC[32];       // Create the UBX-MGA-INI-TIME_UTC message by hand
  memset(iniTimeUTC, 0x00, 32); // Set all unused / reserved bytes and the checksum to zero

  iniTimeUTC[0] = UBX_SYNCH_1;              // Sync char 1
  iniTimeUTC[1] = UBX_SYNCH_2;              // Sync char 2
  iniTimeUTC[2] = UBX_CLASS_MGA;            // Class
  iniTimeUTC[3] = UBX_MGA_INI_TIME_UTC;     // ID
  iniTimeUTC[4] = 24;                       // Length LSB
  iniTimeUTC[5] = 0x00;                     // Length MSB
  iniTimeUTC[6] = 0x10;                     // type
  iniTimeUTC[7] = 0x00;                     // version
  iniTimeUTC[8] = source;                   // ref (source)
  iniTimeUTC[9] = 0x80;                     // leapSecs. Set to 0x80 = unknown
  iniTimeUTC[10] = (uint8_t)(year & 0xFF);  // year LSB
  iniTimeUTC[11] = (uint8_t)(year >> 8);    // year MSB
  iniTimeUTC[12] = month;                   // month starting at 1
  iniTimeUTC[13] = day;                     // day starting at 1
  iniTimeUTC[14] = hour;                    // hour 0:23
  iniTimeUTC[15] = minute;                  // minute 0:59
  iniTimeUTC[16] = second;                  // seconds 0:59
  iniTimeUTC[18] = (uint8_t)(nanos & 0xFF); // nanoseconds LSB
  iniTimeUTC[19] = (uint8_t)((nanos >> 8) & 0xFF);
  iniTimeUTC[20] = (uint8_t)((nanos >> 16) & 0xFF);
  iniTimeUTC[21] = (uint8_t)(nanos >> 24);   // nanoseconds MSB
  iniTimeUTC[22] = (uint8_t)(tAccS & 0xFF);  // seconds part of the accuracy LSB
  iniTimeUTC[23] = (uint8_t)(tAccS >> 8);    // seconds part of the accuracy MSB
  iniTimeUTC[26] = (uint8_t)(tAccNs & 0xFF); // nanoseconds part of the accuracy LSB
  iniTimeUTC[27] = (uint8_t)((tAccNs >> 8) & 0xFF);
  iniTimeUTC[28] = (uint8_t)((tAccNs >> 16) & 0xFF);
  iniTimeUTC[29] = (uint8_t)(tAccNs >> 24); // nanoseconds part of the accuracy MSB

  for (uint8_t i = 2; i < 30; i++) // Calculate the checksum
  {
    iniTimeUTC[30] += iniTimeUTC[i];
    iniTimeUTC[31] += iniTimeUTC[30];
  }

  // Return true if the one packet was pushed successfully
  return (pushAssistNowDataInternal(0, false, iniTimeUTC, 32, mgaAck, maxWait) == 32);
}

// Provide initial position assistance
// The units for ecefX/Y/Z and posAcc (stddev) are cm.
bool DevUBLOXGNSS::setPositionAssistanceXYZ(int32_t ecefX, int32_t ecefY, int32_t ecefZ, uint32_t posAcc, sfe_ublox_mga_assist_ack_e mgaAck, uint16_t maxWait)
{
  uint8_t iniPosXYZ[28];       // Create the UBX-MGA-INI-POS_XYZ message by hand
  memset(iniPosXYZ, 0x00, 28); // Set all unused / reserved bytes and the checksum to zero

  iniPosXYZ[0] = UBX_SYNCH_1;         // Sync char 1
  iniPosXYZ[1] = UBX_SYNCH_2;         // Sync char 2
  iniPosXYZ[2] = UBX_CLASS_MGA;       // Class
  iniPosXYZ[3] = UBX_MGA_INI_POS_XYZ; // ID
  iniPosXYZ[4] = 20;                  // Length LSB
  iniPosXYZ[5] = 0x00;                // Length MSB
  iniPosXYZ[6] = 0x00;                // type
  iniPosXYZ[7] = 0x00;                // version

  union // Use a union to convert from int32_t to uint32_t
  {
    int32_t signedLong;
    uint32_t unsignedLong;
  } signedUnsigned;

  signedUnsigned.signedLong = ecefX;
  iniPosXYZ[10] = (uint8_t)(signedUnsigned.unsignedLong & 0xFF); // LSB
  iniPosXYZ[11] = (uint8_t)((signedUnsigned.unsignedLong >> 8) & 0xFF);
  iniPosXYZ[12] = (uint8_t)((signedUnsigned.unsignedLong >> 16) & 0xFF);
  iniPosXYZ[13] = (uint8_t)(signedUnsigned.unsignedLong >> 24); // MSB

  signedUnsigned.signedLong = ecefY;
  iniPosXYZ[14] = (uint8_t)(signedUnsigned.unsignedLong & 0xFF); // LSB
  iniPosXYZ[15] = (uint8_t)((signedUnsigned.unsignedLong >> 8) & 0xFF);
  iniPosXYZ[16] = (uint8_t)((signedUnsigned.unsignedLong >> 16) & 0xFF);
  iniPosXYZ[17] = (uint8_t)(signedUnsigned.unsignedLong >> 24); // MSB

  signedUnsigned.signedLong = ecefZ;
  iniPosXYZ[18] = (uint8_t)(signedUnsigned.unsignedLong & 0xFF); // LSB
  iniPosXYZ[19] = (uint8_t)((signedUnsigned.unsignedLong >> 8) & 0xFF);
  iniPosXYZ[20] = (uint8_t)((signedUnsigned.unsignedLong >> 16) & 0xFF);
  iniPosXYZ[21] = (uint8_t)(signedUnsigned.unsignedLong >> 24); // MSB

  iniPosXYZ[22] = (uint8_t)(posAcc & 0xFF); // LSB
  iniPosXYZ[23] = (uint8_t)((posAcc >> 8) & 0xFF);
  iniPosXYZ[24] = (uint8_t)((posAcc >> 16) & 0xFF);
  iniPosXYZ[25] = (uint8_t)(posAcc >> 24); // MSB

  for (uint8_t i = 2; i < 26; i++) // Calculate the checksum
  {
    iniPosXYZ[26] += iniPosXYZ[i];
    iniPosXYZ[27] += iniPosXYZ[26];
  }

  // Return true if the one packet was pushed successfully
  return (pushAssistNowDataInternal(0, false, iniPosXYZ, 28, mgaAck, maxWait) == 28);
}

// The units for lat and lon are degrees * 1e-7 (WGS84)
// The units for alt (WGS84) and posAcc (stddev) are cm.
bool DevUBLOXGNSS::setPositionAssistanceLLH(int32_t lat, int32_t lon, int32_t alt, uint32_t posAcc, sfe_ublox_mga_assist_ack_e mgaAck, uint16_t maxWait)
{
  uint8_t iniPosLLH[28];       // Create the UBX-MGA-INI-POS_LLH message by hand
  memset(iniPosLLH, 0x00, 28); // Set all unused / reserved bytes and the checksum to zero

  iniPosLLH[0] = UBX_SYNCH_1;         // Sync char 1
  iniPosLLH[1] = UBX_SYNCH_2;         // Sync char 2
  iniPosLLH[2] = UBX_CLASS_MGA;       // Class
  iniPosLLH[3] = UBX_MGA_INI_POS_LLH; // ID
  iniPosLLH[4] = 20;                  // Length LSB
  iniPosLLH[5] = 0x00;                // Length MSB
  iniPosLLH[6] = 0x01;                // type
  iniPosLLH[7] = 0x00;                // version

  union // Use a union to convert from int32_t to uint32_t
  {
    int32_t signedLong;
    uint32_t unsignedLong;
  } signedUnsigned;

  signedUnsigned.signedLong = lat;
  iniPosLLH[10] = (uint8_t)(signedUnsigned.unsignedLong & 0xFF); // LSB
  iniPosLLH[11] = (uint8_t)((signedUnsigned.unsignedLong >> 8) & 0xFF);
  iniPosLLH[12] = (uint8_t)((signedUnsigned.unsignedLong >> 16) & 0xFF);
  iniPosLLH[13] = (uint8_t)(signedUnsigned.unsignedLong >> 24); // MSB

  signedUnsigned.signedLong = lon;
  iniPosLLH[14] = (uint8_t)(signedUnsigned.unsignedLong & 0xFF); // LSB
  iniPosLLH[15] = (uint8_t)((signedUnsigned.unsignedLong >> 8) & 0xFF);
  iniPosLLH[16] = (uint8_t)((signedUnsigned.unsignedLong >> 16) & 0xFF);
  iniPosLLH[17] = (uint8_t)(signedUnsigned.unsignedLong >> 24); // MSB

  signedUnsigned.signedLong = alt;
  iniPosLLH[18] = (uint8_t)(signedUnsigned.unsignedLong & 0xFF); // LSB
  iniPosLLH[19] = (uint8_t)((signedUnsigned.unsignedLong >> 8) & 0xFF);
  iniPosLLH[20] = (uint8_t)((signedUnsigned.unsignedLong >> 16) & 0xFF);
  iniPosLLH[21] = (uint8_t)(signedUnsigned.unsignedLong >> 24); // MSB

  iniPosLLH[22] = (uint8_t)(posAcc & 0xFF); // LSB
  iniPosLLH[23] = (uint8_t)((posAcc >> 8) & 0xFF);
  iniPosLLH[24] = (uint8_t)((posAcc >> 16) & 0xFF);
  iniPosLLH[25] = (uint8_t)(posAcc >> 24); // MSB

  for (uint8_t i = 2; i < 26; i++) // Calculate the checksum
  {
    iniPosLLH[26] += iniPosLLH[i];
    iniPosLLH[27] += iniPosLLH[26];
  }

  // Return true if the one packet was pushed successfully
  return (pushAssistNowDataInternal(0, false, iniPosLLH, 28, mgaAck, maxWait) == 28);
}

// Find the start of the AssistNow Offline (UBX_MGA_ANO) data for the chosen day
// The daysIntoFture parameter makes it easy to get the data for (e.g.) tomorrow based on today's date
// Returns numDataBytes if unsuccessful
// TO DO: enhance this so it will find the nearest data for the chosen day - instead of an exact match
size_t DevUBLOXGNSS::findMGAANOForDate(const sfe_string_t &dataBytes, size_t numDataBytes, uint16_t year, uint8_t month, uint8_t day, uint8_t daysIntoFuture)
{
  return (findMGAANOForDateInternal((const uint8_t *)dataBytes.c_str(), numDataBytes, year, month, day, daysIntoFuture));
}
size_t DevUBLOXGNSS::findMGAANOForDate(const uint8_t *dataBytes, size_t numDataBytes, uint16_t year, uint8_t month, uint8_t day, uint8_t daysIntoFuture)
{
  return (findMGAANOForDateInternal(dataBytes, numDataBytes, year, month, day, daysIntoFuture));
}
size_t DevUBLOXGNSS::findMGAANOForDateInternal(const uint8_t *dataBytes, size_t numDataBytes, uint16_t year, uint8_t month, uint8_t day, uint8_t daysIntoFuture)
{
  size_t dataPtr = 0;     // Pointer into dataBytes
  bool dateFound = false; // Flag to indicate when the date has been found

  // Calculate matchDay, matchMonth and matchYear
  uint8_t matchDay = day;
  uint8_t matchMonth = month;
  uint8_t matchYear = (uint8_t)(year - 2000);

  // Add on daysIntoFuture
  uint8_t daysIntoFutureCopy = daysIntoFuture;
  while (daysIntoFutureCopy > 0)
  {
    matchDay++;
    daysIntoFutureCopy--;
    switch (matchMonth)
    {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      if (matchDay == 32)
      {
        matchDay = 1;
        matchMonth++;
        if (matchMonth == 13)
        {
          matchMonth = 1;
          matchYear++;
        }
      }
      break;
    case 4:
    case 6:
    case 9:
    case 11:
      if (matchDay == 31)
      {
        matchDay = 1;
        matchMonth++;
      }
      break;
    default: // February
      if (((matchYear % 4) == 0) && (matchDay == 30))
      {
        matchDay = 1;
        matchMonth++;
      }
      else if (((matchYear % 4) > 0) && (matchDay == 29))
      {
        matchDay = 1;
        matchMonth++;
      }
      break;
    }
  }

  while ((!dateFound) && (dataPtr < numDataBytes)) // Keep going until we have found the date or processed all the bytes
  {
    // Start by checking the validity of the packet being pointed to
    bool dataIsOK = true;

    dataIsOK &= (*(dataBytes + dataPtr + 0) == UBX_SYNCH_1);   // Check for 0xB5
    dataIsOK &= (*(dataBytes + dataPtr + 1) == UBX_SYNCH_2);   // Check for 0x62
    dataIsOK &= (*(dataBytes + dataPtr + 2) == UBX_CLASS_MGA); // Check for class UBX-MGA

    size_t packetLength = ((size_t) * (dataBytes + dataPtr + 4)) | (((size_t) * (dataBytes + dataPtr + 5)) << 8); // Extract the length

    uint8_t checksumA = 0;
    uint8_t checksumB = 0;
    // Calculate the checksum bytes
    // Keep going until the end of the packet is reached (payloadPtr == (dataPtr + packetLength))
    // or we reach the end of the AssistNow data (payloadPtr == numDataBytes)
    for (size_t payloadPtr = dataPtr + ((size_t)2); (payloadPtr < (dataPtr + packetLength + ((size_t)6))) && (payloadPtr < numDataBytes); payloadPtr++)
    {
      checksumA += *(dataBytes + payloadPtr);
      checksumB += checksumA;
    }
    // Check the checksum bytes
    dataIsOK &= (checksumA == *(dataBytes + dataPtr + packetLength + ((size_t)6)));
    dataIsOK &= (checksumB == *(dataBytes + dataPtr + packetLength + ((size_t)7)));

    dataIsOK &= ((dataPtr + packetLength + ((size_t)8)) <= numDataBytes); // Check we haven't overrun

    // If the data is valid, check for a date match
    if (dataIsOK)
    {
      if ((*(dataBytes + dataPtr + 3) == UBX_MGA_ANO) && (*(dataBytes + dataPtr + 10) == matchYear) && (*(dataBytes + dataPtr + 11) == matchMonth) && (*(dataBytes + dataPtr + 12) == matchDay))
      {
        debugPrint("findMGAANOForDate: found date match at location ", true); // Important
        debugPrintln(dataPtr, true);
        dateFound = true;
      }
      else
      {
        // The data is valid, but these are not the droids we are looking for...
        dataPtr += packetLength + ((size_t)8); // Point to the next message
      }
    }
    else
    {

      // The data was invalid. Send a debug message and then try to find the next 0xB5
      debugPrint("findMGAANOForDate: bad data - ignored! dataPtr is ", true); // Important
      debugPrintln(dataPtr, true);

      while ((dataPtr < numDataBytes) && (*(dataBytes + ++dataPtr) != UBX_SYNCH_1))
      {
        ; // Increment dataPtr until we are pointing at the next 0xB5 - or we reach the end of the data
      }
    }
  }

  return (dataPtr);
}

// Read the whole navigation data base. The receiver will send all available data from its internal database.
// Data is written to dataBytes. Set maxNumDataBytes to the (maximum) size of dataBytes.
// If the database exceeds maxNumDataBytes, the excess bytes will be lost.
// The function returns the number of database bytes written to dataBytes.
// The return value will be equal to maxNumDataBytes if excess data was received.
// The function will timeout after maxWait milliseconds - in case the final UBX-MGA-ACK was missed.
size_t DevUBLOXGNSS::readNavigationDatabase(uint8_t *dataBytes, size_t maxNumDataBytes, uint16_t maxWait)
{
  // Allocate RAM to store the MGA ACK message
  if (packetUBXMGAACK == nullptr)
    initPacketUBXMGAACK();        // Check that RAM has been allocated for the MGA_ACK data
  if (packetUBXMGAACK == nullptr) // Bail if the RAM allocation failed
  {
    debugPrintln("readNavigationDatabase: packetUBXMGAACK RAM allocation failed!"); // Not important
    return ((size_t)0);
  }
  if (packetUBXMGAACK->head != packetUBXMGAACK->tail) // Does the MGA ACK ringbuffer contain any data?
  {
    debugPrintln("readNavigationDatabase: packetUBXMGAACK contains unprocessed data. Clearing it."); // Not important
    packetUBXMGAACK->tail = packetUBXMGAACK->head; // Clear the buffer by setting the tail equal to the head
  }

  // Allocate RAM to store the MGA DBD messages
  if (packetUBXMGADBD == nullptr)
    initPacketUBXMGADBD();        // Check that RAM has been allocated for the MGA_DBD data
  if (packetUBXMGADBD == nullptr) // Bail if the RAM allocation failed
  {
    debugPrintln("readNavigationDatabase: packetUBXMGADBD RAM allocation failed!", true); // Important
    return ((size_t)0);
  }
  if (packetUBXMGADBD->head != packetUBXMGADBD->tail) // Does the MGA DBD ringbuffer contain any data?
  {
    debugPrintln("readNavigationDatabase: packetUBXMGADBD contains unprocessed data. Clearing it."); // Not important
    packetUBXMGADBD->tail = packetUBXMGADBD->head; // Clear the buffer by setting the tail equal to the head
  }

  // Record what ackAiding is currently set to so we can restore it
  uint8_t currentAckAiding = getAckAiding();
  if (currentAckAiding == 255)
    currentAckAiding = 0; // If the get failed, disable the ACKs when returning
  // Enable ackAiding
  setAckAiding(1);

  // Record what i2cPollingWait is currently set to so we can restore it
  uint8_t currentI2cPollingWait = i2cPollingWait;
  // Set the I2C polling wait to 1ms
  i2cPollingWait = 1;

  // Construct the poll message:
  uint8_t pollNaviDatabase[8];       // Create the UBX-MGA-DBD message by hand
  memset(pollNaviDatabase, 0x00, 8); // Set all unused / reserved bytes and the checksum to zero

  pollNaviDatabase[0] = UBX_SYNCH_1;   // Sync char 1
  pollNaviDatabase[1] = UBX_SYNCH_2;   // Sync char 2
  pollNaviDatabase[2] = UBX_CLASS_MGA; // Class
  pollNaviDatabase[3] = UBX_MGA_DBD;   // ID
  pollNaviDatabase[4] = 0x00;          // Length LSB
  pollNaviDatabase[5] = 0x00;          // Length MSB

  for (uint8_t i = 2; i < 6; i++) // Calculate the checksum
  {
    pollNaviDatabase[6] += pollNaviDatabase[i];
    pollNaviDatabase[7] += pollNaviDatabase[6];
  }

  // Push the poll message to the module.
  // Do not Wait for an ACK - the DBD data will start arriving immediately.
  size_t pushResult = pushAssistNowDataInternal(0, false, pollNaviDatabase, (size_t)8, SFE_UBLOX_MGA_ASSIST_ACK_NO, 0);

  // Check pushResult == 8
  if (pushResult != 8)
  {
    debugPrintln("readNavigationDatabase: pushAssistNowDataInternal failed!"); // Not important
    i2cPollingWait = currentI2cPollingWait; // Restore i2cPollingWait
    setAckAiding(currentAckAiding);         // Restore Ack Aiding
    return ((size_t)0);
  }

  // Now keep checking for the arrival of UBX-MGA-DBD packets and write them to dataBytes
  bool keepGoing = true;
  unsigned long startTime = sfe_millis();
  uint32_t databaseEntriesRX = 0; // Keep track of how many database entries are received
  size_t numBytesReceived = 0;    // Keep track of how many bytes are received

  while (keepGoing && ((sfe_millis() - startTime) < maxWait))
  {
    checkUbloxInternal(&packetCfg, 0, 0); // Call checkUbloxInternal to parse any incoming data. Don't overwrite the requested Class and ID. We could be pushing this from another thread...

    while (packetUBXMGADBD->head != packetUBXMGADBD->tail) // Does the MGA DBD ringbuffer contain any data?
    {
      // The data will be valid - process will have already checked it. So we can simply copy the data into dataBuffer.
      // We do not need to check if there is room to store the entire database entry. pushAssistNowData will check the data before pushing it.
      if (numBytesReceived < maxNumDataBytes)
        *(dataBytes + (numBytesReceived++)) = packetUBXMGADBD->data[packetUBXMGADBD->tail].dbdEntryHeader1;
      if (numBytesReceived < maxNumDataBytes)
        *(dataBytes + (numBytesReceived++)) = packetUBXMGADBD->data[packetUBXMGADBD->tail].dbdEntryHeader2;
      if (numBytesReceived < maxNumDataBytes)
        *(dataBytes + (numBytesReceived++)) = packetUBXMGADBD->data[packetUBXMGADBD->tail].dbdEntryClass;
      if (numBytesReceived < maxNumDataBytes)
        *(dataBytes + (numBytesReceived++)) = packetUBXMGADBD->data[packetUBXMGADBD->tail].dbdEntryID;
      if (numBytesReceived < maxNumDataBytes)
        *(dataBytes + (numBytesReceived++)) = packetUBXMGADBD->data[packetUBXMGADBD->tail].dbdEntryLenLSB;
      if (numBytesReceived < maxNumDataBytes)
        *(dataBytes + (numBytesReceived++)) = packetUBXMGADBD->data[packetUBXMGADBD->tail].dbdEntryLenMSB;
      size_t msgLen = (((size_t)packetUBXMGADBD->data[packetUBXMGADBD->tail].dbdEntryLenMSB) * 256) + ((size_t)packetUBXMGADBD->data[packetUBXMGADBD->tail].dbdEntryLenLSB);
      for (size_t i = 0; i < msgLen; i++)
      {
        if (numBytesReceived < maxNumDataBytes)
          *(dataBytes + (numBytesReceived++)) = packetUBXMGADBD->data[packetUBXMGADBD->tail].dbdEntry[i];
      }
      if (numBytesReceived < maxNumDataBytes)
        *(dataBytes + (numBytesReceived++)) = packetUBXMGADBD->data[packetUBXMGADBD->tail].dbdEntryChecksumA;
      if (numBytesReceived < maxNumDataBytes)
        *(dataBytes + (numBytesReceived++)) = packetUBXMGADBD->data[packetUBXMGADBD->tail].dbdEntryChecksumB;

      // Increment the tail
      packetUBXMGADBD->tail++;
      if (packetUBXMGADBD->tail == UBX_MGA_DBD_RINGBUFFER_LEN)
        packetUBXMGADBD->tail = 0;

      databaseEntriesRX++; // Increment the number of entries received
    }

    // The final MGA-ACK is sent at the end of the DBD packets. So, we need to check the ACK buffer _after_ the DBD buffer.
    while (packetUBXMGAACK->head != packetUBXMGAACK->tail) // Does the MGA ACK ringbuffer contain any data?
    {
      // Check if we've received the correct ACK
      bool idMatch = (packetUBXMGAACK->data[packetUBXMGAACK->tail].msgId == UBX_MGA_DBD); // Check if the message ID matches

      bool dataAckd = true;
      dataAckd &= (packetUBXMGAACK->data[packetUBXMGAACK->tail].msgPayloadStart[0] == (uint8_t)(databaseEntriesRX & 0xFF)); // Check if the ACK contents match databaseEntriesRX
      dataAckd &= (packetUBXMGAACK->data[packetUBXMGAACK->tail].msgPayloadStart[1] == (uint8_t)((databaseEntriesRX >> 8) & 0xFF));
      dataAckd &= (packetUBXMGAACK->data[packetUBXMGAACK->tail].msgPayloadStart[2] == (uint8_t)((databaseEntriesRX >> 16) & 0xFF));
      dataAckd &= (packetUBXMGAACK->data[packetUBXMGAACK->tail].msgPayloadStart[3] == (uint8_t)((databaseEntriesRX >> 24) & 0xFF));

      if (idMatch && dataAckd) // Is the ACK valid?
      {
        debugPrint("readNavigationDatabase: ACK received. databaseEntriesRX is ", true); // Important
        debugPrint(databaseEntriesRX, true);
        debugPrint(". numBytesReceived is ", true);
        debugPrint(numBytesReceived, true);
        debugPrint(". DBD read complete after ", true);
        debugPrint(sfe_millis() - startTime, true);
        debugPrintln(" ms", true);
        keepGoing = false;
      }
      else if (idMatch)
      {
        debugPrint("readNavigationDatabase: unexpected ACK received. databaseEntriesRX is 0x", true); // Important
        debugPrint(databaseEntriesRX, HEX, true);
        debugPrint(". msgPayloadStart is 0x", true);
        for (uint8_t i = 4; i > 0; i--)
        {
          if (packetUBXMGAACK->data[packetUBXMGAACK->tail].msgPayloadStart[i - 1] < 0x10)
            debugPrint("0", true);
          debugPrint(packetUBXMGAACK->data[packetUBXMGAACK->tail].msgPayloadStart[i - 1], HEX, true);
        }
        debugPrint("\r\n", true);
      }

      // Increment the tail
      packetUBXMGAACK->tail++;
      if (packetUBXMGAACK->tail == UBX_MGA_ACK_DATA0_RINGBUFFER_LEN)
        packetUBXMGAACK->tail = 0;
    }
  }

  if (keepGoing) // If keepGoing is still true, we must have timed out
  {
    debugPrintln("readNavigationDatabase: DBD RX timed out!", true); // Important
  }

  i2cPollingWait = currentI2cPollingWait; // Restore i2cPollingWait
  setAckAiding(currentAckAiding);         // Restore Ack Aiding

  return (numBytesReceived);
}

// PRIVATE: Allocate RAM for packetUBXMGADBD and initialize it
bool DevUBLOXGNSS::initPacketUBXMGADBD()
{
  packetUBXMGADBD = new UBX_MGA_DBD_t; // Allocate RAM for the main struct
  if (packetUBXMGADBD == nullptr)
  {
    debugPrintln("initPacketUBXMGADBD: RAM alloc failed!", true); // Important
    return (false);
  }
  packetUBXMGADBD->head = 0; // Initialize the ring buffer pointers
  packetUBXMGADBD->tail = 0;
  return (true);
}

// Support for data logging

// Set the file buffer size. This must be called _before_ .begin
void DevUBLOXGNSS::setFileBufferSize(uint16_t bufferSize)
{
  fileBufferSize = bufferSize;
}

// Return the file buffer size
uint16_t DevUBLOXGNSS::getFileBufferSize(void)
{
  return (fileBufferSize);
}

// Extract numBytes of data from the file buffer. Copy it to destination.
// It is the user's responsibility to ensure destination is large enough.
// Returns the number of bytes extracted - which may be less than numBytes.
uint16_t DevUBLOXGNSS::extractFileBufferData(uint8_t *destination, uint16_t numBytes)
{
  // Check how many bytes are available in the buffer
  uint16_t bytesAvailable = fileBufferSpaceUsed();
  if (numBytes > bytesAvailable) // Limit numBytes if required
    numBytes = bytesAvailable;

  // Start copying at fileBufferTail. Wrap-around if required.
  uint16_t bytesBeforeWrapAround = fileBufferSize - fileBufferTail; // How much space is available 'above' Tail?
  if (bytesBeforeWrapAround > numBytes)                             // Will we need to wrap-around?
  {
    bytesBeforeWrapAround = numBytes; // We need to wrap-around
  }
  memcpy(destination, &ubxFileBuffer[fileBufferTail], bytesBeforeWrapAround); // Copy the data out of the buffer

  // Is there any data leftover which we need to copy from the 'bottom' of the buffer?
  uint16_t bytesLeftToCopy = numBytes - bytesBeforeWrapAround; // Calculate if there are any bytes left to copy
  if (bytesLeftToCopy > 0)                                     // If there are bytes left to copy
  {
    memcpy(&destination[bytesBeforeWrapAround], &ubxFileBuffer[0], bytesLeftToCopy); // Copy the remaining data out of the buffer
    fileBufferTail = bytesLeftToCopy;                                                // Update Tail. The next byte to be read will be read from here.
  }
  else
  {
    fileBufferTail += numBytes; // Only update Tail. The next byte to be read will be read from here.
  }

  return (numBytes); // Return the number of bytes extracted
}

// Returns the number of bytes available in file buffer which are waiting to be read
uint16_t DevUBLOXGNSS::fileBufferAvailable(void)
{
  return (fileBufferSpaceUsed());
}

// Returns the maximum number of bytes which the file buffer contained.
// Handy for checking the buffer is large enough to handle all the incoming data.
uint16_t DevUBLOXGNSS::getMaxFileBufferAvail(void)
{
  return (fileBufferMaxAvail);
}

// Clear the file buffer - discard all contents
void DevUBLOXGNSS::clearFileBuffer(void)
{
  if (fileBufferSize == 0) // Bail if the user has not called setFileBufferSize (probably redundant)
    return;
  fileBufferTail = fileBufferHead;
}

// Reset fileBufferMaxAvail
void DevUBLOXGNSS::clearMaxFileBufferAvail(void)
{
  fileBufferMaxAvail = 0;
}

// PRIVATE: Create the file buffer. Called by .begin
bool DevUBLOXGNSS::createFileBuffer(void)
{
  if (fileBufferSize == 0) // Bail if the user has not called setFileBufferSize
  {
    debugPrintln("createFileBuffer: Warning. fileBufferSize is zero. Data logging is not possible."); // Not important
    return (false);
  }

  if (ubxFileBuffer != nullptr) // Bail if RAM has already been allocated for the file buffer
  {                             // This will happen if you call .begin more than once - without calling .end first
    debugPrintln("createFileBuffer: Warning. File buffer already exists. Skipping..."); // Not important
    return (false);
  }

  ubxFileBuffer = new uint8_t[fileBufferSize]; // Allocate RAM for the buffer

  if (ubxFileBuffer == nullptr) // Check if the new (alloc) was successful
  {
    debugPrintln("createFileBuffer: RAM alloc failed!", true); // Important
    fileBufferSize = 0; // Set file buffer size so user can check with getFileBufferSize (ubxFileBuffer is protected)
    return (false);
  }

  debugPrint("createFileBuffer: fileBufferSize is: ");
  debugPrintln(fileBufferSize);

  fileBufferHead = 0; // Initialize head and tail
  fileBufferTail = 0;

  return (true);
}

// PRIVATE: Check how much space is available in the buffer
uint16_t DevUBLOXGNSS::fileBufferSpaceAvailable(void)
{
  return (fileBufferSize - fileBufferSpaceUsed());
}

// PRIVATE: Check how much space is used in the buffer
uint16_t DevUBLOXGNSS::fileBufferSpaceUsed(void)
{
  if (fileBufferHead >= fileBufferTail) // Check if wrap-around has occurred
  {
    // Wrap-around has not occurred so do a simple subtraction
    return (fileBufferHead - fileBufferTail);
  }
  else
  {
    // Wrap-around has occurred so do a simple subtraction but add in the fileBufferSize
    return ((uint16_t)(((uint32_t)fileBufferHead + (uint32_t)fileBufferSize) - (uint32_t)fileBufferTail));
  }
}

// PRIVATE: Add a UBX packet to the file buffer
bool DevUBLOXGNSS::storePacket(ubxPacket *msg)
{
  // First, check that the file buffer has been created
  if ((ubxFileBuffer == nullptr) || (fileBufferSize == 0))
  {
    debugPrintln("storePacket: file buffer not available!"); // Not important
    return (false);
  }

  // Now, check if there is enough space in the buffer for all of the data
  uint16_t totalLength = msg->len + 8; // Total length. Include sync chars, class, id, length and checksum bytes
  if (totalLength > fileBufferSpaceAvailable())
  {
    debugPrintln("storePacket: insufficient space available! Data will be lost!", true); // Important
    return (false);
  }

  // Store the two sync chars
  uint8_t sync_chars[] = {UBX_SYNCH_1, UBX_SYNCH_2};
  writeToFileBuffer(sync_chars, 2);

  // Store the Class & ID
  writeToFileBuffer(&msg->cls, 1);
  writeToFileBuffer(&msg->id, 1);

  // Store the length. Ensure length is little-endian
  uint8_t msg_length[2];
  msg_length[0] = msg->len & 0xFF;
  msg_length[1] = msg->len >> 8;
  writeToFileBuffer(msg_length, 2);

  // Store the payload
  writeToFileBuffer(msg->payload, msg->len);

  // Store the checksum
  writeToFileBuffer(&msg->checksumA, 1);
  writeToFileBuffer(&msg->checksumB, 1);

  return (true);
}

// PRIVATE: Add theBytes to the file buffer
bool DevUBLOXGNSS::storeFileBytes(uint8_t *theBytes, uint16_t numBytes)
{
  // First, check that the file buffer has been created
  if ((ubxFileBuffer == nullptr) || (fileBufferSize == 0))
  {
    debugPrintln("storeFileBytes: file buffer not available!"); // Not important
    return (false);
  }

  // Now, check if there is enough space in the buffer for all of the data
  if (numBytes > fileBufferSpaceAvailable())
  {
    debugPrintln("storeFileBytes: insufficient space available! Data will be lost!", true); // Important
    return (false);
  }

  // There is room for all the data in the buffer so copy the data into the buffer
  writeToFileBuffer(theBytes, numBytes);

  return (true);
}

// PRIVATE: Write theBytes to the file buffer
void DevUBLOXGNSS::writeToFileBuffer(uint8_t *theBytes, uint16_t numBytes)
{
  // Start writing at fileBufferHead. Wrap-around if required.
  uint16_t bytesBeforeWrapAround = fileBufferSize - fileBufferHead; // How much space is available 'above' Head?
  if (bytesBeforeWrapAround > numBytes)                             // Is there enough room for all the data?
  {
    bytesBeforeWrapAround = numBytes; // There is enough room for all the data
  }
  memcpy(&ubxFileBuffer[fileBufferHead], theBytes, bytesBeforeWrapAround); // Copy the data into the buffer

  // Is there any data leftover which we need to copy to the 'bottom' of the buffer?
  uint16_t bytesLeftToCopy = numBytes - bytesBeforeWrapAround; // Calculate if there are any bytes left to copy
  if (bytesLeftToCopy > 0)                                     // If there are bytes left to copy
  {
    memcpy(&ubxFileBuffer[0], &theBytes[bytesBeforeWrapAround], bytesLeftToCopy); // Copy the remaining data into the buffer
    fileBufferHead = bytesLeftToCopy;                                             // Update Head. The next byte written will be written here.
  }
  else
  {
    fileBufferHead += numBytes; // Only update Head. The next byte written will be written here.
  }

  // Update fileBufferMaxAvail if required
  uint16_t bytesInBuffer = fileBufferSpaceUsed();
  if (bytesInBuffer > fileBufferMaxAvail)
    fileBufferMaxAvail = bytesInBuffer;
}

// Support for RTCM buffering

// Set the RTCM buffer size. This must be called _before_ .begin
void DevUBLOXGNSS::setRTCMBufferSize(uint16_t bufferSize)
{
  rtcmBufferSize = bufferSize;
}

// Return the RTCM buffer size
uint16_t DevUBLOXGNSS::getRTCMBufferSize(void)
{
  return (rtcmBufferSize);
}

// Extract numBytes of data from the RTCM buffer. Copy it to destination.
// It is the user's responsibility to ensure destination is large enough.
// Returns the number of bytes extracted - which may be less than numBytes.
uint16_t DevUBLOXGNSS::extractRTCMBufferData(uint8_t *destination, uint16_t numBytes)
{
  // Check how many bytes are available in the buffer
  uint16_t bytesAvailable = rtcmBufferSpaceUsed();
  if (numBytes > bytesAvailable) // Limit numBytes if required
    numBytes = bytesAvailable;

  // Start copying at rtcmBufferTail. Wrap-around if required.
  uint16_t bytesBeforeWrapAround = rtcmBufferSize - rtcmBufferTail; // How much space is available 'above' Tail?
  if (bytesBeforeWrapAround > numBytes)                             // Will we need to wrap-around?
  {
    bytesBeforeWrapAround = numBytes; // We need to wrap-around
  }
  memcpy(destination, &rtcmBuffer[rtcmBufferTail], bytesBeforeWrapAround); // Copy the data out of the buffer

  // Is there any data leftover which we need to copy from the 'bottom' of the buffer?
  uint16_t bytesLeftToCopy = numBytes - bytesBeforeWrapAround; // Calculate if there are any bytes left to copy
  if (bytesLeftToCopy > 0)                                     // If there are bytes left to copy
  {
    memcpy(&destination[bytesBeforeWrapAround], &rtcmBuffer[0], bytesLeftToCopy); // Copy the remaining data out of the buffer
    rtcmBufferTail = bytesLeftToCopy;                                             // Update Tail. The next byte to be read will be read from here.
  }
  else
  {
    rtcmBufferTail += numBytes; // Only update Tail. The next byte to be read will be read from here.
  }

  return (numBytes); // Return the number of bytes extracted
}

// Returns the number of bytes available in RTCM buffer which are waiting to be read
uint16_t DevUBLOXGNSS::rtcmBufferAvailable(void)
{
  return (rtcmBufferSpaceUsed());
}

// Clear the RTCM buffer - discard all contents
void DevUBLOXGNSS::clearRTCMBuffer(void)
{
  if (rtcmBufferSize == 0) // Bail if the user has not called setRTCMBufferSize (probably redundant)
    return;
  rtcmBufferTail = rtcmBufferHead;
}

// PRIVATE: Create the RTCM buffer. Called by .begin
bool DevUBLOXGNSS::createRTCMBuffer(void)
{
  if (rtcmBufferSize == 0) // Bail if the user has not called setRTCMBufferSize
  {
    return (false);
  }

  if (rtcmBuffer != nullptr) // Bail if RAM has already been allocated for the buffer
  {                          // This will happen if you call .begin more than once - without calling .end first
    return (false);
  }

  rtcmBuffer = new uint8_t[rtcmBufferSize]; // Allocate RAM for the buffer

  if (rtcmBuffer == nullptr) // Check if the new (alloc) was successful
  {
    debugPrintln("createRTCMBuffer: RAM alloc failed!", true); // Important
    rtcmBufferSize = 0; // Set buffer size so user can check with getRTCMBufferSize (rtcmBuffer is protected)
    return (false);
  }

  rtcmBufferHead = 0; // Initialize head and tail
  rtcmBufferTail = 0;

  return (true);
}

// PRIVATE: Check how much space is available in the buffer
uint16_t DevUBLOXGNSS::rtcmBufferSpaceAvailable(void)
{
  return (rtcmBufferSize - rtcmBufferSpaceUsed());
}

// PRIVATE: Check how much space is used in the buffer
uint16_t DevUBLOXGNSS::rtcmBufferSpaceUsed(void)
{
  if (rtcmBufferHead >= rtcmBufferTail) // Check if wrap-around has occurred
  {
    // Wrap-around has not occurred so do a simple subtraction
    return (rtcmBufferHead - rtcmBufferTail);
  }
  else
  {
    // Wrap-around has occurred so do a simple subtraction but add in the rtcmBufferSize
    return ((uint16_t)(((uint32_t)rtcmBufferHead + (uint32_t)rtcmBufferSize) - (uint32_t)rtcmBufferTail));
  }
}

// PRIVATE: Add theBytes to the RTCM buffer
bool DevUBLOXGNSS::storeRTCMBytes(uint8_t *theBytes, uint16_t numBytes)
{
  // First, check that the file buffer has been created
  if ((rtcmBuffer == nullptr) || (rtcmBufferSize == 0))
  {
    return (false);
  }

  // Now, check if there is enough space in the buffer for all of the data
  if (numBytes > rtcmBufferSpaceAvailable())
  {
    return (false);
  }

  // There is room for all the data in the buffer so copy the data into the buffer
  writeToRTCMBuffer(theBytes, numBytes);

  return (true);
}

// PRIVATE: Write theBytes to the RTCM buffer
void DevUBLOXGNSS::writeToRTCMBuffer(uint8_t *theBytes, uint16_t numBytes)
{
  // Start writing at fileBufferHead. Wrap-around if required.
  uint16_t bytesBeforeWrapAround = rtcmBufferSize - rtcmBufferHead; // How much space is available 'above' Head?
  if (bytesBeforeWrapAround > numBytes)                             // Is there enough room for all the data?
  {
    bytesBeforeWrapAround = numBytes; // There is enough room for all the data
  }
  memcpy(&rtcmBuffer[rtcmBufferHead], theBytes, bytesBeforeWrapAround); // Copy the data into the buffer

  // Is there any data leftover which we need to copy to the 'bottom' of the buffer?
  uint16_t bytesLeftToCopy = numBytes - bytesBeforeWrapAround; // Calculate if there are any bytes left to copy
  if (bytesLeftToCopy > 0)                                     // If there are bytes left to copy
  {
    memcpy(&rtcmBuffer[0], &theBytes[bytesBeforeWrapAround], bytesLeftToCopy); // Copy the remaining data into the buffer
    rtcmBufferHead = bytesLeftToCopy;                                          // Update Head. The next byte written will be written here.
  }
  else
  {
    rtcmBufferHead += numBytes; // Only update Head. The next byte written will be written here.
  }
}

void DevUBLOXGNSS::extractRTCM1005(RTCM_1005_data_t *destination, uint8_t *source)
{
  destination->MessageNumber = extractUnsignedBits(source, 0, 12);
  destination->ReferenceStationID = extractUnsignedBits(source, 12, 12);
  destination->ITRFRealizationYear = extractUnsignedBits(source, 24, 6);
  destination->GPSIndicator = extractUnsignedBits(source, 30, 1);
  destination->GLONASSIndicator = extractUnsignedBits(source, 31, 1);
  destination->GalileoIndicator = extractUnsignedBits(source, 32, 1);
  destination->ReferenceStationIndicator = extractUnsignedBits(source, 33, 1);
  destination->AntennaReferencePointECEFX = extractSignedBits(source, 34, 38);
  destination->SingleReceiverOscillatorIndicator = extractUnsignedBits(source, 72, 1);
  destination->Reserved = extractUnsignedBits(source, 73, 1);
  destination->AntennaReferencePointECEFY = extractSignedBits(source, 74, 38);
  destination->QuarterCycleIndicator = extractUnsignedBits(source, 112, 2);
  destination->AntennaReferencePointECEFZ = extractSignedBits(source, 114, 38);
}

void DevUBLOXGNSS::extractRTCM1006(RTCM_1006_data_t *destination, uint8_t *source)
{
  destination->MessageNumber = extractUnsignedBits(source, 0, 12);
  destination->ReferenceStationID = extractUnsignedBits(source, 12, 12);
  destination->ITRFRealizationYear = extractUnsignedBits(source, 24, 6);
  destination->GPSIndicator = extractUnsignedBits(source, 30, 1);
  destination->GLONASSIndicator = extractUnsignedBits(source, 31, 1);
  destination->GalileoIndicator = extractUnsignedBits(source, 32, 1);
  destination->ReferenceStationIndicator = extractUnsignedBits(source, 33, 1);
  destination->AntennaReferencePointECEFX = extractSignedBits(source, 34, 38);
  destination->SingleReceiverOscillatorIndicator = extractUnsignedBits(source, 72, 1);
  destination->Reserved = extractUnsignedBits(source, 73, 1);
  destination->AntennaReferencePointECEFY = extractSignedBits(source, 74, 38);
  destination->QuarterCycleIndicator = extractUnsignedBits(source, 112, 2);
  destination->AntennaReferencePointECEFZ = extractSignedBits(source, 114, 38);
  destination->AntennaHeight = extractUnsignedBits(source, 152, 16);
}

void DevUBLOXGNSS::parseRTCM1005(uint8_t *dataBytes, size_t numDataBytes)
{
  // This is called from inside pushRawData. It thoroughly examines dataBytes and will copy any RTCM 1005 messages it finds into storage.
  // It keeps a local copy of the data so it does not matter if the message spans multiple calls to pushRawData.

  static uint8_t rtcm1005store[RTCM_1005_MSG_LEN_BYTES + 6];
  static uint8_t bytesStored;

  enum parse1005states
  {
    waitingForD3,
    expecting00,
    expecting13,
    expecting3E,
    expectingDn,
    storingBytes,
  };
  static parse1005states parse1005state = waitingForD3;

  for (size_t i = 0; i < numDataBytes; i++) // Step through each byte
  {
    switch (parse1005state)
    {
    case waitingForD3:
      if (*(dataBytes + i) == 0xD3)
      {
        rtcm1005store[0] = 0xD3;
        parse1005state = expecting00;
      }
      break;
    case expecting00:
      if (*(dataBytes + i) == 0x00)
      {
        rtcm1005store[1] = 0x00;
        parse1005state = expecting13;
      }
      else
      {
        parse1005state = waitingForD3;
      }
      break;
    case expecting13:
      if (*(dataBytes + i) == 0x13)
      {
        rtcm1005store[2] = 0x13;
        parse1005state = expecting3E;
      }
      else
      {
        parse1005state = waitingForD3;
      }
      break;
    case expecting3E:
      if (*(dataBytes + i) == 0x3E)
      {
        rtcm1005store[3] = 0x3E;
        parse1005state = expectingDn;
      }
      else
      {
        parse1005state = waitingForD3;
      }
      break;
    case expectingDn:
      if (((*(dataBytes + i)) & 0xF0) == 0xD0)
      {
        rtcm1005store[4] = *(dataBytes + i);
        parse1005state = storingBytes;
        bytesStored = 5;
      }
      else
      {
        parse1005state = waitingForD3;
      }
      break;
    case storingBytes:
      rtcm1005store[bytesStored++] = *(dataBytes + i);
      if (bytesStored == RTCM_1005_MSG_LEN_BYTES + 6) // All data received?
      {
        parse1005state = waitingForD3;
        uint32_t checksum = 0;
        for (size_t j = 0; j < (RTCM_1005_MSG_LEN_BYTES + 3); j++)
          crc24q(rtcm1005store[j], &checksum);
        if (rtcm1005store[RTCM_1005_MSG_LEN_BYTES + 3] == ((checksum >> 16) & 0xFF)) // Check the checksum
          if (rtcm1005store[RTCM_1005_MSG_LEN_BYTES + 4] == ((checksum >> 8) & 0xFF))
            if (rtcm1005store[RTCM_1005_MSG_LEN_BYTES + 5] == (checksum & 0xFF))
            {
              extractRTCM1005(&rtcmInputStorage.rtcm1005, &rtcm1005store[3]);
              rtcmInputStorage.flags.bits.dataValid1005 = 1;
              rtcmInputStorage.flags.bits.dataRead1005 = 0;
              return; // Return now - to avoid processing the remainder of the data
            }
      }
      break;
    }
  }
}

void DevUBLOXGNSS::parseRTCM1006(uint8_t *dataBytes, size_t numDataBytes)
{
  // This is called from inside pushRawData. It thoroughly examines dataBytes and will copy any RTCM 1006 messages it finds into storage.
  // It keeps a local copy of the data so it does not matter if the message spans multiple calls to pushRawData.

  static uint8_t rtcm1006store[RTCM_1006_MSG_LEN_BYTES + 6];
  static uint8_t bytesStored;

  enum parse1006states
  {
    waitingForD3,
    expecting00,
    expecting15,
    expecting3E,
    expectingEn,
    storingBytes,
  };
  static parse1006states parse1006state = waitingForD3;

  for (size_t i = 0; i < numDataBytes; i++) // Step through each byte
  {
    switch (parse1006state)
    {
    case waitingForD3:
      if (*(dataBytes + i) == 0xD3)
      {
        rtcm1006store[0] = 0xD3;
        parse1006state = expecting00;
      }
      break;
    case expecting00:
      if (*(dataBytes + i) == 0x00)
      {
        rtcm1006store[1] = 0x00;
        parse1006state = expecting15;
      }
      else
      {
        parse1006state = waitingForD3;
      }
      break;
    case expecting15:
      if (*(dataBytes + i) == 0x15)
      {
        rtcm1006store[2] = 0x15;
        parse1006state = expecting3E;
      }
      else
      {
        parse1006state = waitingForD3;
      }
      break;
    case expecting3E:
      if (*(dataBytes + i) == 0x3E)
      {
        rtcm1006store[3] = 0x3E;
        parse1006state = expectingEn;
      }
      else
      {
        parse1006state = waitingForD3;
      }
      break;
    case expectingEn:
      if (((*(dataBytes + i)) & 0xF0) == 0xE0)
      {
        rtcm1006store[4] = *(dataBytes + i);
        parse1006state = storingBytes;
        bytesStored = 5;
      }
      else
      {
        parse1006state = waitingForD3;
      }
      break;
    case storingBytes:
      rtcm1006store[bytesStored++] = *(dataBytes + i);
      if (bytesStored == RTCM_1006_MSG_LEN_BYTES + 6) // All data received?
      {
        parse1006state = waitingForD3;
        uint32_t checksum = 0;
        for (size_t j = 0; j < (RTCM_1006_MSG_LEN_BYTES + 3); j++)
          crc24q(rtcm1006store[j], &checksum);

        if (rtcm1006store[RTCM_1006_MSG_LEN_BYTES + 3] == ((checksum >> 16) & 0xFF)) // Check the checksum
          if (rtcm1006store[RTCM_1006_MSG_LEN_BYTES + 4] == ((checksum >> 8) & 0xFF))
            if (rtcm1006store[RTCM_1006_MSG_LEN_BYTES + 5] == (checksum & 0xFF))
            {
              extractRTCM1006(&rtcmInputStorage.rtcm1006, &rtcm1006store[3]);
              rtcmInputStorage.flags.bits.dataValid1006 = 1;
              rtcmInputStorage.flags.bits.dataRead1006 = 0;
              return; // Return now - to avoid processing the remainder of the data
            }
      }
      break;
    }
  }
}

//=-=-=-=-=-=-=-= Specific commands =-=-=-=-=-=-=-==-=-=-=-=-=-=-=
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Changes the I2C address that the u-blox module responds to
// 0x42 is the default but can be changed with this command
// Note: the module stores the address in shifted format - not unshifted.
// We need to shift left by one bit to compensate.
bool DevUBLOXGNSS::setI2CAddress(uint8_t deviceAddress, uint8_t layer, uint16_t maxWait)
{
  return setVal8(UBLOX_CFG_I2C_ADDRESS, deviceAddress << 1, layer, maxWait); // Change the I2C address. Shift left by one bit.
}

// Changes the serial baud rate of the u-blox module, can't return success/fail 'cause ACK from modem
// is lost due to baud rate change
bool DevUBLOXGNSS::setSerialRate(uint32_t baudrate, uint8_t uartPort, uint8_t layer, uint16_t maxWait)
{
  if (uartPort == COM_PORT_UART1)
    return setVal32(UBLOX_CFG_UART1_BAUDRATE, baudrate, layer, maxWait);
  else if (uartPort == COM_PORT_UART2)
    return setVal32(UBLOX_CFG_UART2_BAUDRATE, baudrate, layer, maxWait);
  else
    return false;
}

// Configure a port to output UBX, NMEA, RTCM3 or a combination thereof
bool DevUBLOXGNSS::setI2COutput(uint8_t comSettings, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_I2COUTPROT_UBX, (comSettings & COM_TYPE_UBX) == 0 ? 0 : 1);
  result &= addCfgValset(UBLOX_CFG_I2COUTPROT_NMEA, (comSettings & COM_TYPE_NMEA) == 0 ? 0 : 1);
  result &= sendCfgValset(maxWait);
  result |= setVal8(UBLOX_CFG_I2COUTPROT_RTCM3X, (comSettings & COM_TYPE_RTCM3) == 0 ? 0 : 1, layer, maxWait); // This will be NACK'd if the module does not support RTCM3
  return result;
}
bool DevUBLOXGNSS::setUART1Output(uint8_t comSettings, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_UART1OUTPROT_UBX, (comSettings & COM_TYPE_UBX) == 0 ? 0 : 1);
  result &= addCfgValset(UBLOX_CFG_UART1OUTPROT_NMEA, (comSettings & COM_TYPE_NMEA) == 0 ? 0 : 1);
  result &= sendCfgValset(maxWait);
  result |= setVal8(UBLOX_CFG_UART1OUTPROT_RTCM3X, (comSettings & COM_TYPE_RTCM3) == 0 ? 0 : 1, layer, maxWait); // This will be NACK'd if the module does not support RTCM3
  return result;
}
bool DevUBLOXGNSS::setUART2Output(uint8_t comSettings, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_UART2OUTPROT_UBX, (comSettings & COM_TYPE_UBX) == 0 ? 0 : 1);
  result &= addCfgValset(UBLOX_CFG_UART2OUTPROT_NMEA, (comSettings & COM_TYPE_NMEA) == 0 ? 0 : 1);
  result &= sendCfgValset(maxWait);
  result |= setVal8(UBLOX_CFG_UART2OUTPROT_RTCM3X, (comSettings & COM_TYPE_RTCM3) == 0 ? 0 : 1, layer, maxWait); // This will be NACK'd if the module does not support RTCM3
  return result;
}
bool DevUBLOXGNSS::setUSBOutput(uint8_t comSettings, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_USBOUTPROT_UBX, (comSettings & COM_TYPE_UBX) == 0 ? 0 : 1);
  result &= addCfgValset(UBLOX_CFG_USBOUTPROT_NMEA, (comSettings & COM_TYPE_NMEA) == 0 ? 0 : 1);
  result &= sendCfgValset(maxWait);
  result |= setVal8(UBLOX_CFG_USBOUTPROT_RTCM3X, (comSettings & COM_TYPE_RTCM3) == 0 ? 0 : 1, layer, maxWait); // This will be NACK'd if the module does not support RTCM3
  return result;
}
bool DevUBLOXGNSS::setSPIOutput(uint8_t comSettings, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_SPIOUTPROT_UBX, (comSettings & COM_TYPE_UBX) == 0 ? 0 : 1);
  result &= addCfgValset(UBLOX_CFG_SPIOUTPROT_NMEA, (comSettings & COM_TYPE_NMEA) == 0 ? 0 : 1);
  result &= sendCfgValset(maxWait);
  result |= setVal8(UBLOX_CFG_SPIOUTPROT_RTCM3X, (comSettings & COM_TYPE_RTCM3) == 0 ? 0 : 1, layer, maxWait); // This will be NACK'd if the module does not support RTCM3
  return result;
}

// Configure a port to input UBX, NMEA, RTCM3, SPARTN or a combination thereof
bool DevUBLOXGNSS::setI2CInput(uint8_t comSettings, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_I2CINPROT_UBX, (comSettings & COM_TYPE_UBX) == 0 ? 0 : 1);
  result &= addCfgValset(UBLOX_CFG_I2CINPROT_NMEA, (comSettings & COM_TYPE_NMEA) == 0 ? 0 : 1);
  result &= sendCfgValset(maxWait);
  result |= setVal8(UBLOX_CFG_I2CINPROT_RTCM3X, (comSettings & COM_TYPE_RTCM3) == 0 ? 0 : 1, layer, maxWait);  // This will be NACK'd if the module does not support RTCM3
  result |= setVal8(UBLOX_CFG_I2CINPROT_SPARTN, (comSettings & COM_TYPE_SPARTN) == 0 ? 0 : 1, layer, maxWait); // This will be NACK'd if the module does not support SPARTN
  return result;
}
bool DevUBLOXGNSS::setUART1Input(uint8_t comSettings, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_UART1INPROT_UBX, (comSettings & COM_TYPE_UBX) == 0 ? 0 : 1);
  result &= addCfgValset(UBLOX_CFG_UART1INPROT_NMEA, (comSettings & COM_TYPE_NMEA) == 0 ? 0 : 1);
  result &= sendCfgValset(maxWait);
  result |= setVal8(UBLOX_CFG_UART1INPROT_RTCM3X, (comSettings & COM_TYPE_RTCM3) == 0 ? 0 : 1, layer, maxWait);  // This will be NACK'd if the module does not support RTCM3
  result |= setVal8(UBLOX_CFG_UART1INPROT_SPARTN, (comSettings & COM_TYPE_SPARTN) == 0 ? 0 : 1, layer, maxWait); // This will be NACK'd if the module does not support SPARTN
  return result;
}
bool DevUBLOXGNSS::setUART2Input(uint8_t comSettings, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_UART2INPROT_UBX, (comSettings & COM_TYPE_UBX) == 0 ? 0 : 1);
  result &= addCfgValset(UBLOX_CFG_UART2INPROT_NMEA, (comSettings & COM_TYPE_NMEA) == 0 ? 0 : 1);
  result &= sendCfgValset(maxWait);
  result |= setVal8(UBLOX_CFG_UART2INPROT_RTCM3X, (comSettings & COM_TYPE_RTCM3) == 0 ? 0 : 1, layer, maxWait);  // This will be NACK'd if the module does not support RTCM3
  result |= setVal8(UBLOX_CFG_UART2INPROT_SPARTN, (comSettings & COM_TYPE_SPARTN) == 0 ? 0 : 1, layer, maxWait); // This will be NACK'd if the module does not support SPARTN
  return result;
}
bool DevUBLOXGNSS::setUSBInput(uint8_t comSettings, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_USBINPROT_UBX, (comSettings & COM_TYPE_UBX) == 0 ? 0 : 1);
  result &= addCfgValset(UBLOX_CFG_USBINPROT_NMEA, (comSettings & COM_TYPE_NMEA) == 0 ? 0 : 1);
  result &= sendCfgValset(maxWait);
  result |= setVal8(UBLOX_CFG_USBINPROT_RTCM3X, (comSettings & COM_TYPE_RTCM3) == 0 ? 0 : 1, layer, maxWait);  // This will be NACK'd if the module does not support RTCM3
  result |= setVal8(UBLOX_CFG_USBINPROT_SPARTN, (comSettings & COM_TYPE_SPARTN) == 0 ? 0 : 1, layer, maxWait); // This will be NACK'd if the module does not support SPARTN
  return result;
}
bool DevUBLOXGNSS::setSPIInput(uint8_t comSettings, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_SPIINPROT_UBX, (comSettings & COM_TYPE_UBX) == 0 ? 0 : 1);
  result &= addCfgValset(UBLOX_CFG_SPIINPROT_NMEA, (comSettings & COM_TYPE_NMEA) == 0 ? 0 : 1);
  result &= sendCfgValset(maxWait);
  result |= setVal8(UBLOX_CFG_SPIINPROT_RTCM3X, (comSettings & COM_TYPE_RTCM3) == 0 ? 0 : 1, layer, maxWait);  // This will be NACK'd if the module does not support RTCM3
  result |= setVal8(UBLOX_CFG_SPIINPROT_SPARTN, (comSettings & COM_TYPE_SPARTN) == 0 ? 0 : 1, layer, maxWait); // This will be NACK'd if the module does not support SPARTN
  return result;
}

// Want to see the NMEA messages on the Serial port? Here's how
void DevUBLOXGNSS::setNMEAOutputPort(sfe_print_t &outputPort)
{
  _nmeaOutputPort.init(outputPort); // Store the port from user
}

// Want to see the RTCM messages on the Serial port? Here's how
void DevUBLOXGNSS::setRTCMOutputPort(sfe_print_t &outputPort)
{
  _rtcmOutputPort.init(outputPort); // Store the port from user
}

// Want to see the UBX messages on the Serial port? Here's how
void DevUBLOXGNSS::setUBXOutputPort(sfe_print_t &outputPort)
{
  _ubxOutputPort.init(outputPort); // Store the port from user
}

void DevUBLOXGNSS::setOutputPort(sfe_print_t &outputPort)
{
  _outputPort.init(outputPort); // Store the port from user
}

// Reset to defaults

void DevUBLOXGNSS::factoryReset()
{
  // Copy default settings to permanent
  // Note: this does not load the permanent configuration into the current configuration. Calling factoryDefault() will do that.
  uint8_t clearMemory[13] = {0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0xff};
  cfgCfg(clearMemory, 13, 0);
  hardReset(); // cause factory default config to actually be loaded and used cleanly
}

void DevUBLOXGNSS::hardReset()
{
  // Issue hard reset
  uint8_t softwareResetGNSS[4] = {0xff, 0xff, 0, 0};
  cfgRst(softwareResetGNSS, 4);
}

void DevUBLOXGNSS::softwareResetGNSSOnly()
{
  // Issue controlled software reset (GNSS only)
  uint8_t softwareResetGNSS[4] = {0, 0, 0x02, 0};
  cfgRst(softwareResetGNSS, 4);
}

void DevUBLOXGNSS::softwareEnableGNSS(bool enable)
{
  // Issue controlled software reset (GNSS only)
  uint8_t softwareEnable[4] = {0, 0, 0, 0};
  softwareEnable[2] = enable ? 0x09 : 0x08; // 0x09 = start GNSS, 0x08 = stop GNSS
  cfgRst(softwareEnable, 4);
}

void DevUBLOXGNSS::cfgRst(uint8_t *data, uint8_t len)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_RST;
  packetCfg.len = len;
  packetCfg.startingSpot = 0;
  for (uint8_t i = 0; i < len; i++)
    payloadCfg[i] = *data++;
  sendCommand(&packetCfg, 0); // don't expect ACK
}

// Reset module to factory defaults
// This still works but it is the old way of configuring ublox modules. See getVal and setVal for the new methods
bool DevUBLOXGNSS::factoryDefault(uint16_t maxWait)
{
  uint8_t configSelective[12];

  // Clear packet payload
  memset(configSelective, 0, 12);

  configSelective[0] = 0xFF; // Set any bit in the clearMask field to clear saved config
  configSelective[1] = 0xFF;
  configSelective[8] = 0xFF; // Set any bit in the loadMask field to discard current config and rebuild from lower non-volatile memory layers
  configSelective[9] = 0xFF;

  return (cfgCfg(configSelective, 12, maxWait));
}

// Save configuration to BBR / Flash

// Save current configuration to flash and BBR (battery backed RAM)
// This still works but it is the old way of configuring ublox modules. See getVal and setVal for the new methods
bool DevUBLOXGNSS::saveConfiguration(uint16_t maxWait)
{
  uint8_t configSelective[12];

  // Clear packet payload
  memset(configSelective, 0, 12);

  configSelective[4] = 0xFF; // Set any bit in the saveMask field to save current config to Flash and BBR
  configSelective[5] = 0xFF;

  return (cfgCfg(configSelective, 12, maxWait));
}

// Save the selected configuration sub-sections to flash and BBR (battery backed RAM)
// This still works but it is the old way of configuring ublox modules. See getVal and setVal for the new methods
bool DevUBLOXGNSS::saveConfigSelective(uint32_t configMask, uint16_t maxWait)
{
  uint8_t configSelective[12];

  // Clear packet payload
  memset(configSelective, 0, 12);

  configSelective[4] = configMask & 0xFF; // Set the appropriate bits in the saveMask field to save current config to Flash and BBR
  configSelective[5] = (configMask >> 8) & 0xFF;
  configSelective[6] = (configMask >> 16) & 0xFF;
  configSelective[7] = (configMask >> 24) & 0xFF;

  return (cfgCfg(configSelective, 12, maxWait));
}

bool DevUBLOXGNSS::cfgCfg(uint8_t *data, uint8_t len, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_CFG;
  packetCfg.len = len;
  packetCfg.startingSpot = 0;
  for (uint8_t i = 0; i < len; i++)
    payloadCfg[i] = *data++;
  return (sendCommand(&packetCfg, maxWait) == SFE_UBLOX_STATUS_DATA_SENT); // We are only expecting an ACK
}

// Functions used for RTK and base station setup

// Control Survey-In for NEO-M8P
bool DevUBLOXGNSS::setSurveyMode(uint8_t mode, uint16_t observationTime, float requiredAccuracy, uint8_t layer, uint16_t maxWait)
{
  return (setSurveyModeFull(mode, (uint32_t)observationTime, requiredAccuracy, layer, maxWait));
}
bool DevUBLOXGNSS::setSurveyModeFull(uint8_t mode, uint32_t observationTime, float requiredAccuracy, uint8_t layer, uint16_t maxWait)
{
  uint32_t svinAccLimit = (uint32_t)(requiredAccuracy * 10000.0); // Convert m to 0.1mm

  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_TMODE_MODE, mode);
  result &= addCfgValset(UBLOX_CFG_TMODE_SVIN_MIN_DUR, observationTime);
  result &= addCfgValset(UBLOX_CFG_TMODE_SVIN_ACC_LIMIT, svinAccLimit);
  result &= sendCfgValset(maxWait);

  return result;
}

// Begin Survey-In for NEO-M8P
bool DevUBLOXGNSS::enableSurveyMode(uint16_t observationTime, float requiredAccuracy, uint8_t layer, uint16_t maxWait)
{
  return (setSurveyModeFull(SVIN_MODE_ENABLE, (uint32_t)observationTime, requiredAccuracy, layer, maxWait));
}
bool DevUBLOXGNSS::enableSurveyModeFull(uint32_t observationTime, float requiredAccuracy, uint8_t layer, uint16_t maxWait)
{
  return (setSurveyModeFull(SVIN_MODE_ENABLE, observationTime, requiredAccuracy, layer, maxWait));
}

// Stop Survey-In for NEO-M8P
bool DevUBLOXGNSS::disableSurveyMode(uint8_t layer, uint16_t maxWait)
{
  return (setSurveyMode(SVIN_MODE_DISABLE, 0, 0, layer, maxWait));
}

// Set the ECEF or Lat/Long coordinates of a receiver
// This imediately puts the receiver in TIME mode (fixed) and will begin outputting RTCM sentences if enabled
// This is helpful once an antenna's position has been established. See this tutorial: https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station#gather-raw-gnss-data
//  For ECEF the units are: cm, 0.1mm, cm, 0.1mm, cm, 0.1mm
//  For Lat/Lon/Alt the units are: degrees^-7, degrees^-9, degrees^-7, degrees^-9, cm, 0.1mm
bool DevUBLOXGNSS::setStaticPosition(int32_t ecefXOrLat, int8_t ecefXOrLatHP, int32_t ecefYOrLon, int8_t ecefYOrLonHP, int32_t ecefZOrAlt, int8_t ecefZOrAltHP, bool latLong, uint8_t layer, uint16_t maxWait)
{
  unsignedSigned32 converter32;
  unsignedSigned8 converter8;
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_TMODE_MODE, SVIN_MODE_FIXED);
  result &= addCfgValset(UBLOX_CFG_TMODE_POS_TYPE, (uint8_t)latLong);
  converter32.signed32 = ecefXOrLat;
  result &= addCfgValset(latLong ? UBLOX_CFG_TMODE_LAT : UBLOX_CFG_TMODE_ECEF_X, converter32.unsigned32);
  converter32.signed32 = ecefYOrLon;
  result &= addCfgValset(latLong ? UBLOX_CFG_TMODE_LON : UBLOX_CFG_TMODE_ECEF_Y, converter32.unsigned32);
  converter32.signed32 = ecefZOrAlt;
  result &= addCfgValset(latLong ? UBLOX_CFG_TMODE_HEIGHT : UBLOX_CFG_TMODE_ECEF_Z, converter32.unsigned32);
  converter8.signed8 = ecefXOrLatHP;
  result &= addCfgValset(latLong ? UBLOX_CFG_TMODE_LAT_HP : UBLOX_CFG_TMODE_ECEF_X_HP, converter8.unsigned8);
  converter8.signed8 = ecefYOrLonHP;
  result &= addCfgValset(latLong ? UBLOX_CFG_TMODE_LON_HP : UBLOX_CFG_TMODE_ECEF_Y_HP, converter8.unsigned8);
  converter8.signed8 = ecefZOrAltHP;
  result &= addCfgValset(latLong ? UBLOX_CFG_TMODE_HEIGHT_HP : UBLOX_CFG_TMODE_ECEF_Z_HP, converter8.unsigned8);
  result &= sendCfgValset(maxWait);

  return result;
}

bool DevUBLOXGNSS::setStaticPosition(int32_t ecefXOrLat, int32_t ecefYOrLon, int32_t ecefZOrAlt, bool latlong, uint8_t layer, uint16_t maxWait)
{
  return (setStaticPosition(ecefXOrLat, 0, ecefYOrLon, 0, ecefZOrAlt, 0, latlong, layer, maxWait));
}

// Set the DGNSS differential mode
bool DevUBLOXGNSS::setDGNSSConfiguration(sfe_ublox_dgnss_mode_e dgnssMode, uint8_t layer, uint16_t maxWait)
{
  return setVal8(UBLOX_CFG_NAVHPG_DGNSSMODE, (uint8_t)dgnssMode, layer, maxWait);
}

// Module Protocol Version

// Get the current protocol version of the u-blox module we're communicating with
// This is helpful when deciding if we should call the high-precision Lat/Long (HPPOSLLH) or the regular (POSLLH)
uint8_t DevUBLOXGNSS::getProtocolVersionHigh(uint16_t maxWait)
{
  if (!prepareModuleInfo(maxWait))
    return 0;
  return (moduleSWVersion->protocolVersionHigh);
}
uint8_t DevUBLOXGNSS::getProtocolVersionLow(uint16_t maxWait)
{
  if (!prepareModuleInfo(maxWait))
    return 0;
  return (moduleSWVersion->protocolVersionLow);
}

// Get the firmware version of the u-blox module we're communicating with
uint8_t DevUBLOXGNSS::getFirmwareVersionHigh(uint16_t maxWait)
{
  if (!prepareModuleInfo(maxWait))
    return 0;
  return (moduleSWVersion->firmwareVersionHigh);
}
uint8_t DevUBLOXGNSS::getFirmwareVersionLow(uint16_t maxWait)
{
  if (!prepareModuleInfo(maxWait))
    return 0;
  return (moduleSWVersion->firmwareVersionLow);
}

// Get the firmware type
const char *DevUBLOXGNSS::getFirmwareType(uint16_t maxWait)
{
  static const char unknownFirmware[4] = {'T', 'B', 'D', '\0'};
  if (!prepareModuleInfo(maxWait))
    return unknownFirmware;
  return ((const char *)moduleSWVersion->firmwareType);
}

// Get the module name
const char *DevUBLOXGNSS::getModuleName(uint16_t maxWait)
{
  static const char unknownModule[4] = {'T', 'B', 'D', '\0'};
  if (!prepareModuleInfo(maxWait))
    return unknownModule;
  return ((const char *)moduleSWVersion->moduleName);
}

// PRIVATE: Common code to initialize moduleSWVersion
bool DevUBLOXGNSS::prepareModuleInfo(uint16_t maxWait)
{
  if (moduleSWVersion == nullptr)
    initModuleSWVersion();        // Check that RAM has been allocated for the SW version
  if (moduleSWVersion == nullptr) // Bail if the RAM allocation failed
    return (false);

  if (moduleSWVersion->moduleQueried == false)
    getModuleInfo(maxWait);

  return moduleSWVersion->moduleQueried;
}

// Get the current protocol version of the u-blox module we're communicating with
// This is helpful when deciding if we should call the high-precision Lat/Long (HPPOSLLH) or the regular (POSLLH)
bool DevUBLOXGNSS::getProtocolVersion(uint16_t maxWait) // Old name - deprecated
{
  return getModuleInfo(maxWait);
}

bool DevUBLOXGNSS::getModuleInfo(uint16_t maxWait)
{
  if (moduleSWVersion == nullptr)
    initModuleSWVersion();        // Check that RAM has been allocated for the SW version
  if (moduleSWVersion == nullptr) // Bail if the RAM allocation failed
    return (false);

  // Send packet with only CLS and ID, length of zero. This will cause the module to respond with the contents of that CLS/ID.
  packetCfg.cls = UBX_CLASS_MON;
  packetCfg.id = UBX_MON_VER;

  packetCfg.len = 0;
  packetCfg.startingSpot = 40; // Start at first "extended software information" string

  if (sendCommand(&packetCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are only expecting data (no ACK)
    return (false);                                                       // If command send fails then bail

  // Payload should now contain ~220 characters (depends on module type)

  // We will step through the payload looking at each extension field of 30 bytes
  const char *ptr; // const: glibc's C++ strstr returns const char * for a const argument
  uint8_t fwProtMod = 0; // Flags to show if we extracted the FWVER, PROTVER and MOD data
  for (uint16_t extensionNumber = 0; extensionNumber < ((packetCfg.len - 40) / 30); extensionNumber++)
  {
    // Check for FWVER (should be in extension 1)
    ptr = strstr((const char *)&payloadCfg[(30 * extensionNumber)], "FWVER=");
    if (ptr != nullptr)
    {
      ptr += strlen("FWVER="); // Point to the firmware type (HPG etc.)
      int i = 0;
      while ((i < firmwareTypeLen) && (*ptr != '\0') && (*ptr != ' ')) // Extract the firmware type (3-7 chars)
        moduleSWVersion->firmwareType[i++] = *ptr++;
      moduleSWVersion->firmwareType[i] = '\0'; // NULL-terminate

      if (*ptr == ' ')
        ptr++; // Skip the space

      int firmwareHi = 0;
      int firmwareLo = 0;
      int scanned = sscanf(ptr, "%d.%d", &firmwareHi, &firmwareLo);
      if (scanned == 2) // Check we extracted the firmware version successfully
      {
        moduleSWVersion->firmwareVersionHigh = firmwareHi;
        moduleSWVersion->firmwareVersionLow = firmwareLo;
        fwProtMod |= 0x01; // Record that we got the FWVER
      }
    }
    // Check for PROTVER (should be in extension 2)
    ptr = strstr((const char *)&payloadCfg[(30 * extensionNumber)], "PROTVER=");
    if (ptr != nullptr)
    {
      ptr += strlen("PROTVER="); // Point to the protocol version
      int protHi = 0;
      int protLo = 0;
      int scanned = sscanf(ptr, "%d.%d", &protHi, &protLo);
      if (scanned == 2) // Check we extracted the firmware version successfully
      {
        moduleSWVersion->protocolVersionHigh = protHi;
        moduleSWVersion->protocolVersionLow = protLo;
        fwProtMod |= 0x02; // Record that we got the PROTVER
      }
    }
    // Check for MOD (should be in extension 3)
    // Note: see issue #55. It appears that the UBX-M10050-KB chip does not report MOD
    ptr = strstr((const char *)&payloadCfg[(30 * extensionNumber)], "MOD=");
    if (ptr != nullptr)
    {
      ptr += strlen("MOD="); // Point to the module name
      int i = 0;
      while ((i < moduleNameMaxLen) && (*ptr != '\0') && (*ptr != ' ')) // Copy the module name
        moduleSWVersion->moduleName[i++] = *ptr++;
      moduleSWVersion->moduleName[i] = '\0'; // NULL-terminate
      fwProtMod |= 0x04;                     // Record that we got the MOD
    }
  }

  if ((fwProtMod & 0x04) == 0) // Is MOD missing?
  {
    strncpy(moduleSWVersion->moduleName, "NONE", moduleNameMaxLen);
    fwProtMod |= 0x04; // Record that we updated the MOD
  }

  if (fwProtMod == 0x07) // Did we extract all three?
  {
    debugPrint("getModuleInfo: FWVER: ");
    debugPrint(moduleSWVersion->firmwareVersionHigh);
    debugPrint(".");
    debugPrintln(moduleSWVersion->firmwareVersionLow);
    debugPrint("getModuleInfo: PROTVER: ");
    debugPrint(moduleSWVersion->protocolVersionHigh);
    debugPrint(".");
    debugPrintln(moduleSWVersion->protocolVersionLow);
    debugPrint("getModuleInfo: MOD: ");
    debugPrintln(moduleSWVersion->moduleName);

    moduleSWVersion->moduleQueried = true; // Mark this data as new

    return (true);
  }

  return (false); // We failed
}

// PRIVATE: Allocate RAM for moduleSWVersion and initialize it
bool DevUBLOXGNSS::initModuleSWVersion()
{
  moduleSWVersion = new moduleSWVersion_t; // Allocate RAM for the main struct
  if (moduleSWVersion == nullptr)
  {
    debugPrintln("initModuleSWVersion: RAM alloc failed!", true); // Important
    return (false);
  }
  moduleSWVersion->protocolVersionHigh = 0; // Clear the contents
  moduleSWVersion->protocolVersionLow = 0;
  moduleSWVersion->firmwareVersionHigh = 0;
  moduleSWVersion->firmwareVersionLow = 0;
  moduleSWVersion->firmwareType[0] = 0;
  moduleSWVersion->moduleName[0] = 0;
  moduleSWVersion->moduleQueried = false;
  return (true);
}

// Geofences

// Add a new geofence using UBX-CFG-GEOFENCE
bool DevUBLOXGNSS::addGeofence(int32_t latitude, int32_t longitude, uint32_t radius, uint8_t confidence, bool pinPolarity, uint8_t pin, uint8_t layer, uint16_t maxWait)
{
  if (currentGeofenceParams == nullptr)
    initGeofenceParams();               // Check if RAM has been allocated for currentGeofenceParams
  if (currentGeofenceParams == nullptr) // Abort if the RAM allocation failed
    return (false);

  if (currentGeofenceParams->numFences >= 4)
    return (false); // Quit if we already have four geofences defined

  // Store the new geofence parameters
  currentGeofenceParams->lats[currentGeofenceParams->numFences] = latitude;
  currentGeofenceParams->longs[currentGeofenceParams->numFences] = longitude;
  currentGeofenceParams->rads[currentGeofenceParams->numFences] = radius;
  currentGeofenceParams->numFences += 1; // Increment the number of fences

  unsignedSigned32 converter32;
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_GEOFENCE_CONFLVL, confidence);
  if (pin > 0)
  {
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_PINPOL, (uint8_t)pinPolarity);
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_PIN, pin);
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_USE_PIO, 1);
  }
  else
  {
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_USE_PIO, 0);
  }
  result &= addCfgValset(UBLOX_CFG_GEOFENCE_USE_FENCE1, 1);
  converter32.signed32 = currentGeofenceParams->lats[0];
  result &= addCfgValset(UBLOX_CFG_GEOFENCE_FENCE1_LAT, converter32.unsigned32);
  converter32.signed32 = currentGeofenceParams->longs[0];
  result &= addCfgValset(UBLOX_CFG_GEOFENCE_FENCE1_LON, converter32.unsigned32);
  result &= addCfgValset(UBLOX_CFG_GEOFENCE_FENCE1_RAD, currentGeofenceParams->rads[0]);
  result &= addCfgValset(UBLOX_CFG_GEOFENCE_USE_FENCE2, currentGeofenceParams->numFences > 1 ? 1 : 0);
  if (currentGeofenceParams->numFences > 1)
  {
    converter32.signed32 = currentGeofenceParams->lats[1];
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_FENCE2_LAT, converter32.unsigned32);
    converter32.signed32 = currentGeofenceParams->longs[1];
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_FENCE2_LON, converter32.unsigned32);
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_FENCE2_RAD, currentGeofenceParams->rads[1]);
  }
  result &= addCfgValset(UBLOX_CFG_GEOFENCE_USE_FENCE3, currentGeofenceParams->numFences > 2 ? 1 : 0);
  if (currentGeofenceParams->numFences > 2)
  {
    converter32.signed32 = currentGeofenceParams->lats[2];
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_FENCE3_LAT, converter32.unsigned32);
    converter32.signed32 = currentGeofenceParams->longs[2];
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_FENCE3_LON, converter32.unsigned32);
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_FENCE3_RAD, currentGeofenceParams->rads[2]);
  }
  result &= addCfgValset(UBLOX_CFG_GEOFENCE_USE_FENCE4, currentGeofenceParams->numFences > 3 ? 1 : 0);
  if (currentGeofenceParams->numFences > 3)
  {
    converter32.signed32 = currentGeofenceParams->lats[3];
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_FENCE4_LAT, converter32.unsigned32);
    converter32.signed32 = currentGeofenceParams->longs[3];
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_FENCE4_LON, converter32.unsigned32);
    result &= addCfgValset(UBLOX_CFG_GEOFENCE_FENCE4_RAD, currentGeofenceParams->rads[3]);
  }
  result &= sendCfgValset(maxWait);

  return result;
}

// Clear all geofences using UBX-CFG-GEOFENCE
bool DevUBLOXGNSS::clearGeofences(uint8_t layer, uint16_t maxWait)
{
  if (currentGeofenceParams == nullptr)
    initGeofenceParams();               // Check if RAM has been allocated for currentGeofenceParams
  if (currentGeofenceParams == nullptr) // Abort if the RAM allocation failed
    return (false);

  currentGeofenceParams->numFences = 0; // Zero the number of geofences currently in use

  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_GEOFENCE_USE_FENCE1, 0);
  result &= addCfgValset(UBLOX_CFG_GEOFENCE_USE_FENCE2, 0);
  result &= addCfgValset(UBLOX_CFG_GEOFENCE_USE_FENCE3, 0);
  result &= addCfgValset(UBLOX_CFG_GEOFENCE_USE_FENCE4, 0);
  result &= sendCfgValset(maxWait);

  return result;
}

// Returns the combined geofence state using UBX-NAV-GEOFENCE
bool DevUBLOXGNSS::getGeofenceState(geofenceState &currentGeofenceState, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_NAV;
  packetCfg.id = UBX_NAV_GEOFENCE;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  // Ask module for the geofence status. Loads into payloadCfg.
  if (sendCommand(&packetCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
    return (false);

  currentGeofenceState.status = payloadCfg[5];    // Extract the status
  currentGeofenceState.numFences = payloadCfg[6]; // Extract the number of geofences
  currentGeofenceState.combState = payloadCfg[7]; // Extract the combined state of all geofences
  if (currentGeofenceState.numFences > 0)
    currentGeofenceState.states[0] = payloadCfg[8]; // Extract geofence 1 state
  if (currentGeofenceState.numFences > 1)
    currentGeofenceState.states[1] = payloadCfg[10]; // Extract geofence 2 state
  if (currentGeofenceState.numFences > 2)
    currentGeofenceState.states[2] = payloadCfg[12]; // Extract geofence 3 state
  if (currentGeofenceState.numFences > 3)
    currentGeofenceState.states[3] = payloadCfg[14]; // Extract geofence 4 state

  return (true);
}

// PRIVATE: Allocate RAM for currentGeofenceParams and initialize it
bool DevUBLOXGNSS::initGeofenceParams()
{
  currentGeofenceParams = new geofenceParams_t; // Allocate RAM for the main struct
  if (currentGeofenceParams == nullptr)
  {
    debugPrintln("initGeofenceParams: RAM alloc failed!", true); // Important
    return (false);
  }
  currentGeofenceParams->numFences = 0;
  return (true);
}

// Powers off the GPS device for a given duration to reduce power consumption.
// NOTE: Querying the device before the duration is complete, for example by "getLatitude()" will wake it up!
// Returns true if command has not been not acknowledged.
// Returns false if command has not been acknowledged or maxWait = 0.
bool DevUBLOXGNSS::powerOff(uint32_t durationInMs, uint16_t maxWait)
{
  // use durationInMs = 0 for infinite duration
  debugPrint("Powering off for ");
  debugPrint(durationInMs);
  debugPrintln(" ms");

  // Power off device using UBX-RXM-PMREQ
  packetCfg.cls = UBX_CLASS_RXM; // 0x02
  packetCfg.id = UBX_RXM_PMREQ;  // 0x41
  packetCfg.len = 8;
  packetCfg.startingSpot = 0;

  // duration
  // big endian to little endian, switch byte order
  for (uint8_t i = 0; i < 4; i++)
    payloadCfg[i] = durationInMs >> (8 * i); // Value

  payloadCfg[4] = 0x02; // Flags : set the backup bit
  payloadCfg[5] = 0x00; // Flags
  payloadCfg[6] = 0x00; // Flags
  payloadCfg[7] = 0x00; // Flags

  if (maxWait != 0)
  {
    // check for "not acknowledged" command
    return (sendCommand(&packetCfg, maxWait) != SFE_UBLOX_STATUS_COMMAND_NACK);
  }
  else
  {
    sendCommand(&packetCfg, maxWait);
    return false; // can't tell if command not acknowledged if maxWait = 0
  }
}

// Powers off the GPS device for a given duration to reduce power consumption.
// While powered off it can be woken up by creating a falling or rising voltage edge on the specified pin.
// NOTE: The GPS seems to be sensitve to signals on the pins while powered off. Works best when Microcontroller is in deepsleep.
// NOTE: Querying the device before the duration is complete, for example by "getLatitude()" will wake it up!
// Returns true if command has not been not acknowledged.
// Returns false if command has not been acknowledged or maxWait = 0.
bool DevUBLOXGNSS::powerOffWithInterrupt(uint32_t durationInMs, uint32_t wakeupSources, bool forceWhileUsb, uint16_t maxWait)
{
  // use durationInMs = 0 for infinite duration
  debugPrint("Powering off for ");
  debugPrint(durationInMs);
  debugPrintln(" ms");

  // Power off device using UBX-RXM-PMREQ
  packetCfg.cls = UBX_CLASS_RXM; // 0x02
  packetCfg.id = UBX_RXM_PMREQ;  // 0x41
  packetCfg.len = 16;
  packetCfg.startingSpot = 0;

  payloadCfg[0] = 0x00; // message version

  // bytes 1-3 are reserved - and must be set to zero
  payloadCfg[1] = 0x00;
  payloadCfg[2] = 0x00;
  payloadCfg[3] = 0x00;

  // duration
  // big endian to little endian, switch byte order
  for (uint8_t i = 0; i < 4; i++)
    payloadCfg[4 + i] = durationInMs >> (8 * i); // Value

  // flags

  // disables USB interface when powering off, defaults to true
  if (forceWhileUsb)
  {
    payloadCfg[8] = 0x06; // force | backup
  }
  else
  {
    payloadCfg[8] = 0x02; // backup only (leave the force bit clear - module will stay on if USB is connected)
  }

  payloadCfg[9] = 0x00;
  payloadCfg[10] = 0x00;
  payloadCfg[11] = 0x00;

  // wakeUpSources

  // wakeupPin mapping, defaults to VAL_RXM_PMREQ_WAKEUPSOURCE_EXTINT0

  // Possible values are:
  // VAL_RXM_PMREQ_WAKEUPSOURCE_UARTRX
  // VAL_RXM_PMREQ_WAKEUPSOURCE_EXTINT0
  // VAL_RXM_PMREQ_WAKEUPSOURCE_EXTINT1
  // VAL_RXM_PMREQ_WAKEUPSOURCE_SPICS

  for (uint8_t i = 0; i < 4; i++)
    payloadCfg[12 + i] = wakeupSources >> (8 * i); // Value

  if (maxWait != 0)
  {
    // check for "not acknowledged" command
    return (sendCommand(&packetCfg, maxWait) != SFE_UBLOX_STATUS_COMMAND_NACK);
  }
  else
  {
    sendCommand(&packetCfg, maxWait);
    return false; // can't tell if command not acknowledged if maxWait = 0
  }
}

// Dynamic Platform Model

// Change the dynamic platform model using UBX-CFG-NAV5
// Possible values are:
// PORTABLE,STATIONARY,PEDESTRIAN,AUTOMOTIVE,SEA,
// AIRBORNE1g,AIRBORNE2g,AIRBORNE4g,WRIST,BIKE
// WRIST is not supported in protocol versions less than 18
// BIKE is supported in protocol versions 19.2
bool DevUBLOXGNSS::setDynamicModel(dynModel newDynamicModel, uint8_t layer, uint16_t maxWait)
{
  return setVal8(UBLOX_CFG_NAVSPG_DYNMODEL, (uint8_t)newDynamicModel, layer, maxWait);
}

// Get the dynamic platform model using UBX-CFG-NAV5
// Returns DYN_MODEL_UNKNOWN (255) if the sendCommand fails
uint8_t DevUBLOXGNSS::getDynamicModel(uint8_t layer, uint16_t maxWait)
{
  uint8_t model;

  if (!getVal8(UBLOX_CFG_NAVSPG_DYNMODEL, &model, layer, maxWait))
    return (DYN_MODEL_UNKNOWN);

  return (model); // Return the dynamic model
}

// Reset the odometer
bool DevUBLOXGNSS::resetOdometer(uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_NAV;
  packetCfg.id = UBX_NAV_RESETODO;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  // This is a special case as we are only expecting an ACK but this is not a CFG message
  return (sendCommand(&packetCfg, maxWait, true) == SFE_UBLOX_STATUS_DATA_SENT); // We are only expecting an ACK
}

// Enable / disable the odometer
bool DevUBLOXGNSS::enableOdometer(bool enable, uint8_t layer, uint16_t maxWait)
{
  return setVal8(UBLOX_CFG_ODO_USE_ODO, (uint8_t)enable, layer, maxWait);
}

// Read the odometer configuration
bool DevUBLOXGNSS::getOdometerConfig(uint8_t *flags, uint8_t *odoCfg, uint8_t *cogMaxSpeed, uint8_t *cogMaxPosAcc, uint8_t *velLpGain, uint8_t *cogLpGain, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValget(layer);
  result &= addCfgValget(UBLOX_CFG_ODO_USE_ODO);
  result &= addCfgValget(UBLOX_CFG_ODO_USE_COG);
  result &= addCfgValget(UBLOX_CFG_ODO_OUTLPVEL);
  result &= addCfgValget(UBLOX_CFG_ODO_OUTLPCOG);
  result &= addCfgValget(UBLOX_CFG_ODO_PROFILE);
  result &= addCfgValget(UBLOX_CFG_ODO_COGMAXSPEED);
  result &= addCfgValget(UBLOX_CFG_ODO_COGMAXPOSACC);
  result &= addCfgValget(UBLOX_CFG_ODO_VELLPGAIN);
  result &= addCfgValget(UBLOX_CFG_ODO_COGLPGAIN);
  result &= sendCfgValget(maxWait);

  if (result)
  {
    uint8_t flagsBit = 0;
    uint8_t flagsByte = 0;
    result &= extractConfigValueByKey(&packetCfg, UBLOX_CFG_ODO_USE_ODO, &flagsBit, 1);
    if (flagsBit)
      flagsByte |= UBX_CFG_ODO_USE_ODO;
    result &= extractConfigValueByKey(&packetCfg, UBLOX_CFG_ODO_USE_COG, &flagsBit, 1);
    if (flagsBit)
      flagsByte |= UBX_CFG_ODO_USE_COG;
    result &= extractConfigValueByKey(&packetCfg, UBLOX_CFG_ODO_OUTLPVEL, &flagsBit, 1);
    if (flagsBit)
      flagsByte |= UBX_CFG_ODO_OUT_LP_VEL;
    result &= extractConfigValueByKey(&packetCfg, UBLOX_CFG_ODO_OUTLPCOG, &flagsBit, 1);
    if (flagsBit)
      flagsByte |= UBX_CFG_ODO_OUT_LP_COG;
    *flags = flagsByte;

    result &= extractConfigValueByKey(&packetCfg, UBLOX_CFG_ODO_PROFILE, odoCfg, 1);
    result &= extractConfigValueByKey(&packetCfg, UBLOX_CFG_ODO_COGMAXSPEED, cogMaxSpeed, 1);
    result &= extractConfigValueByKey(&packetCfg, UBLOX_CFG_ODO_COGMAXPOSACC, cogMaxPosAcc, 1);
    result &= extractConfigValueByKey(&packetCfg, UBLOX_CFG_ODO_VELLPGAIN, velLpGain, 1);
    result &= extractConfigValueByKey(&packetCfg, UBLOX_CFG_ODO_COGLPGAIN, cogLpGain, 1);
  }

  return result;
}

// Configure the odometer
bool DevUBLOXGNSS::setOdometerConfig(uint8_t flags, uint8_t odoCfg, uint8_t cogMaxSpeed, uint8_t cogMaxPosAcc, uint8_t velLpGain, uint8_t cogLpGain, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_ODO_USE_ODO, flags & UBX_CFG_ODO_USE_ODO ? 1 : 0);
  result &= addCfgValset(UBLOX_CFG_ODO_USE_COG, flags & UBX_CFG_ODO_USE_COG ? 1 : 0);
  result &= addCfgValset(UBLOX_CFG_ODO_OUTLPVEL, flags & UBX_CFG_ODO_OUT_LP_VEL ? 1 : 0);
  result &= addCfgValset(UBLOX_CFG_ODO_OUTLPCOG, flags & UBX_CFG_ODO_OUT_LP_COG ? 1 : 0);
  result &= addCfgValset(UBLOX_CFG_ODO_PROFILE, odoCfg);
  result &= addCfgValset(UBLOX_CFG_ODO_COGMAXSPEED, cogMaxSpeed);
  result &= addCfgValset(UBLOX_CFG_ODO_COGMAXPOSACC, cogMaxPosAcc);
  result &= addCfgValset(UBLOX_CFG_ODO_VELLPGAIN, velLpGain);
  result &= addCfgValset(UBLOX_CFG_ODO_COGLPGAIN, cogLpGain);
  result &= sendCfgValset(maxWait);
  return result;
}

uint32_t DevUBLOXGNSS::getEnableGNSSConfigKey(sfe_ublox_gnss_ids_e id)
{
  const uint32_t gnssConfigKeys[(uint8_t)SFE_UBLOX_GNSS_ID_UNKNOWN] = {
      UBLOX_CFG_SIGNAL_GPS_ENA,
      UBLOX_CFG_SIGNAL_SBAS_ENA,
      UBLOX_CFG_SIGNAL_GAL_ENA,
      UBLOX_CFG_SIGNAL_BDS_ENA,
      0, // IMES has no ENA key
      UBLOX_CFG_SIGNAL_QZSS_ENA,
      UBLOX_CFG_SIGNAL_GLO_ENA};

  if (id >= SFE_UBLOX_GNSS_ID_UNKNOWN)
    return 0;
  else
    return (gnssConfigKeys[(uint8_t)id]);
}

// Enable/Disable individual GNSS systems using UBX-CFG-GNSS
bool DevUBLOXGNSS::enableGNSS(bool enable, sfe_ublox_gnss_ids_e id, uint8_t layer, uint16_t maxWait)
{
  uint32_t key = getEnableGNSSConfigKey(id);
  return (setVal8(key, enable ? 1 : 0, layer, maxWait));
}

// Check if an individual GNSS system is enabled
bool DevUBLOXGNSS::isGNSSenabled(sfe_ublox_gnss_ids_e id, bool *enabled, uint8_t layer, uint16_t maxWait)
{
  uint32_t key = getEnableGNSSConfigKey(id);
  return (getVal8(key, (uint8_t *)enabled, layer, maxWait));
}
bool DevUBLOXGNSS::isGNSSenabled(sfe_ublox_gnss_ids_e id, uint8_t layer, uint16_t maxWait) // Unsafe
{
  uint32_t key = getEnableGNSSConfigKey(id);
  uint8_t enabled = 0; // Initialized: isGNSSenabled returns false if getVal8 fails
  getVal8(key, &enabled, layer, maxWait);
  return ((bool)enabled);
}

// Reset ESF automatic IMU-mount alignment
bool DevUBLOXGNSS::resetIMUalignment(uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_ESF;
  packetCfg.id = UBX_ESF_RESETALG;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  // This is a special case as we are only expecting an ACK but this is not a CFG message
  return (sendCommand(&packetCfg, maxWait, true) == SFE_UBLOX_STATUS_DATA_SENT); // We are only expecting an ACK
}

// Enable/disable esfAutoAlignment
bool DevUBLOXGNSS::getESFAutoAlignment(bool *enabled, uint8_t layer, uint16_t maxWait)
{
  return getVal8(UBLOX_CFG_SFIMU_AUTO_MNTALG_ENA, (uint8_t *)enabled, layer, maxWait);
}
bool DevUBLOXGNSS::getESFAutoAlignment(uint8_t layer, uint16_t maxWait) // Unsafe overload
{
  uint8_t result = 0;
  getVal8(UBLOX_CFG_SFIMU_AUTO_MNTALG_ENA, &result, layer, maxWait);
  return (bool)result;
}
bool DevUBLOXGNSS::setESFAutoAlignment(bool enable, uint8_t layer, uint16_t maxWait)
{
  return setVal8(UBLOX_CFG_SFIMU_AUTO_MNTALG_ENA, (uint8_t)enable, layer, maxWait);
}

// ubxMONRF is now self-registered - see AGENTS.md "Adding the variable-length UBX messages".
// getRFinformation() (the old poll-only, packetCfg/sendCommand()/extractByte() implementation)
// has been removed - getMONRF() (below, next to getMONCOMMS()) replaces it.

// UBX-CFG-NAVX5 - get/set the ackAiding byte. If ackAiding is 1, UBX-MGA-ACK messages will be sent by the module to acknowledge the MGA data
uint8_t DevUBLOXGNSS::getAckAiding(uint8_t layer, uint16_t maxWait) // Get the ackAiding byte - returns 255 if the sendCommand fails
{
  uint8_t enabled = 0;
  bool success = getVal8(UBLOX_CFG_NAVSPG_ACKAIDING, &enabled, layer, maxWait);
  if (success)
    return enabled;
  return 255;
}
bool DevUBLOXGNSS::setAckAiding(uint8_t ackAiding, uint8_t layer, uint16_t maxWait) // Set the ackAiding byte
{
  return setVal8(UBLOX_CFG_NAVSPG_ACKAIDING, ackAiding, layer, maxWait);
}

// AssistNow Autonomous support
// UBX-CFG-NAVX5 - get the AssistNow Autonomous configuration (aopCfg) - returns 255 if the sendCommand fails
uint8_t DevUBLOXGNSS::getAopCfg(uint8_t layer, uint16_t maxWait)
{
  uint8_t enabled = 0;
  bool success = getVal8(UBLOX_CFG_ANA_USE_ANA, &enabled, layer, maxWait);
  if (success)
    return enabled;
  return 255;
}
// Set the aopCfg byte and the aopOrdMaxErr word
bool DevUBLOXGNSS::setAopCfg(uint8_t aopCfg, uint16_t aopOrbMaxErr, uint8_t layer, uint16_t maxWait)
{
  bool result = newCfgValset(layer);
  result &= addCfgValset(UBLOX_CFG_ANA_USE_ANA, aopCfg);
  if ((aopOrbMaxErr >= 5) && (aopOrbMaxErr <= 1000)) // Maximum acceptable (modeled) orbit error in m. Range is from 5 to 1000.
    result &= addCfgValset(UBLOX_CFG_ANA_ORBMAXERR, aopOrbMaxErr);
  result &= sendCfgValset(maxWait);
  return result;
}

// SPARTN dynamic keys
//"When the receiver boots, the host should send 'current' and 'next' keys in one message." - Use setDynamicSPARTNKeys for this.
//"Every time the 'current' key is expired, 'next' takes its place."
//"Therefore the host should then retrieve the new 'next' key and send only that." - Use setDynamicSPARTNKey for this.
// The key can be provided in binary (uint8_t) format or in ASCII Hex (char) format, but in both cases keyLengthBytes _must_ represent the binary key length in bytes.
bool DevUBLOXGNSS::setDynamicSPARTNKey(uint8_t keyLengthBytes, uint16_t validFromWno, uint32_t validFromTow, const char *key)
{
  uint8_t *binaryKey = new uint8_t[keyLengthBytes]; // Allocate memory to store the binaryKey

  if (binaryKey == nullptr)
  {
    debugPrintln("setDynamicSPARTNKey: binaryKey RAM allocation failed!"); // Not important
    return (false);
  }

  bool ok = true;

  // Convert the ASCII Hex const char to binary uint8_t
  for (uint16_t i = 0; i < ((uint16_t)keyLengthBytes * 2); i += 2)
  {
    if ((key[i] >= '0') && (key[i] <= '9'))
    {
      binaryKey[i >> 1] = (key[i] - '0') << 4;
    }
    else if ((key[i] >= 'a') && (key[i] <= 'f'))
    {
      binaryKey[i >> 1] = (key[i] + 10 - 'a') << 4;
    }
    else if ((key[i] >= 'A') && (key[i] <= 'F'))
    {
      binaryKey[i >> 1] = (key[i] + 10 - 'A') << 4;
    }
    else
    {
      ok = false;
    }

    if ((key[i + 1] >= '0') && (key[i + 1] <= '9'))
    {
      binaryKey[i >> 1] |= key[i + 1] - '0';
    }
    else if ((key[i + 1] >= 'a') && (key[i + 1] <= 'f'))
    {
      binaryKey[i >> 1] |= key[i + 1] + 10 - 'a';
    }
    else if ((key[i + 1] >= 'A') && (key[i + 1] <= 'F'))
    {
      binaryKey[i >> 1] |= key[i + 1] + 10 - 'A';
    }
    else
    {
      ok = false;
    }
  }

  if (ok)
    ok = setDynamicSPARTNKey(keyLengthBytes, validFromWno, validFromTow, (const uint8_t *)binaryKey);

  delete[] binaryKey; // Free the memory allocated for binaryKey

  return (ok);
}

bool DevUBLOXGNSS::setDynamicSPARTNKey(uint8_t keyLengthBytes, uint16_t validFromWno, uint32_t validFromTow, const uint8_t *key)
{
  // Check if there is room for the key in packetCfg. Resize the buffer if not.
  size_t payloadLength = (size_t)keyLengthBytes + 12;
  if (packetCfgPayloadSize < payloadLength)
  {
    if (!setPacketCfgPayloadSize(payloadLength)) // Check if the resize was successful
    {
      return (false);
    }
  }

  // Copy the key etc. into packetCfg
  packetCfg.cls = UBX_CLASS_RXM;
  packetCfg.id = UBX_RXM_SPARTNKEY;
  packetCfg.len = payloadLength;
  packetCfg.startingSpot = 0;

  payloadCfg[0] = 0x01; // version
  payloadCfg[1] = 0x01; // numKeys
  payloadCfg[2] = 0x00; // reserved0
  payloadCfg[3] = 0x00; // reserved0
  payloadCfg[4] = 0x00; // reserved1
  payloadCfg[5] = keyLengthBytes;
  payloadCfg[6] = validFromWno & 0xFF; // validFromWno little-endian
  payloadCfg[7] = validFromWno >> 8;
  payloadCfg[8] = validFromTow & 0xFF; // validFromTow little-endian
  payloadCfg[9] = (validFromTow >> 8) & 0xFF;
  payloadCfg[10] = (validFromTow >> 16) & 0xFF;
  payloadCfg[11] = (validFromTow >> 24) & 0xFF;

  memcpy(&payloadCfg[12], key, keyLengthBytes);

  return (sendCommand(&packetCfg, 0) == SFE_UBLOX_STATUS_SUCCESS); // UBX-RXM-SPARTNKEY is silent. It does not ACK (or NACK)
}

bool DevUBLOXGNSS::setDynamicSPARTNKeys(uint8_t keyLengthBytes1, uint16_t validFromWno1, uint32_t validFromTow1, const char *key1,
                                        uint8_t keyLengthBytes2, uint16_t validFromWno2, uint32_t validFromTow2, const char *key2)
{
  uint8_t *binaryKey1 = new uint8_t[keyLengthBytes1]; // Allocate memory to store binaryKey1

  if (binaryKey1 == nullptr)
  {
    debugPrintln("setDynamicSPARTNKeys: binaryKey1 RAM allocation failed!"); // Not important
    return (false);
  }

  uint8_t *binaryKey2 = new uint8_t[keyLengthBytes2]; // Allocate memory to store binaryKey2

  if (binaryKey2 == nullptr)
  {
    debugPrintln("setDynamicSPARTNKeys: binaryKey2 RAM allocation failed!"); // Not important
    delete[] binaryKey1;
    return (false);
  }

  bool ok = true;

  // Convert the ASCII Hex const char to binary uint8_t
  for (uint16_t i = 0; i < ((uint16_t)keyLengthBytes1 * 2); i += 2)
  {
    if ((key1[i] >= '0') && (key1[i] <= '9'))
    {
      binaryKey1[i >> 1] = (key1[i] - '0') << 4;
    }
    else if ((key1[i] >= 'a') && (key1[i] <= 'f'))
    {
      binaryKey1[i >> 1] = (key1[i] + 10 - 'a') << 4;
    }
    else if ((key1[i] >= 'A') && (key1[i] <= 'F'))
    {
      binaryKey1[i >> 1] = (key1[i] + 10 - 'A') << 4;
    }
    else
    {
      ok = false;
    }

    if ((key1[i + 1] >= '0') && (key1[i + 1] <= '9'))
    {
      binaryKey1[i >> 1] |= key1[i + 1] - '0';
    }
    else if ((key1[i + 1] >= 'a') && (key1[i + 1] <= 'f'))
    {
      binaryKey1[i >> 1] |= key1[i + 1] + 10 - 'a';
    }
    else if ((key1[i + 1] >= 'A') && (key1[i + 1] <= 'F'))
    {
      binaryKey1[i >> 1] |= key1[i + 1] + 10 - 'A';
    }
    else
    {
      ok = false;
    }
  }

  // Convert the ASCII Hex const char to binary uint8_t
  for (uint16_t i = 0; i < ((uint16_t)keyLengthBytes2 * 2); i += 2)
  {
    if ((key2[i] >= '0') && (key2[i] <= '9'))
    {
      binaryKey2[i >> 1] = (key2[i] - '0') << 4;
    }
    else if ((key2[i] >= 'a') && (key2[i] <= 'f'))
    {
      binaryKey2[i >> 1] = (key2[i] + 10 - 'a') << 4;
    }
    else if ((key2[i] >= 'A') && (key2[i] <= 'F'))
    {
      binaryKey2[i >> 1] = (key2[i] + 10 - 'A') << 4;
    }
    else
    {
      ok = false;
    }

    if ((key2[i + 1] >= '0') && (key2[i + 1] <= '9'))
    {
      binaryKey2[i >> 1] |= key2[i + 1] - '0';
    }
    else if ((key2[i + 1] >= 'a') && (key2[i + 1] <= 'f'))
    {
      binaryKey2[i >> 1] |= key2[i + 1] + 10 - 'a';
    }
    else if ((key2[i + 1] >= 'A') && (key2[i + 1] <= 'F'))
    {
      binaryKey2[i >> 1] |= key2[i + 1] + 10 - 'A';
    }
    else
    {
      ok = false;
    }
  }

  if (ok)
    ok = setDynamicSPARTNKeys(keyLengthBytes1, validFromWno1, validFromTow1, (const uint8_t *)binaryKey1,
                              keyLengthBytes2, validFromWno2, validFromTow2, (const uint8_t *)binaryKey2);

  delete[] binaryKey1; // Free the memory allocated for binaryKey1
  delete[] binaryKey2; // Free the memory allocated for binaryKey2

  return (ok);
}

bool DevUBLOXGNSS::setDynamicSPARTNKeys(uint8_t keyLengthBytes1, uint16_t validFromWno1, uint32_t validFromTow1, const uint8_t *key1,
                                        uint8_t keyLengthBytes2, uint16_t validFromWno2, uint32_t validFromTow2, const uint8_t *key2)
{
  // Check if there is room for the key in packetCfg. Resize the buffer if not.
  size_t payloadLength = (size_t)keyLengthBytes1 + (size_t)keyLengthBytes2 + 20;
  if (packetCfgPayloadSize < payloadLength)
  {
    if (!setPacketCfgPayloadSize(payloadLength)) // Check if the resize was successful
    {
      return (false);
    }
  }

  // Copy the key etc. into packetCfg
  packetCfg.cls = UBX_CLASS_RXM;
  packetCfg.id = UBX_RXM_SPARTNKEY;
  packetCfg.len = payloadLength;
  packetCfg.startingSpot = 0;

  payloadCfg[0] = 0x01; // version
  payloadCfg[1] = 0x02; // numKeys
  payloadCfg[2] = 0x00; // reserved0
  payloadCfg[3] = 0x00; // reserved0
  payloadCfg[4] = 0x00; // reserved1
  payloadCfg[5] = keyLengthBytes1;
  payloadCfg[6] = validFromWno1 & 0xFF; // validFromWno little-endian
  payloadCfg[7] = validFromWno1 >> 8;
  payloadCfg[8] = validFromTow1 & 0xFF; // validFromTow little-endian
  payloadCfg[9] = (validFromTow1 >> 8) & 0xFF;
  payloadCfg[10] = (validFromTow1 >> 16) & 0xFF;
  payloadCfg[11] = (validFromTow1 >> 24) & 0xFF;
  payloadCfg[12] = 0x00; // reserved1
  payloadCfg[13] = keyLengthBytes2;
  payloadCfg[14] = validFromWno2 & 0xFF; // validFromWno little-endian
  payloadCfg[15] = validFromWno2 >> 8;
  payloadCfg[16] = validFromTow2 & 0xFF; // validFromTow little-endian
  payloadCfg[17] = (validFromTow2 >> 8) & 0xFF;
  payloadCfg[18] = (validFromTow2 >> 16) & 0xFF;
  payloadCfg[19] = (validFromTow2 >> 24) & 0xFF;

  memcpy(&payloadCfg[20], key1, keyLengthBytes1);
  memcpy(&payloadCfg[20 + keyLengthBytes1], key2, keyLengthBytes2);

  return (sendCommand(&packetCfg, 0) == SFE_UBLOX_STATUS_SUCCESS); // UBX-RXM-SPARTNKEY is silent. It does not ACK (or NACK)
}

// Support for SPARTN parsing
// Mostly stolen from https://github.com/u-blox/ubxlib/blob/master/common/spartn/src/u_spartn_crc.c

uint8_t DevUBLOXGNSS::uSpartnCrc4(const uint8_t *pU8Msg, size_t size)
{
    // Initialize local variables
    uint8_t u8TableRemainder;
    uint8_t u8Remainder = 0; // Initial remainder

    // Compute the CRC value
    // Divide each byte of the message by the corresponding polynomial
    for (size_t x = 0; x < size; x++) {
        u8TableRemainder = pU8Msg[x] ^ u8Remainder;
        u8Remainder = sfe_ublox_u8Crc4Table[u8TableRemainder];
    }

    return u8Remainder;
}

uint8_t DevUBLOXGNSS::uSpartnCrc8(const uint8_t *pU8Msg, size_t size)
{
    // Initialize local variables
    uint8_t u8TableRemainder;
    uint8_t u8Remainder = 0; // Initial remainder

    // Compute the CRC value
    // Divide each byte of the message by the corresponding polynomial
    for (size_t x = 0; x < size; x++) {
        u8TableRemainder = pU8Msg[x] ^ u8Remainder;
        u8Remainder = sfe_ublox_u8Crc8Table[u8TableRemainder];
    }

    return u8Remainder;
}

uint16_t DevUBLOXGNSS::uSpartnCrc16(const uint8_t *pU8Msg, size_t size)
{
    // Initialize local variables
    uint16_t u16TableRemainder;
    uint16_t u16Remainder = 0; // Initial remainder
    uint8_t  u8NumBitsInCrc = (8 * sizeof(uint16_t));

    // Compute the CRC value
    // Divide each byte of the message by the corresponding polynomial
    for (size_t x = 0; x < size; x++) {
        u16TableRemainder = pU8Msg[x] ^ (u16Remainder >> (u8NumBitsInCrc - 8));
        u16Remainder = sfe_ublox_u16Crc16Table[u16TableRemainder] ^ (u16Remainder << 8);
    }

    return u16Remainder;
}

uint32_t DevUBLOXGNSS::uSpartnCrc24(const uint8_t *pU8Msg, size_t size)
{
    // Initialize local variables
    uint32_t u32TableRemainder;
    uint32_t u32Remainder = 0; // Initial remainder
    uint8_t u8NumBitsInCrc = (8 * sizeof(uint8_t) * 3);

    // Compute the CRC value
    // Divide each byte of the message by the corresponding polynomial
    for (size_t x = 0; x < size; x++) {
        u32TableRemainder = pU8Msg[x] ^ (u32Remainder >> (u8NumBitsInCrc - 8));
        u32Remainder = sfe_ublox_u32Crc24Table[u32TableRemainder] ^ (u32Remainder << 8);
        u32Remainder = u32Remainder & 0x00FFFFFF; // Only interested in 24 bits
    }

    return u32Remainder;
}

uint32_t DevUBLOXGNSS::uSpartnCrc32(const uint8_t *pU8Msg, size_t size)
{
    // Initialize local variables
    uint32_t u32TableRemainder;
    uint32_t u32Remainder = 0xFFFFFFFFU; // Initial remainder
    uint8_t u8NumBitsInCrc = (8 * sizeof(uint32_t));
    uint32_t u32FinalXORValue = 0xFFFFFFFFU;

    // Compute the CRC value
    // Divide each byte of the message by the corresponding polynomial
    for (size_t x = 0; x < size; x++) {
        u32TableRemainder = pU8Msg[x] ^ (u32Remainder >> (u8NumBitsInCrc - 8));
        u32Remainder = sfe_ublox_u32Crc32Table[u32TableRemainder] ^ (u32Remainder << 8);
    }

    u32Remainder = u32Remainder ^ u32FinalXORValue;

    return u32Remainder;
}

// Parse SPARTN data
uint8_t * DevUBLOXGNSS::parseSPARTN(uint8_t incoming, bool &valid, uint16_t &len, sfe_ublox_spartn_header_t *header)
{
  typedef enum {
    waitingFor73,
    TF002_TF006,
    TF007,
    TF009,
    TF016,
    TF017,
    TF018
  } parseStates;
  static parseStates parseState = waitingFor73;

  static uint8_t spartn[1100];

  static sfe_ublox_spartn_header_t _header;
  static uint16_t frameCount;
  static uint16_t crcBytes;
  static uint16_t TF007toTF016;

  valid = false;

  switch(parseState)
  {
    case waitingFor73:
      if (incoming == 0x73)
      {
        parseState = TF002_TF006;
        frameCount = 0;
        spartn[0] = incoming;
      }
      break;
    case TF002_TF006:
      spartn[1 + frameCount] = incoming;
      if (frameCount == 0)
      {
        _header.messageType = incoming >> 1;
        _header.payloadLength = incoming & 0x01;
      }
      if (frameCount == 1)
      {
        _header.payloadLength <<= 8;
        _header.payloadLength |= incoming;
      }
      if (frameCount == 2)
      {
        _header.payloadLength <<= 1;
        _header.payloadLength |= incoming >> 7;
        _header.EAF = (incoming >> 6) & 0x01;
        _header.crcType = (incoming >> 4) & 0x03;
        switch (_header.crcType)
        {
          case 0:
            crcBytes = 1;
            break;
          case 1:
            crcBytes = 2;
            break;
          case 2:
            crcBytes = 3;
            break;
          default:
            crcBytes = 4;
            break;
        }
        _header.frameCRC = incoming & 0x0F;
        spartn[3] = spartn[3] & 0xF0; // Zero the 4 LSBs before calculating the CRC
        if (uSpartnCrc4(&spartn[1], 3) == _header.frameCRC)
        {
          spartn[3] = incoming; // Restore TF005 and TF006 now we know the data is valid
          parseState = TF007;
          debugPrint("SPARTN Header CRC is valid: payloadLength ");
          debugPrint(_header.payloadLength);
          debugPrint(" EAF ");
          debugPrint(_header.EAF);
          debugPrint(" crcType ");
          debugPrintln(_header.crcType);
        }
        else
        {
          parseState = waitingFor73;
          debugPrintln("SPARTN Header CRC is INVALID"); // Not important
        }
      }
      frameCount++;
      break;
    case TF007:
      spartn[4] = incoming;
      _header.messageSubtype = incoming >> 4;
      _header.timeTagType = (incoming >> 3) & 0x01;
      debugPrint("SPARTN timeTagType ");
      debugPrintln(_header.timeTagType);
      if (_header.timeTagType == 0)
        TF007toTF016 = 4;
      else
        TF007toTF016 = 6;
      if (_header.EAF > 0)
        TF007toTF016 += 2;
      parseState = TF009;
      frameCount = 1;          
      break;
    case TF009:
      spartn[4 + frameCount] = incoming;
      frameCount++;
      if (frameCount == TF007toTF016)
      {
        if (_header.EAF == 0)
        {
          _header.authenticationIndicator = 0;
          _header.embeddedApplicationLengthBytes = 0;
        }
        else
        {
          _header.authenticationIndicator = (incoming >> 3) & 0x07;
          debugPrint("SPARTN authenticationIndicator ");
          debugPrintln(_header.authenticationIndicator);
          if (_header.authenticationIndicator <= 1)
            _header.embeddedApplicationLengthBytes = 0;
          else
          {
            switch(incoming & 0x07)
            {
              case 0:
                _header.embeddedApplicationLengthBytes = 8; // 64 bits
                break;
              case 1:
                _header.embeddedApplicationLengthBytes = 12; // 96 bits
                break;
              case 2:
                _header.embeddedApplicationLengthBytes = 16; // 128 bits
                break;
              case 3:
                _header.embeddedApplicationLengthBytes = 32; // 256 bits
                break;
              default:
                _header.embeddedApplicationLengthBytes = 64; // 512 / TBD bits
                break;
            }
          }
          debugPrint("SPARTN embeddedApplicationLengthBytes ");
          debugPrintln(_header.embeddedApplicationLengthBytes);
        }
        parseState = TF016;
        frameCount = 0;                  
      }
      break;
    case TF016:
      spartn[4 + TF007toTF016 + frameCount] = incoming;
      frameCount++;
      if (frameCount == _header.payloadLength)
      {
        if (_header.embeddedApplicationLengthBytes > 0)
        {
          parseState = TF017;
          frameCount = 0;
        }
        else               
        {
          parseState = TF018;
          frameCount = 0;
        }
      }
      break;
    case TF017:
      spartn[4 + TF007toTF016 + _header.payloadLength + frameCount] = incoming;
      frameCount++;
      if (frameCount == _header.embeddedApplicationLengthBytes)
      {
        parseState = TF018;
        frameCount = 0;        
      }
      break;
    case TF018:
      spartn[4 + TF007toTF016 + _header.payloadLength + _header.embeddedApplicationLengthBytes + frameCount] = incoming;
      frameCount++;
      if (frameCount == crcBytes)
      {
          parseState = waitingFor73;
          uint16_t numBytes = 4 + TF007toTF016 + _header.payloadLength + _header.embeddedApplicationLengthBytes;
          debugPrint("SPARTN numBytes ");
          debugPrintln(numBytes);
          uint8_t *ptr = &spartn[numBytes];
          switch (_header.crcType)
          {
            case 0:
            {
              uint8_t expected = *ptr;
              if (uSpartnCrc8(&spartn[1], numBytes - 1) == expected) // Don't include the preamble in the CRC
              {
                valid = true;
                len = numBytes + 1;
              }
            }
            break;
            case 1:
            {
              uint16_t expected = *ptr++;
              expected <<= 8;
              expected |= *ptr;
              if (uSpartnCrc16(&spartn[1], numBytes - 1) == expected) // Don't include the preamble in the CRC
              {
                valid = true;
                len = numBytes + 2;
              }
            }
            break;
            case 2:
            {
              uint32_t expected = *ptr++;
              expected <<= 8;
              expected |= *ptr++;
              expected <<= 8;
              expected |= *ptr;
              uint32_t crc = uSpartnCrc24(&spartn[1], numBytes - 1); // Don't include the preamble in the CRC
              if (crc == expected)
              {
                valid = true;
                len = numBytes + 3;
              }
              else
              {
                debugPrint("SPARTN CRC-24 is INVALID: 0x");
                debugPrint(expected, HEX);
                debugPrint(" vs 0x");
                debugPrintln(crc, HEX);
              }
            }
            break;
            default:
            {
              uint32_t expected = *ptr++;
              expected <<= 8;
              expected |= *ptr++;
              expected <<= 8;
              expected |= *ptr++;
              expected <<= 8;
              expected |= *ptr;
              if (uSpartnCrc32(&spartn[1], numBytes - 1) == expected)
              {
                valid = true;
                len = numBytes + 4;
              }
            }
            break;
          }
      }
      break;
  }

  if (header != nullptr)
    memcpy(header, &_header, sizeof(sfe_ublox_spartn_header_t));

  return &spartn[0];
}

// ubxSECUNIQID is now self-registered - see AGENTS.md "Adding the variable-length UBX messages".
bool DevUBLOXGNSS::getSECUNIQID(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_SEC, UBX_SEC_UNIQID, maxWait);
}

// Get the unique chip ID as a hex String, e.g. "0123456789AB" - see ubxSECUNIQID.h. Reads from
// the live _storage populated by the most recent getSECUNIQID() (or getUBX("SEC","UNIQID")); does
// not poll itself. uniqueId is modelled as 6 repeated 1-byte "blocks" (see ubxSECUNIQID.h), read
// with getUbxMessageBlockField() - the same generic mechanism as every other variable-length
// message's blocks.
sfe_string_t DevUBLOXGNSS::getUniqueChipIdStr(void)
{
  ubxMessage *msg = ubxMessages.find(UBX_CLASS_SEC, UBX_SEC_UNIQID);

  sfe_string_t uniqueId;
  char hexByte[3];
  for (uint8_t i = 0; i < getUbxMessageBlockCount(msg); i++)
  {
    ubxAnyType value = getUbxMessageBlockField(msg, i, "byte");
    snprintf(hexByte, sizeof(hexByte), "%02X", (uint8_t)value.U1);
    uniqueId += hexByte;
  }

  return uniqueId;
}

// CONFIGURATION INTERFACE (protocol v27 and above)

// Given a key, load the payload with data that can then be extracted to 8, 16, or 32 bits
// This function takes a full 32-bit key
// Default layer is RAM
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
sfe_ublox_status_e DevUBLOXGNSS::getVal(uint32_t key, uint8_t layer, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_VALGET;
  packetCfg.len = 4 + 4 * 1; // While multiple keys are allowed, we will send only one key at a time
  packetCfg.startingSpot = 0;

  // Clear packet payload
  memset(payloadCfg, 0, packetCfg.len);

  // VALGET uses different memory layer definitions to VALSET
  // because it can only return the value for one layer.
  // So we need to fiddle the layer here.
  // And just to complicate things further, the ZED-F9P only responds
  // correctly to layer 0 (RAM) and layer 7 (Default)!
  uint8_t getLayer = VAL_LAYER_DEFAULT; // 7 is the "Default Layer"
  if (layer == VAL_LAYER_RAM)           // Did the user request the RAM layer?
  {
    getLayer = 0; // Layer 0 is RAM
  }
  else if (layer == VAL_LAYER_BBR) // Did the user request the BBR layer?
  {
    getLayer = 1; // Layer 1 is BBR
  }
  else if (layer == VAL_LAYER_FLASH) // Did the user request the Flash layer?
  {
    getLayer = 2; // Layer 2 is Flash
  }

  payloadCfg[0] = 0;        // Message Version - set to 0
  payloadCfg[1] = getLayer; // Layer

  // Load key into outgoing payload
  key &= ~UBX_CFG_SIZE_MASK;    // Mask off the size identifer bits
  payloadCfg[4] = key >> 8 * 0; // Key LSB
  payloadCfg[5] = key >> 8 * 1;
  payloadCfg[6] = key >> 8 * 2;
  payloadCfg[7] = key >> 8 * 3;

  debugPrint("getVal key: 0x");
  debugPrint(key, HEX);
  debugPrintln();

  // Send VALGET command with this key

  sfe_ublox_status_e retVal = sendCommand(&packetCfg, maxWait);
  debugPrint("getVal: sendCommand returned: ");
  debugPrintln(statusString(retVal));

  // Verify the response is the correct length as compared to what the user called (did the module respond with 8-bits but the user called getVal32?)
  // Response is 8 bytes plus cfg data
  // if(packet->len > 8+1)

  // The response is now sitting in payload, ready for extraction
  return (retVal);
}

// Given a key, return its value
// This function takes a full 32-bit key
// Default layer is RAM
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
bool DevUBLOXGNSS::getVal8(uint32_t key, uint8_t *val, uint8_t layer, uint16_t maxWait)
{
  bool result = getVal(key, layer, maxWait) == SFE_UBLOX_STATUS_DATA_RECEIVED;
  if (result)
    *val = extractByte(&packetCfg, 8);
  return result;
}
uint8_t DevUBLOXGNSS::getVal8(uint32_t key, uint8_t layer, uint16_t maxWait) // Unsafe overload
{
  uint8_t result = 0;
  getVal8(key, &result, layer, maxWait);
  return result;
}
bool DevUBLOXGNSS::getValSigned8(uint32_t key, int8_t *val, uint8_t layer, uint16_t maxWait)
{
  bool result = getVal(key, layer, maxWait) == SFE_UBLOX_STATUS_DATA_RECEIVED;
  if (result)
    *val = extractSignedChar(&packetCfg, 8);
  return result;
}

bool DevUBLOXGNSS::getVal16(uint32_t key, uint16_t *val, uint8_t layer, uint16_t maxWait)
{
  bool result = getVal(key, layer, maxWait) == SFE_UBLOX_STATUS_DATA_RECEIVED;
  if (result)
    *val = extractInt(&packetCfg, 8);
  return result;
}
uint16_t DevUBLOXGNSS::getVal16(uint32_t key, uint8_t layer, uint16_t maxWait) // Unsafe overload
{
  uint16_t result = 0;
  getVal16(key, &result, layer, maxWait);
  return result;
}
bool DevUBLOXGNSS::getValSigned16(uint32_t key, int16_t *val, uint8_t layer, uint16_t maxWait)
{
  bool result = getVal(key, layer, maxWait) == SFE_UBLOX_STATUS_DATA_RECEIVED;
  if (result)
    *val = extractSignedInt(&packetCfg, 8);
  return result;
}

bool DevUBLOXGNSS::getVal32(uint32_t key, uint32_t *val, uint8_t layer, uint16_t maxWait)
{
  bool result = getVal(key, layer, maxWait) == SFE_UBLOX_STATUS_DATA_RECEIVED;
  if (result)
    *val = extractLong(&packetCfg, 8);
  return result;
}
uint32_t DevUBLOXGNSS::getVal32(uint32_t key, uint8_t layer, uint16_t maxWait) // Unsafe overload
{
  uint32_t result = 0;
  getVal32(key, &result, layer, maxWait);
  return result;
}
bool DevUBLOXGNSS::getValSigned32(uint32_t key, int32_t *val, uint8_t layer, uint16_t maxWait)
{
  bool result = getVal(key, layer, maxWait) == SFE_UBLOX_STATUS_DATA_RECEIVED;
  if (result)
    *val = extractSignedLong(&packetCfg, 8);
  return result;
}

bool DevUBLOXGNSS::getVal64(uint32_t key, uint64_t *val, uint8_t layer, uint16_t maxWait)
{
  bool result = getVal(key, layer, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED;
  if (result)
    *val = extractLongLong(&packetCfg, 8);
  return result;
}
uint64_t DevUBLOXGNSS::getVal64(uint32_t key, uint8_t layer, uint16_t maxWait) // Unsafe overload
{
  uint64_t result = 0;
  getVal64(key, &result, layer, maxWait);
  return result;
}
bool DevUBLOXGNSS::getValSigned64(uint32_t key, int64_t *val, uint8_t layer, uint16_t maxWait)
{
  bool result = getVal(key, layer, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED;
  if (result)
    *val = extractSignedLongLong(&packetCfg, 8);
  return result;
}

bool DevUBLOXGNSS::getValFloat(uint32_t key, float *val, uint8_t layer, uint16_t maxWait)
{
  if (sizeof(float) != 4)
    return false;

  bool result = getVal(key, layer, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED;
  if (result)
    *val = extractFloat(&packetCfg, 8);
  return result;
}

bool DevUBLOXGNSS::getValDouble(uint32_t key, double *val, uint8_t layer, uint16_t maxWait)
{
  if (sizeof(double) != 8)
    return false;

  bool result = getVal(key, layer, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED;
  if (result)
    *val = extractDouble(&packetCfg, 8);
  return result;
}

// Given a key, set a N-byte value
// This function takes a full 32-bit key
// Default layer is RAM+BBR
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
bool DevUBLOXGNSS::setValN(uint32_t key, uint8_t *value, uint8_t N, uint8_t layer, uint16_t maxWait)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_VALSET;
  packetCfg.len = 4 + 4 + N; // 4 byte header, 4 byte key ID, N bytes of value
  packetCfg.startingSpot = 0;

  // Clear packet payload
  memset(payloadCfg, 0, packetCfg.len);

  payloadCfg[0] = 0;     // Message Version - set to 0
  payloadCfg[1] = layer; // By default we ask for the BBR layer

  // Load key into outgoing payload
  key &= ~UBX_CFG_SIZE_MASK; // Mask off the size identifer bits
  for (uint8_t i = 0; i < 4; i++)
    payloadCfg[i + 4] = key >> (8 * i); // Key

  // Load user's value
  for (uint8_t i = 0; i < N; i++)
    payloadCfg[i + 8] = *value++;

  // Send VALSET command with this key and value
  return (sendCommand(&packetCfg, maxWait) == SFE_UBLOX_STATUS_DATA_SENT); // We are only expecting an ACK
}

// Given a key, set an 8-bit value
// This function takes a full 32-bit key
// Default layer is RAM+BBR
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
bool DevUBLOXGNSS::setVal8(uint32_t key, uint8_t value, uint8_t layer, uint16_t maxWait)
{
  uint8_t val[1] = {value};
  return (setValN(key, val, 1, layer, maxWait));
}
bool DevUBLOXGNSS::setValSigned8(uint32_t key, int8_t value, uint8_t layer, uint16_t maxWait)
{
  unsignedSigned8 converter;
  converter.signed8 = value;
  return (setVal8(key, converter.unsigned8, layer, maxWait));
}

// Given a key, set a 16-bit value
// This function takes a full 32-bit key
// Default layer is RAM+BBR
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
bool DevUBLOXGNSS::setVal16(uint32_t key, uint16_t value, uint8_t layer, uint16_t maxWait)
{
  uint8_t val[2] = {(uint8_t)(value >> 0), (uint8_t)(value >> 8)};
  return (setValN(key, val, 2, layer, maxWait));
}
bool DevUBLOXGNSS::setValSigned16(uint32_t key, int16_t value, uint8_t layer, uint16_t maxWait)
{
  unsignedSigned16 converter;
  converter.signed16 = value;
  return (setVal16(key, converter.unsigned16, layer, maxWait));
}

// Given a key, set a 32-bit value
// This function takes a full 32-bit key
// Default layer is RAM+BBR
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
bool DevUBLOXGNSS::setVal32(uint32_t key, uint32_t value, uint8_t layer, uint16_t maxWait)
{
  uint8_t val[4] = {(uint8_t)(value >> 0), (uint8_t)(value >> 8), (uint8_t)(value >> 16), (uint8_t)(value >> 24)};
  return (setValN(key, val, 4, layer, maxWait));
}
bool DevUBLOXGNSS::setValSigned32(uint32_t key, int32_t value, uint8_t layer, uint16_t maxWait)
{
  unsignedSigned32 converter;
  converter.signed32 = value;
  return (setVal32(key, converter.unsigned32, layer, maxWait));
}

// Given a key, set a 64-bit value
// This function takes a full 32-bit key
// Default layer is RAM+BBR
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
bool DevUBLOXGNSS::setVal64(uint32_t key, uint64_t value, uint8_t layer, uint16_t maxWait)
{
  uint8_t val[8];

  // Load user's value
  for (uint8_t i = 0; i < 8; i++)
    val[i] = (uint8_t)(value >> (8 * i)); // Value

  return (setValN(key, val, 8, layer, maxWait));
}
bool DevUBLOXGNSS::setValSigned64(uint32_t key, int64_t value, uint8_t layer, uint16_t maxWait)
{
  unsignedSigned64 converter;
  converter.signed64 = value;
  return (setVal64(key, converter.unsigned64, layer, maxWait));
}

bool DevUBLOXGNSS::setValFloat(uint32_t key, float value, uint8_t layer, uint16_t maxWait)
{
  if (sizeof(float) != 4)
  {
    debugPrintln("setValFloat not supported!", true); // Important
    return false;
  }
  unsigned32float converter;
  converter.flt = value;
  return (setVal32(key, converter.unsigned32, layer, maxWait));
}

bool DevUBLOXGNSS::setValDouble(uint32_t key, double value, uint8_t layer, uint16_t maxWait)
{
  if (sizeof(double) != 8)
  {
    debugPrintln("setValDouble not supported!", true); // Important
    return false;
  }
  unsigned64double converter;
  converter.dbl = value;
  return (setVal64(key, converter.unsigned64, layer, maxWait));
}

// Start defining a new (empty) UBX-CFG-VALSET ubxPacket
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
bool DevUBLOXGNSS::newCfgValset(uint8_t layer)
{
  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_VALSET;
  packetCfg.len = 4; // 4 byte header
  packetCfg.startingSpot = 0;

  _numCfgKeys = 0;

  // Clear all of packet payload
  memset(payloadCfg, 0, packetCfgPayloadSize);

  payloadCfg[0] = 0;     // Message Version - set to 0
  payloadCfg[1] = layer; // By default we ask for the BBR layer

  // All done
  return (true);
}

// Add another key and value to an existing UBX-CFG-VALSET ubxPacket
// This function takes a full 32-bit key and N-byte value
bool DevUBLOXGNSS::addCfgValsetN(uint32_t key, uint8_t *value, uint8_t N)
{
  if ((_autoSendAtSpaceRemaining > 0) && (packetCfg.len >= (packetCfgPayloadSize - _autoSendAtSpaceRemaining)))
  {
    debugPrintln("addCfgValsetN: autosend", true); // Important
    if (sendCommand(&packetCfg) != SFE_UBLOX_STATUS_DATA_SENT) // We are only expecting an ACK
      return false;
    packetCfg.len = 4; // 4 byte header
    packetCfg.startingSpot = 0;
    _numCfgKeys = 0;
    memset(&payloadCfg[4], 0, packetCfgPayloadSize - 4);
  }

  if (packetCfg.len >= (packetCfgPayloadSize - (4 + N)))
  {
    debugPrintln("addCfgValsetN: packetCfgPayloadSize reached!", true); // Important
    return false;
  }

  if (_numCfgKeys == CFG_VALSET_MAX_KEYS)
  {
    debugPrintln("addCfgValsetN: key limit reached!", true); // Important
    return false;
  }

  // Load key into outgoing payload
  key &= ~UBX_CFG_SIZE_MASK; // Mask off the size identifer bits
  for (uint8_t i = 0; i < 4; i++)
    payloadCfg[packetCfg.len + i] = key >> (8 * i); // Key

  // Load user's value
  for (uint8_t i = 0; i < N; i++)
    payloadCfg[packetCfg.len + i + 4] = *value++; // Value

  // Update packet length: 4 byte key ID, 8 bytes of value
  packetCfg.len = packetCfg.len + 4 + N;

  _numCfgKeys++;

  // All done
  return (true);
}

// Send the UBX-CFG-VALSET ubxPacket
bool DevUBLOXGNSS::sendCfgValset(uint16_t maxWait)
{
  if (_numCfgKeys == 0)
    return true; // Nothing to send...

  // Send VALSET command with this key and value
  bool success = sendCommand(&packetCfg, maxWait) == SFE_UBLOX_STATUS_DATA_SENT; // We are only expecting an ACK

  if (success)
    _numCfgKeys = 0;

  return success;
}

// Return the number of keys in the CfgValset
uint8_t DevUBLOXGNSS::getCfgValsetLen()
{
  return _numCfgKeys;
}

// Return the number of free bytes remaining in packetCfgPayload
size_t DevUBLOXGNSS::getCfgValsetSpaceRemaining()
{
  return getPacketCfgSpaceRemaining();
}

bool DevUBLOXGNSS::newCfgValget(uint8_t layer) // Create a new, empty UBX-CFG-VALGET. Add entries with addCfgValget8/16/32/64
{
  return (newCfgValget(&packetCfg, packetCfgPayloadSize, layer));
}

bool DevUBLOXGNSS::newCfgValget(ubxPacket *pkt, uint16_t maxPayload, uint8_t layer) // Create a new, empty UBX-CFG-VALGET. Add entries with addCfgValget8/16/32/64
{
  if (cfgValgetValueSizes == nullptr) // Check if RAM has been allocated for cfgValgetValueSizes
  {
    cfgValgetValueSizes = new uint8_t[CFG_VALSET_MAX_KEYS];
  }

  _cfgValgetMaxPayload = maxPayload;

  pkt->cls = UBX_CLASS_CFG;
  pkt->id = UBX_CFG_VALGET;
  pkt->len = 4; // 4 byte header
  pkt->startingSpot = 0;

  _numGetCfgKeys = 0;
  _lenCfgValGetResponse = 0;

  // Clear all of packet payload
  if (pkt == &packetCfg)
  {
    memset(payloadCfg, 0, packetCfgPayloadSize);
  }
  else
  {
    // Custom packet: we don't know how large payload is, so only clear the two skip keys bytes
    pkt->payload[2] = 0; // Set the skip keys bytes to zero
    pkt->payload[3] = 0;
  }

  // VALGET uses different memory layer definitions to VALSET
  // because it can only return the value for one layer.
  // So we need to fiddle the layer here.
  // And just to complicate things further, the ZED-F9P only responds
  // correctly to layer 0 (RAM) and layer 7 (Default)!
  uint8_t getLayer = VAL_LAYER_DEFAULT; // 7 is the "Default Layer"
  if (layer == VAL_LAYER_RAM)           // Did the user request the RAM layer?
  {
    getLayer = 0; // Layer 0 is RAM
  }
  else if (layer == VAL_LAYER_BBR) // Did the user request the BBR layer?
  {
    getLayer = 1; // Layer 1 is BBR
  }
  else if (layer == VAL_LAYER_FLASH) // Did the user request the Flash layer?
  {
    getLayer = 2; // Layer 2 is Flash
  }

  pkt->payload[0] = 0;        // Message Version - set to 0
  pkt->payload[1] = getLayer; // Layer

  if (maxPayload < 9) // Sanity check - make sure there's room for a single L/U1 response
    return false;

  // All done
  return (true);
}

bool DevUBLOXGNSS::addCfgValget(uint32_t key) // Add a new key to an existing UBX-CFG-VALGET ubxPacket
{
  return (addCfgValget(&packetCfg, key));
}

bool DevUBLOXGNSS::addCfgValget(ubxPacket *pkt, uint32_t key) // Add a new key to an existing UBX-CFG-VALGET ubxPacket
{
  // Extract the value size
  uint8_t valueSizeBytes = getCfgValueSizeBytes(key);

  if (_lenCfgValGetResponse >= (_cfgValgetMaxPayload - (4 + (valueSizeBytes))))
  {
    debugPrintln("addCfgValget: packetCfgPayloadSize reached!", true); // Important
    return false;
  }

  if (_numGetCfgKeys == CFG_VALSET_MAX_KEYS)
  {
    debugPrintln("addCfgValget: key limit reached!", true); // Important
    return false;
  }

  // Store the value size in cfgValgetValueSizes
  if (cfgValgetValueSizes != nullptr)
  {
    cfgValgetValueSizes[_numGetCfgKeys] = valueSizeBytes;
  }

  // Load key into outgoing payload
  uint8_t *ptr;
  ptr = pkt->payload;
  ptr += pkt->len;
  key &= ~UBX_CFG_SIZE_MASK; // Mask off the size identifer bits
  for (uint8_t i = 0; i < 4; i++)
  {
    *ptr = key >> (8 * i); // Key
    ptr++;
  }

  // Update packet length: 4 byte key ID
  pkt->len += 4;

  _numGetCfgKeys++;
  _lenCfgValGetResponse += 4 + (valueSizeBytes); // 4 byte key ID, N byte value

  // All done
  return (true);
}

bool DevUBLOXGNSS::sendCfgValget(uint16_t maxWait) // Send the CfgValget (UBX-CFG-VALGET) construct
{
  return (sendCfgValget(&packetCfg, maxWait));
}

bool DevUBLOXGNSS::sendCfgValget(ubxPacket *pkt, uint16_t maxWait) // Send the CfgValget (UBX-CFG-VALGET) construct
{
  if (_numGetCfgKeys == 0)
    return true; // Nothing to send...

  // Send VALSET command with this key and value
  bool success = sendCommand(pkt, maxWait) == SFE_UBLOX_STATUS_DATA_RECEIVED; // We are expecting data and an ACK

  if (success)
    _numGetCfgKeys = 0;

  return success;
}

uint8_t DevUBLOXGNSS::getCfgValueSizeBytes(const uint32_t key)
{
  switch (key & UBX_CFG_SIZE_MASK)
  {
  case UBX_CFG_L:
  case UBX_CFG_U1:
  case UBX_CFG_I1:
  case UBX_CFG_E1:
  case UBX_CFG_X1:
    return 1;
    break;
  case UBX_CFG_U2:
  case UBX_CFG_I2:
  case UBX_CFG_E2:
  case UBX_CFG_X2:
    return 2;
    break;
  case UBX_CFG_U4:
  case UBX_CFG_I4:
  case UBX_CFG_E4:
  case UBX_CFG_X4:
  case UBX_CFG_R4:
    return 4;
    break;
  case UBX_CFG_U8:
  case UBX_CFG_I8:
  case UBX_CFG_X8:
  case UBX_CFG_R8:
    return 8;
    break;
  default:
    return 0; // Error
    break;
  }
  return 0;
}

//=-=-=-=-=-=-=-= "Automatic" Messages =-=-=-=-=-=-=-==-=-=-=-=-=-=-=
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Helper for all setAuto*rate functions that use VALSET (setVal8).
// Sets the message output rate and updates automaticFlags with a three-tier strategy:
//   1. If setVal8 succeeds: flags reflect the confirmed state
//   2. If setVal8 fails: read back the actual rate with getVal8 (ground truth)
//   3. If both fail (e.g. I2C buffer congestion): flags reflect the intended state,
//      preventing the silent-failure mode where getNAVPVT() etc. return false forever
bool DevUBLOXGNSS::setAutoMsgRateVal(uint32_t key, uint8_t rate, bool implicitUpdate, ubxAutomaticFlags &flags, uint8_t layer, uint16_t maxWait)
{
  bool ok = setVal8(key, rate, layer, maxWait);
  if (ok)
  {
    flags.flags.bits.automatic = (rate > 0);
    flags.flags.bits.implicitUpdate = implicitUpdate;
  }
  else
  {
    uint8_t actualRate;
    ok = getVal8(key, &actualRate, layer, maxWait);
    if (ok)
    {
      flags.flags.bits.automatic = (actualRate > 0);
    }
    else
    {
      flags.flags.bits.automatic = (rate > 0);
    }
    flags.flags.bits.implicitUpdate = implicitUpdate;
  }
  return ok;
}

bool DevUBLOXGNSS::getUBX(const char *Class, const char *ID, uint16_t maxWait)
{
  ubxMessage *msg = ubxMessages.findByName(Class, ID);
  if (msg)
    return getUBX(msg->_Class, msg->_ID, maxWait);

  return false;
}
bool DevUBLOXGNSS::getUBX(uint8_t Class, uint8_t ID, uint16_t maxWait)
{
  // v4 scaffolding - see AGENTS.md "Reference Scaffolding" and "moduleQueried"
  if (ubxMessages.initStorage(Class, ID) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  bool automatic = false;
  if (ubxMessages.isAutomatic(Class, ID, &automatic) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  bool implicitUpdate = false;
  if (ubxMessages.implicitUpdate(Class, ID, &implicitUpdate) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  if (automatic && implicitUpdate)
  {
    // The module is automatically reporting this message; just check whether we got unread data
    checkUbloxInternal(&packetCfg, 0, 0); // Parse any incoming data. Don't overwrite the requested Class and ID
    bool queried = false;
    if (ubxMessages.moduleQueried(Class, ID, &queried) != SFE_UBLOX_STATUS_SUCCESS)
      return false;
    if (queried) // Fresh data arrived - report it, then mark it read ("one-shot")
      ubxMessages.setModuleQueried(Class, ID, false);
    return queried;
  }
  else if (automatic && !implicitUpdate)
  {
    // Someone else has to call checkUblox for us...
    return false;
  }
  else
  {
    // Not automatic - poll explicitly for this specific Class/ID
    packetCfg.cls = Class;
    packetCfg.id = ID;
    packetCfg.len = 0;
    packetCfg.startingSpot = 0;

    // The data is parsed as part of processing the response
    sfe_ublox_status_e retVal = sendCommand(&packetCfg, maxWait);

    if ((retVal == SFE_UBLOX_STATUS_DATA_RECEIVED) || (retVal == SFE_UBLOX_STATUS_DATA_OVERWRITTEN))
    {
      // Again, we should treat this as a one-shot
      ubxMessages.setModuleQueried(Class, ID, false);
      return true;
    }
  }

  return false;
}

bool DevUBLOXGNSS::getUBXfield(uint8_t Class, uint8_t ID, const char *field, ubxAnyType *value)
{
  if (ubxMessages.initStorage(Class, ID) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  return (ubxMessages.extractValue(Class, ID, field, value) == SFE_UBLOX_STATUS_SUCCESS);
}

bool DevUBLOXGNSS::setAutoUBX(const char *Class, const char *ID, bool enabled, uint8_t layer, uint16_t maxWait)
{
  return setAutoUBXrate(Class, ID, enabled ? 1 : 0, true, layer, maxWait);
}
bool DevUBLOXGNSS::setAutoUBX(uint8_t Class, uint8_t ID, bool enabled, uint8_t layer, uint16_t maxWait)
{
  return setAutoUBXrate(Class, ID, enabled ? 1 : 0, true, layer, maxWait);
}
bool DevUBLOXGNSS::setAutoUBX(const char *Class, const char *ID, bool enabled, bool implicitUpdate, uint8_t layer, uint16_t maxWait)
{
  return setAutoUBXrate(Class, ID, enabled ? 1 : 0, implicitUpdate, layer, maxWait);
}
bool DevUBLOXGNSS::setAutoUBX(uint8_t Class, uint8_t ID, bool enabled, bool implicitUpdate, uint8_t layer, uint16_t maxWait)
{
  return setAutoUBXrate(Class, ID, enabled ? 1 : 0, implicitUpdate, layer, maxWait);
}
bool DevUBLOXGNSS::setAutoUBXrate(const char *Class, const char *ID, uint8_t rate, bool implicitUpdate, uint8_t layer, uint16_t maxWait)
{
  ubxMessage *msg = ubxMessages.findByName(Class, ID);
  if (msg)
    return setAutoUBXrate(msg->_Class, msg->_ID, rate, implicitUpdate, layer, maxWait);

  return false;
}
bool DevUBLOXGNSS::setAutoUBXrate(uint8_t Class, uint8_t ID, uint8_t rate, bool implicitUpdate, uint8_t layer, uint16_t maxWait)
{
  if (ubxMessages.initStorage(Class, ID) != SFE_UBLOX_STATUS_SUCCESS) // Only attempt this if RAM allocation was successful
    return false;

  if (rate > 127)
    rate = 127;

  uint32_t key;
  if (ubxMessages.getMsgOutKey(Class, ID, _commType, &key) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  // Sets the message output rate and updates automaticFlags with a three-tier strategy:
  //   1. If setVal8 succeeds: flags reflect the confirmed state
  //   2. If setVal8 fails: read back the actual rate with getVal8 (ground truth)
  //   3. If both fail (e.g. I2C buffer congestion): flags reflect the intended state,
  //      preventing the silent-failure mode where getNAVPVT() etc. return false forever
  bool ok = setVal8(key, rate, layer, maxWait);
  if (ok)
  {
    ubxMessages.setModuleQueried(Class, ID, false);
    ubxMessages.setAutomatic(Class, ID, (rate > 0));
    ubxMessages.setImplicitUpdate(Class, ID, implicitUpdate);
  }
  else
  {
    uint8_t actualRate;
    ok = getVal8(key, &actualRate, layer, maxWait);
    if (ok)
    {
      ubxMessages.setAutomatic(Class, ID, (actualRate > 0));
    }
    else
    {
      ubxMessages.setAutomatic(Class, ID, (rate > 0));
    }
    ubxMessages.setModuleQueried(Class, ID, false);
    ubxMessages.setImplicitUpdate(Class, ID, implicitUpdate);
  }
  return ok;
}
bool DevUBLOXGNSS::assumeAutoUBX(const char *Class, const char *ID, bool enabled, bool implicitUpdate)
{
  ubxMessage *msg = ubxMessages.findByName(Class, ID);
  if (msg)
    return assumeAutoUBX(msg->_Class, msg->_ID, enabled, implicitUpdate);

  return false;
}
bool DevUBLOXGNSS::assumeAutoUBX(uint8_t Class, uint8_t ID, bool enabled, bool implicitUpdate)
{
  if (ubxMessages.initStorage(Class, ID) != SFE_UBLOX_STATUS_SUCCESS) // Only attempt this if RAM allocation was successful
    return false;

  bool automatic = false;
  if (ubxMessages.isAutomatic(Class, ID, &automatic) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  bool implicit = false;
  if (ubxMessages.implicitUpdate(Class, ID, &implicit) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  bool changes = automatic != enabled || implicit != implicitUpdate;
  if (changes)
  {
    ubxMessages.setAutomatic(Class, ID, enabled);
    ubxMessages.setImplicitUpdate(Class, ID, implicitUpdate);
  }

  return changes;
}
void DevUBLOXGNSS::flushUBX(const char *Class, const char *ID)
{
  ubxMessage *msg = ubxMessages.findByName(Class, ID);
  if (msg)
    flushUBX(msg->_Class, msg->_ID);
}
void DevUBLOXGNSS::flushUBX(uint8_t Class, uint8_t ID)
{
  ubxMessages.setModuleQueried(Class, ID, false);
}
void DevUBLOXGNSS::logUBX(const char *Class, const char *ID, bool enabled)
{
  ubxMessage *msg = ubxMessages.findByName(Class, ID);
  if (msg)
    logUBX(msg->_Class, msg->_ID, enabled);
}
void DevUBLOXGNSS::logUBX(uint8_t Class, uint8_t ID, bool enabled)
{
  ubxMessages.setAddToFileBuffer(Class, ID, enabled);
}

// v4 NMEA scaffolding

bool DevUBLOXGNSS::getNMEA(const char *msgId, uint16_t maxWait)
{
  if (nmeaMessages.initStorage(msgId) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  bool automatic = false; // We could / should probably use isThisNMEAauto() here...?
  if (nmeaMessages.isAutomatic(msgId, &automatic) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  bool implicitUpdate = false;
  if (nmeaMessages.implicitUpdate(msgId, &implicitUpdate) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  if (automatic && implicitUpdate)
  {
    // The module is automatically reporting this message; just check whether we got unread data
    checkUbloxInternal(&packetCfg, 0, 0); // Parse any incoming data. Don't overwrite the requested Class and ID
    bool queried = false;
    if (nmeaMessages.moduleQueried(msgId, &queried) != SFE_UBLOX_STATUS_SUCCESS)
      return false;
    if (queried) // Fresh data arrived - report it, then mark it read ("one-shot")
      nmeaMessages.setModuleQueried(msgId, false);
    return queried;
  }
  else if (automatic && !implicitUpdate)
  {
    // Someone else has to call checkUblox for us...
    return false;
  }
  else
  {
    // Not automatic - poll explicitly for this specific msgId using the GN talker ID

    // The data is parsed as part of processing the response
    sfe_ublox_status_e retVal = pollNMEA(msgId, maxWait);

    if ((retVal == SFE_UBLOX_STATUS_DATA_RECEIVED) || (retVal == SFE_UBLOX_STATUS_DATA_OVERWRITTEN))
    {
      // Again, we should treat this as a one-shot
      nmeaMessages.setModuleQueried(msgId, false);
      return true;
    }
  }

  return false;
}

bool DevUBLOXGNSS::getNMEAfield(const char *msgId, const char *field, sfe_string_t &value)
{
  if (nmeaMessages.initStorage(msgId) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  return (nmeaMessages.extractValue(msgId, field, value) == SFE_UBLOX_STATUS_SUCCESS);
}

bool DevUBLOXGNSS::setAutoNMEA(const char *msgId, bool enabled, uint8_t layer, uint16_t maxWait)
{
  return setAutoNMEArate(msgId, enabled ? 1 : 0, true, layer, maxWait);
}
bool DevUBLOXGNSS::setAutoNMEA(const char *msgId, bool enabled, bool implicitUpdate, uint8_t layer, uint16_t maxWait)
{
  return setAutoNMEArate(msgId, enabled ? 1 : 0, implicitUpdate, layer, maxWait);
}
bool DevUBLOXGNSS::setAutoNMEArate(const char *msgId, uint8_t rate, bool implicitUpdate, uint8_t layer, uint16_t maxWait)
{
  if (nmeaMessages.initStorage(msgId) != SFE_UBLOX_STATUS_SUCCESS) // Only attempt this if RAM allocation was successful
    return false;

  if (rate > 127)
    rate = 127;

  uint32_t key;
  if (nmeaMessages.getMsgOutKey(msgId, _commType, &key) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  // Sets the message output rate and updates automaticFlags with a three-tier strategy:
  //   1. If setVal8 succeeds: flags reflect the confirmed state
  //   2. If setVal8 fails: read back the actual rate with getVal8 (ground truth)
  //   3. If both fail (e.g. I2C buffer congestion): flags reflect the intended state,
  //      preventing the silent-failure mode where getNAVPVT() etc. return false forever
  bool ok = setVal8(key, rate, layer, maxWait);
  if (ok)
  {
    nmeaMessages.setModuleQueried(msgId, false);
    nmeaMessages.setAutomatic(msgId, (rate > 0));
    nmeaMessages.setImplicitUpdate(msgId, implicitUpdate);
  }
  else
  {
    uint8_t actualRate;
    ok = getVal8(key, &actualRate, layer, maxWait);
    if (ok)
    {
      nmeaMessages.setAutomatic(msgId, (actualRate > 0));
    }
    else
    {
      nmeaMessages.setAutomatic(msgId, (rate > 0));
    }
    nmeaMessages.setModuleQueried(msgId, false);
    nmeaMessages.setImplicitUpdate(msgId, implicitUpdate);
  }
  return ok;
}
bool DevUBLOXGNSS::assumeAutoNMEA(const char *msgId, bool enabled, bool implicitUpdate)
{
  if (nmeaMessages.initStorage(msgId) != SFE_UBLOX_STATUS_SUCCESS) // Only attempt this if RAM allocation was successful
    return false;

  bool automatic = false;
  if (nmeaMessages.isAutomatic(msgId, &automatic) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  bool implicit = false;
  if (nmeaMessages.implicitUpdate(msgId, &implicit) != SFE_UBLOX_STATUS_SUCCESS)
    return false;

  bool changes = automatic != enabled || implicit != implicitUpdate;
  if (changes)
  {
    nmeaMessages.setAutomatic(msgId, enabled);
    nmeaMessages.setImplicitUpdate(msgId, implicitUpdate);
  }

  return changes;
}
void DevUBLOXGNSS::flushNMEA(const char *msgId)
{
  nmeaMessages.setModuleQueried(msgId, false);
}
void DevUBLOXGNSS::logNMEA(const char *msgId, bool enabled)
{
  nmeaMessages.setAddToFileBuffer(msgId, enabled);
}

// UBX-NAV-SAT is now a registered v4 message (ubxNAVSAT) - see AGENTS.md "Adding the
// variable-length UBX messages". setAutoNAVSAT/setAutoNAVSATrate/assumeAutoNAVSAT/flushNAVSAT/
// logNAVSAT/setAutoNAVSATcallbackPtr/initPacketUBXNAVSAT are retired; the generic
// setAutoUBX/setAutoUBXrate/assumeAutoUBX/flushUBX/logUBX/setAutoCallbackPtr (by Class/ID
// UBX_CLASS_NAV/UBX_NAV_SAT, or by name "NAV"/"SAT") do the same job for every registered
// message, NAV-SAT included, with no per-message code required.
bool DevUBLOXGNSS::getNAVSAT(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_SAT, maxWait);
}

// UBX-NAV-SIG is now a registered v4 message (ubxNAVSIG) - see AGENTS.md "Adding the
// variable-length UBX messages". setAutoNAVSIG/setAutoNAVSIGrate/assumeAutoNAVSIG/flushNAVSIG/
// logNAVSIG/setAutoNAVSIGcallbackPtr/initPacketUBXNAVSIG are retired; the generic
// setAutoUBX/setAutoUBXrate/assumeAutoUBX/flushUBX/logUBX/setAutoCallbackPtr (by Class/ID
// UBX_CLASS_NAV/UBX_NAV_SIG, or by name "NAV"/"SIG") do the same job for every registered
// message, NAV-SIG included, with no per-message code required.
bool DevUBLOXGNSS::getNAVSIG(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_SIG, maxWait);
}

// UBX-RXM-PMP is now a registered v4 message (ubxRXMPMP) - see AGENTS.md "Adding support for
// RXM-PMP". setRXMPMPcallbackPtr()/initPacketUBXRXMPMP()/setRXMPMPmessageCallbackPtr()/
// initPacketUBXRXMPMPmessage() are retired; use the generic setAutoCallbackPtr() instead (by
// name "RXM"/"PMP").

// UBX-RXM-QZSSL6 is now a registered v4 message (ubxRXMQZSSL6) - see AGENTS.md "Adding
// support for RXM-QZSSL6". setRXMQZSSL6messageCallbackPtr/initPacketUBXRXMQZSSL6message are
// retired; use the generic setAutoCallbackPtr() instead (by name "RXM"/"QZSSL6"), then
// getUbxMessageFieldCallback()/getUbxMessageBlockFieldCallback()/
// getUbxMessageBlockCountCallback() to read the fields/msgBytes bytes. QZSSL6's old v3
// "push the whole message" use case is now covered generically, for ANY message with a
// callback registered, by getUbxMessageRawLengthCallback()/getUbxMessageRawPtrCallback()
// (added in Phase 30, for ESF-MEAS) - not reimplemented here, same treatment as RXM-PMP
// (Phase 32).

// UBX-RXM-SFRBX is now a registered v4 message (ubxRXMSFRBX) - see AGENTS.md "Adding support
// for RXM-SFRBX". setAutoRXMSFRBX/setAutoRXMSFRBXrate/setAutoRXMSFRBXcallbackPtr/
// assumeAutoRXMSFRBX/flushRXMSFRBX/logRXMSFRBX are retired; the generic setAutoUBX/
// setAutoUBXrate/assumeAutoUBX/flushUBX/logUBX/setAutoCallbackPtr (by Class/ID
// UBX_CLASS_RXM/UBX_RXM_SFRBX, or by name "RXM"/"SFRBX") do the same job, with the addition of
// a genuine multi-message ring buffer (numCallbackCopies == 14, see ubxRXMSFRBX.h) so a burst
// of SFRBX messages arriving within one checkUblox() call is no longer collapsed down to just
// the latest one.
//
// setAutoRXMSFRBXmessageCallbackPtr (the separate raw-full-message callback, for pushing the
// whole packet - including sync/checksum bytes - to e.g. the PointPerfect library) is also
// retired, per the RXM-SFRBX proposal's Part 3: registering ubxRXMSFRBX bypasses the legacy
// switch-based dispatch this callback relied on entirely, so it would otherwise keep compiling
// but silently never fire again. Nothing in examples/ used it. A ring-buffered replacement,
// following the same pattern as the parsed-field callback above, is future work if it's needed.

// Note: RXM-SFRBX is output-only. It cannot be polled. Strictly getRXMSFRBX should be
// deprecated - see issue #167 - but is kept, as a thin wrapper, for backward compatibility.
bool DevUBLOXGNSS::getRXMSFRBX(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_RXM, UBX_RXM_SFRBX, maxWait);
}


// UBX-RXM-RAWX and UBX-RXM-MEASX are now registered v4 messages (ubxRXMRAWX/ubxRXMMEASX) - see
// AGENTS.md "Adding the variable-length UBX messages". setAutoRXMRAWX/setAutoRXMRAWXrate/
// assumeAutoRXMRAWX/flushRXMRAWX/logRXMRAWX/setAutoRXMRAWXcallbackPtr and their RXM-MEASX
// equivalents are retired; the generic setAutoUBX/setAutoUBXrate/assumeAutoUBX/flushUBX/logUBX/
// setAutoCallbackPtr (by Class/ID UBX_CLASS_RXM/UBX_RXM_RAWX or UBX_CLASS_RXM/UBX_RXM_MEASX, or
// by name "RXM"/"RAWX" or "RXM"/"MEASX") do the same job for every registered message, RAWX and
// MEASX included, with no per-message code required.
bool DevUBLOXGNSS::getRXMRAWX(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_RXM, UBX_RXM_RAWX, maxWait);
}

bool DevUBLOXGNSS::getRXMMEASX(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_RXM, UBX_RXM_MEASX, maxWait);
}

// ***** MON COMMS automatic support
// ubxMONCOMMS is now self-registered - see AGENTS.md "Adding the variable-length UBX messages".
// setAutoMONCOMMS/setAutoMONCOMMSrate/setAutoMONCOMMScallbackPtr/assumeAutoMONCOMMS/
// initPacketUBXMONCOMMS/flushMONCOMMS/logMONCOMMS are retired; the generic setAutoUBX/
// setAutoUBXrate/setAutoCallbackPtr/assumeAutoUBX/flushUBX/logUBX (by Class/ID
// UBX_CLASS_MON/UBX_MON_COMMS, or by name "MON"/"COMMS") do the same job, with no per-message
// code required. getMONCOMMS() remains, as a thin wrapper, since it is called directly rather
// than by name.
bool DevUBLOXGNSS::getMONCOMMS(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_MON, UBX_MON_COMMS, maxWait);
}

// ubxMONRF is now self-registered - see AGENTS.md "Adding the variable-length UBX messages".
// Replaces the old getRFinformation(UBX_MON_RF_data_t*, ...); read fields via
// getUBXfield()/getUbxMessageBlockField() (with the "nBlocks" header field to bound the loop).
bool DevUBLOXGNSS::getMONRF(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_MON, UBX_MON_RF, maxWait);
}

// ***** ESF MEAS automatic support
// ubxESFMEAS is now self-registered - see AGENTS.md "Adding support for ESF-MEAS". The old
// setAutoESFMEAS/setAutoESFMEASrate/setAutoESFMEAScallbackPtr/assumeAutoESFMEAS/logESFMEAS
// declarations (formerly in u-blox_GNSS.h) had no definitions anywhere in this file - dead
// declarations, never callable - so there is nothing to retire here beyond removing them from the
// header. The generic setAutoUBX/setAutoUBXrate/setAutoCallbackPtr/assumeAutoUBX/flushUBX/logUBX
// (by Class/ID UBX_CLASS_ESF/UBX_ESF_MEAS, or by name "ESF"/"MEAS") do the same job, with no
// per-message code required. getESFMEAS() is new, as a thin wrapper, matching every other migrated
// message.
bool DevUBLOXGNSS::getESFMEAS(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_ESF, UBX_ESF_MEAS, maxWait);
}

// ***** ESF STATUS automatic support
// ubxESFSTATUS is now self-registered - see AGENTS.md "Adding support for ESF-RAW and
// ESF-STATUS". setAutoESFSTATUS/setAutoESFSTATUSrate/setAutoESFSTATUScallbackPtr/
// assumeAutoESFSTATUS/initPacketUBXESFSTATUS/flushESFSTATUS/logESFSTATUS are retired; the
// generic setAutoUBX/setAutoUBXrate/setAutoCallbackPtr/assumeAutoUBX/flushUBX/logUBX (by
// Class/ID UBX_CLASS_ESF/UBX_ESF_STATUS, or by name "ESF"/"STATUS") do the same job, with no
// per-message code required. getESFSTATUS() remains, as a thin wrapper, since it is called
// directly rather than by name (including from getEsfInfo() below).

bool DevUBLOXGNSS::getEsfInfo(uint16_t maxWait)
{
  return (getESFSTATUS(maxWait));
}

bool DevUBLOXGNSS::getESFSTATUS(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_ESF, UBX_ESF_STATUS, maxWait);
}

// ***** SEC-SIG automatic support
// ubxSECSIG (Version 3 - see ubxSECSIG.h) is now self-registered - see AGENTS.md "Adding the
// variable-length UBX messages". setAutoSECSIG/setAutoSECSIGrate/setAutoSECSIGcallbackPtr/assumeAutoSECSIG/
// initPacketUBXSECSIG/flushSECSIG/logSECSIG are retired; the generic setAutoUBX/setAutoUBXrate/
// setAutoCallbackPtr/assumeAutoUBX/flushUBX/logUBX (by Class/ID UBX_CLASS_SEC/UBX_SEC_SIG, or by
// name "SEC"/"SIG") do the same job, with no per-message code required. getSECSIG() remains, as
// a thin wrapper, since it is called directly rather than by name. The UBX_SEC_SIG_data_t*
// overload is redacted, per explicit instruction.
bool DevUBLOXGNSS::getSECSIG(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_SEC, UBX_SEC_SIG, maxWait);
}

// ***** Helper Functions for NMEA Logging / Processing

// Set the mainTalkerId used by NMEA messages - allows all NMEA messages except GSV to be prefixed with GP instead of GN
bool DevUBLOXGNSS::setMainTalkerID(sfe_ublox_talker_ids_e id, uint8_t layer, uint16_t maxWait)
{
  return (setVal8(UBLOX_CFG_NMEA_MAINTALKERID, (uint8_t)id, layer, maxWait));
}

// Enable/Disable NMEA High Precision Mode - include extra decimal places in the Lat and Lon
bool DevUBLOXGNSS::setHighPrecisionMode(bool enable, uint8_t layer, uint16_t maxWait)
{
  return (setVal8(UBLOX_CFG_NMEA_HIGHPREC, (uint8_t)enable, layer, maxWait));
}

// Log selected NMEA messages to file buffer - if the messages are enabled and if the file buffer exists
// User needs to call setFileBufferSize before .begin
void DevUBLOXGNSS::setNMEALoggingMask(uint32_t messages)
{
  _logNMEA.all = messages;
}
uint32_t DevUBLOXGNSS::getNMEALoggingMask()
{
  return (_logNMEA.all);
}

// Pass selected NMEA messages to processNMEA
void DevUBLOXGNSS::setProcessNMEAMask(uint32_t messages)
{
  _processNMEA.all = messages;
}
uint32_t DevUBLOXGNSS::getProcessNMEAMask()
{
  return (_processNMEA.all);
}

// Private: allocate RAM for incoming non-Auto NMEA messages and initialize it
bool DevUBLOXGNSS::initStorageNMEA()
{
  if (_storageNMEA != nullptr) // Check if storage already exists
    return true;

  _storageNMEA = new NMEA_STORAGE_t; // Allocate RAM for the main struct
  if (_storageNMEA == nullptr)
  {
    debugPrintln("initStorageNMEA: RAM alloc failed!", true); // Important
    return (false);
  }
  _storageNMEA->data = nullptr;

  _storageNMEA->data = new uint8_t[maxNMEAByteCount];
  if (_storageNMEA->data == nullptr)
  {
    debugPrintln("initStorageNMEA: RAM alloc failed!", true); // Important
    return (false);
  }

  return (true);
}

// ***** RTCM Auto Support

// Log selected RTCM messages to file buffer - if the messages are enabled and if the file buffer exists
// User needs to call setFileBufferSize before .begin
bool DevUBLOXGNSS::setRTCMLoggingMask(uint32_t messages)
{
  _logRTCM.all = messages;
  return (initStorageRTCM());
}
uint32_t DevUBLOXGNSS::getRTCMLoggingMask()
{
  return (_logRTCM.all);
}

// Private: allocate RAM for incoming RTCM messages and initialize it
bool DevUBLOXGNSS::initStorageRTCM()
{
  if (_storageRTCM != nullptr) // Check if storage already exists
    return true;

  _storageRTCM = new RTCM_FRAME_t; // Allocate RAM for the main struct
  if (_storageRTCM == nullptr)
  {
    debugPrintln("initStorageRTCM: RAM alloc failed!", true); // Important
    return (false);
  }

  return (true);
}

void DevUBLOXGNSS::crc24q(uint8_t incoming, uint32_t *checksum)
{
  uint32_t crc = *checksum; // Seed is 0

  crc ^= ((uint32_t)incoming) << 16; // XOR-in incoming

  for (uint8_t i = 0; i < 8; i++)
  {
    crc <<= 1;
    if (crc & 0x1000000)
      // CRC-24Q Polynomial:
      // gi = 1 for i = 0, 1, 3, 4, 5, 6, 7, 10, 11, 14, 17, 18, 23, 24
      // 0b 1 1000 0110 0100 1100 1111 1011
      crc ^= 0x1864CFB; // CRC-24Q
  }

  *checksum = crc & 0xFFFFFF;
}

// Return the most recent RTCM 1005: 0 = no data, 1 = stale data, 2 = fresh data
uint8_t DevUBLOXGNSS::getLatestRTCM1005(RTCM_1005_data_t *data)
{
  if (!initStorageRTCM())
    return 0;
  if (!initStorageRTCM1005())
    return 0;

  checkUbloxInternal(&packetCfg, 0, 0); // Call checkUbloxInternal to parse any incoming data. Don't overwrite the requested Class and ID

  memcpy(data, &storageRTCM1005->data, sizeof(RTCM_1005_data_t)); // Copy the complete copy

  uint8_t result = 0;
  if (storageRTCM1005->automaticFlags.flags.bits.dataValid == 1) // Is the copy valid?
  {
    result = 1;
    if (storageRTCM1005->automaticFlags.flags.bits.dataRead == 0) // Has the data already been read?
    {
      result = 2;
      storageRTCM1005->automaticFlags.flags.bits.dataRead = 1; // Mark the data as read
    }
  }

  return (result);
}

bool DevUBLOXGNSS::setRTCM1005callbackPtr(void (*callbackPointerPtr)(RTCM_1005_data_t *))
{
  if (!initStorageRTCM())
    return false;
  if (!initStorageRTCM1005())
    return false;

  if (storageRTCM1005->callbackData == nullptr) // Check if RAM has been allocated for the callback copy
  {
    storageRTCM1005->callbackData = new RTCM_1005_data_t;
  }

  if (storageRTCM1005->callbackData == nullptr)
  {
    debugPrintln("setRTCM1005callbackPtr: RAM alloc failed!", true); // Important
    return (false);
  }

  storageRTCM1005->callbackPointerPtr = callbackPointerPtr;
  return (true);
}

// Private: allocate RAM for incoming RTCM 1005 messages and initialize it
bool DevUBLOXGNSS::initStorageRTCM1005()
{
  if (storageRTCM1005 != nullptr) // Check if storage already exists
    return true;

  storageRTCM1005 = new RTCM_1005_t; // Allocate RAM for the main struct
  if (storageRTCM1005 == nullptr)
  {
    debugPrintln("initStorageRTCM1005: RAM alloc failed!", true); // Important
    return (false);
  }

  storageRTCM1005->callbackPointerPtr = nullptr; // Clear the callback pointers
  storageRTCM1005->callbackData = nullptr;

  storageRTCM1005->automaticFlags.flags.all = 0; // Mark the data as invalid/stale and unread

  return (true);
}

// Return the most recent RTCM 1005 Input - from pushRawData: 0 = no data, 1 = stale data, 2 = fresh data
uint8_t DevUBLOXGNSS::getLatestRTCM1005Input(RTCM_1005_data_t *data)
{
  memcpy(data, &rtcmInputStorage.rtcm1005, sizeof(RTCM_1005_data_t)); // Copy the complete message

  uint8_t result = 0;
  if (rtcmInputStorage.flags.bits.dataValid1005 == 1) // Is the data valid?
  {
    result = 1;
    if (rtcmInputStorage.flags.bits.dataRead1005 == 0) // Has the data already been read?
    {
      result = 2;
      rtcmInputStorage.flags.bits.dataRead1005 = 1; // Mark the data as read
    }
  }

  return result;
}

// Return the most recent RTCM 1006 Input - from pushRawData: 0 = no data, 1 = stale data, 2 = fresh data
uint8_t DevUBLOXGNSS::getLatestRTCM1006Input(RTCM_1006_data_t *data)
{
  memcpy(data, &rtcmInputStorage.rtcm1006, sizeof(RTCM_1006_data_t)); // Copy the complete message

  uint8_t result = 0;
  if (rtcmInputStorage.flags.bits.dataValid1006 == 1) // Is the data valid?
  {
    result = 1;
    if (rtcmInputStorage.flags.bits.dataRead1006 == 0) // Has the data already been read?
    {
      result = 2;
      rtcmInputStorage.flags.bits.dataRead1006 = 1; // Mark the data as read
    }
  }

  return result;
}

// Configure a callback for RTCM 1005 Input - from pushRawData
void DevUBLOXGNSS::setRTCM1005InputcallbackPtr(void (*rtcm1005CallbackPointer)(RTCM_1005_data_t *))
{
  rtcmInputStorage.rtcm1005CallbackPointer = rtcm1005CallbackPointer;
}

// Configure a callback for RTCM 1006 Input - from pushRawData
void DevUBLOXGNSS::setRTCM1006InputcallbackPtr(void (*rtcm1006CallbackPointer)(RTCM_1006_data_t *))
{
  rtcmInputStorage.rtcm1006CallbackPointer = rtcm1006CallbackPointer;
}


// ***** CFG RATE Helper Functions

// Set the rate at which the module will give us an updated navigation solution
// Expects a number that is the updates per second. For example 1 = 1Hz, 2 = 2Hz, etc.
// Max is 40Hz(?!)
bool DevUBLOXGNSS::setNavigationFrequency(uint8_t navFreq, uint8_t layer, uint16_t maxWait)
{
  if (navFreq == 0) // Return now if navFreq is zero
    return (false);

  if (navFreq > 40)
    navFreq = 40; // Limit navFreq to 40Hz so i2cPollingWait is set correctly

  // Adjust the I2C polling timeout based on update rate
  // Do this even if the sendCommand fails
  i2cPollingWaitNAV = 1000 / (((int)navFreq) * 4);                                                // This is the number of ms to wait between checks for new I2C data. Max is 250. Min is 6.
  i2cPollingWait = i2cPollingWaitNAV < i2cPollingWaitHNR ? i2cPollingWaitNAV : i2cPollingWaitHNR; // Set i2cPollingWait to the lower of NAV and HNR

  uint16_t measurementRate = 1000 / navFreq;

  return setVal16(UBLOX_CFG_RATE_MEAS, measurementRate, layer, maxWait);
}

// Get the rate at which the module is outputting nav solutions
// Note: if the measurementRate (which is actually a period) is less than 1000ms, this will return zero
bool DevUBLOXGNSS::getNavigationFrequency(uint8_t *navFreq, uint8_t layer, uint16_t maxWait)
{
  uint16_t measurementRate = 0;

  bool result = getVal16(UBLOX_CFG_RATE_MEAS, &measurementRate, layer, maxWait);

  if ((result) && (measurementRate > 0))
    *navFreq = 1000 / measurementRate; // This may return an int when it's a float, but I'd rather not return 4 bytes

  return result;
}
uint8_t DevUBLOXGNSS::getNavigationFrequency(uint8_t layer, uint16_t maxWait) // Unsafe overload...
{
  uint8_t navFreq = 0;

  getNavigationFrequency(&navFreq, layer, maxWait);

  return navFreq;
}

// Set the elapsed time between GNSS measurements in milliseconds, which defines the rate
bool DevUBLOXGNSS::setMeasurementRate(uint16_t rate, uint8_t layer, uint16_t maxWait)
{
  if (rate < 25) // "Measurement rate should be greater than or equal to 25 ms."
    rate = 25;

  // Adjust the I2C polling timeout based on update rate
  if (rate >= 1000)
    i2cPollingWaitNAV = 250;
  else
    i2cPollingWaitNAV = rate / 4;                                                                 // This is the number of ms to wait between checks for new I2C data
  i2cPollingWait = i2cPollingWaitNAV < i2cPollingWaitHNR ? i2cPollingWaitNAV : i2cPollingWaitHNR; // Set i2cPollingWait to the lower of NAV and HNR

  return setVal16(UBLOX_CFG_RATE_MEAS, rate, layer, maxWait);
}

// Return the elapsed time between GNSS measurements in milliseconds, which defines the rate
bool DevUBLOXGNSS::getMeasurementRate(uint16_t *measRate, uint8_t layer, uint16_t maxWait)
{
  uint16_t measurementRate;

  bool result = getVal16(UBLOX_CFG_RATE_MEAS, &measurementRate, layer, maxWait);

  if (result)
    *measRate = measurementRate;

  return result;
}
uint16_t DevUBLOXGNSS::getMeasurementRate(uint8_t layer, uint16_t maxWait) // Unsafe overload...
{
  uint16_t measurementRate = 0;

  getMeasurementRate(&measurementRate, layer, maxWait);

  return measurementRate;
}

// Set the ratio between the number of measurements and the number of navigation solutions. Unit is cycles. Max is 127.
bool DevUBLOXGNSS::setNavigationRate(uint16_t rate, uint8_t layer, uint16_t maxWait)
{
  return setVal16(UBLOX_CFG_RATE_NAV, rate, layer, maxWait);
}

// Return the ratio between the number of measurements and the number of navigation solutions. Unit is cycles
bool DevUBLOXGNSS::getNavigationRate(uint16_t *navRate, uint8_t layer, uint16_t maxWait)
{
  uint16_t navigationRate;

  bool result = getVal16(UBLOX_CFG_RATE_NAV, &navigationRate, layer, maxWait);

  if (result)
    *navRate = navigationRate;

  return result;
}
uint16_t DevUBLOXGNSS::getNavigationRate(uint8_t layer, uint16_t maxWait) // Unsafe overload...
{
  uint16_t navigationRate = 0;

  getNavigationRate(&navigationRate, layer, maxWait);

  return navigationRate;
}

// ***** DOP Helper Functions

bool DevUBLOXGNSS::getNAVDOP(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_DOP, maxWait);
}

uint16_t DevUBLOXGNSS::getGeometricDOP()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DOP, "gDOP", &value))
    return 0;
  return value.U2;
}

uint16_t DevUBLOXGNSS::getPositionDOP()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DOP, "pDOP", &value))
    return 0;
  return value.U2;
}

uint16_t DevUBLOXGNSS::getTimeDOP()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DOP, "tDOP", &value))
    return 0;
  return value.U2;
}

uint16_t DevUBLOXGNSS::getVerticalDOP()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DOP, "vDOP", &value))
    return 0;
  return value.U2;
}

uint16_t DevUBLOXGNSS::getHorizontalDOP()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DOP, "hDOP", &value))
    return 0;
  return value.U2;
}

uint16_t DevUBLOXGNSS::getNorthingDOP()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DOP, "nDOP", &value))
    return 0;
  return value.U2;
}

uint16_t DevUBLOXGNSS::getEastingDOP()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DOP, "eDOP", &value))
    return 0;
  return value.U2;
}

// ***** ATT Helper Functions

bool DevUBLOXGNSS::getNAVATT(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_ATT, maxWait);
}

float DevUBLOXGNSS::getATTroll() // Returned as degrees
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_ATT, "roll", &value))
    return 0;
  return ((float)value.I4 / 100000.0); // Convert to degrees
}

float DevUBLOXGNSS::getATTpitch() // Returned as degrees
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_ATT, "pitch", &value))
    return 0;
  return ((float)value.I4 / 100000.0); // Convert to degrees
}

float DevUBLOXGNSS::getATTheading() // Returned as degrees
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_ATT, "heading", &value))
    return 0;
  return ((float)value.I4 / 100000.0); // Convert to degrees
}

// ***** PVT Helper Functions

bool DevUBLOXGNSS::getNAVPVT(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_PVT, maxWait);
}

uint32_t DevUBLOXGNSS::getTimeOfWeek()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "iTOW", &value))
    return 0;
  return value.U4;
}

// Get the current year
uint16_t DevUBLOXGNSS::getYear()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "year", &value))
    return 0;
  return value.U2;
}

// Get the current month
uint8_t DevUBLOXGNSS::getMonth()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "month", &value))
    return 0;
  return value.U1;
}

// Get the current day
uint8_t DevUBLOXGNSS::getDay()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "day", &value))
    return 0;
  return value.U1;
}

// Get the current hour
uint8_t DevUBLOXGNSS::getHour()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "hour", &value))
    return 0;
  return value.U1;
}

// Get the current minute
uint8_t DevUBLOXGNSS::getMinute()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "min", &value))
    return 0;
  return value.U1;
}

// Get the current second
uint8_t DevUBLOXGNSS::getSecond()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "sec", &value))
    return 0;
  return value.U1;
}

// Get the current millisecond
uint16_t DevUBLOXGNSS::getMillisecond()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "iTOW", &value))
    return 0;
  return value.U4 % 1000;
}

// Get the current nanoseconds - includes milliseconds
int32_t DevUBLOXGNSS::getNanosecond()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "nano", &value))
    return 0;
  return value.I4;
}

// Get the current Unix epoch time rounded to the nearest second
uint32_t DevUBLOXGNSS::getUnixEpoch()
{
  uint32_t t = SFE_UBLOX_DAYS_FROM_1970_TO_2020;                                         // Jan 1st 2020 as days from Jan 1st 1970
  t += (uint32_t)SFE_UBLOX_DAYS_SINCE_2020[getYear() - 2020];                            // Add on the number of days since 2020
  t += (uint32_t)SFE_UBLOX_DAYS_SINCE_MONTH[getYear() % 4 == 0 ? 0 : 1][getMonth() - 1]; // Add on the number of days since Jan 1st
  t += (uint32_t)getDay() - 1;                                                           // Add on the number of days since the 1st of the month
  t *= 24;                                                                               // Convert to hours
  t += (uint32_t)getHour();                                                              // Add on the hour
  t *= 60;                                                                               // Convert to minutes
  t += (uint32_t)getMinute();                                                            // Add on the minute
  t *= 60;                                                                               // Convert to seconds
  t += (uint32_t)getSecond();                                                            // Add on the second
  return t;
}

// Get the current Unix epoch including microseconds
uint32_t DevUBLOXGNSS::getUnixEpoch(uint32_t &microsecond)
{
  uint32_t t = SFE_UBLOX_DAYS_FROM_1970_TO_2020;                                         // Jan 1st 2020 as days from Jan 1st 1970
  t += (uint32_t)SFE_UBLOX_DAYS_SINCE_2020[getYear() - 2020];                            // Add on the number of days since 2020
  t += (uint32_t)SFE_UBLOX_DAYS_SINCE_MONTH[getYear() % 4 == 0 ? 0 : 1][getMonth() - 1]; // Add on the number of days since Jan 1st
  t += (uint32_t)getDay() - 1;                                                           // Add on the number of days since the 1st of the month
  t *= 24;                                                                               // Convert to hours
  t += (uint32_t)getHour();                                                              // Add on the hour
  t *= 60;                                                                               // Convert to minutes
  t += (uint32_t)getMinute();                                                            // Add on the minute
  t *= 60;                                                                               // Convert to seconds
  t += (uint32_t)getSecond();                                                            // Add on the second
  int32_t us = getNanosecond() / 1000;                                                   // Convert nanos to micros
  microsecond = (uint32_t)us;                                                            // Could be -ve!
  // Adjust t if nano is negative
  if (us < 0)
  {
    microsecond = (uint32_t)(us + 1000000); // Make nano +ve
    t--;                                    // Decrement t by 1 second
  }
  return t;
}

// Get the current date validity
bool DevUBLOXGNSS::getDateValid()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "validDate", &value))
    return 0;
  return value.L;
}

// Get the current time validity
bool DevUBLOXGNSS::getTimeValid()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "validTime", &value))
    return 0;
  return value.L;
}

// Check to see if the UTC time has been fully resolved
bool DevUBLOXGNSS::getTimeFullyResolved()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "fullyResolved", &value))
    return 0;
  return value.L;
}

// Get the confirmed date validity
bool DevUBLOXGNSS::getConfirmedDate()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "confirmedDate", &value))
    return 0;
  return value.L;
}

// Get the confirmed time validity
bool DevUBLOXGNSS::getConfirmedTime()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "confirmedTime", &value))
    return 0;
  return value.L;
}

// Get the current fix type
// 0=no fix, 1=dead reckoning, 2=2D, 3=3D, 4=GNSS, 5=Time fix
uint8_t DevUBLOXGNSS::getFixType()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "fixType", &value))
    return 0;
  return value.U1;
}

// Get whether we have a valid fix (i.e within DOP & accuracy masks)
bool DevUBLOXGNSS::getGnssFixOk()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "gnssFixOK", &value))
    return 0;
  return value.L;
}

// Get whether differential corrections were applied
bool DevUBLOXGNSS::getDiffSoln()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "diffSoln", &value))
    return 0;
  return value.L;
}

// Get whether head vehicle valid or not
bool DevUBLOXGNSS::getHeadVehValid()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "headVehValid", &value))
    return 0;
  return value.L;
}

// Get the carrier phase range solution status
// Useful when querying module to see if it has high-precision RTK fix
// 0=No solution, 1=Float solution, 2=Fixed solution
uint8_t DevUBLOXGNSS::getCarrierSolutionType()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "carrSoln", &value))
    return 0;
  return value.U1;
}

// Get the number of satellites used in fix
uint8_t DevUBLOXGNSS::getSIV()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "numSV", &value))
    return 0;
  return value.U1;
}

// Get the current longitude in degrees
// Returns a long representing the number of degrees *10^-7
int32_t DevUBLOXGNSS::getLongitude()
{
  // v4 scaffolding: reads via the generic field accessor instead of packetUBXNAVPVT directly.
  // No longer self-polls or clears a per-field bit - see AGENTS.md "moduleQueried". Call getNAVPVT()
  // first if you need to ensure the value is fresh.
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "lon", &value))
    return 0;
  return value.I4;
}

// Get the current latitude in degrees
// Returns a long representing the number of degrees *10^-7
int32_t DevUBLOXGNSS::getLatitude()
{
  // v4 scaffolding: reads via the generic field accessor instead of packetUBXNAVPVT directly.
  // No longer self-polls or clears a per-field bit - see AGENTS.md "moduleQueried". Call getNAVPVT()
  // first if you need to ensure the value is fresh.
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "lat", &value))
    return 0;
  return value.I4;
}

// Get the current altitude in mm according to ellipsoid model
int32_t DevUBLOXGNSS::getAltitude()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "height", &value))
    return 0;
  return value.I4;
}

// Get the current altitude in mm according to mean sea level
// Ellipsoid model: https://www.esri.com/news/arcuser/0703/geoid1of3.html
// Difference between Ellipsoid Model and Mean Sea Level: https://eos-gnss.com/elevation-for-beginners/
// Also see: https://portal.u-blox.com/s/question/0D52p00008HKDSkCAP/what-geoid-model-is-used-and-where-is-this-calculated
// and: https://cddis.nasa.gov/926/egm96/egm96.html on 10x10 degree grid
int32_t DevUBLOXGNSS::getAltitudeMSL()
{
  // v4 scaffolding: reads via the generic field accessor instead of packetUBXNAVPVT directly.
  // No longer self-polls or clears a per-field bit - see AGENTS.md "moduleQueried". Call getNAVPVT()
  // first if you need to ensure the value is fresh.
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "hMSL", &value))
    return 0;
  return value.I4;
}

uint32_t DevUBLOXGNSS::getHorizontalAccEst()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "hAcc", &value))
    return 0;
  return value.U4;
}

uint32_t DevUBLOXGNSS::getVerticalAccEst()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "vAcc", &value))
    return 0;
  return value.U4;
}

int32_t DevUBLOXGNSS::getNedNorthVel()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "velN", &value))
    return 0;
  return value.I4;
}

int32_t DevUBLOXGNSS::getNedEastVel()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "velE", &value))
    return 0;
  return value.I4;
}

int32_t DevUBLOXGNSS::getNedDownVel()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "velD", &value))
    return 0;
  return value.I4;
}

// Get the ground speed in mm/s
int32_t DevUBLOXGNSS::getGroundSpeed()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "gSpeed", &value))
    return 0;
  return value.I4;
}

// Get the heading of motion (as opposed to heading of car) in degrees * 10^-5
int32_t DevUBLOXGNSS::getHeading()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "headMot", &value))
    return 0;
  return value.I4;
}

uint32_t DevUBLOXGNSS::getSpeedAccEst()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "sAcc", &value))
    return 0;
  return value.U4;
}

uint32_t DevUBLOXGNSS::getHeadingAccEst()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "headAcc", &value))
    return 0;
  return value.U4;
}

// Get the positional dillution of precision * 10^-2 (dimensionless)
uint16_t DevUBLOXGNSS::getPDOP()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "pDOP", &value))
    return 0;
  return value.U2;
}

bool DevUBLOXGNSS::getInvalidLlh()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "invalidLlh", &value))
    return 0;
  return value.L;
}

int32_t DevUBLOXGNSS::getHeadVeh()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "headVeh", &value))
    return 0;
  return value.I4;
}

int16_t DevUBLOXGNSS::getMagDec()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "magDec", &value))
    return 0;
  return value.I2;
}

uint16_t DevUBLOXGNSS::getMagAcc()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVT, "magAcc", &value))
    return 0;
  return value.U2;
}

// getGeoidSeparation is currently redundant. The geoid separation seems to only be provided in NMEA GGA and GNS messages.
int32_t DevUBLOXGNSS::getGeoidSeparation()
{
  return (0);
}

// ***** POSECEF Helper Functions

bool DevUBLOXGNSS::getNAVPOSECEF(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_POSECEF, maxWait);
}

// Get the current 3D high precision positional accuracy - a fun thing to watch
// Returns a long representing the 3D accuracy in millimeters
uint32_t DevUBLOXGNSS::getPositionAccuracyPOSECEF()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_POSECEF, "pAcc", &value))
    return 0;
  return value.U4;
}

// ***** HPPOSECEF Helper Functions

bool DevUBLOXGNSS::getNAVHPPOSECEF(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_HPPOSECEF, maxWait);
}

// Get the current 3D high precision positional accuracy - a fun thing to watch
// Returns a long representing the 3D accuracy in millimeters
uint32_t DevUBLOXGNSS::getPositionAccuracy()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSECEF, "pAcc", &value))
    return 0;

  uint32_t tempAccuracy = value.U4;

  if ((tempAccuracy % 10) >= 5)
    tempAccuracy += 5; // Round fraction of mm up to next mm if .5 or above
  tempAccuracy /= 10;  // Convert 0.1mm units to mm

  return (tempAccuracy);
}

// Get the current 3D high precision X coordinate
// Returns a long representing the coordinate in cm
int32_t DevUBLOXGNSS::getHighResECEFX()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSECEF, "ecefX", &value))
    return 0;
  return value.I4;
}

// Get the current 3D high precision Y coordinate
// Returns a long representing the coordinate in cm
int32_t DevUBLOXGNSS::getHighResECEFY()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSECEF, "ecefY", &value))
    return 0;
  return value.I4;
}

// Get the current 3D high precision Z coordinate
// Returns a long representing the coordinate in cm
int32_t DevUBLOXGNSS::getHighResECEFZ()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSECEF, "ecefZ", &value))
    return 0;
  return value.I4;
}

// Get the high precision component of the ECEF X coordinate
// Returns a signed byte representing the component as 0.1*mm
int8_t DevUBLOXGNSS::getHighResECEFXHp()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSECEF, "ecefXHp", &value))
    return 0;
  return value.I1;
}

// Get the high precision component of the ECEF Y coordinate
// Returns a signed byte representing the component as 0.1*mm
int8_t DevUBLOXGNSS::getHighResECEFYHp()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSECEF, "ecefYHp", &value))
    return 0;
  return value.I1;
}

// Get the high precision component of the ECEF Z coordinate
// Returns a signed byte representing the component as 0.1*mm
int8_t DevUBLOXGNSS::getHighResECEFZHp()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSECEF, "ecefZHp", &value))
    return 0;
  return value.I1;
}

// ***** HPPOSLLH Helper Functions

bool DevUBLOXGNSS::getNAVHPPOSLLH(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_HPPOSLLH, maxWait);
}

uint32_t DevUBLOXGNSS::getTimeOfWeekFromHPPOSLLH()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSLLH, "iTOW", &value))
    return 0;
  return value.U4;
}

int32_t DevUBLOXGNSS::getHighResLongitude()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSLLH, "lon", &value))
    return 0;
  return value.I4;
}

int32_t DevUBLOXGNSS::getHighResLatitude()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSLLH, "lat", &value))
    return 0;
  return value.I4;
}

int32_t DevUBLOXGNSS::getElipsoid()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSLLH, "height", &value))
    return 0;
  return value.I4;
}

int32_t DevUBLOXGNSS::getMeanSeaLevel()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSLLH, "hMSL", &value))
    return 0;
  return value.I4;
}

int8_t DevUBLOXGNSS::getHighResLongitudeHp()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSLLH, "lonHp", &value))
    return 0;
  return value.I1;
}

int8_t DevUBLOXGNSS::getHighResLatitudeHp()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSLLH, "latHp", &value))
    return 0;
  return value.I1;
}

int8_t DevUBLOXGNSS::getElipsoidHp()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSLLH, "heightHp", &value))
    return 0;
  return value.I1;
}

int8_t DevUBLOXGNSS::getMeanSeaLevelHp()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSLLH, "hMSLHp", &value))
    return 0;
  return value.I1;
}

uint32_t DevUBLOXGNSS::getHorizontalAccuracy()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSLLH, "hAcc", &value))
    return 0;
  return value.U4;
}

uint32_t DevUBLOXGNSS::getVerticalAccuracy()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_HPPOSLLH, "vAcc", &value))
    return 0;
  return value.U4;
}

// ***** PVAT Helper Functions

bool DevUBLOXGNSS::getNAVPVAT(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_PVAT, maxWait);
}

int32_t DevUBLOXGNSS::getVehicleRoll()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVAT, "vehRoll", &value))
    return 0;
  return value.I4;
}

int32_t DevUBLOXGNSS::getVehiclePitch()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVAT, "vehPitch", &value))
    return 0;
  return value.I4;
}

int32_t DevUBLOXGNSS::getVehicleHeading()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVAT, "vehHeading", &value))
    return 0;
  return value.I4;
}

int32_t DevUBLOXGNSS::getMotionHeading()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_PVAT, "motHeading", &value))
    return 0;
  return value.I4;
}

// ***** SVIN Helper Functions

bool DevUBLOXGNSS::getNAVSVIN(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_SVIN, maxWait);
}

bool DevUBLOXGNSS::getSurveyInActive()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_SVIN, "active", &value))
    return 0;
  return value.U1;
}

bool DevUBLOXGNSS::getSurveyInValid()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_SVIN, "valid", &value))
    return 0;
  return value.U1;
}

uint32_t DevUBLOXGNSS::getSurveyInObservationTimeFull() // Return the full uint32_t
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_SVIN, "dur", &value))
    return 0;
  return value.U4;
}

uint16_t DevUBLOXGNSS::getSurveyInObservationTime() // Truncated to 65535 seconds
{
  // dur (Passed survey-in observation time) is U4 (uint32_t) seconds. Here we truncate to 16 bits
  uint32_t tmpObsTime = getSurveyInObservationTimeFull();
  if (tmpObsTime <= 0xFFFF)
  {
    return ((uint16_t)tmpObsTime);
  }
  else
  {
    return (0xFFFF);
  }
}

float DevUBLOXGNSS::getSurveyInMeanAccuracy() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_SVIN, "meanAcc", &value))
    return 0;

  // meanAcc is U4 (uint32_t) in 0.1mm. We convert this to float.
  uint32_t tempFloat = value.U4;
  return (((float)tempFloat) / 10000.0); // Convert 0.1mm to m
}

// ***** TIMELS Helper Functions

bool DevUBLOXGNSS::getNAVTIMELS(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_TIMELS, maxWait);
}

int32_t DevUBLOXGNSS::getTimeToLsEvent()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_TIMELS, "timeToLsEvent", &value))
    return 0;
  return value.I4;
}

int8_t DevUBLOXGNSS::getCurrentLeapSeconds()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_TIMELS, "currLs", &value))
    return 0;
  return value.I1;
}

// ***** RELPOSNED Helper Functions and automatic support

bool DevUBLOXGNSS::getNAVRELPOSNED(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_RELPOSNED, maxWait);
}

float DevUBLOXGNSS::getRelPosN() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_RELPOSNED, "relPosN", &value))
    return 0;
  return (((float)value.I4) / 100.0); // Convert to m
}

float DevUBLOXGNSS::getRelPosE() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_RELPOSNED, "relPosE", &value))
    return 0;
  return (((float)value.I4) / 100.0); // Convert to m
}

float DevUBLOXGNSS::getRelPosD() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_RELPOSNED, "relPosD", &value))
    return 0;
  return (((float)value.I4) / 100.0); // Convert to m
}

float DevUBLOXGNSS::getRelPosAccN() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_RELPOSNED, "accN", &value))
    return 0;
  return (((float)value.U4) / 10000.0); // Convert to m
}

float DevUBLOXGNSS::getRelPosAccE() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_RELPOSNED, "accE", &value))
    return 0;
  return (((float)value.U4) / 10000.0); // Convert to m
}

float DevUBLOXGNSS::getRelPosAccD() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_RELPOSNED, "accD", &value))
    return 0;
  return (((float)value.U4) / 10000.0); // Convert to m
}

// ***** AOPSTATUS Helper Functions

bool DevUBLOXGNSS::getNAVAOPSTATUS(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_AOPSTATUS, maxWait);
}

uint8_t DevUBLOXGNSS::getAOPSTATUSuseAOP()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_AOPSTATUS, "useAOP", &value))
    return 0;
  return value.L;
}

uint8_t DevUBLOXGNSS::getAOPSTATUSstatus()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_AOPSTATUS, "status", &value))
    return 0;
  return value.U1;
}

// ***** DAHEADING Helper Functions and automatic support

bool DevUBLOXGNSS::getNAVDAHEADING(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_DAHEADING, maxWait);
}

float DevUBLOXGNSS::getDAHeadingRelPosN() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DAHEADING, "relPosN", &value))
    return 0;
  return (((float)value.I4) / 100.0); // Convert to m
}

float DevUBLOXGNSS::getDAHeadingRelPosE() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DAHEADING, "relPosE", &value))
    return 0;
  return (((float)value.I4) / 100.0); // Convert to m
}

float DevUBLOXGNSS::getDAHeadingRelPosD() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DAHEADING, "relPosD", &value))
    return 0;
  return (((float)value.I4) / 100.0); // Convert to m
}

float DevUBLOXGNSS::getDAHeadingRelPosAccN() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DAHEADING, "accN", &value))
    return 0;
  return (((float)value.U4) / 10000.0); // Convert to m
}

float DevUBLOXGNSS::getDAHeadingRelPosAccE() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DAHEADING, "accE", &value))
    return 0;
  return (((float)value.U4) / 10000.0); // Convert to m
}

float DevUBLOXGNSS::getDAHeadingRelPosAccD() // Returned as m
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_NAV, UBX_NAV_DAHEADING, "accD", &value))
    return 0;
  return (((float)value.U4) / 10000.0); // Convert to m
}

// ***** TIM TP Helper Functions

bool DevUBLOXGNSS::getTIMTP(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_TIM, UBX_TIM_TP, maxWait);
}

uint32_t DevUBLOXGNSS::getTIMTPtowMS()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_TIM, UBX_TIM_TP, "towMS", &value))
    return 0;
  return value.U4;
}

uint32_t DevUBLOXGNSS::getTIMTPtowSubMS()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_TIM, UBX_TIM_TP, "towSubMS", &value))
    return 0;
  return value.U4;
}

uint16_t DevUBLOXGNSS::getTIMTPweek()
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_TIM, UBX_TIM_TP, "week", &value))
    return 0;
  return value.U2;
}

// Convert TIM TP to Unix epoch including microseconds
// CAUTION! Assumes the time base is UTC and the week number is GPS
uint32_t DevUBLOXGNSS::getTIMTPAsEpoch(uint32_t &microsecond)
{
  uint32_t tow = getTIMTPweek() - SFE_UBLOX_JAN_1ST_2020_WEEK; // Calculate the number of weeks since Jan 1st 2020
  tow *= SFE_UBLOX_SECS_PER_WEEK;                              // Convert weeks to seconds
  tow += SFE_UBLOX_EPOCH_WEEK_2086;                            // Add the TOW for Jan 1st 2020
  tow += getTIMTPtowMS() / 1000;                               // Add the TOW for the next TP

  uint32_t us = getTIMTPtowMS() % 1000; // Extract the milliseconds
  us *= 1000;                           // Convert to microseconds

  double subMS = getTIMTPtowSubMS(); // Get towSubMS (ms * 2^-32)
  subMS *= 2.3283064365386963e-10;   // pow(2.0, -32.0);  // Convert to milliseconds
  subMS *= 1000;                     // Convert to microseconds

  us += (uint32_t)subMS; // Add subMS

  microsecond = us;
  return tow;
}

// ***** MON HW Helper Functions

bool DevUBLOXGNSS::getMONHW(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_MON, UBX_MON_HW, maxWait);
}

// Return the aStatus: 0=INIT, 1=DONTKNOW, 2=OK, 3=SHORT, 4=OPEN
// Previously, getAntennaStatus returned aStatus from MON-HW.
// But MON-HW is only supported on older platforms.
// A safer approach is to report the worst antStatus from MON-RF.
// The sfe_ublox_antenna_status_e is not arranged in order of worseness...
// SFE_UBLOX_ANTENNA_STATUS_SHORT is (IMHO) worse than SFE_UBLOX_ANTENNA_STATUS_OPEN.
// So, we need to be clever when finding the worst status.
sfe_ublox_antenna_status_e DevUBLOXGNSS::getAntennaStatus()
{
  sfe_ublox_antenna_status_e antStatus = SFE_UBLOX_ANTENNA_STATUS_DONTKNOW;

  if (!getUBX(UBX_CLASS_MON, UBX_MON_RF)) // Use the default maxWait if polling
    return antStatus; // Return DONT KNOW

  const sfe_ublox_antenna_status_e antStatusByPriority[] = {
    SFE_UBLOX_ANTENNA_STATUS_INIT,
    SFE_UBLOX_ANTENNA_STATUS_DONTKNOW,
    SFE_UBLOX_ANTENNA_STATUS_OK,
    SFE_UBLOX_ANTENNA_STATUS_OPEN, // OPEN is less worse than SHORT
    SFE_UBLOX_ANTENNA_STATUS_SHORT, // SHORT is worst
  };

  const uint8_t numAntStatus = sizeof(antStatusByPriority) / sizeof(antStatusByPriority[0]);

  uint8_t worstStatus = 0;
  
  ubxMessage *msg = ubxMessages.findByName("MON","RF");

  // Check the antStatus of all blocks
  for (uint8_t b = 0; b < getUbxMessageBlockCount(msg); b++)
  {
    sfe_ublox_antenna_status_e thisAntStatus = (sfe_ublox_antenna_status_e)((uint8_t)getUbxMessageBlockField(msg, b, "antStatus"));
    for (uint8_t p = 0; p < numAntStatus; p++)
      if (thisAntStatus == antStatusByPriority[p]) // Step through, from best to worst, find a match
        if (p > worstStatus)
          worstStatus = p; // Record the worst status of all blocks
  }

  return antStatusByPriority[worstStatus]; // Convert worstStatus back into actual status
}

// ***** Helper functions for the NEO-F10N
bool DevUBLOXGNSS::getLNAMode(sfe_ublox_lna_mode_e *mode, uint8_t layer, uint16_t maxWait)
{
  return getVal8(UBLOX_CFG_HW_RF_LNA_MODE, (uint8_t *)mode, layer, maxWait); // Get the LNA mode
}
bool DevUBLOXGNSS::setLNAMode(sfe_ublox_lna_mode_e mode, uint8_t layer, uint16_t maxWait)
{
  return setVal8(UBLOX_CFG_HW_RF_LNA_MODE, (uint8_t)mode, layer, maxWait); // Set the LNA mode
}
bool DevUBLOXGNSS::getGPSL5HealthOverride(bool *override, uint8_t layer, uint16_t maxWait)
{
  return getVal8(UBLOX_CFG_SIGNAL_GPS_L5_HEALTH_OVERRIDE, (uint8_t *) override, layer, maxWait); // Get the GPS L5 health override status
}
bool DevUBLOXGNSS::setGPSL5HealthOverride(bool override, uint8_t layer, uint16_t maxWait)
{
  return setVal8(UBLOX_CFG_SIGNAL_GPS_L5_HEALTH_OVERRIDE, (uint8_t) override, layer, maxWait); // Set the GPS L5 health override status
}

// ***** ESF Helper Functions

bool DevUBLOXGNSS::getESFALG(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_ESF, UBX_ESF_ALG, maxWait);
}

float DevUBLOXGNSS::getESFroll() // Returned as degrees
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_ESF, UBX_ESF_ALG, "roll", &value))
    return 0;
  return (((float)value.I2) / 100.0); // Convert to degrees
}

float DevUBLOXGNSS::getESFpitch() // Returned as degrees
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_ESF, UBX_ESF_ALG, "pitch", &value))
    return 0;
  return (((float)value.I2) / 100.0); // Convert to degrees
}

float DevUBLOXGNSS::getESFyaw() // Returned as degrees
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_ESF, UBX_ESF_ALG, "yaw", &value))
    return 0;
  return (((float)value.I2) / 100.0); // Convert to degrees
}

// getSensorFusionMeasurement() is redacted, per explicit instruction - see AGENTS.md "Adding
// support for ESF-MEAS" and the comment above its declaration in u-blox_GNSS.h.

// getRawSensorMeasurement() and both overloads of getSensorFusionStatus() are redacted, per
// explicit instruction - see AGENTS.md "Adding support for ESF-RAW and ESF-STATUS" and the
// comment above their declarations in u-blox_GNSS.h.

// ***** HNR Helper Functions

bool DevUBLOXGNSS::getHNRATT(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_HNR, UBX_HNR_ATT, maxWait);
}

// Set the High Navigation Rate
// Returns true if the setHNRNavigationRate is successful
bool DevUBLOXGNSS::setHNRNavigationRate(uint8_t rate, uint8_t layer, uint16_t maxWait)
{
  if (rate == 0) // Return now if rate is zero
    return (false);

  if (rate > 40)
    rate = 40; // Limit rate to 40Hz so i2cPollingWait is set correctly

  // Placeholder for when HNR switches to the configuration interface
  (void)layer;

  // Adjust the I2C polling timeout based on update rate
  // Do this even if the sendCommand is not ACK'd
  i2cPollingWaitHNR = 1000 / (((int)rate) * 4);                                                   // This is the number of ms to wait between checks for new I2C data. Max 250. Min 6.
  i2cPollingWait = i2cPollingWaitNAV < i2cPollingWaitHNR ? i2cPollingWaitNAV : i2cPollingWaitHNR; // Set i2cPollingWait to the lower of NAV and HNR

  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_HNR;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  // Ask module for the current HNR settings. Loads into payloadCfg.
  if (sendCommand(&packetCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED)
    return (false);

  // Load the new navigation rate into payloadCfg
  payloadCfg[0] = rate;

  // Update the navigation rate
  sfe_ublox_status_e result = sendCommand(&packetCfg, maxWait); // We are only expecting an ACK

  return (result == SFE_UBLOX_STATUS_DATA_SENT);
}

// Get the High Navigation Rate
// Returns 0 if the getHNRNavigationRate fails
uint8_t DevUBLOXGNSS::getHNRNavigationRate(uint8_t layer, uint16_t maxWait)
{
  // Placeholder for when HNR switches to the configuration interface
  (void)layer;

  packetCfg.cls = UBX_CLASS_CFG;
  packetCfg.id = UBX_CFG_HNR;
  packetCfg.len = 0;
  packetCfg.startingSpot = 0;

  // Ask module for the current HNR settings. Loads into payloadCfg.
  if (sendCommand(&packetCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED)
    return (0);

  // Return the navigation rate
  return (payloadCfg[0]);
}

float DevUBLOXGNSS::getHNRroll() // Returned as degrees
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_HNR, UBX_HNR_ATT, "roll", &value))
    return 0;
  return (((float)value.I4) / 100000.0); // Convert to degrees
}

float DevUBLOXGNSS::getHNRpitch() // Returned as degrees
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_HNR, UBX_HNR_ATT, "pitch", &value))
    return 0;
  return (((float)value.I4) / 100000.0); // Convert to degrees
}

float DevUBLOXGNSS::getHNRheading() // Returned as degrees
{
  ubxAnyType value;
  if (!getUBXfield(UBX_CLASS_HNR, UBX_HNR_ATT, "heading", &value))
    return 0;
  return (((float)value.I4) / 100000.0); // Convert to degrees
}

// ***** Helper Functions for the remaining registered messages (thin wrappers only -
// see AGENTS.md "getUBX()". None of these has per-field convenience getters yet;
// call getUBXfield(Class, ID, "fieldName", &value) directly to read individual fields)

bool DevUBLOXGNSS::getNAVPOSLLH(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_POSLLH, maxWait);
}

bool DevUBLOXGNSS::getNAVSTATUS(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_STATUS, maxWait);
}

bool DevUBLOXGNSS::getNAVODO(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_ODO, maxWait);
}

bool DevUBLOXGNSS::getNAVVELECEF(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_VELECEF, maxWait);
}

bool DevUBLOXGNSS::getNAVVELNED(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_VELNED, maxWait);
}

bool DevUBLOXGNSS::getNAVTIMEUTC(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_TIMEUTC, maxWait);
}

bool DevUBLOXGNSS::getNAVCLOCK(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_CLOCK, maxWait);
}

bool DevUBLOXGNSS::getNAVEOE(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_NAV, UBX_NAV_EOE, maxWait);
}

bool DevUBLOXGNSS::getRXMCOR(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_RXM, UBX_RXM_COR, maxWait);
}

bool DevUBLOXGNSS::getMONHW2(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_MON, UBX_MON_HW2, maxWait);
}

bool DevUBLOXGNSS::getTIMTM2(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_TIM, UBX_TIM_TM2, maxWait);
}

bool DevUBLOXGNSS::getESFINS(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_ESF, UBX_ESF_INS, maxWait);
}

bool DevUBLOXGNSS::getHNRPVT(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_HNR, UBX_HNR_PVT, maxWait);
}

bool DevUBLOXGNSS::getHNRINS(uint16_t maxWait)
{
  return getUBX(UBX_CLASS_HNR, UBX_HNR_INS, maxWait);
}

// Functions to extract signed and unsigned 8/16/32-bit data from a ubxPacket
// From v2.0: These are public. The user can call these to extract data from custom packets

// Given a spot in the payload array, extract eight bytes and build a uint64_t
uint64_t DevUBLOXGNSS::extractLongLong(ubxPacket *msg, uint16_t spotToStart)
{
  uint64_t val = 0;
  for (uint8_t i = 0; i < 8; i++)
    val |= (uint64_t)msg->payload[spotToStart + i] << (8 * i);
  return (val);
}

// Given a spot in the payload array, extract eight bytes and build a int64_t
int64_t DevUBLOXGNSS::extractSignedLongLong(ubxPacket *msg, uint16_t spotToStart)
{
  unsignedSigned64 converter64;

  converter64.unsigned64 = extractLongLong(msg, spotToStart);
  return (converter64.signed64);
}

// Given a spot in the payload array, extract four bytes and build a long
uint32_t DevUBLOXGNSS::extractLong(ubxPacket *msg, uint16_t spotToStart)
{
  uint32_t val = 0;
  for (uint8_t i = 0; i < 4; i++)
    val |= (uint32_t)msg->payload[spotToStart + i] << (8 * i);
  return (val);
}

// Just so there is no ambiguity about whether a uint32_t will cast to a int32_t correctly...
int32_t DevUBLOXGNSS::extractSignedLong(ubxPacket *msg, uint16_t spotToStart)
{
  unsignedSigned32 converter;
  converter.unsigned32 = extractLong(msg, spotToStart);
  return (converter.signed32);
}

// Given a spot in the payload array, extract two bytes and build an int
uint16_t DevUBLOXGNSS::extractInt(ubxPacket *msg, uint16_t spotToStart)
{
  uint16_t val = (uint16_t)msg->payload[spotToStart + 0] << 8 * 0;
  val |= (uint16_t)msg->payload[spotToStart + 1] << 8 * 1;
  return (val);
}

// Just so there is no ambiguity about whether a uint16_t will cast to a int16_t correctly...
int16_t DevUBLOXGNSS::extractSignedInt(ubxPacket *msg, uint16_t spotToStart)
{
  unsignedSigned16 converter;
  converter.unsigned16 = extractInt(msg, spotToStart);
  return (converter.signed16);
}

// Given a spot, extract a byte from the payload
uint8_t DevUBLOXGNSS::extractByte(ubxPacket *msg, uint16_t spotToStart)
{
  return (msg->payload[spotToStart]);
}

// Given a spot, extract a signed 8-bit value from the payload
int8_t DevUBLOXGNSS::extractSignedChar(ubxPacket *msg, uint16_t spotToStart)
{
  unsignedSigned8 converter;
  converter.unsigned8 = extractByte(msg, spotToStart);
  return (converter.signed8);
}

// Given a spot, extract a signed 32-bit float from the payload
float DevUBLOXGNSS::extractFloat(ubxPacket *msg, uint16_t spotToStart)
{
  unsigned32float converter;
  converter.unsigned32 = extractLong(msg, spotToStart);
  return (converter.flt);
}

// Given a spot, extract a signed 64-bit double from the payload
double DevUBLOXGNSS::extractDouble(ubxPacket *msg, uint16_t spotToStart)
{
  unsigned64double converter;
  converter.unsigned64 = extractLongLong(msg, spotToStart);
  return (converter.dbl);
}

// Given a pointer, extract an unsigned integer with width bits, starting at bit start
uint64_t DevUBLOXGNSS::extractUnsignedBits(uint8_t *ptr, uint16_t start, uint16_t width)
{
  uint64_t result = 0;
  uint16_t count = 0;
  uint8_t bitMask = 0x80;

  // Skip whole bytes (8 bits)
  ptr += start / 8;
  count += (start / 8) * 8;

  // Loop until we reach the start bit
  while (count < start)
  {
    bitMask >>= 1; // Shift the bit mask
    count++;       // Increment the count

    if (bitMask == 0) // Have we counted 8 bits?
    {
      ptr++;          // Point to the next byte
      bitMask = 0x80; // Reset the bit mask
    }
  }

  // We have reached the start bit and ptr is pointing at the correct byte
  // Now extract width bits, incrementing ptr and shifting bitMask as we go
  while (count < (start + width))
  {
    if (*ptr & bitMask) // Is the bit set?
      result |= 1;      // Set the corresponding bit in result

    bitMask >>= 1; // Shift the bit mask
    count++;       // Increment the count

    if (bitMask == 0) // Have we counted 8 bits?
    {
      ptr++;          // Point to the next byte
      bitMask = 0x80; // Reset the bit mask
    }

    if (count < (start + width)) // Do we need to shift result?
      result <<= 1;              // Shift the result
  }

  return result;
}

// Given a pointer, extract an signed integer with width bits, starting at bit start
int64_t DevUBLOXGNSS::extractSignedBits(uint8_t *ptr, uint16_t start, uint16_t width)
{

  unsignedSigned64 result;
  result.unsigned64 = 0;

  uint64_t twosComplement = 0xFFFFFFFFFFFFFFFF;

  bool isNegative;

  uint16_t count = 0;
  uint8_t bitMask = 0x80;

  // Skip whole bytes (8 bits)
  ptr += start / 8;
  count += (start / 8) * 8;

  // Loop until we reach the start bit
  while (count < start)
  {
    bitMask >>= 1; // Shift the bit mask
    count++;       // Increment the count

    if (bitMask == 0) // Have we counted 8 bits?
    {
      ptr++;          // Point to the next byte
      bitMask = 0x80; // Reset the bit mask
    }
  }

  isNegative = *ptr & bitMask; // Record the first bit - indicates in the number is negative

  // We have reached the start bit and ptr is pointing at the correct byte
  // Now extract width bits, incrementing ptr and shifting bitMask as we go
  while (count < (start + width))
  {
    if (*ptr & bitMask)       // Is the bit set?
      result.unsigned64 |= 1; // Set the corresponding bit in result

    bitMask >>= 1;        // Shift the bit mask
    count++;              // Increment the count
    twosComplement <<= 1; // Shift the two's complement mask (clear LSB)

    if (bitMask == 0) // Have we counted 8 bits?
    {
      ptr++;          // Point to the next byte
      bitMask = 0x80; // Reset the bit mask
    }

    if (count < (start + width)) // Do we need to shift result?
      result.unsigned64 <<= 1;   // Shift the result
  }

  // Handle negative number
  if (isNegative)
    result.unsigned64 |= twosComplement; // OR in the two's complement mask

  return result.signed64;
}
