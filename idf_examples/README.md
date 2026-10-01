# SparkFun u-blox GNSS v4: ESP-IDF Examples

These are ESP-IDF versions of a selection of the Arduino examples in the [examples](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/examples) folder. Each one is a complete ESP-IDF project, with the application in `main/main.cpp`. The library is C++, so the examples are C++ too, with `extern "C" void app_main()`.

To build, flash and monitor an example:

```
cd idf_examples/PollingExample1_PositionVelocityTime
idf.py set-target esp32
idf.py menuconfig    # Optional: "Example Configuration" sets the GPIO pins
idf.py -p PORT flash monitor
```

Each example's `main/idf_component.yml` pulls in the library with `override_path: "../../../"`, so it builds against the library in this repository. (`override_path` is removed automatically when an example is downloaded from the ESP-IDF Component Registry.) Each example's `sdkconfig.defaults` sets `CONFIG_FREERTOS_HZ=1000`. This is recommended: it makes the library's short (1ms) delays true sleeps.

The examples show the three ways the library can read messages from a u-blox GNSS module:

* **Polling**: the code asks the module for a message, and waits for the reply. For example `getNAVPVT()`, `getUBX("NAV", "PVT")` or `getNMEA("ZDA")`.
* **Periodic**: the module sends the message automatically, every navigation epoch (or every _n_ epochs). The code checks whether a fresh message has arrived. For example `setAutoUBX("NAV", "HPPOSLLH")` then `getNAVHPPOSLLH()`.
* **Periodic with callback**: the module sends the message automatically. The library calls your callback function when it arrives. Register the callback with `setAutoCallbackPtr()` (UBX) or `setNmeaCallbackPtr()` (NMEA). Call `checkUblox()` and `checkCallbacks()` regularly in your main loop.

Fields are read by name:

* UBX: `getUbxMessageField()` / `getUbxMessageFieldCallback()` return a `ubxAnyType`, which converts to `double` or can be read as its true type (e.g. `.I4`). Messages with repeated blocks (e.g. NAV-SAT) use `getUbxMessageBlockField()` / `getUbxMessageBlockFieldCallback()`.
* NMEA: `getNmeaMessageField()` / `getNmeaMessageFieldCallback()` return a `std::string` on ESP-IDF (a `String` on Arduino). Use `.c_str()` to print them with `printf`, and `atoi()` / `atof()` to convert them to numbers. Messages with repeated blocks (GSV) use `getNmeaMessageBlockFieldCallback()`.

The user creates the bus (`i2c_new_master_bus()`, `spi_bus_initialize()` or `uart_driver_install()`) and passes it to `begin()`. Unless stated otherwise, the examples use I2C: I2C_NUM_0, SDA GPIO 21, SCL GPIO 22, with the ESP32's internal pull-ups disabled (u-blox modules have their own). The examples which enable a message (with `setCfgValset(UBLOX_CFG_MSGOUT_..., n)`) enable it in RAM and Battery-backed-RAM. It is possible to enable it in RAM only by changing the `setCfgValset` to `setCfgValset(UBLOX_CFG_MSGOUT_..., n, VAL_LAYER_RAM)`.

## Polling

| Example | Message(s) | Fields read |
| --- | --- | --- |
| PollingExample1_PositionVelocityTime | UBX NAV-PVT | Using the helper methods `getNAVPVT()`, `getLatitude()`, `getLongitude()`, `getAltitudeMSL()`. And using `getUBX("NAV", "PVT")` and `getUbxMessageField()`: `lat`, `lon` (as `.I4`), `hMSL` |
| PollingExample2_PositionVelocityTime_Serial | UBX NAV-PVT | As PollingExample1 (helper methods), but using UART1 at 38400 baud: TX GPIO 17, RX GPIO 16 |
| PollingExample3_PositionVelocityTime_SPI | UBX NAV-PVT | As PollingExample1 (helper methods), but using SPI2_HOST: SCK GPIO 18, POCI GPIO 19, PICO GPIO 23, CS GPIO 4 |
| PollingExample4_SECUNIQID | UBX SEC-UNIQID | `getSECUNIQID()`, then `getUniqueChipIdStr()`: the module's unique chip ID as a hex `std::string` |
| PollingExample5_GPZDA | NMEA ZDA | `getNMEA("ZDA")` polls the message (ZDA is not output by default): `time`, `day`, `month`, `year` |

## Periodic

| Example | Message(s) | Fields read |
| --- | --- | --- |
| PeriodicExample1_NAVHPPOSLLH | UBX NAV-HPPOSLLH (enabled with `setAutoUBX()`) | `getNAVHPPOSLLH()` returns true when fresh data arrives: `iTOW`, `lat`, `lon` (as `.I4`), `hAcc` |
| PeriodicExample2_GPGGA | NMEA GGA (output by default; `assumeAutoNMEA("GGA", true)`) | `getNMEA("GGA")` returns true when fresh data arrives: `time`, `lat`, `NS`, `lon`, `EW`, `alt` |

## Periodic with callback

| Example | Message(s) | Fields read |
| --- | --- | --- |
| CallbackExample1_NAVHPPOSLLH | UBX NAV-HPPOSLLH (every epoch) | `iTOW`, `lat`, `lon`, `hAcc` |
| CallbackExample2_GPRMC | NMEA RMC (output by default) | `time`, `date`, `lat`, `NS`, `lon`, `EW` |
| CallbackExample4_NAVSAT | UBX NAV-SAT (every 2 epochs) | `numSvs`. Per satellite block: `gnssId`, `svId`, `qualityInd`, `svUsed`, `cno` |
| CallbackExample7_NMEA_GSV | NMEA GSV (every 2 epochs) | `xxGSV` (talker ID), `signalId`. Per satellite block (up to 4 per message): `svid`, `elv`, `az`, `cno` |
| CallbackExample8_MONCOMMS | UBX MON-COMMS (every 2 epochs) | `nPorts`, `protId0`-`protId3`. Per port block: `portId`, `txBytes`, `rxBytes`, `msgs0`-`msgs3`, `skipped` |

## Data logging

These examples log the complete messages to microSD card, in UBX format, using the library's file buffer (`setFileBufferSize()`, `logUBX()`, `fileBufferAvailable()`, `extractFileBufferData()`). Callbacks are used only to count the messages. The SD card is mounted with the ESP-IDF FATFS VFS, and written with `fopen()` / `fwrite()` / `fclose()`. The SD card code is in a small C file in each example's `main` folder. Press any key in the monitor to start logging, and again to stop.

| Example | Message(s) | Details |
| --- | --- | --- |
| DataloggingExample1_RAWX_and_SFRBX | UBX RXM-RAWX and RXM-SFRBX (periodic with callback) | I2C. SD card over SPI (`esp_vfs_fat_sdspi_mount`, `main/sd_card.c`) on SPI2_HOST: SCK GPIO 18, POCI GPIO 19, PICO GPIO 23, CS GPIO 5. LED GPIO 13. Logs to `/sdcard/RXM_RAWX.ubx` |
| DataloggingExample2_DataLogger_IoT_SDIO | UBX RXM-RAWX and RXM-SFRBX at 20Hz (periodic with callback), plus NMEA GGA, GSA, GSV, GST and RMC at 1Hz (logged with `setNMEALoggingMask()`) | For the SparkFun DataLogger IoT. GNSS on SPI2_HOST: SCK GPIO 18, POCI GPIO 19, PICO GPIO 23, CS GPIO 33. SD card over 4-bit SDMMC (`esp_vfs_fat_sdmmc_mount`, `main/sdmmc_card.c`). Polls NAV-PVT (`getNAVPVT()`, `getFixType()`, `getYear()` etc.) to wait for a 3D fix and to name the log file from the date and time (long file names need `CONFIG_FATFS_LFN_HEAP`, set in `sdkconfig.defaults`) |

## Arduino examples not converted

The remaining Arduino examples (CallbackExample3, 5, 6, 9, 10-15) use the same library methods as the examples above, so they work on ESP-IDF too. To convert one, start from the ESP-IDF example that uses the same method (e.g. CallbackExample4_NAVSAT for another UBX message with repeated blocks), then copy the callback from the Arduino example, replacing `Serial.print` with `printf`.
