/*
  To add a new sensor to the system:

  Add the library in OpenLog_Artemis
  Add DEVICE_ name to settings.h
  Add struct_MCP9600 to settings.h - This will define what settings for the sensor we will control

  Add gathering of data to gatherDeviceValues() in Sensors
  Add helper text to printHelperText() in Sensors

  Add class creation to addDevice() in autoDetect
  Add begin fucntions to beginQwiicDevices() in autoDetect
  Add configuration functions to configureDevice() in autoDetect
  Add pointer to configuration menu name to getConfigFunctionPtr() in autodetect
  Add test case to testDevice() in autoDetect
  Add pretty print device name to getDeviceName() in autoDetect

  Add menu title to menuAttachedDevices() list in menuAttachedDevices
  Create a menuConfigure_LPS25HB() function in menuAttachedDevices

  Add settings to the save/load device file settings in nvm
*/


//Let's see what's on the I2C bus
//Scan I2C bus including sub-branches of multiplexers
//Creates a linked list of devices
//Creates appropriate classes for each device
//Begin()s each device in list
//Returns true if devices detected > 0
bool detectQwiicDevices()
{
  printDebug("detectQwiicDevices started\r\n");
  bool somethingDetected = false;

  qwiic.setClock(100000); //During detection, go slow

  qwiic.setPullups(settings.qwiicBusPullUps); //Set pullups. (Redundant. beginQwiic has done this too.) If we don't have pullups, detectQwiicDevices() takes ~900ms to complete. We'll disable pullups if something is detected.

  //24k causes a bunch of unknown devices to be falsely detected.
  //qwiic.setPullups(24); //Set pullups to 24k. If we don't have pullups, detectQwiicDevices() takes ~900ms to complete. We'll disable pullups if something is detected.

  waitForQwiicBusPowerDelay(); // Wait while the qwiic devices power up

  // Note: The MCP9600 (Qwiic Thermocouple) is a fussy device. If we use beginTransmission + endTransmission more than once
  // the second and subsequent times will fail. The MCP9600 only ACKs the first time. The MCP9600 also appears to be able to
  // lock up the I2C bus if you don't discover it and then begin it in one go...
  // The following code has been restructured to try and keep the MCP9600 happy.

  Serial.println(F("Identifying Qwiic Muxes..."));

  //First scan for Muxes. Valid addresses are 0x70 to 0x77 (112 to 119).
  //If any are found, they will be begin()'d causing their ports to turn off
  uint8_t muxCount = 0;
  for (uint8_t address = 0x70 ; address < 0x78 ; address++)
  {
    qwiic.beginTransmission(address);
    if (qwiic.endTransmission() == 0)
    {
      //if (address == 0x74) // Debugging the slippery mux bug - trigger the scope when we test mux 0x74
      //{
      //  digitalWrite(PIN_LOGIC_DEBUG, LOW);
      //  digitalWrite(PIN_LOGIC_DEBUG, HIGH);
      //}
      somethingDetected = true;
      if (settings.printDebugMessages == true)
        Serial.printf("detectQwiicDevices: something detected at address 0x%02X\r\n", address);
      deviceType_e foundType = testMuxDevice(address, 0, 0); //No mux or port numbers for this test
      if (foundType == DEVICE_MULTIPLEXER)
      {
        addDevice(foundType, address, 0, 0); //Add this device to our map
        if (settings.printDebugMessages == true)
          Serial.printf("detectQwiicDevices: multiplexer found at address 0x%02X\r\n", address);
        muxCount++;
      }
    }
  }

  if (muxCount > 0)
  {
    if (settings.printDebugMessages == true)
    {
      Serial.printf("detectQwiicDevices: found %d", muxCount);
      if (muxCount == 1)
        Serial.println(F(" multiplexer"));
      else
        Serial.println(F(" multiplexers"));
    }
    beginQwiicDevices(); //begin() the muxes to disable their ports
  }

  //Before going into mux sub branches, scan the main branch for all remaining devices
  Serial.println(F("Identifying Qwiic Devices..."));
  bool foundMS8607 = false; // The MS8607 appears as two devices (MS8607 and MS5637). We need to skip the MS5637 if we have found a MS8607.
  for (uint8_t address = 1 ; address < 127 ; address++)
  {
    qwiic.beginTransmission(address);
    if (qwiic.endTransmission() == 0)
    {
      somethingDetected = true;
      if (settings.printDebugMessages == true)
        Serial.printf("detectQwiicDevices: something detected at address 0x%02X\r\n", address);
      deviceType_e foundType = testDevice(address, 0, 0); //No mux or port numbers for this test
      if (foundType != DEVICE_UNKNOWN_DEVICE)
      {
        if ((foundType == DEVICE_PRESSURE_MS5637) && (foundMS8607 == true))
        {
          ; // Skip MS5637 as we have already found an MS8607
        }
        else
        {
          if (addDevice(foundType, address, 0, 0) == true) //Records this device. //Returns false if mux/device was already recorded.
          {
            if (settings.printDebugMessages == true)
              Serial.printf("detectQwiicDevices: added %s at address 0x%02X\r\n", getDeviceName(foundType), address);
          }
        }
        if (foundType == DEVICE_PHT_MS8607)
        {
          foundMS8607 = true; // Flag that we have found an MS8607
        }
      }
    }
  }

  if (somethingDetected == false) return (false);

  //If we have muxes, begin scanning their sub nets
  if (muxCount > 0)
  {
    Serial.println(F("Multiplexers found. Scanning sub nets..."));

    //Step into first mux and begin stepping through ports
    for (int muxNumber = 0 ; muxNumber < muxCount ; muxNumber++)
    {
      //The node tree starts with muxes so we can align node numbers
      node *muxNode = getNodePointer(muxNumber);
      QWIICMUX *myMux = (QWIICMUX *)muxNode->classPtr;

      printDebug("detectQwiicDevices: scanning the ports of multiplexer " + (String)muxNumber);
      printDebug("\r\n");

      for (int portNumber = 0 ; portNumber < 8 ; portNumber++) //Assumes we are using a mux with 8 ports max
      {
        myMux->setPort(portNumber);
        foundMS8607 = false; // The MS8607 appears as two devices (MS8607 and MS5637). We need to skip the MS5637 if we have found a MS8607.

        printDebug("detectQwiicDevices: scanning port number " + (String)portNumber);
        printDebug(" on multiplexer " + (String)muxNumber);
        printDebug("\r\n");

        //Scan this new bus for new addresses
        for (uint8_t address = 1 ; address < 127 ; address++)
        {
          // If we found a device on the main branch, we cannot/should not attempt to scan for it on mux branches or bad things may happen
          if (deviceExists(DEVICE_TOTAL_DEVICES, address, 0, 0)) // Check if we found any type of device with this address on the main branch
          {
            if (settings.printDebugMessages == true)
              Serial.printf("detectQwiicDevices: skipping device address 0x%02X because we found one on the main branch\r\n", address);
          }
          else
          {
            qwiic.beginTransmission(address);
            if (qwiic.endTransmission() == 0)
            {
              // We don't need to do anything special for the MCP9600 here, because we can guarantee that beginTransmission + endTransmission
              // have only been used once for each MCP9600 address

              somethingDetected = true;

              deviceType_e foundType = testDevice(address, muxNode->address, portNumber);
              if (foundType != DEVICE_UNKNOWN_DEVICE)
              {
                if ((foundType == DEVICE_PRESSURE_MS5637) && (foundMS8607 == true))
                {
                  ; // Skip MS5637 as we have already found an MS8607
                }
                else
                {
                  if (foundType == DEVICE_MULTIPLEXER) // Let's ignore multiplexers hanging off multiplexer ports. (Multiple muxes on the main branch is OK.)
                  {
                    if (settings.printDebugMessages == true)
                      Serial.printf("detectQwiicDevices: ignoring %s at address 0x%02X.0x%02X.%d\r\n", getDeviceName(foundType), address, muxNode->address, portNumber);
                  }
                  else
                  {
                    if (addDevice(foundType, address, muxNode->address, portNumber) == true) //Record this device, with mux port specifics.
                    {
                      if (settings.printDebugMessages == true)
                        Serial.printf("detectQwiicDevices: added %s at address 0x%02X.0x%02X.%d\r\n", getDeviceName(foundType), address, muxNode->address, portNumber);
                    }
                  }
                }
                if (foundType == DEVICE_PHT_MS8607)
                {
                  foundMS8607 = true; // Flag that we have found an MS8607
                }
              }
            } //End I2c check
          } //End not on main branch check
        } //End I2C scanning
      } //End mux port stepping

      myMux->setPortState(0); // Disable all ports on this mux now that we have finished scanning them.

    } //End mux stepping
  } //End mux > 0

  bubbleSortDevices(head); //This may destroy mux alignment to node 0.

  //*** Let's leave pull-ups set to 1k and only disable them when taking to a u-blox device ***
  //qwiic.setPullups(0); //We've detected something on the bus so disable pullups.

  setMaxI2CSpeed(); //Try for 400kHz but reduce to 100kHz or low if certain devices are attached

  Serial.println(F("Autodetect complete"));

  return (true);
} // /detectQwiicDevices

void menuAttachedDevices()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Configure Attached Devices"));

    int availableDevices = 0;

    //Step through node list
    node *temp = head;

    if (temp == NULL)
      Serial.println(F("**No devices detected on Qwiic bus**"));

    while (temp != NULL)
    {
      //Exclude multiplexers from the list
      if (temp->deviceType != DEVICE_MULTIPLEXER)
      {
        char strAddress[50];
        if (temp->muxAddress == 0)
          sprintf(strAddress, "(0x%02X)", temp->address);
        else
          sprintf(strAddress, "(0x%02X)(Mux:0x%02X Port:%d)", temp->address, temp->muxAddress, temp->portNumber);

        char strDeviceMenu[10];
        sprintf(strDeviceMenu, "%d)", availableDevices++ + 1);

        switch (temp->deviceType)
        {
          case DEVICE_MULTIPLEXER:
            //Serial.printf("%s Multiplexer %s\r\n", strDeviceMenu, strAddress);
            break;
          case DEVICE_GPS_UBLOX:
            Serial.printf("%s u-blox GPS Receiver %s\r\n", strDeviceMenu, strAddress);
            break;
          
          case DEVICE_ADC_ADS122C04:
            Serial.printf("%s ADS122C04 ADC (Qwiic PT100) %s\r\n", strDeviceMenu, strAddress);
            break;
         
        }
      }

      temp = temp->next;
    }

    Serial.printf("%d) Configure Qwiic Settings\r\n", availableDevices++ + 1);

    Serial.println(F("x) Exit"));

    int nodeNumber = getNumber(menuTimeout); //Timeout after x seconds
    if (nodeNumber > 0 && nodeNumber < availableDevices)
    {
      //Lookup the function we need to call based the node number
      FunctionPointer functionPointer = getConfigFunctionPtr(nodeNumber - 1);

      //Get the configPtr for this given node
      void *deviceConfigPtr = getConfigPointer(nodeNumber - 1);
      functionPointer(deviceConfigPtr); //Call the appropriate config menu with a pointer to this node's configPtr

      configureDevice(nodeNumber - 1); //Reconfigure this device with the new settings
    }
    else if (nodeNumber == availableDevices)
    {
      menuConfigure_QwiicBus();
    }
    else if (nodeNumber == STATUS_PRESSED_X)
      break;
    else if (nodeNumber == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(nodeNumber);
  }
}

void menuConfigure_QwiicBus()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Configure Qwiic Bus"));

    Serial.print(F("1) Turn off bus power between readings (>2s): "));
    if (settings.powerDownQwiicBusBetweenReads == true) Serial.println(F("Yes"));
    else Serial.println(F("No"));

    Serial.printf("2) Set Max Qwiic Bus Speed: %d Hz\r\n", settings.qwiicBusMaxSpeed);

    Serial.printf("3) Set minimum Qwiic bus power up delay: %d ms\r\n", settings.qwiicBusPowerUpDelayMs);

    Serial.print(F("4) Qwiic bus pull-ups (internal to the Artemis): "));
    if (settings.qwiicBusPullUps == 1)
      Serial.println(F("1.5k"));
    else if (settings.qwiicBusPullUps == 6)
      Serial.println(F("6k"));
    else if (settings.qwiicBusPullUps == 12)
      Serial.println(F("12k"));
    else if (settings.qwiicBusPullUps == 24)
      Serial.println(F("24k"));
    else
      Serial.println(F("None"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
      settings.powerDownQwiicBusBetweenReads ^= 1;
    else if (incoming == '2')
    {
      Serial.print(F("Enter max frequency to run Qwiic bus: (100000 or 400000): "));
      uint32_t amt = getNumber(menuTimeout);
      if ((amt == 100000) || (amt == 400000))
        settings.qwiicBusMaxSpeed = amt;
      else
        Serial.println(F("Error: Out of range"));
    }
    else if (incoming == '3')
    {
      // 60 seconds is more than long enough for a ZED-F9P to do a warm start after being powered cycled, so that seems a sensible maximum
      // minimumQwiicPowerOnDelay is defined in settings.h
      Serial.printf("Enter the minimum number of milliseconds to wait for Qwiic VCC to stabilize before communication: (%d to 60000): ", minimumQwiicPowerOnDelay);
      unsigned long amt = getNumber(menuTimeout);
      if ((amt >= minimumQwiicPowerOnDelay) && (amt <= 60000))
        settings.qwiicBusPowerUpDelayMs = amt;
      else
        Serial.println(F("Error: Out of range"));
    }
    else if (incoming == '4')
    {
      Serial.print(F("Enter the Artemis pull-up resistance (0 = None; 1 = 1.5k; 6 = 6k; 12 = 12k; 24 = 24k): "));
      uint32_t pur = (uint32_t)getNumber(menuTimeout);
      if ((pur == 0) || (pur == 1) || (pur == 6) || (pur == 12) || (pur == 24))
        settings.qwiicBusPullUps = pur;
      else
        Serial.println(F("Error: Invalid resistance. Possible values are 0,1,6,12,24."));
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }
}

void menuConfigure_Multiplexer(void *configPtr)
{
  //struct_multiplexer *sensor = (struct_multiplexer*)configPtr;

  Serial.println();
  Serial.println(F("Menu: Configure Multiplexer"));

  Serial.println(F("There are currently no configurable options for this device."));
  for (int i = 0; i < 500; i++)
  {
    checkBattery();
    delay(1);
  }
}

//There is short and long range mode
//The Intermeasurement period seems to set the timing budget (PLL of the device)
//Setting the Intermeasurement period too short causes the device to freeze up
//The intermeasurement period that gets written as X gets read as X+1 so we get X and write X-1.
void menuConfigure_VL53L1X(void *configPtr)
{
  struct_VL53L1X *sensorSetting = (struct_VL53L1X*)configPtr;

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Configure VL53L1X Distance Sensor"));

    Serial.print(F("1) Sensor Logging: "));
    if (sensorSetting->log == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    if (sensorSetting->log == true)
    {
      Serial.print(F("2) Log Distance: "));
      if (sensorSetting->logDistance == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("3) Log Range Status: "));
      if (sensorSetting->logRangeStatus == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("4) Log Signal Rate: "));
      if (sensorSetting->logSignalRate == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("5) Set Distance Mode: "));
      if (sensorSetting->distanceMode == VL53L1X_DISTANCE_MODE_SHORT)
        Serial.print(F("Short"));
      else
        Serial.print(F("Long"));
      Serial.println();

      Serial.printf("6) Set Intermeasurement Period: %d ms\r\n", sensorSetting->intermeasurementPeriod);
      Serial.printf("7) Set Offset: %d mm\r\n", sensorSetting->offset);
      Serial.printf("8) Set Cross Talk (counts per second): %d cps\r\n", sensorSetting->crosstalk);
    }
    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
      sensorSetting->log ^= 1;
    else if (sensorSetting->log == true)
    {
      if (incoming == '2')
        sensorSetting->logDistance ^= 1;
      else if (incoming == '3')
        sensorSetting->logRangeStatus ^= 1;
      else if (incoming == '4')
        sensorSetting->logSignalRate ^= 1;
      else if (incoming == '5')
      {
        if (sensorSetting->distanceMode == VL53L1X_DISTANCE_MODE_SHORT)
          sensorSetting->distanceMode = VL53L1X_DISTANCE_MODE_LONG;
        else
          sensorSetting->distanceMode = VL53L1X_DISTANCE_MODE_SHORT;

        //Error check
        if (sensorSetting->distanceMode == VL53L1X_DISTANCE_MODE_LONG && sensorSetting->intermeasurementPeriod < 140)
        {
          sensorSetting->intermeasurementPeriod = 140;
          Serial.println(F("Intermeasurement Period increased to 140ms"));
        }
      }
      else if (incoming == '6')
      {
        int min = 20;
        if (sensorSetting->distanceMode == VL53L1X_DISTANCE_MODE_LONG)
          min = 140;


        Serial.printf("Set timing budget (%d to 1000ms): ", min);
        int amt = getNumber(menuTimeout); //x second timeout
        if (amt < min || amt > 1000)
          Serial.println(F("Error: Out of range"));
        else
          sensorSetting->intermeasurementPeriod = amt;
      }
      else if (incoming == '7')
      {
        Serial.print(F("Set Offset in mm (0 to 4000mm): "));
        int amt = getNumber(menuTimeout); //x second timeout
        if (amt < 0 || amt > 4000)
          Serial.println(F("Error: Out of range"));
        else
          sensorSetting->offset = amt;
      }
      else if (incoming == '8')
      {
        Serial.print(F("Set Crosstalk in Counts Per Second: "));
        int amt = getNumber(menuTimeout); //x second timeout
        if (amt < 0 || amt > 4000)
          Serial.println(F("Error: Out of range"));
        else
          sensorSetting->crosstalk = amt;
      }
      else if (incoming == 'x')
        break;
      else if (incoming == STATUS_GETBYTE_TIMEOUT)
        break;
      else
        printUnknown(incoming);
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

}

void menuConfigure_uBlox(void *configPtr)
{
  struct_uBlox *sensorSetting = (struct_uBlox*)configPtr;

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Configure uBlox GPS Receiver"));

    Serial.print(F("1) Sensor Logging: "));
    if (sensorSetting->log == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    if (sensorSetting->log == true)
    {
      Serial.print(F("2) Log GPS Date: "));
      if (sensorSetting->logDate == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("3) Log GPS Time: "));
      if (sensorSetting->logTime == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("4) Log Longitude/Latitude: "));
      if (sensorSetting->logPosition == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("5) Log Altitude: "));
      if (sensorSetting->logAltitude == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("6) Log Altitude Mean Sea Level: "));
      if (sensorSetting->logAltitudeMSL == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("7) Log Satellites In View: "));
      if (sensorSetting->logSIV == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("8) Log Fix Type: "));
      if (sensorSetting->logFixType == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("9) Log Carrier Solution: "));
      if (sensorSetting->logCarrierSolution == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("10) Log Ground Speed: "));
      if (sensorSetting->logGroundSpeed == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("11) Log Heading of Motion: "));
      if (sensorSetting->logHeadingOfMotion == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("12) Log Position Dilution of Precision (pDOP): "));
      if (sensorSetting->logpDOP == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.flush();

      Serial.print(F("13) Log Interval Time Of Week (iTOW): "));
      if (sensorSetting->logiTOW == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.printf("14) Set I2C Interface Speed (u-blox modules have pullups built in. Remove *all* I2C pullups to achieve 400kHz): %d\r\n", sensorSetting->i2cSpeed);
    }
    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after 10 seconds

    if (incoming == 1)
    {
      sensorSetting->log ^= 1;
    }
    else if (sensorSetting->log == true)
    {
      if (incoming == 2)
        sensorSetting->logDate ^= 1;
      else if (incoming == 3)
        sensorSetting->logTime ^= 1;
      else if (incoming == 4)
        sensorSetting->logPosition ^= 1;
      else if (incoming == 5)
        sensorSetting->logAltitude ^= 1;
      else if (incoming == 6)
        sensorSetting->logAltitudeMSL ^= 1;
      else if (incoming == 7)
        sensorSetting->logSIV ^= 1;
      else if (incoming == 8)
        sensorSetting->logFixType ^= 1;
      else if (incoming == 9)
        sensorSetting->logCarrierSolution ^= 1;
      else if (incoming == 10)
        sensorSetting->logGroundSpeed ^= 1;
      else if (incoming == 11)
        sensorSetting->logHeadingOfMotion ^= 1;
      else if (incoming == 12)
        sensorSetting->logpDOP ^= 1;
      else if (incoming == 13)
        sensorSetting->logiTOW ^= 1;
      else if (incoming == 14)
      {
        if (sensorSetting->i2cSpeed == 100000)
          sensorSetting->i2cSpeed = 400000;
        else
          sensorSetting->i2cSpeed = 100000;
      }
      else if (incoming == STATUS_PRESSED_X)
        break;
      else if (incoming == STATUS_GETNUMBER_TIMEOUT)
        break;
      else
        printUnknown(incoming);
    }
    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

}

bool isUbloxAttached()
{
  //Step through node list
  node *temp = head;

  while (temp != NULL)
  {
    switch (temp->deviceType)
    {
      case DEVICE_GPS_UBLOX:
        return (true);
    }
    temp = temp->next;
  }

  return (false);
}

void getUbloxDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second, int &millisecond, bool &dateValid, bool &timeValid)
{
  //Step through node list
  node *temp = head;

  while (temp != NULL)
  {
    switch (temp->deviceType)
    {
      case DEVICE_GPS_UBLOX:
        {
          qwiic.setPullups(0); //Disable pullups to minimize CRC issues

          SFE_UBLOX_GPS *nodeDevice = (SFE_UBLOX_GPS *)temp->classPtr;
          struct_uBlox *nodeSetting = (struct_uBlox *)temp->configPtr;

          //Get latested date/time from GPS
          //These will be extracted from a single PVT packet
          year = nodeDevice->getYear();
          month = nodeDevice->getMonth();
          day = nodeDevice->getDay();
          hour = nodeDevice->getHour();
          minute = nodeDevice->getMinute();
          second = nodeDevice->getSecond();
          dateValid = nodeDevice->getDateValid();
          timeValid = nodeDevice->getTimeValid();
          millisecond = nodeDevice->getMillisecond();

          qwiic.setPullups(settings.qwiicBusPullUps); //Re-enable pullups
        }
    }
    temp = temp->next;
  }
}
///////////////Hossam edits//////////////////////
void getUbloxCoord(float &Lat, float &Long, int &Alt, int &SIV, int &Fix)
{
  //Step through node list
  node *temp = head;

  while (temp != NULL)
  {
    switch (temp->deviceType)
    {
      case DEVICE_GPS_UBLOX:
        {
          qwiic.setPullups(0); //Disable pullups to minimize CRC issues

          SFE_UBLOX_GPS *nodeDevice = (SFE_UBLOX_GPS *)temp->classPtr;
          struct_uBlox *nodeSetting = (struct_uBlox *)temp->configPtr;

          //Get latested date/time from GPS
          //These will be extracted from a single PVT packet
          Lat  = nodeDevice->getLatitude();
          Long = nodeDevice->getLongitude();
          Alt  = nodeDevice->getAltitude();
          SIV  = nodeDevice->getSIV();
          Fix  = nodeDevice->getFixType();
          qwiic.setPullups(settings.qwiicBusPullUps); //Re-enable pullups
        }
    }
    temp = temp->next;
  }
}

void menuConfigure_ADS122C04(void *configPtr)
{
  struct_ADS122C04 *sensorSetting = (struct_ADS122C04*)configPtr;

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Configure ADS122C04 ADC (Qwiic PT100)"));

    Serial.print(F("1) Sensor Logging: "));
    if (sensorSetting->log == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    if (sensorSetting->log == true)
    {
      Serial.print(F("2) Log Centigrade: "));
      if (sensorSetting->logCentigrade == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("3) Log Fahrenheit: "));
      if (sensorSetting->logFahrenheit == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("4) Log Internal Temperature: "));
      if (sensorSetting->logInternalTemperature == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("5) Log Raw Voltage: "));
      if (sensorSetting->logRawVoltage == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("6) Use 4-Wire Mode: "));
      if (sensorSetting->useFourWireMode == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("7) Use 3-Wire Mode: "));
      if (sensorSetting->useThreeWireMode == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("8) Use 2-Wire Mode: "));
      if (sensorSetting->useTwoWireMode == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("9) Use 4-Wire High Temperature Mode: "));
      if (sensorSetting->useFourWireHighTemperatureMode == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("10) Use 3-Wire High Temperature Mode: "));
      if (sensorSetting->useThreeWireHighTemperatureMode == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));

      Serial.print(F("11) Use 2-Wire High Temperature Mode: "));
      if (sensorSetting->useTwoWireHighTemperatureMode == true) Serial.println(F("Enabled"));
      else Serial.println(F("Disabled"));
    }
    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
      sensorSetting->log ^= 1;
    else if (sensorSetting->log == true)
    {
      if (incoming == 2)
        sensorSetting->logCentigrade ^= 1;
      else if (incoming == 3)
        sensorSetting->logFahrenheit ^= 1;
      else if (incoming == 4)
        sensorSetting->logInternalTemperature ^= 1;
      else if (incoming == 5)
        sensorSetting->logRawVoltage ^= 1;
      else if (incoming == 6)
      {
        sensorSetting->useFourWireMode = true;
        sensorSetting->useThreeWireMode = false;
        sensorSetting->useTwoWireMode = false;
        sensorSetting->useFourWireHighTemperatureMode = false;
        sensorSetting->useThreeWireHighTemperatureMode = false;
        sensorSetting->useTwoWireHighTemperatureMode = false;
      }
      else if (incoming == 7)
      {
        sensorSetting->useFourWireMode = false;
        sensorSetting->useThreeWireMode = true;
        sensorSetting->useTwoWireMode = false;
        sensorSetting->useFourWireHighTemperatureMode = false;
        sensorSetting->useThreeWireHighTemperatureMode = false;
        sensorSetting->useTwoWireHighTemperatureMode = false;
      }
      else if (incoming == 8)
      {
        sensorSetting->useFourWireMode = false;
        sensorSetting->useThreeWireMode = false;
        sensorSetting->useTwoWireMode = true;
        sensorSetting->useFourWireHighTemperatureMode = false;
        sensorSetting->useThreeWireHighTemperatureMode = false;
        sensorSetting->useTwoWireHighTemperatureMode = false;
      }
      else if (incoming == 9)
      {
        sensorSetting->useFourWireMode = false;
        sensorSetting->useThreeWireMode = false;
        sensorSetting->useTwoWireMode = false;
        sensorSetting->useFourWireHighTemperatureMode = true;
        sensorSetting->useThreeWireHighTemperatureMode = false;
        sensorSetting->useTwoWireHighTemperatureMode = false;
      }
      else if (incoming == 10)
      {
        sensorSetting->useFourWireMode = false;
        sensorSetting->useThreeWireMode = false;
        sensorSetting->useTwoWireMode = false;
        sensorSetting->useFourWireHighTemperatureMode = false;
        sensorSetting->useThreeWireHighTemperatureMode = true;
        sensorSetting->useTwoWireHighTemperatureMode = false;
      }
      else if (incoming == 11)
      {
        sensorSetting->useFourWireMode = false;
        sensorSetting->useThreeWireMode = false;
        sensorSetting->useTwoWireMode = false;
        sensorSetting->useFourWireHighTemperatureMode = false;
        sensorSetting->useThreeWireHighTemperatureMode = false;
        sensorSetting->useTwoWireHighTemperatureMode = true;
      }
      else if (incoming == STATUS_PRESSED_X)
        break;
      else if (incoming == STATUS_GETNUMBER_TIMEOUT)
        break;
      else
        printUnknown(incoming);
    }
    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }
}
