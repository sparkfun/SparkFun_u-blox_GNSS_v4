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
 * @file nmeaMessage.h
 *
 * Support for the NMEA message class nmeaMessage
 */

#pragma once

#include "sfe_platform.h"
#include <string.h>

#include "u-blox_config_keys.h" // Needed for the UBLOX_CFG_MSGOUT_NMEA_ID keys

// NMEA message field formats
typedef enum
{
    nmeaDataTypeString, // An array of ASCII char
    nmeaDataTypeTime, // Time in the format HHMMSS. Could have a variable number of decimal places: HHMMSS.SSS
    nmeaDataTypeDDMM, // Degrees Minutes for Latitude: 2 digit degrees, 2 digit minutes. Could have a variable number of decimal places: DDMM.MMMMMMM
    nmeaDataTypeDDDMM, // Degrees Minutes for Longitude: 3 digit degrees, 2 digit minutes. Could have a variable number of decimal places: DDDMM.MMMMMMM
    nmeaDataTypeChar, // A single ASCII character: e.g. N S E W M
    nmeaDataTypeDigit, // A single ASCII digit: 0-9
    nmeaDataTypeNumeric, // A numeric: could be integer or floating point; could be negative
} nmeaFieldFormats_t;

class nmeaMessage; // Forward declaration - see nmeaCallbackDataCommon_t below

// What actually crosses into the user's callback function
typedef struct
{
    char msgId[4]; // Message name - "GGA" etc. - null-terminated
    nmeaMessage *messagePtr; // Opaque - do not dereference directly. Use getNmeaMessagePtr() /
                            // getNmeaMessageField() to read it.
} nmeaCallbackDataCommon_t;

class nmeaMessage
{
public:
    // One entry per field in the message payload.
    typedef struct
    {
        const char fieldName[20];   // Fixed-size array - keeps each field table self-contained (no separately-lived string needed)
        const uint8_t nmeaDataType; // 8-bit type tag, from nmeaFieldFormats_t - see above
        const uint8_t fieldNumber;  // Field number, starting from the Message ID
    } nmeaField;

    /**
     * @brief Construct a new, unregistered nmeaMessage object.
     *
     * Leaves every field at its default (no msgId, no field table, no storage allocated). A
     * subclass calls addNMEA() from its own constructor to actually register its identity and
     * metadata - see addNMEA() below.
     */
    nmeaMessage(void) {}

    /**
     * @brief Destroy this nmeaMessage, freeing any lazily-allocated storage.
     *
     * Frees _storage and _callbackStorage if they were ever allocated (via initStorage() /
     * initCallbackStorage()). Safe to call even if neither was ever allocated, since each is
     * nullptr until first use.
     */
    virtual ~nmeaMessage(void)
    {
        if (_storage != nullptr)
            delete[] _storage;
        if (_callbackStorage != nullptr)
            delete[] _callbackStorage;
    }

    // Does this object represent this message?
    /**
     * @brief Check whether this message object represents the given NMEA message identifier.
     *
     * @param msgId The 3-character NMEA message identifier to test against (e.g. "GGA").
     * @return true if this object's registered _msgId matches, false otherwise.
     */
    bool amI(const char *msgId) const
    {
        return (strcmp(_msgId, msgId) == 0);
    }

    // Lazily allocate _storage - only when the message is actually used. This is how "delete the
    // header, save the RAM" (AGENTS.md) is meant to work for a message that is never instantiated
    // at all - see nmeaMessageVector.h.
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

    /**
     * @brief Lazily allocate this message's callback ring-buffer storage (_callbackStorage), if
     * not already done.
     *
     * Allocates _numCallbackCopies slots of _messageLength bytes each, the first time a
     * callback is registered for this message. Subsequent calls are a no-op.
     *
     * @return true if _callbackStorage is non-null after the call, false if the allocation failed.
     */
    bool initCallbackStorage(void)
    {
        if (_callbackStorage == nullptr)
        {
            _callbackStorage = new uint8_t[_messageLength * _numCallbackCopies];
            if (_callbackStorage != nullptr)
                memset(_callbackStorage, 0, _messageLength * _numCallbackCopies);
        }
        return (_callbackStorage != nullptr);
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

    // Look up one field of this message, by name, in the given byte buffer (either this object's
    // own _storage for a live/polled read, or its _callbackStorage for a callback read.
    // Everything is returned as String. It's just easier that way...
    // 'fieldsOverride'/'numFieldsOverride' let a caller search a field table other than this
    // object's own _fields/_numFields - specifically, a repeated block's own field table (see
    // _blockFields/_numBlockFields below and DevUBLOXGNSS::getNmeaMessageBlockField()/
    // getNmeaMessageBlockFieldCallback() in u-blox_GNSS.cpp), for a message such as NMEA GSV that
    // has a header (described by _fields, fieldNumber < _numHeaderFields) plus a variable number
    // of identically-shaped repeated blocks (each described by _blockFields) plus a footer (also
    // described by _fields, at fieldNumber >= _numHeaderFields). 'blockIndex' selects which block
    // a block-field lookup reads; it's ignored unless 'fieldsOverride' is given. Every existing
    // caller omits all three and gets exactly today's behavior. See AGENTS.md "Adding support for
    // NMEA GSV messages".
    /**
     * @brief Look up one field of this message, by name, in a given NMEA sentence buffer.
     *
     * Walks the comma-delimited sentence in 'buffer' to find field 'fieldName' and returns its
     * text, converted according to that field's nmeaDataType (see nmeaFieldFormats_t above).
     * Shared core used both for a live/polled read (from this object's own _storage) and for a
     * callback read (from _callbackStorage) - see getNmeaMessageField() and
     * getNmeaMessageBlockField()/getNmeaMessageBlockFieldCallback() in u-blox_GNSS.cpp.
     *
     * @param buffer The NMEA sentence bytes to search (this message's _storage, or its
     * _callbackStorage), which must start with '$'.
     * @param fieldName Name of the field to find, matched against each entry's fieldName.
     * @param value Out parameter: filled in with the field's text (converted per its data
     * type), or cleared to an empty String on failure.
     * @param fieldsOverride Optional field table to search instead of this object's own
     * _fields - used to search a repeated block's own field table (see _blockFields, e.g. NMEA
     * GSV's per-satellite blocks). Defaults to nullptr (search _fields).
     * @param numFieldsOverride Number of entries in 'fieldsOverride'; ignored when
     * 'fieldsOverride' is nullptr.
     * @param blockIndex Which repeated block a block-field lookup should read; ignored unless
     * 'fieldsOverride' is given. Defaults to 0.
     * @return true if 'fieldName' was found (and *value filled in); false if 'buffer' is null,
     * malformed, too short, 'blockIndex' is out of range, or the field name was not found.
     */
    bool extractFieldFrom(const uint8_t *buffer, const char *fieldName, sfe_string_t &value,
                           const void *fieldsOverride = nullptr, uint8_t numFieldsOverride = 0,
                           uint16_t blockIndex = 0) const
    {
        if (buffer == nullptr)
            return false;

        if (*buffer != '$') // NMEA messages always start with $
        {
            value = ""; // Clear value just in case
            return false;
        }

        // A block-field lookup with an illegal blockIndex always fails - see AGENTS.md: "If block
        // is illegal ( >= maxNumBlocks), getNmeaMessageBlockFieldCallback should return an empty
        // String." This is a cheap fast-path against the compile-time maximum; the actual
        // per-sentence block count (which can be less than maxNumBlocks - e.g. the last sentence
        // in a GSV group often has fewer than 4 satellites) is checked below too.
        if ((fieldsOverride != nullptr) && (blockIndex >= _maxNumBlocks))
        {
            value = "";
            return false;
        }

        // For a variable-length message (GSV), work out how many blocks THIS sentence actually
        // has - needed both to bounds-check a block-field lookup against the real count (not just
        // the compile-time max, so a caller can't accidentally read into the footer by asking for
        // a block beyond what this specific sentence contains) and to locate a footer field's real
        // position, since it comes after however many blocks are actually present (1..maxNumBlocks).
        uint16_t actualBlockCount = 0;
        if (_blockFields != nullptr)
        {
            uint16_t totalFields = 0;
            for (int y = 1; y < (int)strlen((const char *)buffer); y++)
            {
                if ((buffer[y] == ',') || (buffer[y] == '*'))
                    totalFields++;
            }
            uint8_t numFooterFields = _numFields - _numHeaderFields;
            if (totalFields < ((uint16_t)_numHeaderFields + numFooterFields))
            {
                value = ""; // Sentence too short to even hold the header + footer - malformed
                return false;
            }
            actualBlockCount = (totalFields - _numHeaderFields - numFooterFields) / _numBlockFields;

            if ((fieldsOverride != nullptr) && (blockIndex >= actualBlockCount))
            {
                // Within maxNumBlocks, but beyond what THIS sentence actually contains
                value = "";
                return false;
            }
        }

        const nmeaField *fields = fieldsOverride ? (const nmeaField *)fieldsOverride : (const nmeaField *)_fields;
        uint8_t numFieldsToSearch = fieldsOverride ? numFieldsOverride : _numFields;
        for (uint8_t i = 0; i < numFieldsToSearch; i++)
        {
            if (strncmp(fields[i].fieldName, fieldName, sizeof(fields[i].fieldName)) != 0)
                continue;

            // Work out this field's REAL comma-delimited position in the sentence. For an
            // ordinary fixed-shape message (_blockFields == nullptr) this is always just
            // fields[i].fieldNumber, exactly as before. For a variable-length message like GSV,
            // see AGENTS.md "Adding support for NMEA GSV messages" for the worked example this is
            // checked against.
            uint16_t trueFieldNumber = fields[i].fieldNumber;
            if (_blockFields != nullptr)
            {
                if (fieldsOverride != nullptr)
                {
                    // A block field (e.g. GSV's "svid") - real position is the header, plus every
                    // whole block before this one, plus this field's own position within the block.
                    trueFieldNumber = (uint16_t)_numHeaderFields + ((uint16_t)blockIndex * (uint16_t)_numBlockFields) + fields[i].fieldNumber;
                }
                else if (fields[i].fieldNumber >= _numHeaderFields)
                {
                    // A footer field (e.g. GSV's "signalId") - real position is the header, plus
                    // every block actually present in this sentence, plus this field's own
                    // position past the header.
                    trueFieldNumber = (uint16_t)_numHeaderFields + (actualBlockCount * (uint16_t)_numBlockFields) + (fields[i].fieldNumber - _numHeaderFields);
                }
                // else: an ordinary header field (fieldNumber < _numHeaderFields) - trueFieldNumber is already correct
            }

            const uint8_t *fieldStart = buffer; // Points at the char just before the field data (the preceding delimiter, or '$' for field 0)
            const uint8_t *fieldEnd = buffer + 1; // Point to the first char of the name

            // Count the commas
            int commaCount = 0;
            int x;
            for (x = 1; x < strlen((const char *)buffer); x++) // Assumes sentence is null terminated
            {
                if ((buffer[x] == ',') || (buffer[x] == '*')) // Treat * as a comma delimiter
                {
                    fieldEnd = &buffer[x]; // fieldEnd is the current comma

                    // if commaCount matches trueFieldNumber then buffer[x] is the fieldEnd
                    if (commaCount == trueFieldNumber)
                    {
                        break; // fieldEnd found. We are done
                    }

                    commaCount++; // Increment the count
                    fieldStart = &buffer[x]; // Set fieldStart to this comma
                }
            }

            // If x reached strlen(buffer), the field was not found
            if (x == strlen((const char *)buffer))
            {
                value = ""; // Clear value just in case
                return false;
            }

            // If fieldEnd is 0 or 1 more than fieldStart, the field is empty
            if ((fieldEnd - fieldStart) <= 1)
            {
                value = ""; // Clear value just in case
                return false;
            }

            switch (fields[i].nmeaDataType)
            {
            default:
                value = "Unknown";
                return false;
            case nmeaDataTypeString:
            case nmeaDataTypeTime:
                // Copy from the character after fieldStart
                // to the character before fieldEnd
                fieldStart++;
                value = "";
                while (fieldStart < fieldEnd)
                {
                    value += (char)*fieldStart;
                    fieldStart++;
                }
                return true;
            case nmeaDataTypeDDMM:
            {
                // Convert DDMM to degrees (double)

                fieldStart++;
                double field = (double)(*fieldStart++ - '0') * 10.0;
                field += (double)(*fieldStart++ - '0') * 1.0;
                field += (double)(*fieldStart++ - '0') / 6.0;
                field += (double)(*fieldStart++ - '0') / 60.0;
                int numDPs = 3; // One minute needs at least three decimal places
                if (*fieldStart == '.') // Does it have a decimal point?
                {
                    fieldStart++; // Skip over the decimal point
                    double multiplier = 1.0 / 600.0;
                    while (fieldStart < fieldEnd)
                    {
                        field += (double)(*fieldStart++ - '0') * multiplier;
                        multiplier /= 10.0;
                        numDPs++;
                    }
                }
                sfe_string_from_double(value, field, numDPs);
            }
                return true;
            case nmeaDataTypeDDDMM:
            {
                // Convert DDDMM to degrees (double)
                fieldStart++;
                double field = (double)(*fieldStart++ - '0') * 100.0;
                field += (double)(*fieldStart++ - '0') * 10.0;
                field += (double)(*fieldStart++ - '0') * 1.0;
                field += (double)(*fieldStart++ - '0') / 6.0;
                field += (double)(*fieldStart++ - '0') / 60.0;
                int numDPs = 3; // One minute needs at least three decimal places
                if (*fieldStart == '.') // Does it have a decimal point?
                {
                    fieldStart++; // Skip over the decimal point
                    double multiplier = 1.0 / 600.0;
                    while (fieldStart < fieldEnd)
                    {
                        field += (double)(*fieldStart++ - '0') * multiplier;
                        multiplier /= 10.0;
                        numDPs++;
                    }
                }
                sfe_string_from_double(value, field, numDPs);
            }
                return true;
            case nmeaDataTypeChar:
            case nmeaDataTypeDigit:
                fieldStart++;
                value = "";
                value += (char)*fieldStart;
                return true;
            case nmeaDataTypeNumeric:
            {
                // Convert integer / floating point to double. Could be negative - e.g.
                // nmeaDTM's alt (altitude offset) field.
                fieldStart++;
                bool isNegative = false;
                if (*fieldStart == '-') // Handle a leading minus sign
                {
                    isNegative = true;
                    fieldStart++;
                }
                double field = (double)(*fieldStart++ - '0');
                while ((*fieldStart != '.') && (fieldStart < fieldEnd))
                {
                    field *= 10.0;
                    field += (double)(*fieldStart++ - '0');
                }
                int numDPs = 0;
                if ((*fieldStart == '.') && (fieldStart < fieldEnd))
                {
                    fieldStart++; // Skip over the decimal point
                    double multiplier = 1.0 / 10.0;
                    while (fieldStart < fieldEnd)
                    {
                        field += (double)(*fieldStart++ - '0') * multiplier;
                        multiplier /= 10.0;
                        numDPs++;
                    }
                }
                if (isNegative)
                    field = -field;
                sfe_string_from_double(value, field, numDPs);
            }
                return true;
            }
        }
        value = ""; // Clear value just in case
        return false; // Field name not found
    }

    // Called once, from the subclass's own constructor, to register its identity/metadata into
    // the base class.
    // 'blockFields'/'numBlockFields'/'numHeaderFields'/'maxNumBlocks' describe a variable-length
    // message made of a fixed-size header (already described by 'nmeaFields' above, fieldNumber <
    // numHeaderFields) followed by 1..'maxNumBlocks' identically-shaped repeated blocks, then
    // optionally more 'nmeaFields' entries acting as a footer (fieldNumber >= numHeaderFields) -
    // e.g. GSV's per-satellite blocks. They default to nullptr/0, so every existing message
    // subclass (which passes exactly today's 6 arguments) is unaffected. See AGENTS.md "Adding
    // support for NMEA GSV messages".
    /**
     * @brief Register this message's identity, field table and metadata with the base class.
     *
     * Called once, from the subclass's own constructor, to populate every _* member below with
     * the subclass's message identifier, field table(s) and configuration keys, and to reset
     * all storage pointers and callback bookkeeping to their initial "nothing allocated yet"
     * state.
     *
     * @param msgId The 3-character NMEA message identifier (e.g. "GGA").
     * @param messageLength Maximum message length in bytes (an actual received sentence may be shorter).
     * @param numCallbackCopies Number of ring-buffer slots to allocate for callback storage.
     * @param numFields Number of entries in 'nmeaFields' (header + footer fields only - excludes 'numBlockFields').
     * @param nmeaFields Pointer to this message's own, permanently-lived field table.
     * @param msgOutKeys Four UBLOX_CFG_MSGOUT_* keys (I2C, SPI, UART1, UART2) used to enable/disable
     * this message's automatic output.
     * @param blockFields Optional field table describing one repeated block, for a
     * variable-length message (e.g. NMEA GSV's per-satellite blocks). Defaults to nullptr (no
     * repeated blocks).
     * @param numBlockFields Number of entries in 'blockFields'. Defaults to 0.
     * @param numHeaderFields Number of header fields before the first repeated block (also the
     * field-table position of the first footer field). Defaults to 0.
     * @param maxNumBlocks Upper bound on the number of repeated blocks. Defaults to 0.
     */
    void addNMEA(const char *msgId, uint8_t messageLength, uint8_t numCallbackCopies, uint8_t numFields,
                const void *nmeaFields, const uint32_t *msgOutKeys,
                const void *blockFields = nullptr, uint8_t numBlockFields = 0,
                uint8_t numHeaderFields = 0, uint8_t maxNumBlocks = 0)
    {
        _msgId = msgId;
        _messageLength = messageLength;
        _numCallbackCopies = numCallbackCopies;
        _numFields = numFields;
        _fields = nmeaFields;
        _blockFields = blockFields;
        _numBlockFields = numBlockFields;
        _numHeaderFields = numHeaderFields;
        _maxNumBlocks = maxNumBlocks;
        _storage = nullptr; // Only allocated when needed - see initStorage()
        _callbackStorage = nullptr; // Only allocated when needed - see initCallbackStorage()
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

    const char *_msgId = nullptr;
    uint8_t _messageLength = 0;    // The maximum message length in bytes. Messages could be less than this
    uint8_t _numCallbackCopies = 0; // Number of ring-buffer slots in _callbackStorage - 1 for most messages, 54 for GSV - see AGENTS.md "Adding support for NMEA GSV messages"
    uint8_t *_storage = nullptr;    // Raw payload storage - nullptr until initStorage() is called
    uint8_t _numFields = 0;         // Number of entries in _fields (header + footer fields only - excludes _numBlockFields)
    // Has fresh data arrived since the last time it was reported? A single flag for the whole
    // message, not a per-field bitmask - see AGENTS.md "moduleQueried".
    bool _moduleQueried = false;
    const void *_fields = nullptr; // Points at the subclass's own, permanently-lived `nmeaFields[]` table
    // Variable-length repeated-block support (GSV's per-satellite blocks) - see AGENTS.md "Adding
    // support for NMEA GSV messages". _blockFields is nullptr for every ordinary, fixed-shape
    // message; GSV sets all four. Unlike ubxMessage's binary blocks, there's no fixed byte length
    // per block here (NMEA is ASCII/comma-delimited) - extractFieldFrom() locates a block field's
    // real comma position from _numHeaderFields/_numBlockFields/blockIndex instead of a byte offset.
    const void *_blockFields = nullptr; // Points at the subclass's own `nmeaBlockFields[]` table; nullptr => no repeated blocks
    uint8_t _numBlockFields = 0;        // Number of entries in _blockFields
    uint8_t _numHeaderFields = 0;       // Number of header fields before the first repeated block (also the field-table position of the first footer field)
    uint8_t _maxNumBlocks = 0;          // Upper bound on the number of repeated blocks (e.g. GSV's 4)
    uint8_t *_callbackStorage = nullptr; // Ring buffer of _numCallbackCopies slots, each _messageLength bytes - nullptr until initCallbackStorage() is called
    // Ring-buffer bookkeeping for _callbackStorage - mirrors ubxMessage's, see AGENTS.md "Adding
    // support for RXM-SFRBX" and "Adding support for NMEA GSV messages". The write side (the NMEA
    // dispatch block in DevUBLOXGNSS::process()) writes to _callbackHead and advances it; the read
    // side (DevUBLOXGNSS::checkCallbacks()) reads from _callbackTail and advances it, draining
    // oldest-first (FIFO) while _callbackCount > 0. For _numCallbackCopies <= 1 (every message
    // except GSV) this degenerates to a single slot, both indices always 0 - unchanged from the
    // original behavior.
    uint8_t _callbackHead = 0;      // Next free slot the write side will write into
    uint8_t _callbackTail = 0;      // Next fresh slot checkCallbacks() will read and dispatch
    uint8_t _callbackCount = 0;     // How many slots currently hold fresh, undelivered data (replaces the old single _callbackDataValid bool)
    uint8_t _callbackReadIndex = 0; // Set by checkCallbacks() to _callbackTail immediately before each callback firing, so getNmeaMessageFieldCallback()/getNmeaMessageBlockFieldCallback() know which slot to read
    // Called by DevUBLOXGNSS::checkCallbacks() (via the generic registry walk) once fresh data is waiting
    void (*_callbackPtr)(nmeaCallbackDataCommon_t *) = nullptr;
    bool _automatic = false;                   // Is the module set to output this message periodically?
    bool _implicitUpdate = true;               // true: getNMEA() itself parses new data; false: caller must call checkUblox() itself
    bool _addToFileBuffer = false;             // Set by logNMEA() using setAddToFileBuffer
    uint32_t _msgOutKeys[4] = {0, 0, 0, 0};    // UBLOX_CFG_MSGOUT_* keys for I2C, SPI, UART1, UART2
};

#include "nmeaMessageRegistry.h" // Message self-registration - see AGENTS.md "Message Class self-registration"

