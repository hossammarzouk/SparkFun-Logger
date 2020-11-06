#include <Wire.h> //Needed for I2C to lcd
TwoWire qwiic(1); //Needed to use Wire1 instead of Wire

#include <SerLCD.h> //Click here to get the library: http://librarymanager/All#SparkFun_SerLCD
SerLCD lcd;

int i = 0; // loop counter

void setup()

{
  Serial.begin(115200);
  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH); // power up the qwiic connector

  delay(100);             // need several delays for lcd to start, not sure why!!!
  qwiic.begin();         // start all qwiic channels
  lcd.begin(qwiic);     // Make sure gps is started with correct address
  delay(1000);         // need several delays for lcd to start, not sure why!!!


  Serial.print("LCD example");

  lcd.print("Hello World :)");



}

void loop()
{
  lcd.setCursor(0, 1); lcd.print("Test Counter:");
  lcd.setCursor(13, 1); lcd.print(i);

  i++;
  delay(1000);

}
