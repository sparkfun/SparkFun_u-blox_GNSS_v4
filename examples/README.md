# SparkFun u-blox GNSS v4: Arduino Examples

These examples show the three ways the library can read messages from a u-blox GNSS module:

* **Polling**: the code asks the module for a message, and waits for the reply. For example `getNAVPVT()`, `getUBX("NAV", "PVT")` or `getNMEA("ZDA")`.
* **Periodic**: the module sends the message automatically, every navigation epoch (or every _n_ epochs). The code checks whether a fresh message has arrived. For example `setAutoUBX("NAV", "HPPOSLLH")` then `getNAVHPPOSLLH()`.
* **Periodic with callback**: the module sends the message automatically. The library calls your callback function when it arrives. Register the callback with `setAutoCallbackPtr()` (UBX) or `setNmeaCallbackPtr()` (NMEA). Call `checkUblox()` and `checkCallbacks()` regularly in `loop()`.

Fields are read by name:

* UBX: `getUbxMessageField()` / `getUbxMessageFieldCallback()` return a `ubxAnyType`, which converts to `double` or can be read as its true type (e.g. `.I4`). Messages with repeated blocks (e.g. NAV-SAT) use `getUbxMessageBlockField()` / `getUbxMessageBlockFieldCallback()`.
* NMEA: `getNmeaMessageField()` / `getNmeaMessageFieldCallback()` return a `String`. Messages with repeated blocks (GSV) use `getNmeaMessageBlockFieldCallback()`.

Unless stated otherwise, the examples use I2C (Qwiic) and print to Serial at 115200 baud. The examples which enable a message (with `setCfgValset(UBLOX_CFG_MSGOUT_..., n)`) enable it in RAM and Battery-backed-RAM. It is possible to enable it in RAM only by changing the `setCfgValset` to `setCfgValset(UBLOX_CFG_MSGOUT_..., n, VAL_LAYER_RAM)`.

## Polling

| Example | Message(s) | Fields read |
| --- | --- | --- |
| PollingExample1_PositionVelocityTime | UBX NAV-PVT | Using the helper methods `getNAVPVT()`, `getLatitude()`, `getLongitude()`, `getAltitudeMSL()`. And using `getUBX("NAV", "PVT")` and `getUbxMessageField()`: `lat`, `lon` (as `.I4`), `hMSL` |
| PollingExample2_PositionVelocityTime_Serial | UBX NAV-PVT | As PollingExample1 (helper methods), but using a Serial (UART) port at 38400 baud |
| PollingExample3_PositionVelocityTime_SPI | UBX NAV-PVT | As PollingExample1 (helper methods), but using SPI. CS on pin 4 |
| PollingExample4_SECUNIQID | UBX SEC-UNIQID | `getSECUNIQID()`, then `getUniqueChipIdStr()`: the module's unique chip ID as a hex `String` |
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
| CallbackExample3_GPRMC_and_GPGLL | NMEA RMC and GLL (output by default), using the same callback for both | RMC: `time`, `date`, `lat`, `NS`, `lon`, `EW`. GLL: `time`, `lat`, `NS`, `lon`, `EW`, `status`, `posMode` |
| CallbackExample4_NAVSAT | UBX NAV-SAT (every 2 epochs) | `numSvs`. Per satellite block: `gnssId`, `svId`, `qualityInd`, `svUsed`, `cno` |
| CallbackExample5_NAVSIG | UBX NAV-SIG (every 2 epochs) | `numSigs`. Per signal block: `gnssId`, `svId`, `sigId`, `cno`, `qualityInd`, `health`, `prUsed`, `crUsed` |
| CallbackExample6_RAWX | UBX RXM-RAWX (every 2 epochs) | `rcvTow`, `week`, `numMeas`. Per measurement block: `gnssId`, `svId`, `sigId`, `cno`, `prMes`, `cpMes` |
| CallbackExample7_NMEA_GSV | NMEA GSV (every 2 epochs) | `xxGSV` (talker ID), `signalId`. Per satellite block (up to 4 per message): `svid`, `elv`, `az`, `cno` |
| CallbackExample8_MONCOMMS | UBX MON-COMMS (every 2 epochs) | `nPorts`, `protId0`-`protId3`. Per port block: `portId`, `txBytes`, `rxBytes`, `msgs0`-`msgs3`, `skipped` |
| CallbackExample9_SECSIG | UBX SEC-SIG (every 2 epochs) | `version`, `jamDetEnabled`, `jamState`, `spfDetEnabled`, `spfState`, `jamNumCentFreqs`. Per frequency block: `centFreq`, `jammed` |
| CallbackExample10_ESFMEAS | UBX ESF-MEAS (every epoch). **ZED-F9R** | `timeTag`, `numMeas` (checked against `getUbxMessageBlockCountCallback()`). Per measurement block: `dataType`, `dataField`. Footer: `calibTtag` |
| CallbackExample11_ESFRAW | UBX ESF-RAW (every epoch). **ZED-F9R** | Block count from `getUbxMessageBlockCountCallback()`. Per measurement block: `dataType`, `dataField`, `sTag` |
| CallbackExample12_ESFSTATUS | UBX ESF-STATUS (every epoch). **ZED-F9R** | `iTOW`, `fusionMode`, `numSens`. Per sensor block: `type`, `ready`, `calibStatus`, `freq` |
| CallbackExample13_NEO-D9S_RXMPMP | UBX RXM-PMP. **NEO-D9S** (I2C address 0x43). Also configures the L-band receiver and enables RXM-PMP on UART1 and UART2 | `version`, `numBytesUserData`, `serviceIdentifier`, `uniqueWordBitErrors`, `fecBits`, `ebno` |
| CallbackExample14_NEO-D9C_RXMQZSSL6 | UBX RXM-QZSSL6. **NEO-D9C** (I2C address 0x43). Also enables RXM-QZSSL6 on UART1 and UART2 | `version`, `svId`, `cno`, `bitErrCorr`, `chn`, `msgName`, `chName` |
| CallbackExample15_MONRF | UBX MON-RF (every 2 epochs). Also enables antenna short and open detection | `version`, `msgSource`. Per RF block: `blockId`, `antStatus`, `agcCnt`, `rfBlockGnssBand`. Also calls the helper method `getAntennaStatus()` every 5 seconds: the combined antenna status from all RF blocks |

## Data logging

These examples log the complete messages to microSD card, in UBX format, using the library's file buffer (`setFileBufferSize()`, `logUBX()`, `fileBufferAvailable()`, `extractFileBufferData()`). Callbacks are used only to count the messages. They need an ESP32 (or another processor with plenty of RAM).

| Example | Message(s) | Details |
| --- | --- | --- |
| DataloggingExample1_RAWX_and_SFRBX | UBX RXM-RAWX and RXM-SFRBX (periodic with callback) | I2C. SD card over SPI (CS pin 5), ESP32 Thing Plus C. Press a key to start logging, and again to stop |
| DataloggingExample2_DataLogger_IoT_SDIO | UBX RXM-RAWX and RXM-SFRBX at 20Hz (periodic with callback), plus NMEA GGA, GSA, GSV, GST and RMC at 1Hz (logged with `setNMEALoggingMask()`) | SPI (CS pin 33) at 20Hz. SD card over 4-bit SDIO (SD_MMC), DataLogger IoT. Polls NAV-PVT (`getNAVPVT()`, `getFixType()`, `getYear()` etc.) to wait for a 3D fix and to name the log file from the date and time. Press a key to start logging, and again to stop |
