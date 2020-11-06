#include <SparkFun_ADS122C04_ADC_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_ADS122C0
#include <Wire.h>
TwoWire qwiic(1); //Needed to use Wire1 instead of Wire
SFE_ADS122C04 mySensor;
#define  address_ADC   0x45   // default address of ADC

void setup(void)
{
  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH); // power up the qwiic connector

  qwiic.begin();       // start all qwiic channelsdelay(100);

  Serial.begin(115200);
  while (!Serial)
    ; //Wait for user to open terminal
  Serial.println(F("Qwiic ADC Example"));


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
}
void loop()
{
  //  // Get the temperature in Centigrade
  float temperature      = mySensor.readPT100Centigrade();
  float int_temperature  = mySensor.readInternalTemperature();
  // Print the temperature
  Serial.print(F("External Sensor: "));
  if (temperature < 273) {           // to print zeros if no sensor attached
    Serial.print(temperature);}
    else{
      Serial.print("0");
  }
  Serial.print(F(" C   "));

  Serial.print(F("Internal Sensor: "));
  Serial.print(int_temperature);
  Serial.println(F(" C"));
  delay(1000); //update every 1 second


}
