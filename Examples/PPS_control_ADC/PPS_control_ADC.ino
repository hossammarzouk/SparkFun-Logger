// Logging ADC internal Temp with PPS

#include <SparkFun_ADS122C04_ADC_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_ADS122C0
#include <Wire.h>
TwoWire qwiic(1); //Needed to use Wire1 instead of Wire
SFE_ADS122C04 mySensor;

const byte ledPin = 19;
const byte interruptPin = 11;
volatile bool flag = false;
int freq = 10, leng = 10000;
int Max_G = 3;


// Intial varaiables
float Lat ; float Long ; int Alt = 0 ; int Fix = 0 ; int SIV = 0;
int year = 2000; int month  = 1; int day    = 1;
int hour = 0; int minute = 0; int second = 0; int milliseconds = 0;
bool datevalid, timevalid, dataLogging = false;

#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
#include "SerLCD.h"                        //http://librarymanager/All#SparkFun_SerLCD
#include "RTC.h" //Include RTC library included with the Aruino_Apollo3 core
APM3_RTC myRTC; //Create instance of RTC class

SFE_UBLOX_GPS myGPS;     // Initialize the library with default I2C address 0x42
SerLCD lcd;              // Initialize the library with default I2C address 0x72



#define  address_ADC   0x45   // default address of ADC


void setup()
{
  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH); // power up the qwiic connector

  qwiic.begin();       // start all qwiic channels
  delay(100);
  myGPS.begin(qwiic);   // Make sure gps is started with correct address

  Serial.begin(115200);
  Serial.println(F("Qwiic ADC PPS Example"));

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

  Serial.println("PPS control ADC example");
  Serial.println("Wating for GPS...");

  while (Fix < Max_G)
  {
    getUbloxCoord(Lat, Long, Alt, SIV, Fix);

    lcd.setCursor(4, 2);  lcd.print(SIV);
    lcd.setCursor(13, 2); lcd.print(Fix);

    if (Fix >= Max_G) {
      lcd.setCursor(18, 2); lcd.print(":)");
      lcd.setCursor(0, 3); lcd.print("....Synchronized....");
      delay(3000); lcd.clear();
      lcd.setCursor(0, 0); lcd.print("Recording...");

    }


    //mySensor.enableDebugging(); //Uncomment this line to enable debug messages on Serial
    mySensor.begin(address_ADC, qwiic);

    if (mySensor.isConnected() == false) //Connect to the PT100 using the defaults: Address 0x45 and the Wire port
    {
      Serial.println(F("ADC not detected at default I2C address. Please check wiring. Freezing."));
      while (1)
        ;
    }
    mySensor.configureADCmode(ADS122C04_RAW_MODE); // Configure the PT100 for 2-wire mode (pin 2&3)
    //mySensor.configureADCmode(ADS122C04_2WIRE_MODE); // Configure the PT100 for 2-wire mode (pin 2&3)

    pinMode(ledPin, OUTPUT);
    pinMode(interruptPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(interruptPin), record, RISING );


    UpdatePPS(freq, leng);


  }
}
void loop()
{


  if (flag == true) {
    flag = false;  // clear flag immediatly

    // Get the temperature in Centigrade
    float int_temperature  = mySensor.readInternalTemperature();
    // Print the temperature

    Serial.print(F("Internal Sensor: "));
    Serial.print(int_temperature);
    Serial.println(F(" C"));

    digitalWrite(ledPin, HIGH); delay(leng / 1000);   // Synchronize the delay with width of PPS

    digitalWrite(ledPin, LOW);
  }
}

  void record()
  {
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
