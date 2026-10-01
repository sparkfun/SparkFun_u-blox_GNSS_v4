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
 * @file nmeaMessageVector.h
 */

#pragma once

#include <vector>
#include <string.h>

#include "nmeaMessage.h"

// ===========================

// Support for individual NMEA messages is contained in these files
// Individual files can be commented to save both program memory and RAM

#include "nmeaMessages/nmeaGGA.h"
#include "nmeaMessages/nmeaDTM.h"
#include "nmeaMessages/nmeaGLL.h"
#include "nmeaMessages/nmeaGNS.h"
#include "nmeaMessages/nmeaGST.h"
#include "nmeaMessages/nmeaGSV.h"
#include "nmeaMessages/nmeaRMC.h"
#include "nmeaMessages/nmeaVTG.h"
#include "nmeaMessages/nmeaZDA.h"

// ===========================

#include "u-blox_external_typedefs.h" // sfe_ublox_status_e
#include "sfe_debug.h"                 // v4 scaffolding - shared base for debugPrint()/debugPrintln(), see AGENTS.md

class nmeaMessageVector : public SparkFun_UBLOX_GNSS::SfeDebugPrint
{
public:
    std::vector<nmeaMessage *> nmeaMessageVectors;

    /**
     * @brief Construct the vector and populate it with one instance of every registered NMEA message.
     *
     * Asks nmeaMessageRegistry to build one instance of every message class that self-registered
     * via nmeaRegisterMessage() in its own header.
     */
    nmeaMessageVector(void)
    {
        nmeaMessageRegistry::get().buildAll(nmeaMessageVectors);
    }

    /**
     * @brief Destroy the vector, deleting every message object it owns.
     */
    ~nmeaMessageVector(void)
    {
        for (auto msg : nmeaMessageVectors)
            delete msg;
    }

    /**
     * @brief Find the registered message object for a given NMEA message identifier.
     *
     * @param msgId The 3-character NMEA message identifier to look up (e.g. "GGA").
     * @return Pointer to the matching nmeaMessage, or nullptr if no message is registered under that id.
     */
    nmeaMessage *find(const char *msgId)
    {
        for (auto msg : nmeaMessageVectors)
        {
            if (msg->amI(msgId))
                return msg;
        }
        return nullptr;
    }

    // Look up a registered message by its name.
    /**
     * @brief Find the registered message object by its name. Equivalent to find().
     *
     * @param msgId The 3-character NMEA message identifier to look up (e.g. "GGA").
     * @return Pointer to the matching nmeaMessage, or nullptr if no message is registered under that id.
     */
    nmeaMessage *findByName(const char *msgId)
    {
        return find(msgId);
    }

    /**
     * @brief Lazily allocate a registered message's raw payload storage.
     *
     * @param msgId The NMEA message identifier to look up.
     * @return SFE_UBLOX_STATUS_SUCCESS on success, SFE_UBLOX_STATUS_INVALID_ARG if no message
     * is registered under msgId, or SFE_UBLOX_STATUS_MEM_ERR if the allocation failed.
     */
    sfe_ublox_status_e initStorage(const char *msgId)
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        return (msg->initStorage() ? SFE_UBLOX_STATUS_SUCCESS : SFE_UBLOX_STATUS_MEM_ERR);
    }

    /**
     * @brief Get whether a registered message is set to be output periodically by the module.
     *
     * @param msgId The NMEA message identifier to look up.
     * @param automatic Out parameter: filled in with the message's automatic-output flag.
     * @return SFE_UBLOX_STATUS_SUCCESS on success, SFE_UBLOX_STATUS_INVALID_ARG if no message
     * is registered under msgId.
     */
    sfe_ublox_status_e isAutomatic(const char *msgId, bool *automatic)
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        *automatic = msg->_automatic;
        return SFE_UBLOX_STATUS_SUCCESS;
    }
    /**
     * @brief Set whether a registered message is set to be output periodically by the module.
     *
     * @param msgId The NMEA message identifier to look up.
     * @param automatic The new automatic-output flag value.
     * @return SFE_UBLOX_STATUS_SUCCESS on success, SFE_UBLOX_STATUS_INVALID_ARG if no message
     * is registered under msgId.
     */
    sfe_ublox_status_e setAutomatic(const char *msgId, bool automatic)
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        msg->_automatic = automatic;
        return SFE_UBLOX_STATUS_SUCCESS;
    }

    /**
     * @brief Get a registered message's implicit-update flag.
     *
     * true means the generic getNMEA()-style call itself parses newly-arrived data for this
     * message; false means the caller must call checkUblox() itself first.
     *
     * @param msgId The NMEA message identifier to look up.
     * @param implicitUpdateOut Out parameter: filled in with the message's implicit-update flag.
     * @return SFE_UBLOX_STATUS_SUCCESS on success, SFE_UBLOX_STATUS_INVALID_ARG if no message
     * is registered under msgId.
     */
    sfe_ublox_status_e implicitUpdate(const char *msgId, bool *implicitUpdateOut)
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        *implicitUpdateOut = msg->_implicitUpdate;
        return SFE_UBLOX_STATUS_SUCCESS;
    }
    /**
     * @brief Set a registered message's implicit-update flag.
     *
     * @param msgId The NMEA message identifier to look up.
     * @param implicitUpdateIn The new implicit-update flag value.
     * @return SFE_UBLOX_STATUS_SUCCESS on success, SFE_UBLOX_STATUS_INVALID_ARG if no message
     * is registered under msgId.
     */
    sfe_ublox_status_e setImplicitUpdate(const char *msgId, bool implicitUpdateIn)
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        msg->_implicitUpdate = implicitUpdateIn;
        return SFE_UBLOX_STATUS_SUCCESS;
    }

    // A single bool per message - has fresh data arrived since the last time it was reported?
    // Not a per-field bitmask - see AGENTS.md "moduleQueried".
    /**
     * @brief Get whether fresh data has arrived for a registered message since it was last reported.
     *
     * @param msgId The NMEA message identifier to look up.
     * @param queried Out parameter: filled in with the message's moduleQueried flag.
     * @return SFE_UBLOX_STATUS_SUCCESS on success, SFE_UBLOX_STATUS_INVALID_ARG if no message
     * is registered under msgId.
     */
    sfe_ublox_status_e moduleQueried(const char *msgId, bool *queried)
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        *queried = msg->_moduleQueried;
        return SFE_UBLOX_STATUS_SUCCESS;
    }
    /**
     * @brief Set whether fresh data has arrived for a registered message since it was last reported.
     *
     * @param msgId The NMEA message identifier to look up.
     * @param queried The new moduleQueried flag value.
     * @return SFE_UBLOX_STATUS_SUCCESS on success, SFE_UBLOX_STATUS_INVALID_ARG if no message
     * is registered under msgId.
     */
    sfe_ublox_status_e setModuleQueried(const char *msgId, bool queried)
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        msg->_moduleQueried = queried;
        return SFE_UBLOX_STATUS_SUCCESS;
    }

    /**
     * @brief Get whether a registered message is set to be added to the file buffer (logNMEA()).
     *
     * @param msgId The NMEA message identifier to look up.
     * @param adding Out parameter: filled in with the message's addToFileBuffer flag.
     * @return SFE_UBLOX_STATUS_SUCCESS on success, SFE_UBLOX_STATUS_INVALID_ARG if no message
     * is registered under msgId.
     */
    sfe_ublox_status_e getAddToFileBuffer(const char *msgId, bool *adding)
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        *adding = msg->_addToFileBuffer;
        return SFE_UBLOX_STATUS_SUCCESS;
    }
    /**
     * @brief Set whether a registered message is added to the file buffer (used by logNMEA()).
     *
     * @param msgId The NMEA message identifier to look up.
     * @param adding The new addToFileBuffer flag value.
     * @return SFE_UBLOX_STATUS_SUCCESS on success, SFE_UBLOX_STATUS_INVALID_ARG if no message
     * is registered under msgId.
     */
    sfe_ublox_status_e setAddToFileBuffer(const char *msgId, bool adding)
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        msg->_addToFileBuffer = adding;
        return SFE_UBLOX_STATUS_SUCCESS;
    }

    /**
     * @brief Get a registered message's CFG-MSGOUT key for a given communication port.
     *
     * @param msgId The NMEA message identifier to look up.
     * @param commType Which port's key to return (I2C, SPI, UART1, UART2).
     * @param key Out parameter: filled in with the UBLOX_CFG_MSGOUT_* key.
     * @return SFE_UBLOX_STATUS_SUCCESS on success, SFE_UBLOX_STATUS_INVALID_ARG if no message
     * is registered under msgId.
     */
    sfe_ublox_status_e getMsgOutKey(const char *msgId, uint8_t commType, uint32_t *key)
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        *key = msg->getMsgOutKey(commType);
        return SFE_UBLOX_STATUS_SUCCESS;
    }

    /**
     * @brief Register (or clear) the user callback function for a registered message.
     *
     * @param msgId The NMEA message identifier to look up.
     * @param callbackPtr Function to call once fresh data is waiting for this message, or
     * nullptr to clear any previously-registered callback.
     * @return SFE_UBLOX_STATUS_SUCCESS on success, SFE_UBLOX_STATUS_INVALID_ARG if no message
     * is registered under msgId.
     */
    sfe_ublox_status_e setCallback(const char *msgId, void (*callbackPtr)(nmeaCallbackDataCommon_t *))
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        msg->_callbackPtr = callbackPtr;
        return SFE_UBLOX_STATUS_SUCCESS;
    }

    // Copy 'len' freshly-received payload bytes into this message's storage and mark it fresh.
    // Note: not currently called anywhere - the live NMEA dispatch path is the inline code in
    // DevUBLOXGNSS::process(), which this mirrors (including its ring-buffered callback write
    // side - see AGENTS.md "Adding support for NMEA GSV messages"). Kept working and consistent
    // with the live path rather than removed, in case a future caller wants a single entry point.
    /**
     * @brief Copy freshly-received sentence bytes into a registered message's storage.
     *
     * Mirrors the inline NMEA dispatch/ring-buffer logic in DevUBLOXGNSS::process() - see the
     * comment above this function's definition for why it exists but isn't currently called
     * from that live path.
     *
     * @param msgId The NMEA message identifier to look up.
     * @param payload Pointer to the newly-received sentence bytes.
     * @param len Number of bytes received (clamped to the message's _messageLength before
     * being copied/stored).
     * @return SFE_UBLOX_STATUS_SUCCESS on success, SFE_UBLOX_STATUS_INVALID_ARG if no message
     * is registered under msgId, or SFE_UBLOX_STATUS_MEM_ERR if storage could not be allocated.
     */
    sfe_ublox_status_e storePayload(const char *msgId, const uint8_t *payload, uint16_t len)
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        if (!msg->initStorage())
            return SFE_UBLOX_STATUS_MEM_ERR;
        if (len > msg->_messageLength)
            len = msg->_messageLength;
        memcpy(msg->_storage, payload, len);
        msg->_moduleQueried = true;

        if (msg->_callbackPtr != nullptr)
        {
            if (msg->initCallbackStorage())
            {
                if (msg->_numCallbackCopies <= 1)
                {
                    memcpy(msg->_callbackStorage, payload, len);
                    msg->_callbackCount = 1; // head/tail stay at 0 - a degenerate 1-slot ring
                }
                else if (msg->_callbackCount < msg->_numCallbackCopies) // Ring has a free slot
                {
                    uint8_t *slot = msg->_callbackStorage + ((uint32_t)msg->_callbackHead * msg->_messageLength);
                    memcpy(slot, payload, len);
                    msg->_callbackHead = (uint8_t)((msg->_callbackHead + 1) % msg->_numCallbackCopies);
                    msg->_callbackCount++;
                }
                else
                {
                    // ring is full - drop this payload, keep what's already buffered
                    debugPrint("NMEA storePayload: msgId ", true); // Important
                    debugPrint(msgId, true);
                    debugPrintln(" _callbackStorage ring buffer full. Message lost.", true);
                }
            }
        }

        return SFE_UBLOX_STATUS_SUCCESS;
    }

    // Look up one field of one message by name and fill in 'value'.
    /**
     * @brief Look up one named field of a registered message's live sentence.
     *
     * @param msgId The NMEA message identifier to look up.
     * @param field Name of the field to extract.
     * @param value Out parameter: filled in with the field's text if found.
     * @return SFE_UBLOX_STATUS_SUCCESS if found, SFE_UBLOX_STATUS_INVALID_ARG if no message is
     * registered under msgId or the field was not found, or SFE_UBLOX_STATUS_MEM_ERR if no data
     * has arrived for this message yet (its _storage is still unallocated).
     */
    sfe_ublox_status_e extractValue(const char *msgId, const char *field, sfe_string_t &value)
    {
        nmeaMessage *msg = find(msgId);
        if (msg == nullptr)
            return SFE_UBLOX_STATUS_INVALID_ARG;
        if (msg->_storage == nullptr)
            return SFE_UBLOX_STATUS_MEM_ERR; // No data has arrived for this message yet

        return msg->extractFieldFrom(msg->_storage, field, value) ? SFE_UBLOX_STATUS_SUCCESS : SFE_UBLOX_STATUS_INVALID_ARG;
    }
};
