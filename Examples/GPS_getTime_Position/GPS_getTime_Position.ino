
#include <Wire.h> //Needed for I2C to GPS
TwoWire qwiic(1); //Needed to use Wire1 instead of Wire

#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
SFE_UBLOX_GPS myGPS;

long lastTime = 0; //Simple local timer. Limits amount if I2C traffic to Ublox module.

void setup()
{
  Serial.begin(115200);

  Serial.println("SparkFun Ublox Example");

  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH); // power up the qwiic connector

  qwiic.begin();       // start all qwiic channels
  myGPS.begin(qwiic);  // Make sure gps is started with correct address

  myGPS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)

}

void loop()
{
  //Query module only every 2 second. Doing it more often will just cause I2C traffic.
  //The module only responds when a new position is available
  if (millis() - lastTime > 1000)
  {
    lastTime = millis(); //Update the timer

    long latitude = myGPS.getLatitude();
    Serial.print(F("Lat: "));
    Serial.print(latitude);

    long longitude = myGPS.getLongitude();
    Serial.print(F(" Long: "));
    Serial.print(longitude);
    Serial.print(F(" (degrees * 10^-7)"));

    long altitude = myGPS.getAltitude();
    Serial.print(F(" Alt: "));
    Serial.print(altitude);
    Serial.print(F(" (mm)"));

    byte SIV = myGPS.getSIV();
    Serial.print(F(" SIV: "));
    Serial.print(SIV);

    byte Fix = myGPS.getFixType();
    Serial.print(F(" Fix: "));
    Serial.print(Fix);

    byte RTK = myGPS.getCarrierSolutionType();
    Serial.print(" RTK: ");
    Serial.print(RTK);
    if (RTK == 1) Serial.print(F("High precision float fix!"));
    if (RTK == 2) Serial.print(F("High precision fix!"));
    Serial.println();
    
    Serial.print(myGPS.getYear());
    Serial.print("-");
    Serial.print(myGPS.getMonth());
    Serial.print("-");
    Serial.print(myGPS.getDay());
    Serial.print(" ");
    Serial.print(myGPS.getHour());
    Serial.print(":");
    Serial.print(myGPS.getMinute());
    Serial.print(":");
    Serial.print(myGPS.getSecond());

    Serial.print("  Time is ");
    if (myGPS.getTimeValid() == false)
    {
      Serial.print("not ");
    }
    Serial.print("valid  Date is ");
    if (myGPS.getDateValid() == false)
    {
      Serial.print("not ");
    }
    Serial.print("valid");

    Serial.println(); Serial.println();
  }
}
