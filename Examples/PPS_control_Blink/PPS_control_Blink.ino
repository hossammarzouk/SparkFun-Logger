
const byte ledPin = 19;
const byte interruptPin = 11;
volatile bool flag = false;
int MeasureCounter = 0, ind = 0 ;
int freq = 10, leng = 10000;
int Max_G = 2;

// Intial varaiables
float Lat ; float Long ; int Alt = 0 ; int Fix = 0 ; int SIV = 0;
int year = 2000; int month  = 1; int day    = 1;
int hour = 0; int minute = 0; int second = 0; int milliseconds = 0;
bool datevalid, timevalid, dataLogging = false;

#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
#include "SerLCD.h"                        //http://librarymanager/All#SparkFun_SerLCD
#include "RTC.h" //Include RTC library included with the Aruino_Apollo3 core
APM3_RTC myRTC; //Create instance of RTC class

#include <Wire.h>
TwoWire qwiic(1); //Needed to use Wire1 instead of Wire

SFE_UBLOX_GPS myGPS;     // Initialize the library with default I2C address 0x42
SerLCD lcd;              // Initialize the library with default I2C address 0x72
void setup() {

  Serial.begin(115200);

  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH); // power up the qwiic connector
  qwiic.begin();       // start all qwiic channelsdelay(100);
  myGPS.begin(qwiic);  // Make sure gps is started with correct address



  pinMode(ledPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, RISING );

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

  Serial.println("PPS control blinking example");
  Serial.println("Wating for GPS...");

  qwiic.begin();

  while (Fix < Max_G)
  {
    getUbloxCoord(Lat, Long, Alt, SIV, Fix);

    lcd.setCursor(4, 2);  lcd.print(SIV);
    lcd.setCursor(13, 2); lcd.print(Fix);

    if (Fix  >= Max_G) {
      lcd.setCursor(18, 2); lcd.print(":)");
      lcd.setCursor(0, 3); lcd.print("....Synchronized....");
      delay(3000); lcd.clear();
      lcd.setCursor(0, 0); lcd.print("Recording...");

    }
  }

  UpdatePPS(freq, leng);

}

void loop() {


  if (flag == true ) {
    flag = false;  // clear flag immediatly
    Serial.print("Measure count: ");
    Serial.println(ind);
    digitalWrite(ledPin, HIGH); delay(leng / 1000);   // Synchronize the delay with width of PPS

    ind++;
    digitalWrite(ledPin, LOW);
  }
 
}

void blink() {
  flag = true;
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
