# SparkFun u-blox GNSS v4: Utilities

Two Python tools for checking and processing u-blox UBX log files, such as the `.ubx` files written by the [DataloggingExample1](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/examples/DataloggingExample1_RAWX_and_SFRBX) and [DataloggingExample2](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/blob/main/examples/DataloggingExample2_DataLogger_IoT_SDIO) examples.

Both tools need Python 3 and use only the Python standard library. Neither tool modifies the original file: any output is written to a new file.

## UBX_Integrity_Checker.py

Checks the integrity of a UBX log file. The file can contain a mix of UBX, NMEA and RTCM messages. Every message is checked:

* UBX: sync characters (0xB5 0x62) and the 2-byte Fletcher checksum
* NMEA: the `*` checksum, the CR LF terminator, and the message length
* RTCM: the 0xD3 preamble and the 3-byte CRC-24Q

If an error is found (a dropped byte, or a corrupt message), the checker reports where sync was lost, rewinds to the last known good data, and re-syncs on the next valid message. Optionally, it writes a repaired copy of the file which contains only the valid (checksum-verified) messages.

At the end, it prints a summary: the number of bytes processed (and a warning if this does not match the file size), the longest valid UBX / NMEA / RTCM message, the total count of each message type (UBX as class and ID, e.g. `0x02 0x15` for RXM-RAWX and `0x02 0x13` for RXM-SFRBX; NMEA by name, e.g. `GNGGA`; RTCM by number, e.g. `1005`), and the number of successful re-syncs.

```
python UBX_Integrity_Checker.py <ubxFile> [-r REPAIRFILE] [--GNTXT] [-rw REWINDS]
```

| Argument | Description |
| --- | --- |
| `ubxFile` | Required. The path to the UBX file to check |
| `-r REPAIRFILE`, `--repairFile REPAIRFILE` | Optional. Write a repaired copy of the file, containing only the valid messages, to `REPAIRFILE` |
| `--GNTXT` | Optional. Print the contents of any NMEA GNTXT (text) messages found in the file |
| `-rw REWINDS`, `--rewinds REWINDS` | Optional. The maximum number of times the checker will rewind and try to re-sync before giving up. Default: 100 |

Examples:

```
python UBX_Integrity_Checker.py RXM_RAWX.ubx
python UBX_Integrity_Checker.py RXM_RAWX.ubx -r RXM_RAWX_repaired.ubx --GNTXT
```

The checker can also be used from your own Python code:

```python
from UBX_Integrity_Checker import UBX_Integrity_Checker

checker = UBX_Integrity_Checker('RXM_RAWX.ubx', repairFile=None, printGNTXT=False, maxRewinds=100)
checker.checkIntegrity()
```

## UBX_RAWX_Aligner.py

Aligns the receiver time of week (`rcvTow`) in the UBX RXM-RAWX messages in a UBX log file, by rounding it to the nearest whole second (or to a chosen number of decimal places). The UBX checksum of each RXM-RAWX message is recalculated. All other messages are copied unchanged. This can help post-processing software (e.g. RTKLIB) match the measurement epochs to those of a base station or reference network.

The aligned data is written to a new file, with `.aligned` added to the name. E.g. `RXM_RAWX.ubx` is written to `RXM_RAWX.aligned.ubx`. The file can contain UBX and NMEA messages (but not RTCM). Like the integrity checker, the aligner re-syncs if it finds an error, and prints a summary at the end, including the largest alignment change.

```
python UBX_RAWX_Aligner.py [ubxFile] [decimalPlaces]
```

| Argument | Description |
| --- | --- |
| `ubxFile` | Optional. The path to the UBX file. If it is not given, you are asked for it. The first `.ubx` file in the current directory is offered as the default: press Enter to accept it |
| `decimalPlaces` | Optional. The number of decimal places to round `rcvTow` to. Default: 0 (whole seconds). E.g. `1` rounds to the nearest 0.1 seconds. To use this, `ubxFile` must be given too |

The aligner also asks: `Could this file contain any NMEA messages? (Y/n)`. Press Enter (or `Y`) if the file contains NMEA messages. Answer `n` if it contains UBX only.

Example:

```
python UBX_RAWX_Aligner.py RXM_RAWX.ubx
```
