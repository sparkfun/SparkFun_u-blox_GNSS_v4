# SparkFun u-blox GNSS Library - v4

<table class="table table-hover table-striped table-bordered">
  <tr align="center">
   <td><a href="https://www.sparkfun.com/sparkfun-allband-gnss-rtk-breakout-zed-x20p-qwiic.html"><img src="https://www.sparkfun.com/media/catalog/product/cache/f3020b7489dcfc4d1d147cf4dad07b7f/2/8/28871-zed-x20p-breakout-feature.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/sparkfun-gps-rtk2-board-zed-f9p-qwiic-gps-15136.html"><img src="https://www.sparkfun.com/media/catalog/product/cache/f3020b7489dcfc4d1d147cf4dad07b7f/1/5/15136-SparkFun_GPS-RTK2_Board_-_ZED-F9P__Qwiic_-03.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/sparkfun-gps-rtk-sma-breakout-zed-f9p-qwiic.html"><img src="https://www.sparkfun.com/media/catalog/product/cache/f3020b7489dcfc4d1d147cf4dad07b7f/1/6/16481-SparkFun_GPS-RTK-SMA_Breakout_-_ZED-F9P__Qwiic_-01a.jpg"></a></td>
   <td><a href="https://www.sparkfun.com/sparkfun-gnss-receiver-breakout-max-m10s-qwiic.html"><img src="https://www.sparkfun.com/media/catalog/product/cache/f3020b7489dcfc4d1d147cf4dad07b7f/1/8/18037-SparkFun_GNSS_Receiver_Breakout_-_MAX-M10S__Qwiic_-01_Default.jpg"></a></td>
    <td><a href="https://www.sparkfun.com/sparkfun-gps-rtk-dead-reckoning-breakout-zed-f9r-qwiic-gps-22693.html"><img src="https://www.sparkfun.com/media/catalog/product/cache/f3020b7489dcfc4d1d147cf4dad07b7f/2/2/22693-_GPS_SparkFun_RTK_Dead_Reckoning_Breakout_ZED-F9R-_01.jpg"></a></td>
  </tr>
  <tr align="center">
    <td><a href="https://www.sparkfun.com/sparkfun-allband-gnss-rtk-breakout-zed-x20p-qwiic.html">SparkFun Allband GNSS RTK Breakout - ZED-X20P (GPS-28871)</a></td>
    <td><a href="https://www.sparkfun.com/sparkfun-gps-rtk2-board-zed-f9p-qwiic-gps-15136.html">SparkFun GPS-RTK2 - ZED-F9P (GPS-15136)</a></td>
    <td><a href="https://www.sparkfun.com/sparkfun-gps-rtk-sma-breakout-zed-f9p-qwiic.html">SparkFun GPS-RTK-SMA - ZED-F9P (GPS-16481)</a></td>
    <td><a href="https://www.sparkfun.com/sparkfun-gnss-receiver-breakout-max-m10s-qwiic.html">SparkFun GNSS Receiver Breakout - MAX-M10S (GPS-18037)</a></td>
    <td><a href="https://www.sparkfun.com/sparkfun-gps-rtk-dead-reckoning-breakout-zed-f9r-qwiic-gps-22693.html">SparkFun GPS-RTK Dead Reckoning Breakout - ZED-F9R (GPS-22693)</a></td>
  </tr>
</table>

u-blox make some incredible GNSS receivers covering everything from low-cost, highly configurable modules such as the MAX-M10S all the way up to the surveyor grade ZED-X20P with precision of the diameter of a dime. This library supports configuration and control of u-blox devices over I<sup>2</sup>C (called DDC by u-blox), Serial and SPI. This version uses the u-blox Configuration Interface to: detect the module; configure message intervals; etc.. We wrote it for the most recent u-blox modules which no longer support messages like UBX-CFG-PRT or UBX-CFG-MSG.

![GitHub License](https://img.shields.io/github/license/sparkfun/SparkFun_u-blox_GNSS_v4)
![Release](https://img.shields.io/github/v/release/sparkfun/SparkFun_u-blox_GNSS_v4)
![Release Date](https://img.shields.io/github/release-date/sparkfun/SparkFun_u-blox_GNSS_v4)
![Documentation - build](https://img.shields.io/github/actions/workflow/status/sparkfun/SparkFun_u-blox_GNSS_v4/build-deploy-ghpages.yml?label=doc%20build)
[![Compile Test](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/actions/workflows/compile-sketch.yml/badge.svg)](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/actions/workflows/compile-sketch.yml)
[![IDF Compile Test](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/actions/workflows/compile-idf-example.yml/badge.svg)](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/actions/workflows/compile-idf-example.yml)
![GitHub issues](https://img.shields.io/github/issues/sparkfun/SparkFun_u-blox_GNSS_v4)

## Arduino and ESP-IDF

With release v4.1.0, this library is compatible with both the Arduino IDE and the Espressif ESP-IDF.

It includes examples for both platforms: [**examples**](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/examples) contains the Arduino examples; [**idf_examples**](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/idf_examples) contains the IDF examples.
All examples have been tested on ESP32 hardware.

Please see <a href="#arduino-library-manager">Arduino Library Manager</a> and <a href="#esp-idf-component"><b>ESP-IDF Component</b></a> below for more details.

## v4 vs. v3

This library is the new and improved version of the very popular SparkFun u-blox GNSS Arduino Library. v4 contains some big changes and improvements:

* Written by AI, directed by SparkFun
  * We used Claude to rewrite this library, giving it careful direction using v3 as the starting point
  * It was quite the journey, taking around eight working days from start to highly polished finish
  * If you want to see how we did it, the files are in the [AGENTS](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/AGENTS) folder
* v4 is a fresh start
  * It avoids the repetitive coding style of v3
  * Each UBX message type is supported by its own code Class
  * Each NMEA message type is also supported by its own code Class
  * **Unneeded [UBX](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/src/ubxMessageVector.h#L55-L56) and [NMEA](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/src/nmeaMessageVector.h#L39-L40)  message classes can be commented - to save both RAM and program memory**
  * Message fields (both UBX and NMEA) can be found and extracted **by name**
  * Both Polling and Periodic messages are supported - with Callbacks for Periodic messages
* v4 is **not** backward-compatible with v3
  * But we have included many helper methods to make migrating to v4 as easy as possible
  * Please see [PollingExample1_PositionVelocityTime](examples/PollingExample1_PositionVelocityTime/PollingExample1_PositionVelocityTime.ino) for details

## Compatibility

v4 of the library provides support for generation X20, F9 and M10 u-blox GNSS modules, which support the Configuration Interface

<a name="arduino-library-manager"></a>
## Arduino Library Manager

This library can be installed through the Arduino Library Manager.

* Add it by searching for `SparkFun u-blox GNSS v4` in the Library Manager
* Arduino examples are in [**examples**](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/examples)

<a name="esp-idf-component"></a>
## ESP-IDF Component

This library can also be used as a native component for the Espressif ESP-IDF - without the Arduino core.

* Add it to your project with: `idf.py add-dependency "sparkfun/sparkfun_u-blox_gnss_v4"`
* ESP-IDF v5.3 or later is required. The library uses the ESP-IDF `i2c_master`, `spi_master` and `uart` drivers
* The API is C++. Your `main` file needs to be `main.cpp`, with `extern "C" void app_main(void)`
* You create the bus, then pass it to `begin()`:
  * I2C: `i2c_new_master_bus()` then `myGNSS.begin(i2cBus)`
  * SPI: `spi_bus_initialize()` then `myGNSS.begin(SPI2_HOST, csGpio)`
  * UART: `uart_driver_install()`, `uart_param_config()` and `uart_set_pin()` then `myGNSS.begin(UART_NUM_1)`
* `enableDebugging()` prints to the console (stdout) by default
* NMEA field getters return `std::string` (`sfe_string_t`) instead of the Arduino `String`
* A 1000Hz FreeRTOS tick (`CONFIG_FREERTOS_HZ=1000`) is recommended
* ESP-IDF examples are in [**idf_examples**](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/idf_examples). Build, flash and monitor with `idf.py -p PORT flash monitor`

## Dockerfiles

We have included two Dockerfiles (`Arduino.Dockerfile` and `IDF.Dockerfile`) which you may find useful. We wrote them to allow us to test the Arduino and IDF examples quickly, without needing to open the Arduino IDE or the ESP IDF. The Dockerfiles use command line tools to compile the selected example in an Ubuntu container.

You don't _need_ to use the batch files or Dockerfiles. We just included them in case you find them useful.

The `.bat` batch files (`Arduino_compile_example.bat` and `IDF_compile_example.bat`) were written for Windows. Sorry about that. Hopefully you can convert them into (e.g.) `bash` scripts as needed.

The `Flasher.bat` batch file will upload the selected example binary onto an ESP32. It searches for a CH340 COM port - as used on the [SparkFun Thing Plus - ESP32 WROOM (USB-C)](https://www.sparkfun.com/sparkfun-thing-plus-esp32-wroom-usb-c.html) - and uses that for the upload. Or you can add the COM port as an `arg`.

`Flasher.bat` assumes you have the `python` version of `esptool` installed and available. If you want to use `esptool.exe`: replace `python -m esptool` with `esptool.exe`.

The Dockerfiles of course need Docker installed and running. Please ensure you have the Docker Desktop running when you use the batch files and Dockerfiles.

To compile, flash and test an Arduino example, `cd` into the `SparkFun_u-blox_GNSS_v4` folder and run:

```
Arduino_compile_example.bat PollingExample1_PositionVelocityTime
Flasher.bat Arduino PollingExample1_PositionVelocityTime
```

To compile, flash and test an IDF example, `cd` into the `SparkFun_u-blox_GNSS_v4` folder and run:

```
IDF_compile_example.bat sparkfun_u-blox_gnss_v4 PollingExample1_PositionVelocityTime
Flasher.bat IDF PollingExample1_PositionVelocityTime
```

To upload using your own COM port:

```
Flasher.bat Arduino PollingExample1_PositionVelocityTime COM1
```

or

```
Flasher.bat IDF PollingExample1_PositionVelocityTime COM1
```

The first time you run each Dockerfile, it will take a long time to create the Ubuntu container and install the relevant command line tools. Subsequent runs will be much quicker.

## Repository Contents

* [**examples**](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/examples) - Example sketches for the library (.ino). Run these from the Arduino IDE.
* [**idf_examples**](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/idf_examples) - Example projects for the ESP-IDF (main.cpp, CMakeLists.txt, idf_component.yml).
* [**src**](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/src) - Source files for the library (.cpp, .h).
* [**keywords.txt**](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/keywords.txt) - Keywords from this library that will be highlighted in the Arduino IDE.
* [**library.properties**](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/library.properties) - General library properties for the Arduino package manager.
* [**keys**](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/keys) - The u-blox Configuration Interface Key IDs extracted from multiple Interface Descriptions
* [**Utils**](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/Utils) - Python utilities we wrote to help analyze UBX/NMEA/RTCM data and UBX format log files

## Documentation

API documentation is generated with Doxygen and published to GitHub Pages from the `main` branch.

## License Information

This library is _**open source**_!

Please see [LICENSE.md](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/LICENSE.md) for full details.

- Your friends at SparkFun.
