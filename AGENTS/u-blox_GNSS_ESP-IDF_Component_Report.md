# SparkFun u-blox GNSS v4 as an ESP-IDF Component

**Investigation report and proposal**

Prepared for: Paul Clark, SparkFun Electronics\
Prepared by: Claude (Cowork), 23 September 2026\
Library version examined: SparkFun_u-blox_GNSS_v4 `main` @ `08943c2` (23 Sep 2026, `library.properties` 4.0.0)

---

## 1. Summary

**Recommendation:** use the thin-interface-layer approach (option 2), but build it **into the v4 library itself** rather than into a separate fork. Do not emulate the Arduino classes, as the Soldered component does. Instead, give `sfe_bus` a native ESP-IDF back end that sits alongside the existing Arduino back end. A small `sfe_platform.h` header covers everything else: time, delay, GPIO, strings and print output. The same source tree then builds as an Arduino library and as an ESP-IDF component, and CMake and the preprocessor choose the right back end.

Why this approach:

- **v4 is already well structured for it.** All I2C, SPI and UART traffic goes through the abstract `GNSSDeviceBus` interface, which has only 8 methods. The UBX/NMEA protocol code (about 19,000 lines) never calls `Wire`, `SPI` or `Stream` directly. The remaining Arduino-specific code is small and easy to list (section 4).
- **One source of truth.** v4 is still changing quickly (see `AGENTS/v4-migration-status.md`, Phases 12–33). A fork would soon fall behind. The Soldered component shows this happening: it is frozen at the older v2 library.
- **No Arduino runtime.** Users of the component get plain ESP-IDF: the `i2c_master`, `spi_master` and `uart` drivers, FreeRTOS, `esp_err_t` and `ESP_LOG`. They do not need the Arduino core, its 1000 Hz tick requirement or its link to particular ESP-IDF versions.
- **Arduino users are unaffected.** Their API and examples stay exactly as they are.

**"Arduino as an ESP-IDF component" (option 1) is not recommended as the product.** The existing library already works that way today with no changes. However, it ties users to the ESP-IDF version that arduino-esp32 supports (arduino-esp32 3.3.12 targets ESP-IDF v5.5, while ESP-IDF stable is now v6.x). It also forces `CONFIG_FREERTOS_HZ=1000` and pulls the whole Arduino core into every project. A registry component built on it would add little value.

**Alternatives I considered** (section 5): Soldered-style Arduino-class emulation, a C-language facade, and adopting the SparkFun Toolkit bus abstraction. A C facade is a worthwhile *later* addition. The other two are weaker than the recommendation.

**One point in the brief needs changing: examples cannot be pure `main.c`.** The v4 API is C++: classes, overloads, `std::vector`, `ubxAnyType` and callbacks that take C++ types. The ESP-IDF examples should therefore be `main.cpp` with `extern "C" void app_main(void)`. This is standard and fully supported in ESP-IDF, and apart from that the examples keep the `main.c` layout. If a C API is wanted, a thin `extern "C"` facade can be added as Phase 7 (section 7).

---

## 2. ESP-IDF and its coding style (light-touch review)

| Topic | ESP-IDF practice |
|---|---|
| Entry point | `void app_main(void)`, which runs in a FreeRTOS task. There is no `setup()`/`loop()`: the application writes its own `while (1) { … vTaskDelay(…); }` or creates extra tasks. |
| Build system | CMake. The project has a top-level `CMakeLists.txt` (`include($ENV{IDF_PATH}/tools/cmake/project.cmake)`), and each component calls `idf_component_register(SRCS … INCLUDE_DIRS … REQUIRES …)`. |
| Dependencies | IDF Component Manager. Each component has an `idf_component.yml` manifest (version, description, url, repository, license, targets, dependencies including `idf: ">=x.y"`, `files` include/exclude, `examples`). Components are published to the ESP Component Registry. |
| Configuration | Kconfig/`menuconfig`. A component can ship a `Kconfig` file, and its options appear as `CONFIG_…` macros. Examples usually ship `sdkconfig.defaults` and `main/Kconfig.projbuild` (for pin numbers). |
| Peripheral drivers | Handle-based. For example, `i2c_new_master_bus()` → `i2c_master_bus_handle_t`, then `i2c_master_bus_add_device()` → `i2c_master_dev_handle_t`. SPI: `spi_bus_initialize()` + `spi_bus_add_device()`. UART: `uart_driver_install()` + `uart_param_config()` + `uart_set_pin()`. Functions return `esp_err_t`. |
| Driver versions | The legacy I2C driver (`driver/i2c.h`) is **End-of-Life in v6.0 and will be removed in v7.0**. New code must use `driver/i2c_master.h`. In v6.0 the legacy `driver` component also stopped re-exporting `esp_driver_i2c`, `esp_driver_spi`, `esp_driver_uart` and so on, so a component must name them in `REQUIRES`. |
| Time/delay | `esp_timer_get_time()` (µs, 64-bit), `vTaskDelay(pdMS_TO_TICKS(ms))`, `esp_rom_delay_us()` for busy-waits. The default tick is **100 Hz** (`CONFIG_FREERTOS_HZ`), which matters for porting (section 6.4). |
| Logging | `ESP_LOGE/W/I/D/V(TAG, fmt, …)` with per-tag runtime levels, and `printf` to the console UART/USB-JTAG. |
| Language | C by default. C++ is fully supported (`.cpp` sources, STL). **C++ exceptions are disabled by default** (`CONFIG_COMPILER_CXX_EXCEPTIONS=n`), so a failed `new` aborts rather than throwing. |
| Style | Espressif components use `snake_case` C APIs with a component prefix and `esp_err_t` returns. C++ components (for example `esp-modbus`, many registry drivers) are common and accepted. |

---

## 3. Arduino and its coding style (light-touch review)

- **Entry point:** the core calls `setup()` once, then `loop()` forever. On ESP32, arduino-esp32 runs these inside a FreeRTOS "loopTask" on top of ESP-IDF.
- **Global peripheral objects:** `Wire` (`TwoWire`), `SPI` (`SPIClass`) and `Serial`/`Serial1` (`HardwareSerial`, which derives from `Stream`, which derives from `Print`). Libraries take references to these, for example `begin(TwoWire &wirePort = Wire)`.
- **`TwoWire`:** a buffered transaction model. `beginTransmission(addr)` → `write()` … → `endTransmission(stop)` sends the data. `requestFrom(addr, n)` reads *n* bytes into an internal buffer, and `available()`/`read()` then take them out of that buffer. The buffer size depends on the platform (32 bytes on AVR, 128 on ESP32).
- **`SPIClass`:** `beginTransaction(SPISettings(clock, order, mode))` → the library toggles CS itself with `digitalWrite` → `transfer(byte)` → `endTransaction()`.
- **`Stream`/`Print`:** a byte-stream interface (`available()`, `read()`, `readBytes()`, `write()`) with formatting helpers (`print(x, HEX)`, `println()`). `Print` is the usual "output port" type for debug output and data relays.
- **Helpers:** `millis()`, `micros()`, `delay()`, `pinMode()`, `digitalWrite()`, the `String` class, the `F()`/`PROGMEM` flash-string macros, and `HEX`/`DEC`.

---

## 4. Where v4 depends on Arduino

I grepped `src/` at commit `08943c2`. The hardware dependency is already isolated, and what is left is limited and mechanical to port.

| Dependency | Where | Porting action |
|---|---|---|
| `#include <Arduino.h>` (+ `Wire.h`, `SPI.h`) | `SparkFun_u-blox_GNSS_v4.h`, `sfe_bus.h/.cpp`, `sfe_debug.h`, `u-blox_GNSS.h/.cpp`, `ubxMessage.h`, `nmeaMessage.h` | Replace with a single `#include "sfe_platform.h"`. |
| `TwoWire`, `SPIClass`, `SPISettings`, `Stream` | `sfe_bus.h/.cpp` (`SfeI2C`, `SfeSPI`, `SfeSerial`), plus the `begin(...)` overloads in `SparkFun_u-blox_GNSS_v4.h` | Add ESP-IDF implementations of the three bus classes and ESP-IDF `begin(...)` overloads. Guard the Arduino ones. |
| `Print &` in the public API | `enableDebugging()`, `setNMEAOutputPort()`, `setRTCMOutputPort()`, `setUBXOutputPort()`, `setOutputPort()`; held in `SfePrint` | Make `SfePrint` wrap a small abstract output sink. On ESP-IDF, provide stdout/UART/callback sinks. |
| `millis()` (72 uses), `micros()` (16), `delay()` (14) | Mostly `u-blox_GNSS.cpp` wait/poll loops | `sfe_millis()`, `sfe_micros()`, `sfe_delay()` in `sfe_platform.h` (or ESP-IDF definitions of the same names). **Watch the tick rate** (section 6.4). |
| `pinMode()`/`digitalWrite()` | SPI chip-select in `sfe_bus.cpp`; the optional `debugPin` in `u-blox_GNSS.cpp` | `gpio_config()` / `gpio_set_level()`. |
| `String` (≈50 non-comment uses) | `nmeaMessage.h::extractFieldFrom()`, `nmeaMessageVector.h::extractValue()`, the four `getNmeaMessage…Field…()` getters, `pushAssistNowData(const String&…)`. Uses include `String(float, numDPs)` and `+=` | Add a platform type alias: `String` on Arduino, `std::string` on ESP-IDF, plus 2–3 helpers for float-with-decimals and char append. Seven examples use `String`. |
| `HEX`, `DEC`, `F()`, `__FlashStringHelper`, `PROGMEM` | `sfe_bus.h` (`SfePrint`), debug prints | Define as no-ops/constants on ESP-IDF (ESP32 flash is memory-mapped). |
| Platform `#if`s (`ARDUINO_ARCH_SAMD` → `SerialUSB`, `PARTICLE`) | `u-blox_GNSS.h`, `sfe_bus.cpp` | Add an `ESP_PLATFORM && !ARDUINO` branch. |
| `new`/`delete` without `std::nothrow` | `u-blox_GNSS.cpp` (payload, SPI, file and storage buffers) | Works, but on ESP-IDF a failed allocation aborts instead of returning `nullptr`. Switch to `new (std::nothrow)`, which also benefits Arduino. |
| `std::vector` | message registries/vectors | No change; available in ESP-IDF. |
| RTOS lock hooks (`createLock()/lock()/unlock()/deleteLock()`) | `u-blox_GNSS.h` | Already virtual no-ops. Optionally supply a FreeRTOS-mutex version on ESP-IDF (Kconfig option). |

No protocol-level code (`processUBX`, the registries, the message classes) needs logic changes.

---

## 5. Options analysis

### Option 1: "Arduino as an ESP-IDF component"

Add `espressif/arduino-esp32` as a dependency, then call `initArduino()` from `app_main()`, or enable autostart and keep `setup()`/`loop()`.

- ✅ No library changes. Examples need only `main.cpp` and `initArduino()`.
- ❌ Version lock. Arduino core 3.3.12 is documented as compatible with **ESP-IDF v5.5**, but ESP-IDF stable is v6.x. Every IDF release has to wait for arduino-esp32 to catch up.
- ❌ `CONFIG_FREERTOS_HZ=1000` is mandatory. The Arduino core, its libraries and its build time are added to every user project, and there is a risk of Arduino drivers and user ESP-IDF drivers both claiming the same peripheral.
- ❌ Espressif's own documentation labels it "for advanced users". A registry component that just re-exports the Arduino library adds almost nothing, because users can already do this themselves.

### Option 2a: thin shim that emulates Arduino classes (the Soldered approach)

Soldered took the v2 library and wrote `arduino_compat.h`. It is a fake `TwoWire`/`SPIClass`/`Stream`/`String`/`millis`/`delay` backed by ESP-IDF handles, plus a C wrapper API (`ublox_gnss_*`) for `main.c` users.

- ✅ Very few edits to the vendored library. C users get a simple API.
- ❌ It emulates Arduino semantics instead of using ESP-IDF's. For example, it defers the I2C write so that `endTransmission(false)` + `requestFrom()` becomes one `i2c_master_transmit_receive()`, and it sends SPI one byte per transaction.
- ❌ The shim only covers the parts of the Arduino API that v2 happened to use. Its `String` stub is a pointer wrapper with only `c_str()`, which would **not** support v4's NMEA field getters (`String(float, n)`, `+=`).
- ❌ Its C wrapper exposes only a curated subset (PVT, HPPOSLLH, RELPOSNED …). Most of the library, and v4's generic `getUBX("NAV","PVT")` / field-name API, would be unreachable.
- ❌ It is a vendored fork, so updates are manual. Its `delay(ms)` maps to `vTaskDelay(pdMS_TO_TICKS(ms))`, so the library's `delay(1)` calls become **0 ticks at the default 100 Hz tick** (a yield, not a delay).
- ⚠️ **Licence:** the Soldered component is **GPL-3.0**, while SparkFun's library is **MIT**. Copying Soldered code would put GPL obligations on the MIT library. Use it only as a reference, and write SparkFun's implementation independently.

### Option 2b: native ESP-IDF bus back end built into v4 (**recommended**)

Implement `SfeI2C`/`SfeSPI`/`SfeSerial` directly on ESP-IDF handles, selected at compile time, together with a small `sfe_platform.h`. Add ESP-IDF `begin()` overloads. Ship `CMakeLists.txt` + `idf_component.yml` in the same repository as `library.properties`.

- ✅ Uses ESP-IDF correctly: one combined I2C write-read, multi-byte SPI transactions, the UART driver's ring buffer.
- ✅ The full v4 API is available on ESP-IDF on day one, including callbacks, the generic field API and NMEA.
- ✅ One code base, Arduino unaffected, and it keeps pace with v4 automatically.
- ✅ Works with the Arduino-as-component setup as well: when both `ESP_PLATFORM` and `ARDUINO` are defined, the Arduino back end is chosen.
- ❌ Needs roughly 15–20 careful edits to shared headers and a new `.cpp`, and regression testing on Arduino. The work is modest.

### Option 3: C facade (complement, not alternative)

An `extern "C"` API (`sfe_gnss_handle_t`, `sfe_gnss_begin_i2c()`, `sfe_gnss_get_ubx(h, "NAV", "PVT")`, `sfe_gnss_get_field_double(h, msg, "lat")`, …). v4's name-based field API makes a **generic** C facade practical: roughly 30 functions could cover most use cases without a wrapper per message. This is worth doing after 2b if pure-C users matter.

### Option 4: adopt the SparkFun Toolkit bus abstraction

`sparkfun/SparkFun_Toolkit` defines `sfTkIBus`/`sfTkII2C`/`sfTkISPI`/`sfTkISerial` interfaces. It currently ships **Arduino implementations only** (`sfTkArdI2C`, `sfTkArdSPI`). Moving v4 onto it would mean replacing `GNSSDeviceBus` and writing ESP-IDF Toolkit back ends as well. This is a sound long-term direction across SparkFun's libraries, but it is a bigger change than this project needs. v4's `AGENTS.md` also says `sfe_bus` "is to be retained". Recommendation: keep 2b's ESP-IDF classes close to Toolkit semantics so they could later move into the Toolkit.

### Comparison

| | Opt 1 Arduino-as-component | Opt 2a Arduino-class shim | **Opt 2b native back end** | Opt 4 Toolkit |
|---|---|---|---|---|
| Library edits | none | minimal (fork) | moderate (in place) | large |
| Arduino runtime needed | yes | no | **no** | no |
| Tracks v4 updates | yes | manual | **automatic** | automatic |
| ESP-IDF version freedom | tied to arduino-esp32 | good | **good** | good |
| Uses ESP-IDF drivers idiomatically | n/a | partly | **yes** | yes |
| Full v4 API on ESP-IDF | yes | only if shim is complete | **yes** | yes |
| Registry value | low | medium | **high** | high |

---

## 6. Proposed design (option 2b)

### 6.1 Repository layout (single source, dual-published)

```
SparkFun_u-blox_GNSS_v4/
├── library.properties          # Arduino (unchanged)
├── CMakeLists.txt              # NEW – ESP-IDF component
├── idf_component.yml           # NEW – registry manifest
├── Kconfig                     # NEW – optional menuconfig options
├── src/
│   ├── sfe_platform.h          # NEW – time/delay/gpio/string/print portability
│   ├── sfe_bus.h               # Arduino + ESP-IDF class declarations, #if-selected
│   ├── sfe_bus.cpp             # Arduino implementation  (wrapped in #if SFE_ARDUINO)
│   ├── sfe_bus_esp_idf.cpp     # NEW – ESP-IDF implementation (wrapped in #if SFE_ESP_IDF)
│   └── … (unchanged protocol code)
├── examples/                   # Arduino sketches (unchanged)
└── idf_examples/               # NEW – ESP-IDF example projects
    └── PollingExample1_PositionVelocityTime/
        ├── CMakeLists.txt
        ├── sdkconfig.defaults
        └── main/{CMakeLists.txt, idf_component.yml, Kconfig.projbuild, main.cpp}
```

The ESP-IDF examples go in `idf_examples/`, not `examples/`. The Arduino IDE and `arduino-lint` expect `examples/*/*.ino`, and the registry auto-discovers `examples/`. In the manifest, point the registry at `idf_examples/` with the `examples:` field, and drop the Arduino sketches from the uploaded archive with `files: exclude`. Guard every `.cpp` with `#if` so that the Arduino IDE compiles `sfe_bus_esp_idf.cpp` to nothing, and ESP-IDF compiles `sfe_bus.cpp` to nothing.

*If SparkFun prefers a separate component repository* (this project folder): keep the v4 `src/` as a git submodule or a scripted sync, and put only the build files, examples and CI in the component repo. The platform edits still belong upstream in v4, so the code is never forked.

### 6.2 Platform selection — `sfe_platform.h`

```cpp
#pragma once
#if defined(ARDUINO)
  #define SFE_ARDUINO 1
  #include <Arduino.h>
  #include <Wire.h>
  #include <SPI.h>
  typedef String sfe_string_t;
#elif defined(ESP_PLATFORM)
  #define SFE_ESP_IDF 1
  #include <stdint.h>
  #include <string>
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #include "esp_timer.h"
  #include "driver/gpio.h"
  #ifndef HEX
    #define HEX 16
    #define DEC 10
  #endif
  #define F(x) (x)
  typedef std::string sfe_string_t;
  static inline unsigned long millis() { return (unsigned long)(esp_timer_get_time() / 1000ULL); }
  static inline unsigned long micros() { return (unsigned long)esp_timer_get_time(); }
  void delay(unsigned long ms);   // see 6.4
#else
  #error "Unsupported platform"
#endif
```

Keeping the Arduino names `millis`, `micros` and `delay` on ESP-IDF, as inline functions scoped to this library, avoids touching about 100 call sites. The alternative is a `sfe_` prefix with a one-off search and replace. Either works; the prefix is cleaner if the library is ever used alongside Arduino-as-component code. **I recommend the `sfe_` prefix** (for example `sfe_millis()`), mapped to the native functions on each platform.

### 6.3 ESP-IDF bus classes (`sfe_bus_esp_idf.cpp`)

**I2C: `SfeI2C`**

- `init(i2c_master_bus_handle_t bus, uint8_t address, uint32_t speedHz = 400000)` calls `i2c_master_bus_add_device()` and stores both handles. A second overload, `init(i2c_master_dev_handle_t dev, …)`, accepts a device the user has already created.
- `ping()` → `i2c_master_probe(bus, address, timeout)`.
- `available()` → `i2c_master_transmit_receive(dev, {0xFD}, 1, buf, 2, timeout)`. This is a single transaction with a repeated start, which is exactly what the u-blox DDC protocol requires.
- `readBytes()` → `i2c_master_receive(dev, data, len, timeout)`. This is a "current address" read from 0xFF.
- `writeBytes()` → `i2c_master_transmit()`.
- `setI2CTransactionSize()`: the default is 32 (an AVR limit). ESP-IDF has no Wire buffer limit, so the ESP-IDF `begin()` can default to a larger value, for example 128–255, which cuts bus overhead.
- Use a timeout (for example 100 ms) rather than `-1`, so that a disconnected module cannot hang the task.

**SPI: `SfeSPI`**

- `init(spi_host_device_t host, gpio_num_t cs, uint32_t clockHz = 4000000)` calls `spi_bus_add_device()` with `mode = 0` and `spics_io_num = -1`. CS is driven manually with `gpio_set_level()`, matching the Arduino semantics that the byte-at-a-time `startWriteReadByte()` … `endWriteReadByte()` sequence relies on. An overload accepts an existing `spi_device_handle_t` plus CS pin.
- `writeBytes()`/`writeReadBytes()`/`readBytes()`: send **one** `spi_transaction_t` for the whole buffer with `spi_device_polling_transmit()`, rather than one per byte as the Arduino version does. Allocate transfer buffers with DMA-capable memory, or use `SPI_TRANS_USE_TXDATA` for short transfers.
- `startWriteReadByte()` → `spi_device_acquire_bus()` + CS low. `writeReadByte()` → an 8-bit polling transaction. `endWriteReadByte()` → CS high + `spi_device_release_bus()`.
- Note that the Arduino `init()` rejects `cs == 0`. On ESP-IDF, use `GPIO_NUM_NC` as the sentinel instead.

**UART: `SfeSerial`**

- `init(uart_port_t port)` assumes the user has already called `uart_driver_install()`, `uart_param_config()` and `uart_set_pin()`, which is the normal ESP-IDF split. The RX buffer should be at least 1–2 kB for high-rate RAWX. `init` then calls `uart_flush_input()`, the equivalent of the Arduino "discard stale RX".
- `available()` → `uart_get_buffered_data_len()`. `readBytes()` → `uart_read_bytes(port, data, len, 0)`. `writeBytes()` → `uart_write_bytes()`.

**Output sinks: `SfePrint`**

Replace the `Print *` in `SfePrint` with a small interface:

```cpp
class SfeOutput { public: virtual size_t write(const uint8_t *buf, size_t len) = 0; virtual ~SfeOutput() {} };
```

On Arduino, `SfeArduinoPrintOutput` wraps a `Print &`, so the existing API (`enableDebugging(Serial)`) is unchanged. On ESP-IDF, provide:

- `SfeStdoutOutput` (the default for `enableDebugging()`)
- `SfeUartOutput(uart_port_t)` (for relaying NMEA/RTCM to another UART)
- `SfeCallbackOutput(void (*fn)(const uint8_t*, size_t, void*), void*)` (for sockets, files or queues)

`SfePrint::print(uint32_t, HEX)` formatting moves into `SfePrint` itself using `snprintf`, so every back end only has to implement `write()`.

**Public `begin()` overloads (ESP-IDF only)**

```cpp
bool SFE_UBLOX_GNSS::begin(i2c_master_bus_handle_t bus, uint8_t address = 0x42,
                           uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false);
bool SFE_UBLOX_GNSS_SPI::begin(spi_host_device_t host, gpio_num_t cs, uint32_t spiSpeed = 4000000,
                               uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false);
bool SFE_UBLOX_GNSS_SERIAL::begin(uart_port_t port,
                                  uint16_t maxWait = kUBLOXGNSSDefaultMaxWait, bool assumeSuccess = false);
```

The Arduino overloads, whose defaults refer to the `Wire`, `SPI` and `Serial` globals, are wrapped in `#if SFE_ARDUINO`.

### 6.4 Timing, delay and the FreeRTOS tick

The library polls in loops that call `delay(1)` ("allow an RTOS to get an elbow in"), `delay(spiPollingWait)` and `delay(10)`. On ESP-IDF's default 100 Hz tick, `pdMS_TO_TICKS(1)` is **0**, so `vTaskDelay(0)` only yields to tasks of equal priority. It does not let the IDLE task run, and a long `maxWait` can then trip the task watchdog. Rounding up to one tick makes every `delay(1)` take 10 ms, which slows polling.

Recommended `delay()` for ESP-IDF:

```cpp
void delay(unsigned long ms) {
  TickType_t t = pdMS_TO_TICKS(ms);
  if (t == 0) { if (ms) esp_rom_delay_us(ms * 1000); taskYIELD(); }   // sub-tick: short busy-wait
  else vTaskDelay(t);
}
```

In addition, ship `CONFIG_FREERTOS_HZ=1000` in each example's `sdkconfig.defaults` and state the recommendation in the README. At 1000 Hz, `delay(1)` is a true 1 ms sleep. The library must still work, only less efficiently, at 100 Hz.

### 6.5 Strings

- Change the NMEA field getters to return `sfe_string_t`: `String` on Arduino (no change for users) and `std::string` on ESP-IDF.
- Inside `nmeaMessage.h`, replace `String(field, numDPs)` with a helper `sfe_string_from_double(double v, int dps)` (`snprintf("%.*f")`), and replace `value += String((char)c)` with `sfe_string_append(value, c)`. There are two or three helpers in total, with separate implementations for each platform.
- Keep `pushAssistNowData(const String&…)` Arduino-only. The `const uint8_t *` overload already exists.

### 6.6 Build files

**`CMakeLists.txt`** (component root):

```cmake
idf_component_register(
    SRCS "src/u-blox_GNSS.cpp" "src/sfe_debug.cpp" "src/sfe_bus.cpp" "src/sfe_bus_esp_idf.cpp"
    INCLUDE_DIRS "src"
    REQUIRES esp_driver_i2c esp_driver_spi esp_driver_uart esp_driver_gpio esp_timer freertos)
```

**`idf_component.yml`** (component root):

```yaml
version: "4.0.0"
description: "SparkFun u-blox GNSS v4 – I2C, SPI and UART driver for u-blox GNSS modules (UBX, NMEA, RTCM)"
url: "https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4"
repository: "https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4.git"
issues: "https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/issues"
license: "MIT"
maintainers: ["SparkFun Electronics <techsupport@sparkfun.com>"]
tags: [gnss, gps, u-blox, ubx, nmea, rtcm, rtk, sparkfun]
dependencies:
  idf: ">=5.3"          # new i2c_master driver + split esp_driver_* components; confirm in CI
files:
  exclude: ["examples/**", "docs/**", "Utils/**", "keys/**", "AGENTS/**", "Dockerfile", "*.bat"]
examples:
  - path: idf_examples/PollingExample1_PositionVelocityTime
  # … one entry per example
```

**Optional `Kconfig`:** `SFE_UBLOX_GNSS_USE_FREERTOS_LOCK` (FreeRTOS mutex in `lock()`/`unlock()`), a default I2C transaction size, and compile-time buffer counts such as `UBX_ESF_MEAS_CALLBACK_BUFFERS`.

### 6.7 Example template (PollingExample1 → `main.cpp`)

```cpp
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "SparkFun_u-blox_GNSS_v4.h"

static SFE_UBLOX_GNSS myGNSS;   // I2C. See PollingExample2/3 for UART/SPI

extern "C" void app_main(void)
{
    printf("SparkFun u-blox Example\n");

    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = I2C_NUM_0;
    bus_cfg.sda_io_num = (gpio_num_t)CONFIG_EXAMPLE_I2C_SDA_GPIO;   // from Kconfig.projbuild
    bus_cfg.scl_io_num = (gpio_num_t)CONFIG_EXAMPLE_I2C_SCL_GPIO;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    // myGNSS.enableDebugging(); // Uncomment for debug messages on stdout
    while (!myGNSS.begin(bus)) {
        printf("u-blox GNSS not detected at default I2C address. Retrying...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    while (true) {
        if (myGNSS.getNAVPVT()) {
            printf("Lat: %ld Long: %ld (degrees * 10^-7) Alt: %ld (mm)\n",
                   (long)myGNSS.getLatitude(), (long)myGNSS.getLongitude(), (long)myGNSS.getAltitudeMSL());
        }
        if (myGNSS.getUBX("NAV", "PVT")) {
            ubxMessage *msg = myGNSS.ubxMessages.findByName("NAV", "PVT");
            ubxAnyType lon = myGNSS.getUbxMessageField(msg, "lon");
            printf("Lat: %ld Long: %ld\n", (long)(int32_t)myGNSS.getUbxMessageField(msg, "lat"), (long)lon.I4);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

The files that go with it:

- `main/CMakeLists.txt`: `idf_component_register(SRCS "main.cpp" INCLUDE_DIRS ".")`
- `main/idf_component.yml`:
  ```yaml
  dependencies:
    sparkfun/sparkfun_u-blox_gnss_v4:
      version: "*"
      override_path: "../../../"
  ```
  With `override_path`, the example builds from the repo itself. Registry users get the published version.
- `main/Kconfig.projbuild`: pin numbers, UART port/baud, SPI host/CS.
- `sdkconfig.defaults`: `CONFIG_FREERTOS_HZ=1000`.

---

## 7. Step-by-step implementation plan

Each phase ends with a check that can be verified. Phases 1–3 happen upstream in v4, on a branch, and the Arduino examples must still compile after each one.

The durations in the phase headings are **human engineer-days**: how long an experienced embedded developer working alone would take. Section 7.1 explains this and gives an estimate for the Claude-plus-hardware way of working used on v4.

**Phase 0: Set up the toolchain (≈0.5 engineer-day)**

1. Install ESP-IDF v5.5 LTS and v6.x (EIM or VS Code extension), plus the current Arduino IDE/`arduino-cli` for regression builds.
2. Create a branch `esp-idf-component` in SparkFun_u-blox_GNSS_v4.
3. Check the Arduino baseline: `compile_example.bat`, or `arduino-cli compile` for ESP32 and one non-ESP board, on all 23 examples. Record the warnings.

**Phase 1: Portability header, no behaviour change (≈1 engineer-day)**

4. Add `src/sfe_platform.h` (6.2) with an Arduino branch only at first.
5. Replace `#include <Arduino.h>` / `<Wire.h>` / `<SPI.h>` in the 8 files listed in section 4 with `#include "sfe_platform.h"`.
6. Rename `millis`, `micros` and `delay` to `sfe_millis()`, `sfe_micros()` and `sfe_delay()`, and `pinMode`/`digitalWrite` to `sfe_pin_output()`/`sfe_pin_write()`. Use mechanical search and replace, then review.
7. Introduce `sfe_string_t` plus the helpers (6.5), and convert `nmeaMessage.h` and `nmeaMessageVector.h`.
8. Change `new` to `new (std::nothrow)` wherever the result is already null-checked.
9. ✔ All Arduino examples compile with no new warnings. Run one hardware smoke test (PollingExample1 on an ESP32 over I2C).

**Phase 2: Output-sink abstraction (≈0.5 engineer-day)**

10. Add `SfeOutput`, and move the `print` number formatting into `SfePrint`.
11. Add `SfeArduinoPrintOutput`, and keep the `Print &` overloads of `enableDebugging()`/`set…OutputPort()` under `#if SFE_ARDUINO`.
12. ✔ Arduino: debug output and `setNMEAOutputPort(Serial)` behave as before.

**Phase 3: ESP-IDF back end (≈2–3 engineer-days)**

13. Add the ESP-IDF branch of `sfe_platform.h` (including the `delay` of 6.4).
14. Split `sfe_bus.h` class declarations with `#if SFE_ARDUINO` / `#elif SFE_ESP_IDF`, and write `sfe_bus_esp_idf.cpp` (6.3).
15. Add the ESP-IDF `begin()` overloads to `SparkFun_u-blox_GNSS_v4.h`, and the ESP-IDF default for `enableDebugging()` (stdout).
16. Handle `u-blox_GNSS.h` platform conditionals (the `SerialUSB`/`Serial` defaults) and the `debugPin` GPIO.
17. Add `CMakeLists.txt`, `idf_component.yml` and (optionally) `Kconfig` at the repo root.
18. ✔ `idf.py build` of a scratch project with only `SFE_UBLOX_GNSS gnss;` compiles cleanly with no warnings (ESP-IDF promotes many warnings to errors by default) on esp32, esp32s3 and esp32c6 (Xtensa + RISC-V).

**Phase 4: First three examples and hardware bring-up (≈2 engineer-days) → *"Success Indicator"***

19. Convert `PollingExample1` (I2C), `PollingExample2` (UART) and `PollingExample3` (SPI) using the 6.7 template.
20. Flash each to an ESP32 connected to a ZED-F9P/ZED-X20P. Check: `begin()` succeeds, PVT prints, and `enableDebugging()` output looks correct.
21. Stress test: `CallbackExample6_RAWX` over I2C and SPI at 10–20 Hz. Watch for dropped packets and task-watchdog warnings, and tune the transaction size and UART RX buffer.
22. ✔ **Success indicator met:** a modified example compiles and runs on ESP32 hardware.

**Phase 5: Remaining examples (≈2–3 engineer-days)**

23. Convert the other 20 examples (mapping in section 8). The Callback examples keep `checkUblox()`/`checkCallbacks()` in the `while` loop. Optionally, show a variant that runs them in a dedicated FreeRTOS task.
24. `DataloggingExample1_RAWX_and_SFRBX`: replace Arduino `SD` with `esp_vfs_fat_sdspi_mount()` (or SDMMC) + `fopen`/`fwrite`.
25. ✔ Every example builds in CI, and the hardware-available ones (not D9S/D9C, see v4 status notes) run.

**Phase 6: CI and registry publication (≈1 engineer-day)**

26. GitHub Actions:
    - Build every `idf_examples/*` with `espressif/esp-idf-ci-action` across a matrix of ESP-IDF {5.3, 5.5, 6.x} × targets {esp32, esp32s3, esp32c3, esp32c6}.
    - Keep the Arduino compile job and `arduino-lint`, so both halves stay green.
27. Write `README` sections for ESP-IDF usage and a `CHANGELOG.md`.
28. Dry run with `compote component upload --dry-run` (or `idf.py` packaging). Check the archive contents and the manifest warnings.
29. Publish to the ESP Component Registry under a `sparkfun` namespace. Automate later releases with `espressif/upload-components-ci-action` on tag.

**Phase 7 (optional): C facade (≈1–2 engineer-days)**

30. Add `include/sfe_ublox_gnss_c.h` with an `extern "C"` API built on the generic name-based field accessors. Add one `main.c` example to show pure-C use.

### 7.1 Effort estimates: what they mean

**Basis of the engineer-day figures.** These are conventional estimates for one human developer who knows C++ and ESP-IDF. They include writing, compiling, debugging and bench testing. Rough total: **about 10–13 engineer-days** (Phases 0–6), plus 1–2 for the optional Phase 7. They are *not* estimates of Claude session time.

**Claude-plus-hardware view.** On v4 the working pattern has been: Claude writes and edits the code, and Paul compiles, flashes and tests on real hardware. The cost is then counted in two units: **Claude sessions**, where one session is one usage-limit window like this investigation, and **hardware test rounds**, where one round is Paul compiling, flashing, observing and reporting back.

| Phase | Engineer-days | Claude sessions (est.) | Hardware test rounds (Paul) | What gates progress |
|---|---|---|---|---|
| 0 Toolchain | 0.5 | 0–1 | 1 (Arduino baseline) | Toolchain installs on Paul's PC |
| 1 Portability header | 1 | 1 (Phases 1 + 2 together) | 1 (Arduino regression + smoke test) | Arduino compile check |
| 2 Output sinks | 0.5 | (with Phase 1) | (with Phase 1) | Arduino compile check |
| 3 ESP-IDF back end | 2–3 | 1–2 | 0–1 (compile only) | `idf.py build` access |
| 4 First examples + bring-up | 2 | 1–2 | **2–4** | Real bus timing on ESP32 + u-blox |
| 5 Remaining 20 examples | 2–3 | 1–2 | 1–2 | Batch hardware runs |
| 6 CI + registry | 1 | 1 | 0 (GitHub Actions) | CI runs, namespace decision |
| **Total (0–6)** | **10–13** | **≈5–9** | **≈5–9** | |
| 7 C facade (optional) | 1–2 | 1 | 1 | |

How to read the table:

- **Writing the code is the fast part.** Most of the changes are mechanical and follow a pattern: platform header, bus classes, search-and-replace, build files, and 23 examples from one template. Claude can produce them quickly.
- **Testing is the slow part.** Elapsed time is set mostly by the number of test rounds and by how quickly each one can fit into Paul's day, not by coding speed. Phase 4 carries most of the uncertainty: I2C/SPI transaction sizes, the UART buffer under RAWX load, and tick-rate and watchdog behaviour (section 6.4).
- **Compile access matters most.** The v4 migration notes record that Docker, and so `compile_example.bat`, was not available in the sandboxes tried. If Claude can run `idf.py build` and `arduino-cli compile` itself, either in the cloud workspace or through the linked computer's shell, compile errors get fixed within the session. Hardware rounds are then only needed for runtime behaviour, and the session and round counts move towards the low end. Without compile access, every compile error costs a round trip through Paul, and the counts move to the high end or beyond.
- **Confidence.** The engineer-day figures are moderately reliable. The session counts are **low-confidence**: usage per session depends on how much code has to be read back and on how many debugging iterations are needed, and neither can be measured well in advance. The test-round count (≈5–9 in total, 4–7 of them up to and including the Phase 4 success indicator) is the most useful number for planning.

---

## 8. Example conversion map

| Arduino example | Bus | ESP-IDF notes |
|---|---|---|
| PollingExample1_PositionVelocityTime | I2C | Template (6.7) |
| PollingExample2_PositionVelocityTime_Serial | UART | `uart_driver_install()` on UART1, `begin(UART_NUM_1)` |
| PollingExample3_PositionVelocityTime_SPI | SPI | `spi_bus_initialize()` + `begin(SPI2_HOST, CS)` |
| PollingExample4_SECUNIQID, PollingExample5_GPZDA | I2C | Uses `String`; switch to `std::string` / `.c_str()` |
| PeriodicExample1_NAVHPPOSLLH, PeriodicExample2_GPGGA | I2C | Periodic → `while` + `vTaskDelay`; GPGGA uses `String` |
| CallbackExample1–15 (NAVHPPOSLLH, GPRMC, GPRMC+GPGLL, NAVSAT, NAVSIG, RAWX, NMEA_GSV, MONCOMMS, SECSIG, ESFMEAS, ESFRAW, ESFSTATUS, NEO-D9S_RXMPMP, NEO-D9C_RXMQZSSL6, MONRF) | I2C | Callback functions unchanged apart from `Serial.print` → `printf`. GSV (39 `String` uses) needs the most edits. D9S/D9C can be compile-only (service/hardware unavailable). |
| DataloggingExample1_RAWX_and_SFRBX | I2C + SD | FATFS over SDSPI; file buffer API unchanged |

A mechanical rule for all examples: `Serial.print(a); Serial.print(b); Serial.println(c);` becomes one `printf("%… %… %…\n", …)`. Where an example prints with `HEX` or decimal places, use `%X` or `%.nf`.

---

## 9. Risks and open questions

1. **Arduino regression risk.** The shared headers change. This is mitigated by phases that each leave the Arduino build green, and by CI.
2. **SPI byte-at-a-time path.** Per-transaction overhead on ESP-IDF (µs-scale) may limit throughput at high rates. Buffer where possible, and measure in Phase 4.
3. **Tick rate.** Document the 1000 Hz recommendation, and make sure the library is still correct at 100 Hz (6.4).
4. **Registry naming.** Decide the namespace/name, for example `sparkfun/sparkfun_u-blox_gnss_v4`, and whether this project folder becomes the component repo or the component lives inside v4 (recommended).
5. **Minimum ESP-IDF version.** `>=5.3` is proposed. Confirm it with the CI matrix. Supporting older versions would need the legacy I2C driver, which is not recommended.
6. **Thread safety.** The default lock hooks are no-ops. If users call the library from several tasks, enable the FreeRTOS-mutex Kconfig option.
7. **Licensing.** Keep the implementation clean-room with respect to the GPL-3.0 Soldered component. MIT is compatible with the registry.

---

## 10. References

- ESP-IDF Programming Guide: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/
- ESP-IDF v6.0 peripherals migration guide (legacy I2C EOL; driver component split): https://docs.espressif.com/projects/esp-idf/en/stable/esp32/migration-guides/release-6.x/6.0/peripherals.html
- Arduino as an ESP-IDF component (3.3.12 ↔ ESP-IDF v5.5, `CONFIG_FREERTOS_HZ=1000`): https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html
- IDF Component Manager manifest reference: https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/manifest_file.html
- ESP Component Registry: https://components.espressif.com/
- Soldered u-blox GPS GNSS component: https://components.espressif.com/components/solderedelectronics/soldered-u-blox-gps-gnss-esp-idf-component/versions/0.0.3/readme and https://github.com/SolderedElectronics/Soldered-u-blox-GPS-GNSS-ESP-IDF-Component (examined at HEAD, 3 Sep 2026; GPL-3.0; based on SparkFun v2)
- SparkFun u-blox GNSS v4: https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4 (examined at `08943c2`)
- SparkFun Toolkit: https://github.com/sparkfun/SparkFun_Toolkit
