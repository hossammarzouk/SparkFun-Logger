// Update PPS configrations

// The structure of ubxPacket is:
// uint8_t cls           : The message Class
// uint8_t id            : The message ID
// uint16_t len          : Length of the payload. Does not include cls, id, or checksum bytes
// uint16_t counter      : Keeps track of number of overall bytes received. Some responses are larger than 255 bytes.
// uint16_t startingSpot : The counter value needed to go past before we begin recording into payload array
// uint8_t *payload      : The payload
// uint8_t checksumA     : Given to us by the module. Checked against the rolling calculated A/B checksums.
// uint8_t checksumB
// sfe_ublox_packet_validity_e valid            : Goes from NOT_DEFINED to VALID or NOT_VALID when checksum is checked
// sfe_ublox_packet_validity_e classAndIDmatch  : Goes from NOT_DEFINED to VALID or NOT_VALID when the Class and ID match the requestedClass and requestedID

// sendCommand will return:
// SFE_UBLOX_STATUS_DATA_RECEIVED if the data we requested was read / polled successfully
// SFE_UBLOX_STATUS_DATA_SENT     if the data we sent was writted successfully (ACK'd)
// Other values indicate errors. Please see the sfe_ublox_status_e enum for further details.

// Referring to the u-blox M8 Receiver Description and Protocol Specification we see that
// the navigation rate is configured using the UBX-CFG-RATE message. So let's load our
// custom packet with the correct information so we can read (poll / get) the current settings.

bool UpdatePPS(int rate,int ratio) {

  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes

  // The next line creates and initialises the packet information which wraps around the payload
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls           = UBX_CLASS_CFG; // This is the message Class
  customCfg.id           = UBX_CFG_TP5;    // This is the message ID
  customCfg.len          = 0;              // Setting the len (length) to zero let's us poll the current settings
  customCfg.startingSpot = 0;              // Always set the startingSpot to zero (unless you really know what you are doing)
  uint16_t maxWait       = 1100;          // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)
if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
   return(false);
   Serial.print("Error updating pps");while(1);
        }
        
  // We also need to tell sendCommand how long it should wait for a reply
 
 //............. start setting flages.............
    ////the flags are set in byte 28 (see ublox protocal page 224)
    
  byte Flags = customPayload[28];
  bitWrite(Flags, 3, 1); // change frequency unit from period to frequency
  //bitWrite(Flags, 4, 0); // change duration unit from lenghth to ratio
  customPayload[28] = Flags;
  //............. Done setting flags.............

  //............. start setting frequency.............
  
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
  
  //............. Done setting frequency.............


   //............. start setting pulse ratio.............
  
//  //update pulseLensRationLock
//  int ratio = 1000000; // set default ratio to 1000 50%

  customPayload[20] = ratio & 0xFF; // Store it in the payload
  customPayload[21] = ratio >>  8;
  customPayload[22] = ratio >> 8 * 2;
  customPayload[23] = ratio >>  8 * 3;
//  
//  //update pulseLensRation (we use only pulse ratio lock but update this just in case)
//  customPayload[16]  = ratio & 0xFF; // Store it in the payload
//  customPayload[17]  = ratio >>  8;
//  customPayload[18]  = ratio >> 8 * 2;
//  customPayload[19]  = ratio >>  8 * 3;
  
  //............. Done  setting pulse ratio.............
  if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_SENT) // We are expecting data and an ACK
  {
   return(false);
   Serial.print("Error updating pps");while(1);
        }
 // return (true);
  
 return (true);
 
 }
 
 
 
bool ReadPPS() {
  uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes

  // The next line creates and initialises the packet information which wraps around the payload
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};


  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_TP5; // This is the message ID
  customCfg.len = 0; // Setting the len (length) to zero let's us poll the current settings
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  // We also need to tell sendCommand how long it should wait for a reply
  uint16_t maxWait = 1100; // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)

   if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.println(F("Error reading pps ")); return (false);}

  
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

  return(true);

}
