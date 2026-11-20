/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * Making NAV-PVT periodic ("Automatic") on the module's UART2 port
 * By: Paul Clark
 * SparkFun Electronics
 *
 * This example connects to the u-blox module's UART2 port (not UART1) and makes the NAV-PVT
 * message periodic on UART2 by calling setAutoUBXrate("NAV", "PVT", 1, true, VAL_LAYER_RAM);
 * It reproduces the code from GitHub issue #4:
 * https://github.com/sparkfun/SparkFun_u-blox_GNSS_v4/issues/4
 *
 * connectedToUART2() tells the library that the microcontroller is connected to the module's
 * UART2. The library then uses the UART2 configuration keys - e.g. UBLOX_CFG_MSGOUT_UBX_NAV_PVT_UART2
 * when setAutoUBX() or setAutoUBXrate() enables a periodic message.
 *
 * Why setUART2Output(COM_TYPE_UBX) is needed:
 *
 * By default, the X20's UART2 accepts UBX messages but does not output them. From the u-blox X20 HPG 2.11
 * Interface Description, "Configuration defaults" (Tables 109 and 110):
 *   CFG-UART2INPROT-UBX   = 1 (true)  - UART2 accepts UBX messages
 *   CFG-UART2OUTPROT-UBX  = 0 (false) - UART2 does not output UBX messages
 *   CFG-UART2OUTPROT-NMEA = 0 (false) - or NMEA messages
 *
 * Even so, begin() connects over UART2 with the default configuration. begin() checks the connection
 * by polling a configuration value (UBX-CFG-VALGET) and waiting for the reply and its acknowledgement
 * (UBX-ACK-ACK). The module sends replies and acknowledgements back to the sender - on the port the
 * request arrived on - even when UBX output is disabled on that port. (Tested on a ZED-X20P running
 * HPG 2.11.)
 *
 * The OUTPROT settings control the messages the module outputs by itself, like the periodic NAV-PVT.
 * So, like the code in issue #4, the example calls setUART2Output(COM_TYPE_UBX) to enable UBX output on
 * UART2. Without it, setAutoUBXrate() enables NAV-PVT on UART2, but the module does not output it.
 * Note: setUART2Output(COM_TYPE_UBX) also disables NMEA and RTCM3 output on UART2.
 *
 * The example also calls setUART2Input() with UBX, NMEA, RTCM3 and SPARTN - the X20 defaults - to make
 * sure UBX input is enabled on UART2. (If UBX input has been disabled on UART2, the library can not talk
 * to the module over UART2. Use u-center, or another port, to enable it first.)
 *
 * All of these settings are made in RAM only. They are cleared when the module is powered off.
 *
 * After setAutoUBXrate(), the example reads back the NAV-PVT output rate for each port, so you
 * can see which port's configuration key was written. You should see "UART2: 1". (Before the
 * library fixed issue #4, setAutoUBXrate wrote the SPI key instead: "SPI: 1", "UART2: 0".)
 * Power-cycle the module before running this example, so that the rates start from the defaults.
 *
 * Feel like supporting open source hardware?
 * Buy a board from SparkFun!
 * https://www.sparkfun.com/sparkfun-allband-gnss-rtk-breakout-zed-x20p-qwiic.html
 * https://www.sparkfun.com/sparkfun-gps-rtk2-board-zed-f9p-qwiic-gps-15136.html
 * https://www.sparkfun.com/sparkfun-gps-rtk-sma-breakout-zed-f9p-qwiic.html
 * https://www.sparkfun.com/sparkfun-gps-rtk-dead-reckoning-breakout-zed-f9r-qwiic-gps-22693.html
 * (The MAX-M10S does not have a UART2)
 *
 * Hardware Connections:
 * Connect: GNSS TX2 to microcontroller RX; GNSS RX2 to microcontroller TX; and GND
 * Plus 3V3 or 5V depending on your needs
 * Open the serial monitor at 115200 baud to see the output
 */

#include <SparkFun_u-blox_GNSS_v4.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v4

SFE_UBLOX_GNSS_SERIAL myGNSS; // SFE_UBLOX_GNSS_SERIAL uses Serial (UART). This is the object from issue #4

#define GNSS_UART_PORT Serial1 // The microcontroller UART connected to the module's UART2. Change this if required
#define GNSS_UART_BAUD 38400   // The module's UART2 baud rate. u-blox X20 and F9 modules default to 38400 baud. Change this if required
#define GNSS_NAV_FREQUENCY 1   // The navigation rate (Hz). (The value used in issue #4 is not known)

unsigned long lastPVT; // millis() when the last NAV-PVT arrived

// Read back and print the NAV-PVT output rate for one port
void printPVTRate(const char *portName, uint32_t key)
{
  uint8_t rate;
  Serial.print(portName);
  Serial.print(": ");
  if (myGNSS.getVal8(key, &rate, VAL_LAYER_RAM))
    Serial.print(rate);
  else
    Serial.print("?");
  Serial.print("  ");
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("SparkFun u-blox Example");

  GNSS_UART_PORT.begin(GNSS_UART_BAUD);

  //myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  myGNSS.connectedToUART2(); // Tell the library we are connected to UART2 (not UART1). Call this before begin()

  while (myGNSS.begin(GNSS_UART_PORT) == false) //Connect to the u-blox module using GNSS_UART_PORT (defined above)
  {
    Serial.println("u-blox GNSS not detected on UART2. Retrying...");
    delay (1000);
  }
  Serial.println("Connected to the module on UART2");

  // Enable UBX output (OUTPROT) on UART2 - needed for the periodic NAV-PVT. This also turns off NMEA and RTCM3 output
  myGNSS.setUART2Output(COM_TYPE_UBX, VAL_LAYER_RAM);

  // Make sure UBX input (INPROT) is enabled on UART2. Keep NMEA, RTCM3 and SPARTN input enabled (the X20 defaults)
  myGNSS.setUART2Input(COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3 | COM_TYPE_SPARTN, VAL_LAYER_RAM);

  myGNSS.setNavigationFrequency(GNSS_NAV_FREQUENCY, VAL_LAYER_RAM); // Set the navigation rate

  // Enable periodic ("automatic") NAV-PVT messages on UART2: every navigation epoch, with implicit update
  if (myGNSS.setAutoUBXrate("NAV", "PVT", 1, true, VAL_LAYER_RAM) == true)
    Serial.println("setAutoUBXrate(\"NAV\", \"PVT\") succeeded");
  else
    Serial.println("setAutoUBXrate(\"NAV\", \"PVT\") failed!");

  // Read back the NAV-PVT output rate for each port. We expect UART2: 1
  Serial.print("NAV-PVT output rate - ");
  printPVTRate("I2C", UBLOX_CFG_MSGOUT_UBX_NAV_PVT_I2C);
  printPVTRate("SPI", UBLOX_CFG_MSGOUT_UBX_NAV_PVT_SPI);
  printPVTRate("UART1", UBLOX_CFG_MSGOUT_UBX_NAV_PVT_UART1);
  printPVTRate("UART2", UBLOX_CFG_MSGOUT_UBX_NAV_PVT_UART2);
  Serial.println();

  lastPVT = millis();
}

void loop()
{
  // Because we called setAutoUBXrate("NAV", "PVT", ...), the library knows
  // NAV-PVT messages will be arriving periodically on UART2.
  // getNAVPVT() does not poll. It returns true (once) when a fresh NAV-PVT message has arrived.
  // Read the PVT fields within the `if getNAVPVT()`:

  if (myGNSS.getNAVPVT() == true)
  {
    lastPVT = millis();

    Serial.println();

    Serial.print("TOW: ");
    Serial.print(myGNSS.getTimeOfWeek()); // Use the helper methods
    Serial.print(" (ms) Fix: ");
    Serial.print(myGNSS.getFixType());
    Serial.print(" SIV: ");
    Serial.print(myGNSS.getSIV());
    Serial.print(" Lat: ");
    Serial.print(myGNSS.getLatitude());
    Serial.print(" Long: ");
    Serial.print(myGNSS.getLongitude());
    Serial.print(" (degrees * 10^-7) Alt: ");
    Serial.print(myGNSS.getAltitudeMSL());
    Serial.println(" (mm)");
  }

  // NAV-PVT should arrive every 1/GNSS_NAV_FREQUENCY seconds. Warn if it stops arriving
  if (millis() - lastPVT > 3000)
  {
    Serial.println();
    Serial.println("No NAV-PVT received on UART2 for 3 seconds!");
    lastPVT = millis();
  }

  Serial.print(".");
  delay(50);
}
