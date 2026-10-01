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
 * @file ubxMessage.h
 * 
 */

/*
  v4 scaffolding: the per-message base class, plus the small set of shared types it needs
  (ubxAnyType, and the ubxDataType8bit() reduction macro). See AGENTS.md "Reference Scaffolding"
  and "moduleQueried" for the design this implements.

  This corrects several issues noted in AGENTS.md's "Known bugs in the prototype" against the
  preliminary prototype referenced there:
    - _moduleQueried is a single bool per message (not a per-field bitmask / numQueriedWords) -
      see AGENTS.md "moduleQueried".
    - Objects of this class (and its subclasses) are meant to be held as pointers with a stable
      lifetime (see ubxMessageVector.h), not copied by value - so _fields always stays valid.
    - ubxDataType8bit() did not previously exist anywhere in this codebase (only in the
      prototype); it is defined here as it is needed by both ubxNAVPVT.h and ubxMessageVector.h.
*/

#pragma once

#include "sfe_platform.h"
#include <string.h>

#include "u-blox_config_keys.h" // UBX_CFG_* type tags - needed by ubxAnyType::operator double() and ubxMessage::extractFieldFrom() below

// Reduces one of the UBX_CFG_* type-tag constants (defined in u-blox_config_keys.h, and already
// used to tag VALGET/VALSET config keys) down to an 8-bit value suitable for storing in
// ubxMessage::ubxField::ubxDataType. One type vocabulary serves both config values and message
// payload fields - see AGENTS.md "Field data types are reused from the config-key encoding".
#define ubxDataType8bit(ubxType) ((uint8_t)(((ubxType >> 20) | (ubxType >> 12)) & 0xFF))

// The generic typed-value carrier used to move a single field's value across the
// Class/ID/field-name generic boundary (see DevUBLOXGNSS::getUBXfield() in u-blox_GNSS.h/.cpp, and
// getUbxMessageField() at the bottom of this file).
typedef struct ubxAnyType
{
    uint8_t ubxDataType; // Which union member is valid, from ubxDataType8bit()
    union
    {
        bool L;
        uint8_t U1;
        uint16_t U2;
        uint32_t U4;
        uint64_t U8;
        int8_t I1;
        int16_t I2;
        int32_t I4;
        int64_t I8;
        float R4;
        double R8;
    };

    // A single, unambiguous implicit numeric conversion. See AGENTS.md "getUbxMessagePtr
    // Factory design pattern": "getUbxMessageField will also need to use a Factory
    // method / design pattern to handle the different return types. If this is not possible,
    // identify the nearest alternative strategy which is possible." A non-template C++ function
    // cannot return a different static type per call depending on a runtime field-name string (the
    // field's type isn't known until the name is looked up at runtime) - and a template can't help
    // either, since there's no compile-time type to parameterize on. The nearest alternative is to
    // keep returning this single tagged-union type, but give it exactly one conversion operator, so
    // callers like `Serial.print(value)` or `value / 10` (see CallbackExample1_NAVHPPOSLLH.ino) get
    // one unambiguous conversion path regardless of which union member is actually valid. `double`
    // is the target because it exactly represents every integer type this library tags up to 32
    // bits, and to 2^53 for the rarer 64-bit fields - only a genuine 64-bit value outside that range
    // would lose precision, and none of the currently-registered messages have one. The trade-off:
    // Serial.print() will show a trailing ".00" for integer fields (double's default 2 decimal
    // places) rather than a clean integer - cosmetic, not a correctness issue.
    /**
     * @brief Convert this tagged union to a double, based on which member ubxDataType selects.
     *
     * The single, unambiguous implicit numeric conversion for ubxAnyType - see the design note
     * above this operator's definition for why one conversion to double (rather than a
     * template or a per-type Factory) is the chosen approach.
     *
     * @return The stored value widened/converted to double, or 0.0 for an unrecognized
     * ubxDataType tag (including the "field not found" sentinel set by getUbxMessageField()).
     */
    operator double() const
    {
        switch (ubxDataType)
        {
        case ubxDataType8bit(UBX_CFG_L):
            return L;
        case ubxDataType8bit(UBX_CFG_U1):
        case ubxDataType8bit(UBX_CFG_E1):
        case ubxDataType8bit(UBX_CFG_X1):
            return U1;
        case ubxDataType8bit(UBX_CFG_I1):
            return I1;
        case ubxDataType8bit(UBX_CFG_U2):
        case ubxDataType8bit(UBX_CFG_E2):
        case ubxDataType8bit(UBX_CFG_X2):
            return U2;
        case ubxDataType8bit(UBX_CFG_I2):
            return I2;
        case ubxDataType8bit(UBX_CFG_U4):
        case ubxDataType8bit(UBX_CFG_E4):
        case ubxDataType8bit(UBX_CFG_X4):
            return U4;
        case ubxDataType8bit(UBX_CFG_I4):
            return I4;
        case ubxDataType8bit(UBX_CFG_R4):
            return R4;
        case ubxDataType8bit(UBX_CFG_U8):
        case ubxDataType8bit(UBX_CFG_X8):
            return (double)U8;
        case ubxDataType8bit(UBX_CFG_I8):
            return (double)I8;
        case ubxDataType8bit(UBX_CFG_R8):
            return R8;
        default:
            return 0.0; // Unknown tag, or the "field not found" sentinel set by getUbxMessageField()
        }
    }
} ubxAnyType;

class ubxMessage; // Forward declaration - see ubxCallbackDataCommon_t below

// What actually crosses into the user's callback function - see AGENTS.md "class ubxMessage needs
// separate callback storage" / "setAutoCallbackPtr" / "getUbxMessagePtr Factory design
// pattern". AGENTS.md envisaged this containing "an enum representing the type of data structure
// (UBX_NAV_PVT_data_t, UBX_NAV_HPPOSLLH_data_t, etc.)" - but the whole point of the v4 registry is
// that there is no longer a distinct C struct type per message, so there is nothing for such an
// enum to name. `messagePtr` carries the same information generically: it is the opaque handle
// getUbxMessagePtr()/getUbxMessageField() use to navigate back to the message's
// own field table and callback storage, for any message, without a per-message enum to maintain.
typedef struct
{
    uint8_t Class; // Convenience - which message this is, without needing to dereference messagePtr
    uint8_t ID;
    ubxMessage *messagePtr; // Opaque - do not dereference directly. Use getUbxMessagePtr() /
                            // getUbxMessageField() to read it.
} ubxCallbackDataCommon_t;

class ubxMessage
{
public:
    // One entry per field in the message payload.
    typedef struct
    {
        const char fieldName[20];  // Fixed-size array - keeps each field table self-contained (no separately-lived string needed)
        const uint8_t ubxDataType; // 8-bit type tag, from ubxDataType8bit() - see above
        const uint8_t startByte;   // Byte offset into the message PAYLOAD (i.e. into this object's _storage)
        const int8_t startBit;     // Bit offset within startByte if this is a sub-field; -1 otherwise
        const int8_t bitWidth;     // Bit width if this is a sub-field; -1 otherwise
    } ubxField;

    // One entry per version of a message whose payload shape has changed across firmware/protocol
    // versions. Not yet used by any message - see AGENTS.md "Still undesigned even after this
    // prototype" ("Message versioning"). Reserved for future use.
    typedef struct
    {
        const uint8_t version;
        const uint8_t numFields;
        const ubxField &firstField;
    } ubxMessageVersion;

    /**
     * @brief Construct a new, unregistered ubxMessage object.
     *
     * Leaves every field at its default (Class/ID 0, no field table, no storage allocated). A
     * subclass calls addClassID() from its own constructor to actually register its identity
     * and metadata - see addClassID() below.
     */
    ubxMessage(void) {}

    /**
     * @brief Destroy this ubxMessage, freeing any lazily-allocated storage.
     *
     * Frees _storage, _callbackStorage, _callbackActualLength and _callbackRawFrame if they
     * were ever allocated (via initStorage() / initCallbackStorage()). Safe to call even if
     * none of them were ever allocated, since each is nullptr until first use.
     */
    virtual ~ubxMessage(void)
    {
        if (_storage != nullptr)
            delete[] _storage;
        if (_callbackStorage != nullptr)
            delete[] _callbackStorage;
        if (_callbackActualLength != nullptr)
            delete[] _callbackActualLength;
        if (_callbackRawFrame != nullptr)
            delete[] _callbackRawFrame;
    }

    // Does this object represent this Class/ID?
    /**
     * @brief Check whether this message object represents the given UBX Class/ID.
     *
     * @param Class The UBX message class to test against.
     * @param ID The UBX message ID (within Class) to test against.
     * @return true if this object's registered Class and ID both match, false otherwise.
     */
    bool amI(uint8_t Class, uint8_t ID) const
    {
        return (Class == _Class) && (ID == _ID);
    }

    // Lazily allocate _storage - only when the message is actually used. This is how "delete the
    // header, save the RAM" (AGENTS.md) is meant to work for a message that is never instantiated
    // at all - see ubxMessageVector.h.
    /**
     * @brief Lazily allocate this message's raw payload storage (_storage), if not already done.
     *
     * Allocates and zero-fills a buffer of _messageLength bytes on first call; subsequent calls
     * are a no-op. This is what lets a message that is never instantiated/used avoid consuming
     * any RAM for its payload - see the class-level comment above for the "delete the header,
     * save the RAM" design intent.
     *
     * @return true if _storage is non-null after the call (already allocated, or successfully
     * allocated now); false if the allocation failed.
     */
    bool initStorage(void)
    {
        if (_storage == nullptr)
        {
            _storage = new uint8_t[_messageLength];
            if (_storage != nullptr)
                memset(_storage, 0, _messageLength);
        }
        return (_storage != nullptr);
    }

    // Lazily allocate _callbackStorage - only once a callback is actually registered for this
    // message (see DevUBLOXGNSS::setAutoCallbackPtr()). Kept separate from _storage so a fast
    // producer (processUBXpacket, via storePayload()) can freeze a copy for the callback to read
    // without racing a consumer that hasn't got round to reading _storage yet - see AGENTS.md
    // "class ubxMessage needs separate callback storage". _numCallbackCopies is usually 1 (a
    // degenerate single-slot ring); RXM-SFRBX uses 14 to hold a burst of messages that can arrive
    // in a single checkUblox() call before checkCallbacks() drains them - see AGENTS.md "Adding
    // support for RXM-SFRBX" and ubxMessageVector::storePayload() / DevUBLOXGNSS::checkCallbacks().
    // Also lazily allocates _callbackActualLength and _callbackRawFrame alongside _callbackStorage,
    // for every message with a callback registered (not just ESF-MEAS) - see AGENTS.md "Adding
    // support for ESF-MEAS". _callbackActualLength is a per-ring-slot real received length, needed
    // because storePayload() no longer trusts a message's own header count field (see
    // getBlockCount() below) and because a shorter message reused a ring slot that previously held
    // a longer one, leaving stale trailing bytes in _callbackStorage. _callbackRawFrame is a
    // separate, larger per-ring-slot buffer (8 + _messageLength bytes/slot) holding the COMPLETE
    // raw UBX frame (sync bytes + Class/ID/length + payload + checksum) for relaying a message
    // verbatim from inside a callback - see getUbxMessageRawLengthCallback()/
    // getUbxMessageRawPtrCallback() in u-blox_GNSS.cpp. This roughly doubles the RAM cost of
    // _callbackStorage for any message with a callback registered (duplicating the payload bytes
    // into a second, checksum-and-header-wrapped buffer) - accepted as the price of a general,
    // automatic capability rather than restructuring _callbackStorage's existing payload-only
    // layout, which every field-extraction call site across every other registered message already
    // depends on.
    /**
     * @brief Lazily allocate this message's callback ring-buffer storage, if not already done.
     *
     * Allocates _callbackStorage (_numCallbackCopies slots of _messageLength bytes each),
     * _callbackActualLength (one uint16_t per slot) and _callbackRawFrame (one complete raw
     * UBX frame per slot) together, the first time a callback is registered for this message -
     * see DevUBLOXGNSS::setAutoCallbackPtr(). Subsequent calls are a no-op for whichever of the
     * three are already allocated.
     *
     * @return true if all three buffers are non-null after the call, false if any allocation failed.
     */
    bool initCallbackStorage(void)
    {
        if (_callbackStorage == nullptr)
        {
            _callbackStorage = new uint8_t[_messageLength * _numCallbackCopies];
            if (_callbackStorage != nullptr)
                memset(_callbackStorage, 0, _messageLength * _numCallbackCopies);
        }
        if ((_callbackStorage != nullptr) && (_callbackActualLength == nullptr))
        {
            _callbackActualLength = new uint16_t[_numCallbackCopies];
            if (_callbackActualLength != nullptr)
                memset(_callbackActualLength, 0, sizeof(uint16_t) * _numCallbackCopies);
        }
        if ((_callbackStorage != nullptr) && (_callbackRawFrame == nullptr))
        {
            uint32_t frameBytes = ((uint32_t)_messageLength + 8) * _numCallbackCopies;
            _callbackRawFrame = new uint8_t[frameBytes];
            if (_callbackRawFrame != nullptr)
                memset(_callbackRawFrame, 0, frameBytes);
        }
        return (_callbackStorage != nullptr) && (_callbackActualLength != nullptr) && (_callbackRawFrame != nullptr);
    }

    /**
     * @brief Get this message's CFG-MSGOUT key for the given communication port.
     *
     * @param commType Index into _msgOutKeys - which port's key to return (I2C, SPI, UART1, UART2).
     * @return The UBLOX_CFG_MSGOUT_* configuration key used to enable/disable this message on that port.
     */
    uint32_t getMsgOutKey(uint8_t commType) const
    {
        return _msgOutKeys[commType];
    }

    // Extract 'width' (1, 2, 4 or 8) little-endian bytes starting at byte offset 'offset' as an
    // unsigned value. Shared by extractFieldFrom() below (moved here from ubxMessageVector.h so it
    // can be used from either _storage or _callbackStorage, and by any ubxMessage, without a
    // circular ubxMessage.h <-> ubxMessageVector.h include).
    /**
     * @brief Extract a little-endian unsigned integer of 1, 2, 4 or 8 bytes from a buffer.
     *
     * @param storage The byte buffer to read from.
     * @param offset Byte offset within 'storage' of the first (least significant) byte.
     * @param width Number of bytes to read (1, 2, 4 or 8).
     * @return The decoded unsigned value, widened to uint64_t.
     */
    static uint64_t extractUnsignedBytes(const uint8_t *storage, uint16_t offset, uint8_t width)
    {
        uint64_t val = 0;
        for (uint8_t i = 0; i < width; i++)
            val |= ((uint64_t)storage[offset + i]) << (8 * i);
        return val;
    }

    // Extract 'bitWidth' bits (<= 32) starting at bit 'startBit' within the byte at 'offset'.
    /**
     * @brief Extract a sub-byte bit field from a buffer.
     *
     * @param storage The byte buffer to read from.
     * @param offset Byte offset within 'storage' of the byte containing 'startBit'.
     * @param startBit Bit offset (0-7) within the byte at 'offset' where the field starts.
     * @param bitWidth Width of the field in bits (<= 32).
     * @return The extracted bit field, right-aligned and masked to 'bitWidth' bits.
     */
    static uint32_t extractBits(const uint8_t *storage, uint16_t offset, uint8_t startBit, uint8_t bitWidth)
    {
        uint32_t acc = 0;
        uint8_t bytesNeeded = (uint8_t)((startBit + bitWidth + 7) / 8);
        for (uint8_t i = 0; i < bytesNeeded; i++)
            acc |= ((uint32_t)storage[offset + i]) << (8 * i);
        uint32_t mask = (bitWidth >= 32) ? 0xFFFFFFFFu : (uint32_t)((1UL << bitWidth) - 1UL);
        return (acc >> startBit) & mask;
    }

    // Look up one field of this message, by name, in the given byte buffer (either this object's
    // own _storage for a live/polled read, or its _callbackStorage for a callback read - see
    // getUbxMessageField() below). This is the shared core that used to be duplicated
    // between the live-read path and the (new) callback-read path; ubxMessageVector::extractValue()
    // delegates to this too.
    // 'fieldsOverride'/'numFieldsOverride' let a caller search a field table other than this
    // object's own _fields/_numFields - specifically, a repeated block's own field table (see
    // _blockFields/_numBlockFields below and DevUBLOXGNSS::getUbxMessageBlockField() /
    // getUbxMessageBlockFieldCallback() in u-blox_GNSS.cpp), for a message such as UBX-NAV-SAT
    // that has a header (described by _fields) plus a variable number of identically-shaped
    // repeated blocks (each described by _blockFields). Every existing caller omits these two
    // arguments and gets exactly today's behavior.
    /**
     * @brief Look up one field of this message, by name, in a given payload buffer.
     *
     * Shared core used both for a live/polled read (from this object's own _storage) and for a
     * callback read (from a slot of _callbackStorage) - see getUbxMessageField() below and
     * ubxMessageVector::extractValue().
     *
     * @param buffer The payload bytes to search (this message's _storage, or one
     * _callbackStorage slot).
     * @param fieldName Name of the field to find, matched against each entry's fieldName.
     * @param value Out parameter: filled in with the field's tagged value if found.
     * @param fieldsOverride Optional field table to search instead of this object's own
     * _fields - used to search a repeated block's own field table (see _blockFields). Defaults
     * to nullptr (search _fields).
     * @param numFieldsOverride Number of entries in 'fieldsOverride'; ignored when
     * 'fieldsOverride' is nullptr.
     * @return true if 'fieldName' was found (and *value filled in); false if 'buffer' is null
     * or the field name was not found in the searched table.
     */
    bool extractFieldFrom(const uint8_t *buffer, const char *fieldName, ubxAnyType *value,
                           const ubxField *fieldsOverride = nullptr, uint8_t numFieldsOverride = 0) const
    {
        if (buffer == nullptr)
            return false;

        const ubxField *fields = fieldsOverride ? fieldsOverride : (const ubxField *)_fields;
        uint8_t numFields = fieldsOverride ? numFieldsOverride : _numFields;
        for (uint8_t i = 0; i < numFields; i++)
        {
            if (strncmp(fields[i].fieldName, fieldName, sizeof(fields[i].fieldName)) != 0)
                continue;

            value->ubxDataType = fields[i].ubxDataType;

            if (fields[i].startBit >= 0) // A sub-field: always extracted as an unsigned value
            {
                value->U4 = extractBits(buffer, fields[i].startByte, (uint8_t)fields[i].startBit, (uint8_t)fields[i].bitWidth);
                return true;
            }

            switch (fields[i].ubxDataType)
            {
            case ubxDataType8bit(UBX_CFG_L):
                value->L = (bool)buffer[fields[i].startByte];
                return true;
            case ubxDataType8bit(UBX_CFG_U1):
            case ubxDataType8bit(UBX_CFG_E1):
            case ubxDataType8bit(UBX_CFG_X1):
                value->U1 = (uint8_t)extractUnsignedBytes(buffer, fields[i].startByte, 1);
                return true;
            case ubxDataType8bit(UBX_CFG_I1):
                value->I1 = (int8_t)extractUnsignedBytes(buffer, fields[i].startByte, 1);
                return true;
            case ubxDataType8bit(UBX_CFG_U2):
            case ubxDataType8bit(UBX_CFG_E2):
            case ubxDataType8bit(UBX_CFG_X2):
                value->U2 = (uint16_t)extractUnsignedBytes(buffer, fields[i].startByte, 2);
                return true;
            case ubxDataType8bit(UBX_CFG_I2):
                value->I2 = (int16_t)extractUnsignedBytes(buffer, fields[i].startByte, 2);
                return true;
            case ubxDataType8bit(UBX_CFG_U4):
            case ubxDataType8bit(UBX_CFG_E4):
            case ubxDataType8bit(UBX_CFG_X4):
                value->U4 = (uint32_t)extractUnsignedBytes(buffer, fields[i].startByte, 4);
                return true;
            case ubxDataType8bit(UBX_CFG_I4):
                value->I4 = (int32_t)extractUnsignedBytes(buffer, fields[i].startByte, 4);
                return true;
            case ubxDataType8bit(UBX_CFG_R4):
            {
                uint32_t bits = (uint32_t)extractUnsignedBytes(buffer, fields[i].startByte, 4);
                memcpy(&value->R4, &bits, sizeof(float));
                return true;
            }
            case ubxDataType8bit(UBX_CFG_U8):
            case ubxDataType8bit(UBX_CFG_X8):
                value->U8 = extractUnsignedBytes(buffer, fields[i].startByte, 8);
                return true;
            case ubxDataType8bit(UBX_CFG_I8):
                value->I8 = (int64_t)extractUnsignedBytes(buffer, fields[i].startByte, 8);
                return true;
            case ubxDataType8bit(UBX_CFG_R8):
            {
                uint64_t bits = extractUnsignedBytes(buffer, fields[i].startByte, 8);
                memcpy(&value->R8, &bits, sizeof(double));
                return true;
            }
            default:
                return false; // Unknown ubxDataType
            }
        }
        return false; // Field name not found
    }

    // Defensively computes the real number of repeated blocks present in 'buffer' (either this
    // object's live _storage, or one slot of its _callbackStorage), for any message that set
    // _blockFields via addClassID() - see AGENTS.md "Adding support for ESF-MEAS" and "Adding
    // support for ESF-RAW and ESF-STATUS". 'actualLength' is the real received byte length of
    // 'buffer' (_actualLength for _storage, or the matching _callbackActualLength[] slot for
    // _callbackStorage) - NOT _messageLength, which is only the allocated maximum.
    //
    // If this message set _blockCountField (currently only ESF-MEAS - a message whose own header
    // block-count field is documented as unreliable, e.g. ESF-MEAS's numMeas: "optional, can be
    // obtained from message size"), returns the minimum of: that header field's own value; how
    // many whole blocks actually fit in 'actualLength'; and _maxBlocks.
    //
    // If this message did NOT set _blockCountField, there are two cases. Most variable-length
    // messages (NAV-SAT, MON-COMMS, SEC-SIG, ...) DO have a trustworthy header count field, but
    // the caller is expected to read it directly (e.g. "nPorts", "numSens") and loop with that,
    // exactly as those messages' own file-header comments describe - this function is not needed
    // for them, though calling it anyway is harmless (see below). ESF-RAW is different: it has NO
    // block-count field in its wire format AT ALL (u-blox's own note on the old
    // UBX_ESF_RAW_data_t.numEsfRawBlocks: "this is not contained in the ESF RAW message. It is
    // calculated from the message length.") - for a message like that, this function is the ONLY
    // way to know how many blocks are actually present, so it falls back to using 'actualLength'
    // alone (bounded by _maxBlocks), with no header value to intersect against. Returns 0 if this
    // message has no block support at all (_blockFields == nullptr), or buffer/actualLength don't
    // leave room for even the block header.
    /**
     * @brief Defensively compute how many repeated blocks are actually present in a buffer.
     *
     * For a variable-length message registered with block support (see addClassID()'s
     * blockFields/... parameters), returns the number of complete repeated blocks that fit
     * within 'actualLength', cross-checked against this message's own header count field (if
     * it set one via _blockCountField) and capped at _maxBlocks. See the longer design note
     * above this function's definition for how the with-/without-a-trustworthy-header-field
     * cases differ.
     *
     * @param buffer The payload buffer to measure (this message's live _storage, or a
     * _callbackStorage slot).
     * @param actualLength The real received byte length of 'buffer' (not _messageLength).
     * @return The defensive block count, or 0 if this message has no block support at all
     * (_blockFields == nullptr) or 'buffer' is null.
     */
    uint16_t getBlockCount(const uint8_t *buffer, uint16_t actualLength) const
    {
        if ((_blockFields == nullptr) || (buffer == nullptr))
            return 0;

        uint16_t byLength = 0;
        if ((actualLength > _blockHeaderLength) && (_blockLength > 0))
            byLength = (uint16_t)((actualLength - _blockHeaderLength) / _blockLength);

        uint16_t count = byLength;
        if (_blockCountField != nullptr) // Cross-check against the header field too, if this message set one
        {
            ubxAnyType headerValue;
            uint16_t headerCount = 0;
            if (extractFieldFrom(buffer, _blockCountField, &headerValue))
                headerCount = (uint16_t)(double)headerValue;
            if (headerCount < count)
                count = headerCount;
        }

        if (count > _maxBlocks)
            count = _maxBlocks;
        return count;
    }

    // Attempts to extract field 'fieldName' from this message's optional trailing footer group
    // (_footerFields/_numFooterFields/_footerLength - set via addClassID(), e.g. ESF-MEAS's
    // calibTtag) - see AGENTS.md "Adding support for ESF-MEAS". 'blockCount' is the DEFENSIVE block
    // count for this particular message (see getBlockCount() above, NOT _maxBlocks), used to
    // compute where the footer actually starts for THIS message. Returns false - leaving *value
    // untouched, same convention as extractFieldFrom() - if this message has no footer at all, or
    // if 'actualLength' is too short for the footer to actually have been present in this
    // particular message. This is what lets a caller distinguish "no footer this time" from
    // "footer value happens to be zero" - inferring presence from the footer bytes themselves would
    // be unsafe, since a ring slot or _storage can hold stale trailing bytes from a previous, longer
    // message (see ubxMessageVector::storePayload()).
    /**
     * @brief Extract a named field from this message's optional trailing footer group, if present.
     *
     * @param buffer The payload buffer to read from.
     * @param actualLength The real received byte length of 'buffer'.
     * @param blockCount The defensive block count for this particular message (see
     * getBlockCount()), used to compute where the footer starts.
     * @param fieldName Name of the footer field to find.
     * @param value Out parameter: filled in with the field's tagged value if found.
     * @return true if this message has a footer, the footer was actually present given
     * 'actualLength', and 'fieldName' was found within it; false otherwise (leaving *value
     * untouched).
     */
    bool extractFooterFieldFrom(const uint8_t *buffer, uint16_t actualLength, uint16_t blockCount,
                                 const char *fieldName, ubxAnyType *value) const
    {
        if ((_footerFields == nullptr) || (_numFooterFields == 0) || (buffer == nullptr))
            return false;

        uint16_t footerOffset = _blockHeaderLength + (blockCount * _blockLength);
        if (actualLength < (uint16_t)(footerOffset + _footerLength))
            return false; // This particular message did not actually include the footer

        return extractFieldFrom(buffer + footerOffset, fieldName, value,
                                 (const ubxField *)_footerFields, _numFooterFields);
    }

    // Synthesizes the COMPLETE raw UBX frame (sync bytes + Class + ID + length + payload +
    // checksum) for ring slot 'slotIndex' of _callbackRawFrame, so it can be relayed verbatim (e.g.
    // pushed straight to another device/UART) from inside a callback - see AGENTS.md "Adding
    // support for ESF-MEAS" and getUbxMessageRawLengthCallback()/getUbxMessageRawPtrCallback() in
    // u-blox_GNSS.cpp. Called by ubxMessageVector::storePayload() whenever _callbackRawFrame has
    // been allocated (i.e. a callback is registered - see initCallbackStorage()). 'len' is assumed
    // to already be clamped to _messageLength by the caller, exactly like the copy written into
    // _callbackStorage, so the length field this writes always matches the payload bytes actually
    // present in the frame (0xB5/0x62 are UBX_SYNCH_1/UBX_SYNCH_2 from u-blox_Class_and_ID.h,
    // written as literals here rather than adding that #include to this already-leaf header).
    /**
     * @brief Synthesize a complete raw UBX frame for one callback ring-buffer slot.
     *
     * Writes sync bytes, Class, ID, length, a copy of the payload, and the checksum into
     * _callbackRawFrame at 'slotIndex', so the frame can later be relayed verbatim from inside
     * a callback - see getUbxMessageRawLengthCallback()/getUbxMessageRawPtrCallback() in
     * u-blox_GNSS.cpp. No-op if _callbackRawFrame has not been allocated yet.
     *
     * @param slotIndex Which ring-buffer slot to write into.
     * @param payload Pointer to the payload bytes to copy into the frame.
     * @param len Payload length in bytes (assumed already clamped to _messageLength by the caller).
     * @param checksumA First UBX checksum byte (CK_A).
     * @param checksumB Second UBX checksum byte (CK_B).
     */
    void writeCallbackRawFrame(uint8_t slotIndex, const uint8_t *payload, uint16_t len, uint8_t checksumA, uint8_t checksumB)
    {
        if (_callbackRawFrame == nullptr)
            return;
        uint8_t *frame = _callbackRawFrame + ((uint32_t)slotIndex * ((uint32_t)_messageLength + 8));
        frame[0] = 0xB5; // UBX_SYNCH_1
        frame[1] = 0x62; // UBX_SYNCH_2
        frame[2] = _Class;
        frame[3] = _ID;
        frame[4] = (uint8_t)(len & 0xFF);
        frame[5] = (uint8_t)((len >> 8) & 0xFF);
        memcpy(frame + 6, payload, len);
        frame[6 + len] = checksumA;
        frame[6 + len + 1] = checksumB;
    }

    // Called once, from the subclass's own constructor, to register its identity/metadata into
    // the base class.
    // 'blockFields'/'numBlockFields'/'blockHeaderLength'/'blockLength'/'maxBlocks' describe a
    // variable-length message made of a fixed-size header (already described by 'ubxFields'
    // above) followed by 0..'maxBlocks' identically-shaped repeated blocks - e.g. UBX-NAV-SAT's
    // per-SV blocks. They default to nullptr/0, so every existing message subclass (which passes
    // exactly today's 9 arguments) is unaffected. See AGENTS.md "Adding the variable-length UBX
    // messages".
    // 'blockCountField'/'footerFields'/'numFooterFields'/'footerLength' extend the variable-length
    // support above for a message whose header block-count field cannot be trusted, and/or which
    // has an optional trailing footer group after the last real block (e.g. ESF-MEAS's numMeas/
    // calibTtag) - see AGENTS.md "Adding support for ESF-MEAS" and getBlockCount()/
    // extractFooterFieldFrom() above. They default to nullptr/nullptr/0/0, so every message
    // registered before ESF-MEAS (which passes exactly today's 14 arguments) is unaffected.
    /**
     * @brief Register this message's identity, field table and metadata with the base class.
     *
     * Called once, from the subclass's own constructor, to populate every _* member below with
     * the subclass's Class/ID, field table(s) and configuration keys, and to reset all storage
     * pointers and callback bookkeeping to their initial "nothing allocated yet" state.
     *
     * @param Class The UBX message class.
     * @param ID The UBX message ID (within Class).
     * @param classStr Human-readable 3-letter class mnemonic (e.g. "NAV").
     * @param idStr Human-readable message mnemonic (e.g. "PVT").
     * @param messageLength Payload length in bytes (the fixed/header part for a
     * variable-length message).
     * @param numCallbackCopies Number of ring-buffer slots to allocate for callback storage.
     * @param numFields Number of entries in 'ubxFields'.
     * @param ubxFields Pointer to this message's own, permanently-lived field table.
     * @param msgOutKeys Four UBLOX_CFG_MSGOUT_* keys (I2C, SPI, UART1, UART2) used to
     * enable/disable this message's automatic output.
     * @param blockFields Optional field table describing one repeated block, for a
     * variable-length message (e.g. UBX-NAV-SAT's per-SV blocks). Defaults to nullptr (no
     * repeated blocks).
     * @param numBlockFields Number of entries in 'blockFields'. Defaults to 0.
     * @param blockHeaderLength Bytes before the first repeated block. Defaults to 0.
     * @param blockLength Bytes per repeated block. Defaults to 0.
     * @param maxBlocks Upper bound on the number of repeated blocks. Defaults to 0.
     * @param blockCountField Optional name of a header field to cross-check defensively
     * against the actual received length, for a message whose header count field cannot be
     * trusted (e.g. ESF-MEAS's numMeas). Defaults to nullptr.
     * @param footerFields Optional field table describing an optional trailing footer group
     * (e.g. ESF-MEAS's calibTtag). Defaults to nullptr.
     * @param numFooterFields Number of entries in 'footerFields'. Defaults to 0.
     * @param footerLength Bytes in the footer group. Defaults to 0.
     */
    void addClassID(uint8_t Class, uint8_t ID, const char *classStr, const char *idStr,
                     uint16_t messageLength, uint8_t numCallbackCopies, uint8_t numFields,
                     const void *ubxFields, const uint32_t *msgOutKeys,
                     const void *blockFields = nullptr, uint8_t numBlockFields = 0,
                     uint16_t blockHeaderLength = 0, uint16_t blockLength = 0, uint16_t maxBlocks = 0,
                     const char *blockCountField = nullptr,
                     const void *footerFields = nullptr, uint8_t numFooterFields = 0, uint16_t footerLength = 0)
    {
        _Class = Class;
        _ID = ID;
        _classStr = classStr;
        _idStr = idStr;
        _messageLength = messageLength;
        _numCallbackCopies = numCallbackCopies;
        _numFields = numFields;
        _fields = ubxFields;
        _blockFields = blockFields;
        _numBlockFields = numBlockFields;
        _blockHeaderLength = blockHeaderLength;
        _blockLength = blockLength;
        _maxBlocks = maxBlocks;
        _blockCountField = blockCountField;
        _footerFields = footerFields;
        _numFooterFields = numFooterFields;
        _footerLength = footerLength;
        _actualLength = 0;
        _storage = nullptr; // Only allocated when needed - see initStorage()
        _callbackStorage = nullptr; // Only allocated when needed - see initCallbackStorage()
        _callbackActualLength = nullptr; // Only allocated when needed - see initCallbackStorage()
        _callbackRawFrame = nullptr;     // Only allocated when needed - see initCallbackStorage()
        _callbackHead = 0;
        _callbackTail = 0;
        _callbackCount = 0;
        _callbackReadIndex = 0;
        _moduleQueried = false;
        _callbackPtr = nullptr;
        _automatic = false;
        _implicitUpdate = true;
        _addToFileBuffer = false;
        memcpy(_msgOutKeys, msgOutKeys, sizeof(uint32_t) * 4);
    }

    uint8_t _Class = 0;
    uint8_t _ID = 0;
    const char *_classStr = nullptr;
    const char *_idStr = nullptr;
    uint16_t _messageLength = 0;    // The message PAYLOAD length in bytes (matches e.g. UBX_NAV_PVT_LEN)
    uint8_t _numCallbackCopies = 0; // Number of ring-buffer slots in _callbackStorage - 1 for most messages, >1 for burst messages like RXM-SFRBX/ESF-MEAS - see AGENTS.md "Adding support for RXM-SFRBX"
    uint8_t *_storage = nullptr;    // Raw payload storage - nullptr until initStorage() is called
    uint8_t _numFields = 0;
    // Has fresh data arrived since the last time it was reported? A single flag for the whole
    // message, not a per-field bitmask - see AGENTS.md "moduleQueried".
    bool _moduleQueried = false;
    const void *_fields = nullptr; // Points at the subclass's own, permanently-lived `ubxFields[]` table
    // Variable-length/repeated-block support (e.g. UBX-NAV-SAT's per-SV blocks) - see AGENTS.md
    // "Adding the variable-length UBX messages". _blockFields is nullptr for every ordinary,
    // fixed-shape message; a message made of a header plus repeated blocks sets all five.
    const void *_blockFields = nullptr;   // Points at the subclass's own `ubxBlockFields[]` table; nullptr => no repeated blocks
    uint8_t _numBlockFields = 0;          // Number of entries in _blockFields
    uint16_t _blockHeaderLength = 0;      // Bytes before the first repeated block (e.g. 8 for NAV-SAT)
    uint16_t _blockLength = 0;            // Bytes per repeated block (e.g. 12 for NAV-SAT)
    uint16_t _maxBlocks = 0;              // Upper bound on the number of repeated blocks (e.g. UBX_NAV_SAT_MAX_BLOCKS)
    // Defensive block-count / optional footer support - see AGENTS.md "Adding support for
    // ESF-MEAS" and getBlockCount()/extractFooterFieldFrom() above. nullptr/nullptr/0/0 for every
    // message registered before ESF-MEAS (unaffected).
    const char *_blockCountField = nullptr; // Names a header field to cross-check defensively against actual received length, for a message whose header count field cannot be trusted (e.g. ESF-MEAS's numMeas)
    const void *_footerFields = nullptr;    // Points at the subclass's own `ubxFooterFields[]` table, if any; nullptr => no footer
    uint8_t _numFooterFields = 0;           // Number of entries in _footerFields
    uint16_t _footerLength = 0;             // Bytes in the footer group
    uint16_t _actualLength = 0; // The real received payload byte length of the most recent _storage write, set by storePayload() - see AGENTS.md "Adding support for ESF-MEAS". 0 until the first storePayload() call. Needed because storage bytes beyond a shorter message's length can hold stale data from a previous, longer message.
    uint8_t *_callbackStorage = nullptr; // Ring buffer of _numCallbackCopies slots, each _messageLength bytes - nullptr until initCallbackStorage() is called
    uint16_t *_callbackActualLength = nullptr; // Per-ring-slot counterpart to _actualLength, parallel to _callbackStorage - nullptr until initCallbackStorage() is called
    uint8_t *_callbackRawFrame = nullptr;      // Per-ring-slot COMPLETE raw UBX frame (sync+Class+ID+len+payload+checksum), each 8+_messageLength bytes - nullptr until initCallbackStorage() is called. See getUbxMessageRawLengthCallback()/getUbxMessageRawPtrCallback() in u-blox_GNSS.cpp.
    // Ring-buffer bookkeeping for _callbackStorage - see AGENTS.md "Adding support for RXM-SFRBX".
    // ubxMessageVector::storePayload() (the write side) writes to _callbackHead and advances it;
    // DevUBLOXGNSS::checkCallbacks() (the read side) reads from _callbackTail and advances it,
    // draining oldest-first (FIFO) while _callbackCount > 0. For _numCallbackCopies <= 1 this
    // degenerates to a single slot, both indices always 0 - unchanged from the original behavior.
    uint8_t _callbackHead = 0;      // Next free slot storePayload() will write into
    uint8_t _callbackTail = 0;      // Next fresh slot checkCallbacks() will read and dispatch
    uint8_t _callbackCount = 0;     // How many slots currently hold fresh, undelivered data (replaces the old single _callbackDataValid bool)
    uint8_t _callbackReadIndex = 0; // Set by checkCallbacks() to _callbackTail immediately before each callback firing, so getUbxMessageFieldCallback()/getUbxMessageBlockFieldCallback() know which slot to read
    // Called by DevUBLOXGNSS::checkCallbacks() (via the generic registry walk) once fresh data is
    // waiting - see AGENTS.md "setAutoCallbackPtr". Takes a ubxCallbackDataCommon_t*, not a raw
    // uint8_t* - see AGENTS.md "class ubxMessage needs separate callback storage".
    void (*_callbackPtr)(ubxCallbackDataCommon_t *) = nullptr;
    bool _automatic = false;                   // Is the module set to output this message periodically?
    bool _implicitUpdate = true;               // true: getUBX() itself parses new data; false: caller must call checkUblox() itself
    bool _addToFileBuffer = false;             // Set by logUBX() using setAddToFileBuffer
    uint32_t _msgOutKeys[4] = {0, 0, 0, 0};    // UBLOX_CFG_MSGOUT_* keys for I2C, SPI, UART1, UART2
};

#include "ubxMessageRegistry.h" // Message self-registration - see AGENTS.md "Message Class self-registration"

