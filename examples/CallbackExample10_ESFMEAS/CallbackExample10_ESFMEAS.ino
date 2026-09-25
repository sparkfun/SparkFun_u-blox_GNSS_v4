/*
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 * 
 * Configuring the GNSS to automatically send ESF MEAS reports over I2C and display them using a callback
 * By: Paul Clark
 * SparkFun Electronics
 *
 * This example configures the External Sensor Fusion MEAS sensor messages on the ZED-F9R and
 * shows how to access the ESF data using callbacks.
 *  
 * Feel like supporting open source hardware?
 * Buy a board from SparkFun!
 * https://www.sparkfun.com/sparkfun-gps-rtk-dead-reckoning-breakout-zed-f9r-qwiic-gps-22693.html
 *
 * Hardware Connections:
 * Plug a Qwiic cable into the GNSS and your microcontroller board
 * Open the serial monitor at 115200 baud to see the output
 */

#include <Wire.h> //Needed for I2C to GPS

#include <SparkFun_u-blox_GNSS_v4.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v4

SFE_UBLOX_GNSS myGNSS;

// Callback: printESFMEASdata will be called when new ESF MEAS data arrives
void printESFMEASdata(ubxCallbackDataCommon_t *theData)
{
  ubxMessage *msg = myGNSS.getUbxMessagePtr(theData);

  if (1) // Change to 0 to disable the callback prints. Useful when trying to capture debug...
  {
    Serial.println();

    // Print the timeTag
    uint32_t timeTag = (uint32_t)myGNSS.getUbxMessageFieldCallback(msg, "timeTag");
    Serial.print(F("Time:         "));
    Serial.println(timeTag);

    // numMeas indicates how many sensor groups the data contains.
    // As a test, compare it to getUbxMessageBlockCount()
    uint8_t numMeas = (uint8_t)myGNSS.getUbxMessageFieldCallback(msg, "numMeas");
    Serial.print(F("Measurements: "));
    Serial.println(numMeas);
    if (numMeas != myGNSS.getUbxMessageBlockCountCallback(msg))
    {
      Serial.print("!!! numMeas (");
      Serial.print(numMeas);
      Serial.print(") does not match getUbxMessageBlockCount (");
      Serial.print(myGNSS.getUbxMessageBlockCountCallback(msg));
      Serial.println(") !!!");
    }
    for (uint8_t i = 0; i < numMeas; i++)
    {
      // Print the sensor data type
      // From the M8 interface description:
      //   0: None
      // 1-4: Reserved
      //   5: z-axis gyroscope angular rate deg/s * 2^-12 signed
      //   6: front-left wheel ticks: Bits 0-22: unsigned tick value. Bit 23: direction indicator (0=forward, 1=backward)
      //   7: front-right wheel ticks: Bits 0-22: unsigned tick value. Bit 23: direction indicator (0=forward, 1=backward)
      //   8: rear-left wheel ticks: Bits 0-22: unsigned tick value. Bit 23: direction indicator (0=forward, 1=backward)
      //   9: rear-right wheel ticks: Bits 0-22: unsigned tick value. Bit 23: direction indicator (0=forward, 1=backward)
      //  10: speed ticks: Bits 0-22: unsigned tick value. Bit 23: direction indicator (0=forward, 1=backward)
      //  11: speed m/s * 1e-3 signed
      //  12: gyroscope temperature deg Celsius * 1e-2 signed
      //  13: y-axis gyroscope angular rate deg/s * 2^-12 signed
      //  14: x-axis gyroscope angular rate deg/s * 2^-12 signed
      //  16: x-axis accelerometer specific force m/s^2 * 2^-10 signed
      //  17: y-axis accelerometer specific force m/s^2 * 2^-10 signed
      //  18: z-axis accelerometer specific force m/s^2 * 2^-10 signed
      uint8_t dataType = (uint8_t)myGNSS.getUbxMessageBlockFieldCallback(msg, i, "dataType");
      switch (dataType)
      {
      case 5:
        Serial.print(F("Z Gyro:       "));
        break;
      case 6:
        Serial.print(F("Front Left:   "));
        break;
      case 7:
        Serial.print(F("Front Right:  "));
        break;
      case 8:
        Serial.print(F("Rear Left:    "));
        break;
      case 9:
        Serial.print(F("Rear Right:   "));
        break;
      case 10:
        Serial.print(F("Speed Ticks:  "));
        break;
      case 11:
        Serial.print(F("Speed:        "));
        break;
      case 12:
        Serial.print(F("Temp:         "));
        break;
      case 13:
        Serial.print(F("Y Gyro:       "));
        break;
      case 14:
        Serial.print(F("X Gyro:       "));
        break;
      case 16:
        Serial.print(F("X Accel:      "));
        break;
      case 17:
        Serial.print(F("Y Accel:      "));
        break;
      case 18:
        Serial.print(F("Z Accel:      "));
        break;
      default:
        break;
      }

      uint32_t dataField = (uint32_t)myGNSS.getUbxMessageBlockFieldCallback(msg, i, "dataField");

      // Tick data
      if ((dataType >= 6) && (dataType <= 10))
      {
        if ((dataField & (1 << 23)) > 0)
          Serial.print(F("-")); // Backward
        else
          Serial.print(F("+")); // Forward
        Serial.println(dataField & 0x007FFFFF);
      }
      // Speed
      else if (dataType == 11)
      {
        union
        {
          int32_t signed32;
          uint32_t unsigned32;
        } signedUnsigned; // Avoid any ambiguity casting uint32_t to int32_t
        // The dataField is 24-bit signed, stored in the 24 LSBs of a uint32_t
        signedUnsigned.unsigned32 = dataField << 8; // Shift left by 8 bits to correctly align the data
        float speed = signedUnsigned.signed32;      // Extract the signed data. Convert to float
        speed /= 256.0;                             // Divide by 256 to undo the shift
        speed *= 0.001;                             // Convert from m/s * 1e-3 to m/s
        Serial.println(speed, 3);
      }
      // Gyro data
      else if ((dataType == 5) || (dataType == 13) || (dataType == 14))
      {
        union
        {
          int32_t signed32;
          uint32_t unsigned32;
        } signedUnsigned; // Avoid any ambiguity casting uint32_t to int32_t
        // The dataField is 24-bit signed, stored in the 24 LSBs of a uint32_t
        signedUnsigned.unsigned32 = dataField << 8; // Shift left by 8 bits to correctly align the data
        float rate = signedUnsigned.signed32;       // Extract the signed data. Convert to float
        rate /= 256.0;                              // Divide by 256 to undo the shift
        rate *= 0.000244140625;                     // Convert from deg/s * 2^-12 to deg/s
        Serial.println(rate);
      }
      // Accelerometer data
      else if ((dataType == 16) || (dataType == 17) || (dataType == 18))
      {
        union
        {
          int32_t signed32;
          uint32_t unsigned32;
        } signedUnsigned; // Avoid any ambiguity casting uint32_t to int32_t
        // The dataField is 24-bit signed, stored in the 24 LSBs of a uint32_t
        signedUnsigned.unsigned32 = dataField << 8; // Shift left by 8 bits to correctly align the data
        float force = signedUnsigned.signed32;      // Extract the signed data. Convert to float
        force /= 256.0;                             // Divide by 256 to undo the shift
        force *= 0.0009765625;                      // Convert from m/s^2 * 2^-10 to m/s^2
        Serial.println(force);
      }
      // Gyro Temperature
      else if (dataType == 12)
      {
        union
        {
          int32_t signed32;
          uint32_t unsigned32;
        } signedUnsigned; // Avoid any ambiguity casting uint32_t to int32_t
        // The dataField is 24-bit signed, stored in the 24 LSBs of a uint32_t
        signedUnsigned.unsigned32 = dataField << 8;  // Shift left by 8 bits to correctly align the data
        float temperature = signedUnsigned.signed32; // Extract the signed data. Convert to float
        temperature /= 256.0;                        // Divide by 256 to undo the shift
        temperature *= 0.01;                         // Convert from C * 1e-2 to C
        Serial.println(temperature);
      }
    }

    // Print the calibTtag - if present
    uint32_t calibTtag = (uint32_t)myGNSS.getUbxMessageFooterFieldCallback(msg, "calibTtag");
    Serial.print(F("Rx Time:      "));
    Serial.println(calibTtag);
  }
}

void setup()
{
  Serial.begin(460800); // <-- Use a fast baud rate to avoid the Serial prints slowing the code
  delay(1000); 
  Serial.println("SparkFun u-blox Example");

  Wire.begin(); // Start I2C
  Wire.setClock(400000); // <-- Use 400kHz I2C

  //myGNSS.enableDebugging(Serial, true); // Uncomment this line to enable the important debug messages on Serial

  while (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println("u-blox GNSS not detected at default I2C address. Retrying...");
    delay (1000);
  }

  // setCfgValset() writes to the RAM and Battery-backed-RAM layers by default (VAL_LAYER_RAM_BBR).
  // To use a different layer, add it as the third parameter. E.g. for RAM only:
  //   setCfgValset(UBLOX_CFG_MSGOUT_..., n, VAL_LAYER_RAM)
  // VAL_LAYER_ALL also saves the setting in Flash (if the module has Flash).

  // Enable the ESF MEAS Message on I2C, every navigation cycle
  myGNSS.setCfgValset(UBLOX_CFG_MSGOUT_UBX_ESF_MEAS_I2C, 1);

  // Set up a callback for ESF MEAS messages. Call newSECSIG() each time one arrives.
  // Note: this does not enable the SEC SIG message. The message is assumed to be periodic.
  // All this does is register the callback.
  myGNSS.setAutoCallbackPtr("ESF", "MEAS", &printESFMEASdata);
}

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.
}
