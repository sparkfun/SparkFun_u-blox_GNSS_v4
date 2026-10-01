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
 * @file sfe_debug.h
 */

/*
  v4 scaffolding: debugPrint()/debugPrintln() extracted from DevUBLOXGNSS into their own small
  base class, SfeDebugPrint, so other classes can inherit real, working debug-print support too -
  not just DevUBLOXGNSS. Added to let ubxMessageVector and nmeaMessageVector print diagnostics
  from deep inside their own methods (e.g. a full callback ring buffer dropping a message) without
  a callback or pointer back into DevUBLOXGNSS.

  ubxMessageVector and nmeaMessageVector are owned BY VALUE as public members of DevUBLOXGNSS
  (ubxMessages, nmeaMessages) - the opposite containment direction from inheritance - and
  DevUBLOXGNSS's own header (u-blox_GNSS.h) includes ubxMessageVector.h/nmeaMessageVector.h, not
  the other way around. So this class lives in its own leaf header with no dependency on
  u-blox_GNSS.h, and DevUBLOXGNSS, ubxMessageVector and nmeaMessageVector each inherit it
  independently.

  Plain (non-static) inheritance means each of the three gets its OWN copy of _debugSerial/
  _printDebug/_printLimitedDebug - so on its own, a ubxMessages.debugPrint(...) call would compile
  but never print anything, because myGNSS.enableDebugging() only ever sets DevUBLOXGNSS's own
  inherited copy of that state, not ubxMessages'/nmeaMessages' separate copies. `static` shared
  state would fix that but wrongly couple debug on/off across every SFE_UBLOX_GNSS instance in a
  sketch. Instead, copyDebugStateFrom() lets DevUBLOXGNSS::enableDebugging()/disableDebugging()
  (see u-blox_GNSS.cpp) explicitly push its own state onto ubxMessages/nmeaMessages right after
  updating it, so all three stay in sync per-instance without any shared/static state.
*/

#pragma once

#include "sfe_platform.h"
#include "sfe_bus.h"

namespace SparkFun_UBLOX_GNSS
{

  class SfeDebugPrint
  {
  public:
    /**
     * @brief Print a debug message (no trailing newline), if debugging is enabled.
     *
     * A no-op unless _printDebug is true; further suppressed if _printLimitedDebug is true and
     * 'important' is false - see enableDebugging()/enableLimitedDebugging() in u-blox_GNSS.cpp.
     *
     * @param message Null-terminated string to print.
     * @param important If true, print even when limited debugging is active. Defaults to false.
     */
    void debugPrint(const char *message, bool important = false);             // Safely print debug statements
    /**
     * @brief Print a debug value in decimal (no trailing newline), if debugging is enabled.
     *
     * @param value The value to print.
     * @param important If true, print even when limited debugging is active. Defaults to false.
     */
    void debugPrint(uint32_t value, bool important = false);                  // Safely print debug values
    /**
     * @brief Print a debug value in a given base (no trailing newline), if debugging is enabled.
     *
     * @param value The value to print.
     * @param printBase Number base to print in (e.g. HEX, DEC).
     * @param important If true, print even when limited debugging is active. Defaults to false.
     */
    void debugPrint(uint32_t value, int printBase, bool important = false);   // Safely print debug values in a given base (e.g. HEX)
    /**
     * @brief Print a debug message followed by a newline, if debugging is enabled.
     *
     * @param message Null-terminated string to print.
     * @param important If true, print even when limited debugging is active. Defaults to false.
     */
    void debugPrintln(const char *message, bool important = false);           // Safely print debug statements
    /**
     * @brief Print a debug value in decimal followed by a newline, if debugging is enabled.
     *
     * @param value The value to print.
     * @param important If true, print even when limited debugging is active. Defaults to false.
     */
    void debugPrintln(uint32_t value, bool important = false);                // Safely print debug values
    /**
     * @brief Print a debug value in a given base followed by a newline, if debugging is enabled.
     *
     * @param value The value to print.
     * @param printBase Number base to print in (e.g. HEX, DEC).
     * @param important If true, print even when limited debugging is active. Defaults to false.
     */
    void debugPrintln(uint32_t value, int printBase, bool important = false); // Safely print debug values in a given base (e.g. HEX)
    /**
     * @brief Print a blank debug line, if debugging is enabled and not in limited-debug mode.
     */
    void debugPrintln(void);                                                  // Safely print a blank debug line

    // Copies debug state (which port to print to, and whether printing is enabled) from another
    // SfeDebugPrint object. Public - and taking the state rather than pulling it - because the
    // caller is normally DevUBLOXGNSS, reaching in from OUTSIDE to push its own state onto its
    // separately-inherited ubxMessages/nmeaMessages member objects. See the file comment above.
    /**
     * @brief Copy debug state (output stream and enabled flags) from another SfeDebugPrint.
     *
     * Lets DevUBLOXGNSS::enableDebugging()/disableDebugging() push its own state onto its
     * separately-inherited ubxMessages/nmeaMessages member objects right after updating it, so
     * all three stay in sync per-instance without any shared/static state - see the file-level
     * comment above.
     *
     * @param other The SfeDebugPrint object whose debug state to copy.
     */
    void copyDebugStateFrom(const SfeDebugPrint &other)
    {
      _debugSerial = other._debugSerial;
      _printDebug = other._printDebug;
      _printLimitedDebug = other._printLimitedDebug;
    }

  protected:
    SfePrint _debugSerial;           // The stream to send debug messages to if enabled
    bool _printDebug = false;        // Flag to print the serial commands we are sending to the Serial port for debug
    bool _printLimitedDebug = false; // Flag to print limited debug messages. Useful for I2C debugging or high navigation rates
  };

} // namespace SparkFun_UBLOX_GNSS
