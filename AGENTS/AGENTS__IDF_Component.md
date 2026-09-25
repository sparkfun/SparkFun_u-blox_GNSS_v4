# SparkFun u-blox GNSS ESP IDF Component

## Background

The [SparkFun u-blox GNSS v4 Arduino Library](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4) is an Arduino Integrated Development Environment (IDE) code Library written to communicate with u-blox GNSS modules over I2C, UART or SPI.
It works well. It runs successfully on Espressif ESP32 boards.

We want to rework this library so it can become a Component for the Espressif (ESP) IoT Development Framework (IDF) development environment.

The ESP IDF is a classic C development suite supporting Espressif microcontrollers. It avoids the `setup()` and `main()` coding style of the Arduino environment.

As we understand it, there are two main ways to achieve this:
1) Use the "Arduino as an ESP-IDF component"
    "You can use the Arduino framework as an ESP-IDF component. This allows you to use the Arduino framework in your ESP-IDF projects with the full flexibility of the ESP-IDF. This method is recommended for advanced users. To use this method, you will need to have the ESP-IDF toolchain installed."
2) Write a thin interface layer for the Arduino Library to convert the Arduino-specific functions and methods into ESP IDF functions and methods

One example of (2) is the "Soldered u-blox GPS GNSS Component". Please see the follow the link in the [Important References](#important-references) section below for more details. Soldered Electronics have: taken a copy of the older ["v2" SparkFun u-blox GNSS Arduino Library](https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library); written the thin layer to convert the Arduino `TwoWire` (I2C), `SPIClass` (SPI) and `Stream` (UART Serial) methods to ESP IDF device handle method calls.

Our preference is to follow (2). Rewriting the SparkFun GNSS v4 `sfe_bus` `class` and methods is a more elegant solution than using "Arduino as an ESP-IDF component"

## The Objective

Adapt the SparkFun u-blox GNSS v4 Arduino Library so it becomes a Component for the Espressif (ESP) IoT Development Framework (IDF) development environment.

The intent is to submit the finished "SparkFun u-blox GNSS Component" for inclusion in The ESP Component Registry.

The existing Library examples (in the Library `examples` folder) should all be retained and adapted to become IDF examples. Each example should be rewritten in the `main.c` style; with `idf_component.yml`, and `CMakeLists.txt` files.

## What we want Claude to do

At the time of writing, we want Calude to do the following:

* A light-touch investigation of the Espressif ESP IDF and its coding style
* A light-touch investigation of the Arduino IDE and its coding style - including the `Stream` (`Serial` UART), `TwoWire` (I2C) and `SPI` interfaces
* A modest investigation of how it would be possible to convert the SparkFun u-blox GNSS v4 Arduino Library into a ESP IDF Component
* A recommendation of which approach to follow:
    * Is the "thin interface layer" the best and correct approach?
    * Or should the "Arduino as an ESP-IDF component" approach be used?
    * Is there a better, alternative approach which we have missed?
* A proposal outlining how the code changes would be carried out. A step by step guide would be very welcome.

Write this up as a investigation report and proposal. Save it to the project folder on this device in .md and .pdf format.

The investigation should use no more than 75% of the Pro Plan usage limit for the current session. Report writing can use the remainder.

## Initial Success Indicator

Investigations concluded. Report written and saved to this device.

## Success Indicator - after the development work is complete

Successful compilation of one or more modified SparkFun u-blox GNSS examples; and successful execution on a ESP32 board connected to a u-blox GNSS.

## Important References

[ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/#esp-idf-programming-guide)

[Arduino as an ESP-IDF component](https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html)

[The ESP Component Registry](https://components.espressif.com/)

[Soldered u-blox GPS GNSS Component](https://components.espressif.com/components/solderedelectronics/soldered-u-blox-gps-gnss-esp-idf-component/versions/0.0.3/readme)

[Soldered-u-blox-GPS-GNSS-ESP-IDF-Component GitHub Repository](https://github.com/SolderedElectronics/Soldered-u-blox-GPS-GNSS-ESP-IDF-Component)

