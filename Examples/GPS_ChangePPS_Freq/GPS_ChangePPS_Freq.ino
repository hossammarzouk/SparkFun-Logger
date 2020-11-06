/*
  Send Custom Command
  By: Paul Clark (PaulZC)
  Date: April 20th, 2020

  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

  This example shows how you can create and send a custom UBX packet
  using the SparkFun u-blox library.

  Previously it was possible to create and send a custom packet
  through the library but it would always appear to timeout as
  some of the internal functions referred to the internal private
  struct packetCfg.
  The most recent version of the library allows sendCommand to
  use a custom packet as if it were packetCfg and so:
  - sendCommand will return a sfe_ublox_status_e enum as if
    it had been called from within the library
  - the custom packet will be updated with data returned by the module
    (previously this was not possible from outside the library)

  Feel like supporting open source hardware?
  Buy a board from SparkFun!
  ZED-F9P RTK2: https://www.sparkfun.com/products/15136
  NEO-M8P RTK: https://www.sparkfun.com/products/15005
  SAM-M8Q: https://www.sparkfun.com/products/15106

  Hardware Connections:
  Plug a Qwiic cable into the GPS and a BlackBoard
  If you don't have a platform with a Qwiic connection use the SparkFun Qwiic Breadboard Jumper (https://www.sparkfun.com/products/14425)
  Open the serial monitor at 115200 baud to see the output
*/

// global varaiables (datatypes obtainded from ublox proctocol page 222)
uint8_t  tpIdx, Version;
int16_t  antCableDelay, rfGroupDelay;
uint32_t freqPeriod = 5, freqPeriodLock, pulseLenRatio, pulseLenRatioLock;
int32_t  userConfigDelay;
byte     flags;
int freq = 5, width = 100000;

// Intial varaiables
float Lat ; float Long ; int Alt = 0 ; int Fix = 0 ; int SIV = 0;
int year = 2000; int month  = 1; int day    = 1;
int hour = 0; int minute = 0; int second = 0; int millisecond = 0;
bool datevalid, timevalid, dataLog;
int Max_G = 2;


const byte ledPin = 19;
const byte interruptPin = 11;
volatile byte state = LOW, event = 0, LastSample = 0;
volatile int MeasureCounter = 0;
#define NAV_RATE 20 // The new navigation rate in Hz (measurements per second)

#include <Wire.h> //Needed for I2C to GPS
TwoWire qwiic(1); //Needed to use Wire1 instead of Wire
#include "SerLCD.h"                        //http://librarymanager/All#SparkFun_SerLCD

SerLCD lcd;
#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
SFE_UBLOX_GPS myGPS;

void setup()
{

  Serial.begin(115200); // You may need to increase this for high navigation rates!
  while (!Serial)
    ; //Wait for user to open terminal
  Serial.println("SparkFun Ublox Example");
  Serial.println("Waitinng for GPS");

  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH); // power up the qwiic connector
  qwiic.begin();       // start all qwiic channelsdelay(100);
  myGPS.begin(qwiic);  // Make sure gps is started with correct address


  pinMode(ledPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);

  myGPS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGPS.setNavigationFrequency(4);


  delay(100);          // need several delays for lcd to start, not sure why!!!
  lcd.begin(qwiic);   // Make sure lcd is started with correct address
  delay(1000);

  lcd.setCursor(0, 0); lcd.print("SparkFun logger v1.0");
  lcd.setCursor(0, 1); lcd.print("Waiting for GPS...");
  lcd.setCursor(0, 2); lcd.print("SAT:");
  lcd.setCursor(4, 2); lcd.print(SIV);
  lcd.setCursor(8, 2); lcd.print("FIX:");
  lcd.setCursor(13, 2); lcd.print(Fix);
  lcd.setCursor(18, 2); lcd.print("):");
  delay(100);

  while (Fix < Max_G) { // wait for Fix 3 and proceed

    getUbloxCoord(Lat, Long, Alt, SIV, Fix);    // Get the positon from GPS

    lcd.setCursor(4, 2);  lcd.print(SIV);     // display sat and fix only at screen
    lcd.setCursor(13, 2); lcd.print(Fix);
    delay(1000);                              //update once a second not to case traffic

    if (Fix >= Max_G) {                        // if fix is 3 clear screen and proceed

      lcd.setCursor(18, 2); lcd.print(":)");
      lcd.setCursor(0, 3); lcd.print("....Synchronized....");
      delay(2000);                      // wait to put a smile on the user face :)
      lcd.clear();
delay(100);
      lcd.clear();
 
  //myGPS.setAutoPVT(true); // Enable AutoPVT. The module will generate measurements automatically without being polled.
myGPS.getPVT(300);
    }
  }
 // ReadPPS();
 UpdatePPS(freq,width);  // freq (HZ), lenght of pulse (us)
//
ReadPPS();
  
attachInterrupt(digitalPinToInterrupt(interruptPin), blink, RISING );

  //bool value = ReadPPS();
  //  if (value == true) {
  //    Serial.println(freqPeriod); Serial.println(freqPeriodLock);
  //  } else {
  //    Serial.println("Error");
  //  }
  //Serial.print(F("tpIdx is: ")); Serial.println(tpIdx);
  //Serial.print(F("antCableDelay is: ")); Serial.println(antCableDelay);
  //Serial.print(F("rfGroupDelay is: ")); Serial.println(rfGroupDelay);
  //Serial.print(F("freqPeriod is: ")); Serial.println(freqPeriod);
  //Serial.print(F("freqPeriodLock is: ")); Serial.println(freqPeriodLock);
  //Serial.print(F("pulseLenRatio is: ")); Serial.println(pulseLenRatio);
  //Serial.print(F("pulseLenRatioLock is: ")); Serial.println(pulseLenRatioLock);
  //Serial.print(F("userConfigDelay is: ")); Serial.println(userConfigDelay);
  //Serial.print(F("flags is: ")); Serial.println(flags,BIN);
//myGPS.setPVT();
}

void loop()
{
  
//
  char gpsTime[10]; //09:14:37
  getUbloxDateTime(year, month, day, hour, minute, second, millisecond, datevalid, timevalid); // get gps time
  sprintf(gpsTime, "%02d:%02d:%02d.%03d", hour, minute, second, millisecond);
  lcd.setCursor(0, 0); lcd.print("Recording.");
  lcd.setCursor(0, 1); lcd.print("Time:");
  lcd.setCursor(5, 1); lcd.print(gpsTime);
//Serial.println(myGPS.getYear());
   getUbloxCoord(Lat, Long, Alt, SIV, Fix);    // Get the positon from GPS
  
      lcd.setCursor(0, 2); lcd.print("Lat :");
      lcd.setCursor(5, 2); lcd.print(Lat / 10000000);
      lcd.setCursor(0, 3); lcd.print("Long:");
      lcd.setCursor(5, 3); lcd.print(Long / 10000000);
      lcd.setCursor(13, 2); lcd.print("SAT:");
      lcd.setCursor(17, 2); lcd.print(SIV);
      if (SIV < 10) {                           // to remove extra digit if sat no reduced
        lcd.setCursor(18, 2);
        lcd.print(" ");
      }
      lcd.setCursor(13, 3); lcd.print("FIX:");
      lcd.setCursor(17, 3); lcd.print(Fix);
      Serial.println(gpsTime);
  

  //  myRTC.setTime(hour, minute, second, (milliseconds / 10), day, month, (year - 2000)); //Manually set RTC


}

void blink() {
  state = !state;
    digitalWrite(ledPin, state);

  //LastSample = MeasureCounter;
  //Serial.println(LastSample);
  MeasureCounter++;
  //   Serial.print(F("Time: ")); Serial.print(gpsTime);
  // Serial.println(MeasureCounter);
  //  Serial.print(my);

  //Serial.print(F("Time: ")); Serial.print(gpsTime);
  Serial.print(F("   Sample: "));  Serial.println(MeasureCounter);
  // Serial.println(F(" (mm)"));
}


void getUbloxDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second, int &millisecond, bool & dateValid, bool & timeValid)
{

  qwiic.setPullups(0); //Disable pullups to minimize CRC issues


  year = myGPS.getYear();
  month = myGPS.getMonth();
  day = myGPS.getDay();
  hour = myGPS.getHour();
  minute = myGPS.getMinute();
  second = myGPS.getSecond();
  dateValid = myGPS.getDateValid();
  timeValid = myGPS.getTimeValid();
  millisecond = myGPS.getMillisecond();

  qwiic.setPullups(1); //Re-enable pullups

}

void getUbloxCoord(float & Lat, float & Long, int &Alt, int &SIV, int &Fix)
{

  qwiic.setPullups(0); //Disable pullups to minimize CRC issues

  //Get latested date/time from GPS
  //These will be extracted from a single PVT packet
  Lat  = myGPS.getLatitude();
  Long = myGPS.getLongitude();
  Alt  = myGPS.getAltitude();
  SIV  = myGPS.getSIV();
  Fix  = myGPS.getFixType();
  qwiic.setPullups(1); //Re-enable pullups

}
