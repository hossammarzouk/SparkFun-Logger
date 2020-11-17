// ----------------------Recording setting----------------------------------------------------------
const int Freq      = 128;               // sampling frequency(and pps freq)
const int PPS_width = 1000;             // pps duty cycle in micro seconds
const int gain      = 16;                // Gain for ADC
//--------------------------------------------------------------------------------------------------

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <Wire.h>
TwoWire qwiic(1); //Will use pads 8/9
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//microSD Interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <SPI.h>
#include <SdFat.h> //SdFat (FAT32) by Bill Greiman: http://librarymanager/All#SdFat
//SdFat SD;
//SdFile  myFile;
//SdFile  myHeaderFile;
char myFileName[30]       = "ede_S999_D000000_FN001.ets"; //We keep a record of this file name so that we can re-open it upon wakeup from sleep
char myHeaderFileName[30] = "ede_S999_D000000.txt"; //We keep a record of this file name so that we can re-open it upon wakeup from sleep

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Add RTC interface for Artemis
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "RTC.h" //Include RTC library included with the Aruino_Apollo3 core
APM3_RTC myRTC; //Create instance of RTC class
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Add GPS board
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "SparkFun_Ublox_Arduino_Library.h" //http://librarymanager/All#SparkFun_Ublox_GPS
SFE_UBLOX_GPS myGPS;      // Initialize the library with default I2C address 0x42
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Add ADC libraray
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <SparkFun_ADS122C04_ADC_Arduino_Library.h> // Click here to get the library: http://librarymanager/All#SparkFun_ADS122C0
SFE_ADS122C04 myADC;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Add LCD libraray
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "SerLCD.h"                        //http://librarymanager/All#SparkFun_SerLCD
SerLCD lcd;               // Initialize the library with default I2C address 0x72
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Pin defintions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
const byte PIN_MICROSD_POWER = 15;
const byte PIN_MICROSD_CHIP_SELECT = 23;
const byte PIN_PWR_LED = 29;
const byte PIN_STAT_LED = 19;
const byte PIN_QWIIC_POWER = 18;
const byte PIN_TRIGGER = 11;
const byte PIN_STOP_LOGGING = 32;
const byte BREAKOUT_PIN_TX = 12;
const byte BREAKOUT_PIN_RX = 13;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Address defintions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#define  address_GPS   0x42   // default address of GPS
#define  address_ADC   0x45   // default address of ADC
#define  address_LCD   0x72   // default address of LCD
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
int Max_G = 3;
bool Error = true;
volatile static bool triggerEdgeSeen = false; //Flag to indicate if a trigger interrupt has been seen
volatile static bool stopLoggingSeen = false; //Flag to indicate if we should stop logging

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// add customized GPS class
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "GPS.h"
GPS GPS;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// add customized ADC class
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "ADC.h"
adc adc;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// add customized configuration class
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "Config.h"
Config Config;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


// Test variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
int32_t SampleCounter = 0;
const int BufferSize  = 512 ; //(equal to 512 (64*2*4), no of samples*channels*no of bytes per each channel
int BufferCounter     = 0;
char outputData[512 * 2]; //Factor of 512 for easier recording to SD in 512 chunks
String buffer;
bool isready = 0, sready = 0;
int u = 0;

struct datastore {
  int32_t counter;
  int32_t drdy;
  int32_t adc;

};
datastore myData[BufferSize];
unsigned long lastDataLogSyncTime = 0; //Used to record to SD every half second

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup() {

  Serial.begin(115200);
  Serial.println("Wait for GPS..");

  Config.beginQwiic();
  Config.CheckAll();
  GPS.WaitMaxG(Max_G);
  GPS.UpdateRTC();
  GPS.setNavFreq(10);   // while 18 Hz is the max, 10 Hz is mores stable!!
  GPS.setPPSfreq(Freq, PPS_width); delay(2000); // i think we should wait for PPS stablize
  Config.StartInterput();
  adc.Start();  //Start ADC config
  adc.setGain(gain);  //gain
  Config.MakeMeasDir();
  Config.MakeMeasFile();


  Config.MakeHeaderFile();
  GPS.WaitFullMinute();






  //    if (!myFile.createContiguous(myFileName, 512 * 256000)) {
  //      Serial.println("createContiguous failed");while(1);
  //    }


  lcd.clear();
  Serial.println("Recording..");
  lcd.setCursor(0, 0); lcd.print("Recording...");
  // lcd.setCursor(0, 1); lcd.print(myFileName);
  lcd.setCursor(0, 2); lcd.print(Config.FolderName);
  lcd.setFastBacklight(0);
  lastDataLogSyncTime = millis();

  attachInterrupt(digitalPinToInterrupt(PIN_TRIGGER), recordISR, RISING );
  attachInterrupt(digitalPinToInterrupt(PIN_STOP_LOGGING), stopLoggingISR, CHANGE); // Enable the interrupt


}

void loop() {

  if (triggerEdgeSeen == true) {
    Config.LedOn(); delay(PPS_width / 1000);
    //getdata();


    myData[BufferCounter].counter  = SampleCounter;
    myData[BufferCounter].drdy     = adc.getDrdy();
    myData[BufferCounter].adc      = adc.ReadData_Raw();

    Serial.print(F("Counter: "));
    Serial.print( myData[BufferCounter].counter);

    Serial.print(F(" DRDY: "));
    Serial.print( myData[BufferCounter].drdy);

    Serial.print(F(" ADC(V): "));
    float v = ((float)myData[BufferCounter].adc)   * 244.14e-9 / gain;
    Serial.println(v, 10);

    BufferCounter++;

    // CLear flages
    Config.LedOff();
    triggerEdgeSeen = false;
    isready = true;
  }

  if (BufferCounter  == BufferSize)
  { unsigned int chunkSize = Config.myFile.write((const uint8_t *)&myData, sizeof(myData));
    BufferCounter = 0;
    //unsigned int chunkSize =
    Serial.print("------------------------------------------------------Sent(bytes) :  ");
    Serial.println(chunkSize);
  }

  if (stopLoggingSeen == true) // Has the user pressed the stop logging button?
  {
    Config.stopLogging();
  }

  // if (isready == true) {
  //      unsigned int chunkSize = sensorDataFile.availableForWrite();
  //   Serial.print("Data Size1 :  ");
  //    Serial.println(chunkSize);
  //unsigned int chunkSize2 = sensorDataFile.available();
  //   Serial.print("Data Size2 :  ");
  //    Serial.println(chunkSize2);

  // myFile.write(outputData);
  // uint32_t recordLength = myFile.write(outputData);
  // sensorDataFile.printField(i, term);
  //   Serial.print("record Size: ");
  // Serial.println(recordLength);
  // uint32_t recordLength = myFile.write(outputData);


  //  isready = false;
  // }
  //
  //
  //  if ((millis() - lastDataLogSyncTime > 10000) && (Freq == 128))
  //  if ((millis() - lastDataLogSyncTime > 10000) )
  //
  //  {
  //    lastDataLogSyncTime = millis();
  //    myFile.sync();
  //    //Config.updateDataFileAccess(&myFile);
  //
  //    Serial.println("------------------------------------");
  //
  //    //Config.updateDataFileAccess(&myFile);
  //    //    //               // sensorDataFile.sync();
  //  }
  //
  // if (sready == 1)
  //{
  // myFile.sync();
  //Config.updateDataFileAccess(&myFile);
  //Serial.println("-----------");
  //sready = 0;
  // sensorDataFile.sync();
  // }
}

//void getdata() {
//
//  char tempData[50];
//  outputData[0] = '\0'; //Clear string contents
//  char RTCTime[12];
//
//  //myRTC.getTime();
//  //sprintf(RTCTime, "%02d:%02d:%02d.%02d,", myRTC.hour, myRTC.minute, myRTC.seconds, myRTC.hundredths);
//  //strcat(outputData, RTCTime);
//  // buffer += RTCTime;
//  //i++ ;
//  sprintf(tempData, "%d", SampleCounter);
//  // buffer += tempData;
//  //buffer += "\n";
//  //Serial.write(buffer.c_str());
//  strcat(outputData, tempData);
//  strcat(outputData, "\n");
//
//}

void recordISR() {
  triggerEdgeSeen = true;
  SampleCounter++;
}


void stopLoggingISR() {
  stopLoggingSeen = true;
}
