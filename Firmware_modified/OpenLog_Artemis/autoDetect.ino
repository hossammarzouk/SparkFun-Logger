/*
  Autodetect theory of operation:

  The TCA9548A I2C mux introduces a can of worms but enables multiple (up to 64) of
  a single I2C device to be connected to a given I2C bus. You just have to turn on/off
  a given port while you communicate with said device.

  This is how the autodection algorithm works:
   Scan bus for muxes (0x70 to 0x77)
   Begin() any muxes. This causes them to turn off all their ports.
   With any possible muxes turned off, finish scanning main branch
   Any detected device is stored as a node in a linked list containing their address and device type,
   If muxes are found, begin scanning mux0/port0. Any new device is stored with their address and mux address/port.
   Begin() all devices in our linked list. Connections through muxes are performed as needed.

  All of this works and has the side benefit of enabling regular devices, that support multiple address, to
  auto-detect, begin(), and behave as before, but now in multiples.

  In the case where a device has two I2C address that are used in one library (ex: MS8607) the first address is stored in
  the node list, the 2nd address is ignored.

  Future work:

  Theoretically you could attach 8 muxes configured to 0x71 off the 8 ports of an 0x70 mux. We could
  do this for other muxes as well to create a mux monster:
   - 0x70 - (port 0) 0x71 - 8 ports - device * 8
                     0x72 - 8 ports - device * 8
                     0x73 - 8 ports - device * 8
                     0x74 - 8 ports - device * 8
                     0x76 - 8 ports - device * 8
                     0x77 - 8 ports - device * 8
  This would allow a maximum of 8 * 7 * 8 = 448 of the *same* I2C device address to be
  connected at once. We don't support this sub-muxing right now. So the max we support
  is 64 identical address devices. That should be enough.
*/

//Given node number, get a pointer to the node
node *getNodePointer(uint8_t nodeNumber)
{
  //Search the list of nodes
  node *temp = head;

  int counter = 0;
  while (temp != NULL)
  {
    if (nodeNumber == counter)
      return (temp);
    counter++;
    temp = temp->next;
  }

  return (NULL);
}

node *getNodePointer(deviceType_e deviceType, uint8_t address, uint8_t muxAddress, uint8_t portNumber)
{
  //Search the list of nodes
  node *temp = head;
  while (temp != NULL)
  {
    if (temp->address == address)
      if (temp->muxAddress == muxAddress)
        if (temp->portNumber == portNumber)
          if (temp->deviceType == deviceType)
            return (temp);

    temp = temp->next;
  }
  return (NULL);
}

//Given nodenumber, pull out the device type
deviceType_e getDeviceType(uint8_t nodeNumber)
{
  node *temp = getNodePointer(nodeNumber);
  if (temp == NULL) return (DEVICE_UNKNOWN_DEVICE);
  return (temp->deviceType);
}

//Given nodeNumber, return the config pointer
void *getConfigPointer(uint8_t nodeNumber)
{
  //Search the list of nodes
  node *temp = getNodePointer(nodeNumber);
  if (temp == NULL) return (NULL);
  return (temp->configPtr);
}

//Given a bunch of ID'ing info, return the config pointer to a node
void *getConfigPointer(deviceType_e deviceType, uint8_t address, uint8_t muxAddress, uint8_t portNumber)
{
  //Search the list of nodes
  node *temp = getNodePointer(deviceType, address, muxAddress, portNumber);
  if (temp == NULL) return (NULL);
  return (temp->configPtr);
}

//Add a device to the linked list
//Creates a class but does not begin or configure the device
bool addDevice(deviceType_e deviceType, uint8_t address, uint8_t muxAddress, uint8_t portNumber)
{
  //Ignore devices we've already logged. This was causing the mux to get tested, a begin() would happen, and the mux would be reset.
  if (deviceExists(deviceType, address, muxAddress, portNumber) == true) return false;

  //Create class instantiation for this device
  //Create logging details for this device

  node *temp = new node; //Create the node in memory

  //Setup this node
  temp->deviceType = deviceType;
  temp->address = address;
  temp->muxAddress = muxAddress;
  temp->portNumber = portNumber;

  //Instantiate a class and settings struct for this device
  switch (deviceType)
  {
    case DEVICE_MULTIPLEXER:
      {
        temp->classPtr = new QWIICMUX; //This allocates the memory needed for this class
        temp->configPtr = new struct_multiplexer;
      }
      break;

    case DEVICE_GPS_UBLOX:
      {
        temp->classPtr = new SFE_UBLOX_GPS;
        temp->configPtr = new struct_uBlox;
      }
      break;

    case DEVICE_ADC_ADS122C04:
      {
        temp->classPtr = new SFE_ADS122C04;
        temp->configPtr = new struct_ADS122C04;
      }
      break;
  }

  //Link to next node
  temp->next = NULL;
  if (head == NULL)
  {
    head = temp;
    tail = temp;
    temp = NULL;
  }
  else
  {
    tail->next = temp;
    tail = temp;
  }

  return true;
}

//Begin()'s all devices in the node list
bool beginQwiicDevices()
{
  bool everythingStarted = true;

  waitForQwiicBusPowerDelay(); // Wait while the qwiic devices power up - if required

  qwiicPowerOnDelayMillis = settings.qwiicBusPowerUpDelayMs; // Set qwiicPowerOnDelayMillis to the _minimum_ defined by settings.qwiicBusPowerUpDelayMs. It will be increased if required.

  //Step through the list
  node *temp = head;

  if (temp == NULL)
  {
    printDebug(F("beginQwiicDevices: No devices detected\r\n"));
    return (true);
  }

  while (temp != NULL)
  {
    openConnection(temp->muxAddress, temp->portNumber); //Connect to this device through muxes as needed

    if (settings.printDebugMessages == true)
    {
      Serial.printf("beginQwiicDevices: attempting to begin deviceType %s", getDeviceName(temp->deviceType));
      Serial.printf(" at address 0x%02X using mux address 0x%02X and port number %d\r\n", temp->address, temp->muxAddress, temp->portNumber);
    }

    //Attempt to begin the device
    switch (temp->deviceType)
    {
      case DEVICE_MULTIPLEXER:
        {
          QWIICMUX *tempDevice = (QWIICMUX *)temp->classPtr;
          struct_multiplexer *nodeSetting = (struct_multiplexer *)temp->configPtr; //Create a local pointer that points to same spot as node does
          if (nodeSetting->powerOnDelayMillis > qwiicPowerOnDelayMillis) qwiicPowerOnDelayMillis = nodeSetting->powerOnDelayMillis; // Increase qwiicPowerOnDelayMillis if required
          temp->online = tempDevice->begin(temp->address, qwiic); //Address, Wire port
        }
        break;

      case DEVICE_GPS_UBLOX:
        {
          qwiic.setPullups(0); //Disable pullups for u-blox comms.
          SFE_UBLOX_GPS *tempDevice = (SFE_UBLOX_GPS *)temp->classPtr;
          struct_uBlox *nodeSetting = (struct_uBlox *)temp->configPtr; //Create a local pointer that points to same spot as node does
          if (nodeSetting->powerOnDelayMillis > qwiicPowerOnDelayMillis) qwiicPowerOnDelayMillis = nodeSetting->powerOnDelayMillis; // Increase qwiicPowerOnDelayMillis if required
          temp->online = tempDevice->begin(qwiic, temp->address); //Wire port, Address
          qwiic.setPullups(settings.qwiicBusPullUps); //Re-enable pullups.
        }
        break;

      case DEVICE_ADC_ADS122C04:
        {
          SFE_ADS122C04 *tempDevice = (SFE_ADS122C04 *)temp->classPtr;
          struct_ADS122C04 *nodeSetting = (struct_ADS122C04 *)temp->configPtr; //Create a local pointer that points to same spot as node does
          if (nodeSetting->powerOnDelayMillis > qwiicPowerOnDelayMillis) qwiicPowerOnDelayMillis = nodeSetting->powerOnDelayMillis; // Increase qwiicPowerOnDelayMillis if required
          if (tempDevice->begin(temp->address, qwiic) == true) //Address, Wire port. Returns true on success.
            temp->online = true;
        }
        break;

      default:
        Serial.printf("beginQwiicDevices: device type not found: %d\r\n", temp->deviceType);
        break;
    }

    if (temp->online == true)
    {
      printDebug("beginQwiicDevices: device is online\r\n");
    }
    else
    {
      printDebug("beginQwiicDevices: device is **NOT** online\r\n");
      everythingStarted = false;
    }

    temp = temp->next;
  }

  return everythingStarted;
}

//Pretty print all the online devices
void printOnlineDevice()
{
  int deviceCount = 0;

  //Step through the list
  node *temp = head;

  if (temp == NULL)
  {
    printDebug(F("printOnlineDevice: No devices detected\r\n"));
    return;
  }

  while (temp != NULL)
  {
    char sensorOnlineText[75];
    if (temp->online)
    {
      if (temp->muxAddress == 0)
        sprintf(sensorOnlineText, "%s online at address 0x%02X\r\n", getDeviceName(temp->deviceType), temp->address);
      else
        sprintf(sensorOnlineText, "%s online at address 0x%02X.0x%02X.%d\r\n", getDeviceName(temp->deviceType), temp->address, temp->muxAddress, temp->portNumber);

      deviceCount++;
    }
    else
    {
      sprintf(sensorOnlineText, "%s failed to respond\r\n", getDeviceName(temp->deviceType));
    }
    Serial.print(sensorOnlineText);

    temp = temp->next;
  }

  if (settings.printDebugMessages == true)
    Serial.printf("Device count: %d\r\n", deviceCount);
}

//Given the node number, apply the node's configuration settings to the device
void configureDevice(uint8_t nodeNumber)
{
  node *temp = getNodePointer(nodeNumber);
  configureDevice(temp);
}

//Given the node pointer, apply the node's configuration settings to the device
void configureDevice(node * temp)
{
  uint8_t deviceType = (uint8_t)temp->deviceType;

  openConnection(temp->muxAddress, temp->portNumber); //Connect to this device through muxes as needed

  switch (deviceType)
  {
    case DEVICE_MULTIPLEXER:
      //Nothing to configure
      break;
    case DEVICE_GPS_UBLOX:
      {
        qwiic.setPullups(0); //Disable pullups for u-blox comms.

        SFE_UBLOX_GPS *sensor = (SFE_UBLOX_GPS *)temp->classPtr;
        struct_uBlox *nodeSetting = (struct_uBlox *)temp->configPtr;

        sensor->setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)

        sensor->saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Save (only) the current ioPortsettings to flash and BBR
        //  UpdatePPS2(5,10000);
        //sensor->setAutoPVT(true); //Tell the GPS to "send" each solution
        sensor->setAutoPVT(false); //We will poll the device for PVT solutions
        if (1000000ULL / settings.usBetweenReadings <= 1) //If we are slower than 1Hz logging rate
          // setNavigationFrequency expects a uint8_t to define the number of updates per second
          // So the slowest rate we can set with setNavigationFrequency is 1Hz
          // (Whereas UBX_CFG_RATE can actually support intervals as slow as 65535ms)
          sensor->setNavigationFrequency(1); //Set output rate to 1Hz
        else if (1000000ULL / settings.usBetweenReadings <= 10) //If we are slower than 10Hz logging rate
          sensor->setNavigationFrequency((uint8_t)(1000000ULL / settings.usBetweenReadings)); //Set output rate equal to our query rate
        else
          sensor->setNavigationFrequency(10); //Set nav freq to 10Hz. Max output depends on the module used.

        qwiic.setPullups(settings.qwiicBusPullUps); //Re-enable pullups.

        ///...........Hossam edits.............
        int rate = 10, ratio = 10000;
sensor->setNavigationFrequency(10);
        uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes

        // The next line creates and initialises the packet information which wraps around the payload
        ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

        customCfg.cls           = UBX_CLASS_CFG; // This is the message Class
        customCfg.id           = UBX_CFG_TP5;    // This is the message ID
        customCfg.len          = 0;              // Setting the len (length) to zero let's us poll the current settings
        customCfg.startingSpot = 0;              // Always set the startingSpot to zero (unless you really know what you are doing)
        uint16_t maxWait       = 1100;          // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)
        if ( sensor->sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
        {
          Serial.print("Error updating pps"); while (1);
        }

        byte Flags = customPayload[28];
        bitWrite(Flags, 3, 1); // change frequency unit from period to frequency
        //bitWrite(Flags, 4, 0); // change duration unit from lenghth to ratio
        customPayload[28] = Flags;
        //update freqPeriodLock
        customPayload[12] = rate ; // Store it in the payload
        customPayload[13] = rate >>  8;
        customPayload[14] = rate >> 8 * 2;
        customPayload[15] = rate >>  8 * 3;

        //update freqPeriod (we use only freq lock but update this just in case)
        customPayload[8]  = rate ; // Store it in the payload
        customPayload[9]  = rate >>  8;
        customPayload[10] = rate >> 8 * 2;
        customPayload[11] = rate >>  8 * 3;

        //  //update pulseLensRationLock
      
        customPayload[20] = ratio & 0xFF; // Store it in the payload
        customPayload[21] = ratio >>  8;
        customPayload[22] = ratio >> 8 * 2;
        customPayload[23] = ratio >>  8 * 3;
        //
        //............. Done  setting pulse ratio.............
//        if ( sensor->sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_SENT) // We are expecting data and an ACK
//        {
//
//          Serial.print("Error updating pps");
//        }
//
sensor->sendCommand(&customCfg, maxWait);
   customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes

  // The next line creates and initialises the packet information which wraps around the payload
//  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};


  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_TP5; // This is the message ID
  customCfg.len = 0; // Setting the len (length) to zero let's us poll the current settings
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  // We also need to tell sendCommand how long it should wait for a reply
//  uint16_t maxWait = 1100; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

   if ( sensor->sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("Error reading pps ")); }

  
  tpIdx                =  customPayload[0];  
  Version              =  customPayload[1];  
  antCableDelay        = (customPayload[5]  << 8 ) | (customPayload[4]); 
  rfGroupDelay         = (customPayload[7]  << 8)  | (customPayload[6]); 
  freqPeriod           = (customPayload[11] << 8 * 3) | (customPayload[10] << 8 * 2) | (customPayload[9]  << 8) | customPayload[8];
  freqPeriodLock       = (customPayload[15] << 8 * 3) | (customPayload[14] << 8 * 2) | (customPayload[13] << 8) | customPayload[12]; 
  pulseLenRatio        = (customPayload[19] << 8 * 3) | (customPayload[18] << 8 * 2) | (customPayload[17] << 8) | customPayload[16];
  pulseLenRatioLock    = (customPayload[23] << 8 * 3) | (customPayload[22] << 8 * 2) | (customPayload[21] << 8) | customPayload[20]; 
  userConfigDelay      = (customPayload[27] << 8 * 3) | (customPayload[26] << 8 * 2) | (customPayload[25] << 8) | customPayload[24]; 
  flags                =  customPayload[28];
  
//////Printing test
Serial.print(F("tpIdx is: ")); Serial.println(tpIdx);
Serial.print(F("Version is: ")); Serial.println(Version);

Serial.print(F("antCableDelay is: ")); Serial.println(antCableDelay);
Serial.print(F("rfGroupDelay is: ")); Serial.println(rfGroupDelay);
Serial.print(F("freqPeriod is: ")); Serial.println(freqPeriod);
Serial.print(F("freqPeriodLock is: ")); Serial.println(freqPeriodLock);
Serial.print(F("pulseLenRatio is: ")); Serial.println(pulseLenRatio);
Serial.print(F("pulseLenRatioLock is: ")); Serial.println(pulseLenRatioLock);
Serial.print(F("userConfigDelay is: ")); Serial.println(userConfigDelay);
Serial.print(F("flags is: ")); Serial.println(flags,BIN);








      }
      break;
    case DEVICE_ADC_ADS122C04:
      {
        SFE_ADS122C04 *sensor = (SFE_ADS122C04 *)temp->classPtr;
        struct_ADS122C04 *sensorSetting = (struct_ADS122C04 *)temp->configPtr;

        //Configure the wite mode for readPT100Centigrade and readPT100Fahrenheit
        //(readInternalTemperature and readRawVoltage change and restore the mode automatically)
        if (sensorSetting->useFourWireMode)
          sensor->configureADCmode(ADS122C04_4WIRE_MODE);
        else if (sensorSetting->useThreeWireMode)
          sensor->configureADCmode(ADS122C04_3WIRE_MODE);
        else if (sensorSetting->useTwoWireMode)
          sensor->configureADCmode(ADS122C04_2WIRE_MODE);
        else if (sensorSetting->useFourWireHighTemperatureMode)
          sensor->configureADCmode(ADS122C04_4WIRE_HI_TEMP);
        else if (sensorSetting->useThreeWireHighTemperatureMode)
          sensor->configureADCmode(ADS122C04_3WIRE_HI_TEMP);
        else if (sensorSetting->useTwoWireHighTemperatureMode)
          sensor->configureADCmode(ADS122C04_2WIRE_HI_TEMP);
      }
      break;
    default:
      Serial.printf("configureDevice: Unknown device type %d: %s\r\n", deviceType, getDeviceName((deviceType_e)deviceType));
      break;
  }
}

//Apply device settings to each node
void configureQwiicDevices()
{
  //Step through the list
  node *temp = head;

  while (temp != NULL)
  {
    configureDevice(temp);
    temp = temp->next;
  }
}

//Returns a pointer to the menu function that configures this particular device type
FunctionPointer getConfigFunctionPtr(uint8_t nodeNumber)
{
  FunctionPointer ptr = NULL;

  node *temp = getNodePointer(nodeNumber);
  if (temp == NULL) return (NULL);
  deviceType_e deviceType = temp->deviceType;

  switch (deviceType)
  {
    case DEVICE_MULTIPLEXER:
      ptr = (FunctionPointer)menuConfigure_Multiplexer;
      break;
    case DEVICE_GPS_UBLOX:
      ptr = (FunctionPointer)menuConfigure_uBlox;
      break;
    case DEVICE_ADC_ADS122C04:
      ptr = (FunctionPointer)menuConfigure_ADS122C04;
      break;

    default:
      Serial.println(F("getConfigFunctionPtr: Unknown device type"));
      Serial.flush();
      break;
  }

  return (ptr);
}

//Search the linked list for a given address
//Returns true if this device address already exists in our system
bool deviceExists(deviceType_e deviceType, uint8_t address, uint8_t muxAddress, uint8_t portNumber)
{
  node *temp = head;
  while (temp != NULL)
  {
    if (temp->address == address)
      if (temp->muxAddress == muxAddress)
        if (temp->portNumber == portNumber)
          if (temp->deviceType == deviceType) return (true);

    //Devices that were discovered on the main branch will be discovered over and over again
    //If a device has a 0/0 mux/port address, it's on the main branch and exists on all
    //sub branches.
    if (temp->address == address)
      if (temp->muxAddress == 0)
        if (temp->portNumber == 0)
        {
          if (temp->deviceType == deviceType) return (true);
          // Use DEVICE_TOTAL_DEVICES as a special case.
          // Return true if the device address exists on the main branch so we can avoid looking for it on mux branches.
          if (deviceType == DEVICE_TOTAL_DEVICES) return (true);
        }

    temp = temp->next;
  }
  return (false);
}

//Given the address of a device, enable muxes appropriately to open connection access device
//Return true if connection was opened
bool openConnection(uint8_t muxAddress, uint8_t portNumber)
{
  if (head == NULL)
  {
    Serial.println(F("OpenConnection Error: No devices in list"));
    return false;
  }

  if (muxAddress == 0) //This device is on main branch, nothing needed
    return true;

  //Get the pointer to the node that contains this mux address
  node *muxNode = getNodePointer(DEVICE_MULTIPLEXER, muxAddress, 0, 0);
  QWIICMUX *myMux = (QWIICMUX *)muxNode->classPtr;

  //Connect to this mux and port
  myMux->setPort(portNumber);

  return (true);
}

//Bubble sort the given linked list by the device address
//https://www.geeksforgeeks.org/c-program-bubble-sort-linked-list/
void bubbleSortDevices(struct node * start)
{
  int swapped, i;
  struct node *ptr1;
  struct node *lptr = NULL;

  //Checking for empty list
  if (start == NULL) return;

  do
  {
    swapped = 0;
    ptr1 = start;

    while (ptr1->next != lptr)
    {
      if (ptr1->address > ptr1->next->address)
      {
        swap(ptr1, ptr1->next);
        swapped = 1;
      }
      ptr1 = ptr1->next;
    }
    lptr = ptr1;
  }
  while (swapped);
}

//Swap data of two nodes a and b
void swap(struct node * a, struct node * b)
{
  node temp;

  temp.deviceType = a->deviceType;
  temp.address = a->address;
  temp.portNumber = a->portNumber;
  temp.muxAddress = a->muxAddress;
  temp.online = a->online;
  temp.classPtr = a->classPtr;
  temp.configPtr = a->configPtr;

  a->deviceType = b->deviceType;
  a->address = b->address;
  a->portNumber = b->portNumber;
  a->muxAddress = b->muxAddress;
  a->online = b->online;
  a->classPtr = b->classPtr;
  a->configPtr = b->configPtr;

  b->deviceType = temp.deviceType;
  b->address = temp.address;
  b->portNumber = temp.portNumber;
  b->muxAddress = temp.muxAddress;
  b->online = temp.online;
  b->classPtr = temp.classPtr;
  b->configPtr = temp.configPtr;
}

//The functions below are specific to the steps of auto-detection rather than node manipulation
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Available Qwiic devices
//We no longer use defines in the search table. These are just here for reference.
#define ADR_VEML6075 0x10
#define ADR_MPR0025PA1 0x18
#define ADR_NAU7802 0x2A
#define ADR_VL53L1X 0x29
#define ADR_SNGCJA5 0x33
#define ADR_AHT20 0x38
#define ADR_MS8607 0x40 //Humidity portion of the MS8607 sensor
#define ADR_UBLOX 0x42 //But can be set to any address
#define ADR_ADS122C04 0x45 //Alternates: 0x44, 0x41 and 0x40
#define ADR_TMP117 0x48 //Alternates: 0x49, 0x4A, and 0x4B
#define ADR_SGP30 0x58
#define ADR_CCS811 0x5B //Alternates: 0x5A
#define ADR_LPS25HB 0x5D //Alternates: 0x5C
#define ADR_VCNL4040 0x60
#define ADR_SCD30 0x61
#define ADR_MCP9600 0x60 //0x60 to 0x67
#define ADR_MULTIPLEXER 0x70 //0x70 to 0x77
#define ADR_SHTC3 0x70
#define ADR_MS5637 0x76
//#define ADR_MS8607 0x76 //Pressure portion of the MS8607 sensor. We'll catch the 0x40 first
#define ADR_BME280 0x77 //Alternates: 0x76

//Given an address, returns the device type if it responds as we would expect
//Does not test for multiplexers. See testMuxDevice for dedicated mux testing.
deviceType_e testDevice(uint8_t i2cAddress, uint8_t muxAddress, uint8_t portNumber)
{
  switch (i2cAddress)
  {


    case 0x40:
      {

        //Confidence: High - Configures ADC mode
        SFE_ADS122C04 sensor1;
        if (sensor1.begin(i2cAddress, qwiic) == true) //Address, Wire port
          return (DEVICE_ADC_ADS122C04);
      }
      break;
    case 0x41:
      {
        //Confidence: High - Configures ADC mode
        SFE_ADS122C04 sensor;
        if (sensor.begin(i2cAddress, qwiic) == true) //Address, Wire port
          return (DEVICE_ADC_ADS122C04);
      }
      break;
    case 0x42:
      {
        //Confidence: High - Sends/receives CRC checked data response
        qwiic.setPullups(0); //Disable pullups to minimize CRC issues
        SFE_UBLOX_GPS sensor;
        if (settings.printDebugMessages == true) sensor.enableDebugging(); // Enable debug messages if required
        if (sensor.begin(qwiic, i2cAddress) == true) //Wire port, address
        {
          qwiic.setPullups(settings.qwiicBusPullUps); //Re-enable pullups to prevent ghosts at 0x43 onwards
          return (DEVICE_GPS_UBLOX);
        }
        qwiic.setPullups(settings.qwiicBusPullUps); //Re-enable pullups for normal discovery
      }
      break;
    case 0x44:
    case 0x45:
      {
        //Confidence: High - Configures ADC mode
        SFE_ADS122C04 sensor;
        if (sensor.begin(i2cAddress, qwiic) == true) //Address, Wire port
          return (DEVICE_ADC_ADS122C04);
      }
      break;

    case 0x71:
      {
        //Ignore devices we've already recorded. This was causing the mux to get tested, a begin() would happen, and the mux would be reset.
        if (deviceExists(DEVICE_MULTIPLEXER, i2cAddress, muxAddress, portNumber) == true) return (DEVICE_MULTIPLEXER);
      }
      break;
    case 0x72:
      {
        //Ignore devices we've already recorded. This was causing the mux to get tested, a begin() would happen, and the mux would be reset.
        if (deviceExists(DEVICE_MULTIPLEXER, i2cAddress, muxAddress, portNumber) == true) return (DEVICE_MULTIPLEXER);
      }
      break;
    case 0x73:
      {
        //Ignore devices we've already recorded. This was causing the mux to get tested, a begin() would happen, and the mux would be reset.
        if (deviceExists(DEVICE_MULTIPLEXER, i2cAddress, muxAddress, portNumber) == true) return (DEVICE_MULTIPLEXER);
      }
      break;
    case 0x74:
      {
        //Ignore devices we've already recorded. This was causing the mux to get tested, a begin() would happen, and the mux would be reset.
        if (deviceExists(DEVICE_MULTIPLEXER, i2cAddress, muxAddress, portNumber) == true) return (DEVICE_MULTIPLEXER);
      }
      break;
    case 0x75:
      {
        //Ignore devices we've already recorded. This was causing the mux to get tested, a begin() would happen, and the mux would be reset.
        if (deviceExists(DEVICE_MULTIPLEXER, i2cAddress, muxAddress, portNumber) == true) return (DEVICE_MULTIPLEXER);
      }
      break;

    default:
      {
        if (muxAddress == 0)
          Serial.printf("Unknown device at address (0x%02X)\r\n", i2cAddress);
        else
          Serial.printf("Unknown device at address (0x%02X)(Mux:0x%02X Port:%d)\r\n", i2cAddress, muxAddress, portNumber);
        return DEVICE_UNKNOWN_DEVICE;
      }
      break;
  }
  Serial.printf("Known I2C address but device failed identification at address 0x%02X\r\n", i2cAddress);
  return DEVICE_UNKNOWN_DEVICE;
}

//Given an address, returns the device type if it responds as we would expect
//This version is dedicated to testing muxes and uses a custom .begin to avoid the slippery mux problem
deviceType_e testMuxDevice(uint8_t i2cAddress, uint8_t muxAddress, uint8_t portNumber)
{
  switch (i2cAddress)
  {
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
      {
        //Ignore devices we've already recorded. This was causing the mux to get tested, a begin() would happen, and the mux would be reset.
        if (deviceExists(DEVICE_MULTIPLEXER, i2cAddress, muxAddress, portNumber) == true) return (DEVICE_MULTIPLEXER);

        //Confidence: Medium - Write/Read/Clear to 0x00
        if (multiplexerBegin(i2cAddress, qwiic) == true) //Address, Wire port
          return (DEVICE_MULTIPLEXER);
      }
      break;
    default:
      {
        if (muxAddress == 0)
          Serial.printf("Unknown device at address (0x%02X)\r\n", i2cAddress);
        else
          Serial.printf("Unknown device at address (0x%02X)(Mux:0x%02X Port:%d)\r\n", i2cAddress, muxAddress, portNumber);
        return DEVICE_UNKNOWN_DEVICE;
      }
      break;
  }
  return DEVICE_UNKNOWN_DEVICE;
}

//Returns true if mux is present
//Tests for device ack to I2C address
//Then tests if device behaves as we expect
//Leaves with all ports disabled
bool multiplexerBegin(uint8_t deviceAddress, TwoWire &wirePort)
{
  wirePort.beginTransmission(deviceAddress);
  if (wirePort.endTransmission() != 0)
    return (false); //Device did not ACK

  //Write to device, expect a return
  setMuxPortState(0xA4, deviceAddress, wirePort, EXTRA_MUX_STARTUP_BYTES); //Set port register to a known value - using extra bytes to avoid the mux problem
  uint8_t response = getMuxPortState(deviceAddress, wirePort);
  setMuxPortState(0x00, deviceAddress, wirePort, 0); //Disable all ports - seems to work just fine without extra bytes (not sure why...)
  if (response == 0xA4) //Make sure we got back what we expected
  {
    response = getMuxPortState(deviceAddress, wirePort); //Make doubly sure we got what we expected
    if (response == 0x00)
    {
      return (true); //All good
    }
  }
  return (false);
}

//Writes a 8-bit value to mux
//Overwrites any other bits
//This allows us to enable/disable multiple ports at same time
bool setMuxPortState(uint8_t portBits, uint8_t deviceAddress, TwoWire &wirePort, int extraBytes)
{
  wirePort.beginTransmission(deviceAddress);
  for (int i = 0; i < extraBytes; i++)
  {
    wirePort.write(0x00); // Writing these extra bytes seems key to avoiding the slippery mux problem
  }
  wirePort.write(portBits);
  if (wirePort.endTransmission() != 0)
    return (false); //Device did not ACK
  return (true);
}

//Gets the current port state
//Returns byte that may have multiple bits set
uint8_t getMuxPortState(uint8_t deviceAddress, TwoWire &wirePort)
{
  //Read the current mux settings
  wirePort.beginTransmission(deviceAddress);
  wirePort.requestFrom(deviceAddress, 1);
  return (wirePort.read());
}

//Given a device number return the string associated with that entry
const char* getDeviceName(deviceType_e deviceNumber)
{
  switch (deviceNumber)
  {
    case DEVICE_MULTIPLEXER:
      return "Multiplexer";
      break;

    case DEVICE_GPS_UBLOX:
      return "GPS-ublox";
      break;

    case DEVICE_ADC_ADS122C04:
      return "ADC-ADS122C04";
      break;

  }
  return "None";
}
