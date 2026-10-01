/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * Configuring the GNSS to automatically send MON COMMS reports over I2C and display them using a callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * This example shows how to configure the u-blox GNSS to send MON COMMS reports automatically
 * and access the data via a callback.
 *  
 * Feel like supporting open source hardware?
 * Buy a board from SparkFun!
 * https://www.sparkfun.com/sparkfun-allband-gnss-rtk-breakout-zed-x20p-qwiic.html
 * https://www.sparkfun.com/sparkfun-gps-rtk2-board-zed-f9p-qwiic-gps-15136.html
 * https://www.sparkfun.com/sparkfun-gps-rtk-sma-breakout-zed-f9p-qwiic.html
 * https://www.sparkfun.com/sparkfun-gnss-receiver-breakout-max-m10s-qwiic.html
 * https://www.sparkfun.com/sparkfun-gps-rtk-dead-reckoning-breakout-zed-f9r-qwiic-gps-22693.html
 *
 * Hardware Connections:
 * Plug a Qwiic cable into the GNSS and your microcontroller board
 * Open the serial monitor at 115200 baud to see the output
 */

#include <Wire.h> //Needed for I2C to GPS

#include <SparkFun_u-blox_GNSS_v4.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v4

SFE_UBLOX_GNSS myGNSS;

// Callback: newMONCOMMS will be called when new MON COMMS data arrives
void newMONCOMMS(ubxCallbackDataCommon_t *theData)
{
  ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

  Serial.println();

  uint8_t nPorts = (uint8_t)myGNSS.getUbxMessageFieldCallback(msg, "nPorts");
  Serial.print(F("New MON COMMS data received. It contains data for "));
  Serial.print(nPorts);
  if (nPorts == 1)
    Serial.println(F(" port."));
  else
    Serial.println(F(" ports."));

  // Mimic the data shown in u-center
  for (uint8_t port = 0; port < nPorts; port++) // For each port
  {
    // Check the port ID is valid
    bool validPort = false;
    switch ((uint16_t)myGNSS.getUbxMessageBlockFieldCallback(msg, port, "portId"))
    {
      case COM_PORT_ID_I2C:
      case COM_PORT_ID_UART1:
      case COM_PORT_ID_UART2: // X20P uses 0x0200
      case COM_PORT_ID_UART2 + 1: // ZED-F9P uses 0x0201
      case COM_PORT_ID_USB:
      case COM_PORT_ID_SPI:
        validPort = true;
      break;      
      default:
        //Serial.printf("Unknown / reserved portId 0x%04X\r\n", (uint16_t)myGNSS.getUbxMessageBlockFieldCallback(msg, port, "portId"));
      break;      
    }

    if (validPort)
    {
      switch ((uint16_t)myGNSS.getUbxMessageBlockFieldCallback(msg, port, "portId")) // Print the port ID
      {
        case COM_PORT_ID_I2C:
          Serial.print(F("I2C       "));
        break;
        case COM_PORT_ID_UART1:
          Serial.print(F("UART1     "));
        break;
        case COM_PORT_ID_UART2:
          Serial.print(F("UART2 X20 "));
        break;
        case COM_PORT_ID_UART2 + 1:
          Serial.print(F("UART2 F9  "));
        break;
        case COM_PORT_ID_USB:
          Serial.print(F("USB       "));
        break;
        case COM_PORT_ID_SPI:
          Serial.print(F("SPI       "));
        break;
      }

      Serial.print(": txBytes ");
      String txBytes = String((uint32_t)myGNSS.getUbxMessageBlockFieldCallback(msg, port, "txBytes"));
      Serial.print(txBytes);
      for (int i = 0; i < 10 - txBytes.length(); i++)
        Serial.print(" ");
      
      Serial.print(" : rxBytes ");
      String rxBytes = String((uint32_t)myGNSS.getUbxMessageBlockFieldCallback(msg, port, "rxBytes"));
      Serial.print(rxBytes);
      for (int i = 0; i < 10 - rxBytes.length(); i++)
        Serial.print(" ");

      for (int i = 0; i < 4; i++)
      {
        char protId_str[strlen("protId0") + 1];
        snprintf(protId_str, sizeof(protId_str), "protId%d", i);
        uint8_t protId = (uint8_t)myGNSS.getUbxMessageFieldCallback(msg, protId_str);
        if (protId < 0xFF)
        {
          switch (protId)
          {
            case 0:
              Serial.print(F(" : UBX     "));
            break;
            case 1:
              Serial.print(F(" : NMEA    "));
            break;
            case 2:
              Serial.print(F(" : RTCM2   "));
            break;
            case 5:
              Serial.print(F(" : RTCM3   "));
            break;
            case 6:
              Serial.print(F(" : SPARTN  "));
            break;
            default:
              Serial.print(F(" : UNKNOWN "));
            break;
          }
          char msgs_str[strlen("msgs0") + 1];
          snprintf(msgs_str, sizeof(msgs_str), "msgs%d", i);
          String msgs = String((uint16_t)myGNSS.getUbxMessageBlockFieldCallback(msg, port, msgs_str));
          Serial.print(msgs);
          for (int i = 0; i < 5 - msgs.length(); i++)
            Serial.print(" ");
        }
      }
      
      Serial.print(" : skipped ");
      Serial.print((uint32_t)myGNSS.getUbxMessageBlockFieldCallback(msg, port, "skipped"));
      
      Serial.println();
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000); 
  Serial.println("SparkFun u-blox Example");

  Wire.begin(); // Start I2C

  //myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

  while (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println("u-blox GNSS not detected at default I2C address. Retrying...");
    delay (1000);
  }

  // setCfgValset() writes to the RAM and Battery-backed-RAM layers by default (VAL_LAYER_RAM_BBR).
  // To use a different layer, add it as the third parameter. E.g. for RAM only:
  //   setCfgValset(UBLOX_CFG_MSGOUT_..., n, VAL_LAYER_RAM)
  // VAL_LAYER_ALL also saves the setting in Flash (if the module has Flash).

  // Enable the MON COMMS Message on I2C, every two navigation cycles
  myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_MON_COMMS_I2C, 2);

  // Set up a callback for MON COMMS messages. Call newMONCOMMS() each time one arrives.
  // Note: this does not enable the COMMS message. The message is assumed to be periodic.
  // All this does is register the callback.
  myGNSS.setAutoCallbackPtr("MON", "COMMS", &newMONCOMMS);
}

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  Serial.print(".");
  delay(50);
}

// Print a uint8_t, right-justified with space padding as needed
void printPadded(uint8_t val, uint8_t padding)
{
  uint8_t digits = 1;
  if (val >= 100)
    digits = 3;
  else if (val >= 10)
    digits = 2;
  for (uint8_t p = padding; p > digits; p--)
    Serial.print(" ");
  Serial.print(val);
}
