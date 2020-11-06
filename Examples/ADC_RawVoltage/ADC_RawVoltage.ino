#include <SparkFun_ADS122C04_ADC_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_ADS122C0
#include <Wire.h>
TwoWire qwiic(1); //Needed to use Wire1 instead of Wire
SFE_ADS122C04 mySensor;
#define  address_ADC   0x45   // default address of ADC
int i = 0;
void setup(void)
{

  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH); // power up the qwiic connector

  qwiic.begin();       // start all qwiic channelsdelay(100);

  Serial.begin(115200);
  while (!Serial)
    ; //Wait for user to open terminal
  Serial.println(F("Qwiic PT100 Example"));

  //mySensor.enableDebugging(); //Uncomment this line to enable debug messages on Serial
  mySensor.begin(address_ADC, qwiic);

  if (mySensor.isConnected() == false) //Connect to the PT100 using the defaults: Address 0x45 and the Wire port
  {
    Serial.println(F("Qwiic PT100 not detected at default I2C address. Please check wiring. Freezing."));
    while (1)
      ;
  }

  mySensor.configureADCmode(ADS122C04_RAW_MODE); // Configure the PT100 for raw mode
  mySensor.setDataRate(ADS122C04_DATA_RATE_600SPS); // try to increase sampling freq, but failed!!
}

void loop()
{
  // Get the raw voltage as int32_t
  int32_t raw_v = mySensor.readRawVoltage();

  // Convert to Volts (method 1)
  float volts_1 = ((float)raw_v) * 244.14e-9;

  // Convert to Volts (method 2)
  float volts_2 = ((float)raw_v) / 4096000;

  // Print the temperature and voltage
  Serial.print(i);
  Serial.print(F("_The raw voltage is 0x"));
  Serial.print(raw_v, HEX);
  Serial.print(F("\t"));
  Serial.print(volts_1, 7); // Print the voltage with 7 decimal places
  Serial.print(F("V\t"));
  Serial.print(volts_2, 7); // Print the voltage with 7 decimal places
  Serial.println(F("V"));
  i++;
  //delay(1/mySensor.getDataRate()/1000); //Don't pound the I2C bus too hard
}
