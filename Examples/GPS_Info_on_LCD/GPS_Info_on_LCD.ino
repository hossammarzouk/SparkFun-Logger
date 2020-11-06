#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
#include "SerLCD.h"                        //http://librarymanager/All#SparkFun_SerLCD
#include "RTC.h" //Include RTC library included with the Aruino_Apollo3 core
APM3_RTC myRTC; //Create instance of RTC class

#include <Wire.h>
TwoWire qwiic(1); //Needed to use Wire1 instead of Wire

SFE_UBLOX_GPS myGPS;     // Initialize the library with default I2C address 0x42
SerLCD lcd;              // Initialize the library with default I2C address 0x72


// Intial varaiables
float Lat ; float Long ; int Alt = 0 ; int Fix = 0 ; int SIV = 0;
int year = 2000; int month  = 1; int day    = 1;
int hour = 0; int minute = 0; int second = 0; int milliseconds = 0;
bool datevalid, timevalid, dataLogging = false;

void setup() {

  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH); // power up the qwiic connector

  qwiic.begin();       // start all qwiic channelsdelay(100);
  myGPS.begin(qwiic);  // Make sure gps is started with correct address

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

  while (Fix < 3)   // wait for Fix 3 and proceed
  {
    getUbloxCoord(Lat, Long, Alt, SIV, Fix);    // Get the positon from GPS

    lcd.setCursor(4, 2);  lcd.print(SIV);     // display sat and fix only at screen
    lcd.setCursor(13, 2); lcd.print(Fix);
    delay(1000);                              //update once a second not to case traffic

    if (Fix == 3) {                        // if fix is 3 clear screen and proceed

      lcd.setCursor(18, 2); lcd.print(":)");
      lcd.setCursor(0, 3); lcd.print("....Synchronized....");
      delay(5000);                      // wait to put a smile on the user face :)
      lcd.clear();
      
       Serial.begin(115200);      
    }
  }
}

void loop() {

  char gpsTime[7]; //09:14:37
  getUbloxDateTime(year, month, day, hour, minute, second, milliseconds, datevalid, timevalid); // get gps time
  sprintf(gpsTime, "%02d:%02d:%02d", hour, minute, second);
  lcd.setCursor(0, 0); lcd.print("Ready for recording.");
  lcd.setCursor(0, 1); lcd.print("Time:");
  lcd.setCursor(5, 1); lcd.print(gpsTime);

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
  //Serial.println(gpsTime);

   
  myRTC.setTime(hour, minute, second, (milliseconds / 10), day, month, (year - 2000)); //Manually set RTC

}



void getUbloxDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second, int &millisecond, bool &dateValid, bool &timeValid)
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

void getUbloxCoord(float &Lat, float &Long, int &Alt, int &SIV, int &Fix)
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
