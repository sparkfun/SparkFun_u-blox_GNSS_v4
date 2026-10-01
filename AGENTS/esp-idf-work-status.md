# ESP-IDF component work: status

Repo: C:\Users\pc235\Documents\GitHub\SparkFun_u-blox_GNSS_v4 (PaulZC fork), branch `esp-idf-component`, based on 08943c2.
Paul does all commits, builds (ESP-IDF + Arduino IDE) and hardware tests; he pastes the results into the chat.
Registry name: `sparkfun/sparkfun_u-blox_gnss_v4`. Approach: option 2b (native ESP-IDF bus back end inside v4). See the report.
A copy of this file is kept in the project as claude/esp-idf-work-status.md.

## Done (24 Sep 2026): Phases 1–3 + Phase 4 example conversion
- New: src/sfe_platform.h (sfe_millis/micros/delay, sfe_pin_*, sfe_string_t, sfe_string_from_double, DEC/HEX)
- New: src/sfe_bus_esp_idf.h/.cpp (SfeI2C on i2c_master, SfeSPI on spi_master with manual CS, SfeSerial on uart; SfeOutput/SfeStdoutOutput/SfeUartOutput/SfeCallbackOutput; SfePrint; sfeStdout)
- sfe_bus.h/.cpp: Arduino classes wrapped in #if SFE_ARDUINO; typedef sfe_print_t (Print on Arduino, SfeOutput on IDF)
- u-blox_GNSS.h/.cpp, nmeaMessage.h, nmeaMessageVector.h, sfe_debug.h, ubxMessage.h: includes → sfe_platform.h; millis/delay/pinMode → sfe_*; String → sfe_string_t; Print& → sfe_print_t&; ESP-IDF enableDebugging default = sfeStdout; one strstr `char *ptr` → `const char *ptr`
- SparkFun_u-blox_GNSS_v4.h: ESP-IDF begin() overloads: I2C(bus, addr), SPI(host, cs, speed) / (spi_device_handle_t, cs), UART(uart_port_t); plus the SUPER class
- Root: CMakeLists.txt, idf_component.yml (idf >=5.3), Kconfig (I2C clock, I2C timeout)
- idf_examples/PollingExample1 (I2C), 2 (UART), 3 (SPI): main.cpp, Kconfig.projbuild pins, sdkconfig.defaults (FREERTOS_HZ=1000)
- README: ESP-IDF section; .gitignore: build outputs; idf_component.yml excludes *Dockerfile, *.bin, *.elf, *.bat

## Build results (24 Sep 2026)
- Arduino (arduino-cli, esp32:esp32 3.3.11, Arduino.Dockerfile): PollingExample1 compiles. Fixed: String(double, int) overload ambiguity in sfe_platform.h
- ESP-IDF v6.1 (GCC 15.2, -Werror, IDF.Dockerfile): PollingExample1 compiles and links. Fixed: Docker WORKDIR /${COMPONENT} (component dir name was empty); uart_port_t is an enum in IDF v6 (_port{UART_NUM_0})
- IDF_compile_example.bat args: COMPONENT EXAMPLE
- Hardware: Arduino PollingExample1 runs correctly on ESP32 + u-blox over I2C (Arduino regression OK)
- Hardware: ESP-IDF v6.1 PollingExample1 runs correctly on ESP32 (rev v3.0) + u-blox over I2C. PHASE 4 SUCCESS INDICATOR MET (24 Sep 2026)
- Hardware: ESP-IDF enableDebugging() output works (stdout sink, HEX formatting). NAV-PVT polls take 525–1025 ms with "checkUbloxI2C: 2856 bytes available" each poll
- Hardware: Arduino debug output matches: ~2732 bytes available per poll, NAV-PVT polls 915–1020 ms. ESP-IDF timing is equivalent to Arduino (polled replies wait for the next 1 Hz epoch; the module also outputs periodic data on I2C)
- Hardware: ESP-IDF PollingExample2 (UART1, 38400 baud) runs correctly with debug enabled. getVal uses the UART1 key (0x10730001); NAV-PVT polls 307–979 ms
- Example 3 SPI CS default changed to GPIO 4 (matches the Arduino example)
- Hardware: ESP-IDF PollingExample3 (SPI2_HOST, CS GPIO 4, 4 MHz) runs correctly with debug enabled. getVal uses the SPI key (0x10790001); spiBuffer 308 bytes; NAV-PVT polls 794–1005 ms
- ALL THREE BUSES (I2C, UART, SPI) CONFIRMED ON HARDWARE WITH ESP-IDF v6.1 (24 Sep 2026)
- Modules: I2C and UART tests used a ZED-X20P; the SPI test used an older ZED-F9P with D_SEL set for SPI (explains 18 vs 31 SVs and no diffSoln)
- Converted (24 Sep 2026): PeriodicExample1_NAVHPPOSLLH, PeriodicExample2_GPGGA, CallbackExample1_NAVHPPOSLLH, CallbackExample2_GPRMC (all I2C; added to idf_component.yml examples list)
- Hardware: ESP-IDF PeriodicExample1_NAVHPPOSLLH runs correctly (debug enabled): setAutoUBX CFG-VALSET ACKed; HPPOSLLH arrives at 1 Hz; printed fields match payload bytes; progress dots appear live (fflush OK)
- Hardware: ESP-IDF PeriodicExample2_GPGGA runs correctly: NMEA field getters return std::string; DDMM→degrees via sfe_string_from_double gives 8 dp (54.86654750); UTC time advances 1 s per GGA. NAV-HPPOSLLH still arriving because PeriodicExample1 enabled it in RAM+BBR
- Hardware: ESP-IDF CallbackExample1_NAVHPPOSLLH runs correctly: setCfgValset ACKed; static callback fires once per HPPOSLLH (1 Hz) via checkUblox()/checkCallbacks(); printed fields match payload bytes
- Hardware: ESP-IDF CallbackExample2_GPRMC runs correctly: NMEA callback fires once per RMC; std::string fields (time, date 240926, NS, EW) and DDMM→degrees OK. All 7 converted ESP-IDF examples now run on hardware
- Converted (24 Sep 2026; builds OK on ESP-IDF v6.1, 334 KB): DataloggingExample1_RAWX_and_SFRBX. SD card via ESP-IDF FATFS VFS (esp_vfs_fat_sdspi_mount) in main/sd_card.c (C, because of the sdmmc/sdspi C initializer macros); fopen/fwrite/fclose; key press via non-blocking stdin (fcntl O_NONBLOCK + fgetc); freeze() uses vTaskDelay; file opened with "a" (append, as the Arduino comment says; Arduino-ESP32 FILE_WRITE is actually "w")
- Hardware: ESP-IDF DataloggingExample1_RAWX_and_SFRBX STRESS TEST PASSED (24 Sep 2026, ZED-X20P, I2C, transaction size 32, Tera Term): ~193 s logging, 539,720 bytes to a 32 GB SDHC card. UBX_Integrity_Checker: 96 RAWX + 3596 SFRBX, no checksum failures, longest message 3672 bytes; counts match the callback counters exactly. No 80%-full buffer warning; enableDebugging(sfeStdout, true) printed no important errors. Key press via non-blocking stdin works. Benign IDF warning at unmount: "W gpio: conflict found for GPIO[5]" (SD CS)
- Converted (24 Sep 2026; builds OK on ESP-IDF v6.1, 333 KB): DataloggingExample2_DataLogger_IoT_SDIO (new Arduino example from Paul): GNSS on SPI2_HOST (CS 33) at 4 MHz, 20 Hz nav, RAWX+SFRBX every epoch, NMEA at 1 Hz; SD via SDMMC 4-bit slot 1 (esp_vfs_fat_sdmmc_mount, main/sdmmc_card.c); timestamped long file name needs CONFIG_FATFS_LFN_HEAP (in sdkconfig.defaults); STAT LED 25 (the .ino loop uses LED_BUILTIN); EN_3V3_SW 32; IMU_CS 5 and MAG_CS 27 held high.
- Hardware (24 Sep 2026): DataloggingExample2 FAILED as first converted: no status lines, file buffer and SFRBX callback ring buffer full, then task watchdog (IDLE0) at ~24 s. Diagnosis: checkUbloxSpi() read one byte per writeReadByte() (one spi_device_polling_transmit each on ESP-IDF) and loops until the module has no data - at 20 Hz it never caught up, so checkUblox() never returned (no SD writes, no checkCallbacks, IDLE0 starved)
- FIX (24 Sep 2026, not yet tested): checkUbloxSpi() rewritten (shared code, Arduino too) to read blocks of spiTransactionSize (max 128) bytes with writeReadBytes(), skipping 0xFF filler when currentSentence is NONE; stops when a block ends with 0xFF outside a sentence, or after 16384 bytes per call so checkUblox() always returns
- Hardware (24 Sep 2026): Arduino DataloggingExample2 with the new checkUbloxSpi() runs (~50 s, 2,512,620 bytes, file clean): 1059 RAWX (~20 Hz), 1819 SFRBX, NMEA only 17 epochs (~1 per 3 s instead of 1 Hz - to investigate, possibly module-side TX buffer drops). Callback counts lag the file (SFRBX callback ring buffer full) - expected, see the example header note
- Hardware (24 Sep 2026): ESP-IDF DataloggingExample2 with the new checkUbloxSpi(): ~70 s, 3,371,432 bytes, clean, no watchdog, no file-buffer warnings, 1444 RAWX (~20 Hz). Only SFRBX callback ring-buffer-full warnings
- Hardware (24 Sep 2026): same, with UBX_RXM_SFRBX_CALLBACK_BUFFERS = 60 and RAWX numCallbackCopies = 4 (local test edits): ULTIMATE STRESS TEST PASSED - ~80 s, 3,760,382 bytes, clean, NO warnings at all. File: 1679 RAWX, 2699 SFRBX; callback counters 1663 / 2643 at the last status line (the difference is the final second before stopping)
- NMEA note: in every 20 Hz run, NMEA appears once per ~60 RAWX (1059/17, 1444/24, 1679/28), i.e. every 3 s, on Arduino and ESP-IDF alike. Likely explanation: the ZED-F9P computes navigation solutions at ~20/3 Hz with this many constellations (RAWX measurements still at 20 Hz), so NMEA at "every 20 solutions" = every 3 s. Module behaviour, not the port - unconfirmed
- Callback-buffer test edits reverted by Paul (UBX_RXM_SFRBX_CALLBACK_BUFFERS back to 50, RAWX numCallbackCopies back to 1)
- Hardware (24 Sep 2026): ESP-IDF PollingExample3 (SPI) regression with the new checkUbloxSpi() PASSED (debug enabled): getVal ACK after 28 ms (was 61 ms), NAV-PVT polls 215–1015 ms, clean. The Arduino SPI path with the new code was exercised by Arduino DataloggingExample2
- Hardware (24 Sep 2026): Arduino PollingExample3 (SPI) regression with the new checkUbloxSpi() PASSED (debug enabled): getVal ACK after 47 ms, correctly skipped an unrequested periodic NAV-PVT that arrived first, 16 clean NAV-PVT polls. The new checkUbloxSpi() is now verified on both platforms
- Selected (24 Sep 2026): of the remaining 15 Arduino examples, convert only those that exercise untested ESP-IDF code paths. Skipped: CallbackExample3/5/6/9/15 (patterns already proven), ESF 10-12 (need ZED-F9R), D9S/D9C 13-14 (no service / hardware to test)
- Converted (24 Sep 2026, stub syntax check only, not yet built): CallbackExample7_NMEA_GSV (std::string NMEA block fields), PollingExample5_GPZDA (polled NMEA via getNMEA), PollingExample4_SECUNIQID (getUniqueChipIdStr -> std::string), CallbackExample4_NAVSAT (variable-length UBX block fields), CallbackExample8_MONCOMMS (block fields; port diagnostics). All I2C, SDA 21 / SCL 22. Added to the examples list in idf_component.yml
- Hardware (24 Sep 2026): ESP-IDF CallbackExample7_NMEA_GSV PASSED (ZED-X20P, I2C): full GSV table every 2 epochs for GPS L1/L2/L5, GLONASS L1/L2, Galileo E1/E5a/E6, BeiDou B1C/B2a/B3; SBAS 33-64 -> 121/123/136 and GLONASS 65-96 -> 1-32 remaps correct. std::string NMEA field and block-field getters confirmed
- Hardware (24 Sep 2026): ESP-IDF PollingExample5_GPZDA PASSED (ZED-X20P, I2C): polled ZDA once per second, consecutive UTC times, date 24/09/2026, no gaps or duplicates, no watchdog despite the loop having no explicit delay (the library's polling wait yields). Polled NMEA (getNMEA / pollNMEA) + getNmeaMessageField -> std::string confirmed
- Hardware (24 Sep 2026): ESP-IDF PollingExample4_SECUNIQID PASSED (ZED-X20P, I2C): "Unique chip ID: 0xB8D3F70F5C54" (6 bytes, uppercase hex), ~90 ms after app_main starts; app_main then returns cleanly. getSECUNIQID + getUniqueChipIdStr -> std::string confirmed
- Hardware (24 Sep 2026): ESP-IDF CallbackExample4_NAVSAT PASSED (ZED-X20P, I2C): NAV-SAT every 2 epochs, 53 SVs per message (644-byte payload), all 53 blocks decoded: gnssId, svId (SBAS 121/123/136), qualityInd 0/1/7, svUsed flag bit, cno. Variable-length UBX block accessors (getUbxMessageFieldCallback / getUbxMessageBlockFieldCallback incl. bit fields) confirmed
- Hardware (24 Sep 2026): ESP-IDF CallbackExample8_MONCOMMS PASSED (ZED-X20P, I2C): MON-COMMS every 2 epochs, 3 ports (I2C, UART1, USB), txBytes/rxBytes/msgs[]/skipped all decoded; I2C txBytes ~2 KB/s, skipped 0. I2C rx NMEA count 181 = the ZDA polls sent by PollingExample5 (counters persist since module power-up). ALL 5 SELECTED EXAMPLES NOW PASS ON HARDWARE
- Observation (MONCOMMS run): at start-up, two I2C probe timeouts ("i2c.master: I2C hardware timeout detected" / "probe device timeout") plus "GPIO 21/22 is not usable, maybe conflict with others" (logged by the IDF driver's FSM reset / bus recovery re-applying the pins). begin() succeeded on its third isConnected attempt, no retry message. Likely cause: the ESP32 was reset (flash) while the previous app (NAVSAT, 644-byte reads) was mid-I2C-read, leaving the module holding SDA; the IDF driver's recovery clears it. Possible library mitigation (not done, needs Paul's OK): call i2c_master_bus_reset() in SfeI2C::init() before the first probe. Check: does it happen after a power cycle, or only after reset-while-streaming?
- Follow-up (24 Sep 2026): after a power cycle (module counters restarted) and a hardware reset, MONCOMMS starts cleanly: no probe timeouts, no GPIO warnings. The timeouts were only seen after re-flashing while the NAVSAT app (long 644-byte reads, high I2C duty cycle) was running - consistent with a slave left mid-read. Self-recovering (IDF bus recovery + begin()'s 3 isConnected attempts)
- Decision (Paul, 24 Sep 2026): leave as is - no library change (no i2c_master_bus_reset in SfeI2C::init) and no README note

## Conventions for the ESP-IDF examples
- I2C: `busConfig.flags.enable_internal_pullup = false; // u-blox modules have their own internal active pull-ups` (Paul, 24 Sep 2026: extra pull-ups have caused I2C problems with u-blox modules). Tested OK on PollingExample1
- Example conversion: Serial.print → printf (%s with .c_str() for NMEA string fields); the loop's "." progress dots use fflush(stdout) because stdout is line-buffered

## Done (25 Sep 2026): documentation and PR preparation
- READMEs added: examples/README.md, idf_examples/README.md, Utils/README.md (all three included in the Doxygen docs; USE_MDFILE_AS_MAINPAGE = ./README.md so the sub-folder READMEs are not treated as the main page)
- Main README: section links and LICENSE.md link fixed so they work on GitHub and in the Doxygen HTML; Arduino examples link fixed
- Examples: one comment per example above the first setCfgValset() / newCfgValset() explaining how to choose the VAL_LAYER (default VAL_LAYER_RAM_BBR)
- Utils/UBX_RAWX_Aligner.py: decimalPlaces argument fixed (int(sys.argv[2]))
- Version 4.1.0 in library.properties, README and idf_component.yml
- Final pre-PR check: 19 commits ahead of upstream 08943c2, pushed to origin; no build artifacts tracked; no enableDebugging() active; line endings consistent (LF in repo, CRLF checkout); library and IDF examples stub-compile cleanly; example changes since the last hardware test are comments only

## PR (25 Sep 2026)
- PR #2 opened: PaulZC:esp-idf-component -> sparkfun:main (https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/pull/2)
- New workflow .github/workflows/compile-idf-example.yml (IDF Compile Test): builds idf_examples/DataloggingExample2_DataLogger_IoT_SDIO for esp32 in espressif/idf:release-v5.3 and release-v6.1 containers; triggers: pull_request + workflow_dispatch only. README badge added
- First run: IDF v5.3 failed - GCC 13 at -Og: -Werror=maybe-uninitialized false positives in u-blox_GNSS.cpp (getUBX, getNMEA, assumeAutoUBX, assumeAutoNMEA). Fix: 14 locals initialized (bool = false / uint8_t = 0); isGNSSenabled now returns false if getVal8 fails. Verified with GCC 13 at -O0/-Og/-O1/-O2/-Os/-O3
- STM32 Arduino job failed once with 'API rate limit exceeded' (infrastructure) - passed on re-run
- ALL 11 CHECKS PASSED (Arduino: 9 platforms; IDF: v5.3 and v6.1). No conflicts with base branch

## Next
- SparkFun: review and merge PR #2
- After the merge: tag the v4.1.0 release; register / publish sparkfun/sparkfun_u-blox_gnss_v4 in the ESP-IDF Component Registry
- Optional: add more idf_examples to the IDF Compile Test matrix; i2cTransactionSize tuning
- Deferred: new(std::nothrow); FreeRTOS lock option; Arduino-as-component path in CMakeLists (untested); benign "W gpio: conflict found for GPIO[5]" at SD-over-SPI unmount; FAT file timestamps (set the system time from GNSS)
