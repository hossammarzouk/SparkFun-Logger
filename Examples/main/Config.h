class Config {
    // properties
  private:
  public:
#define SD_CONFIG SdSpiConfig(PIN_MICROSD_CHIP_SELECT, DEDICATED_SPI)

    typedef SdExFat sd_t;
    sd_t SD;
    typedef ExFile file_t;
    file_t myFile;
    file_t myHeaderFile;

    char *FolderName;
    //Methods
    void StartInterput() {
      pinMode(PIN_STAT_LED, OUTPUT);
      pinMode(PIN_TRIGGER, INPUT_PULLUP);
      pinMode(PIN_STOP_LOGGING, INPUT_PULLUP);
    }

    void LedOn() {
      digitalWrite(PIN_STAT_LED, HIGH);
    }
    void LedOff() {
      digitalWrite(PIN_STAT_LED, LOW);
    }

    void CheckAll() {
      //lcd.setBacklight(255, 255, 255);
      lcd.setCursor(0, 0); lcd.print("SparkFun logger v1.0");
      lcd.setCursor(0, 1); lcd.print("Self test...");
      delay(100); // Make sure the first 2 lines printed
      while (Error == true) {
        Error = false;
        lcd.setCursor(0, 2); lcd.print("GPS");
        if (myGPS.isConnected() == true) {
          lcd.setCursor(4, 2); lcd.print(":)");
          Error == false;
        }
        else {
          Error = true;
          lcd.setCursor(4, 2); lcd.print(";(");
        }

        lcd.setCursor(8, 2); lcd.print("ADC");
        if (myADC.isConnected() == true) {
          Error == false;
          lcd.setCursor(12, 2); lcd.print(":)");
        }
        else {
          Error = true;
          lcd.setCursor(12, 2); lcd.print(";(");
        }

        lcd.setCursor(15, 2); lcd.print("SD");
        //       if (SD.begin(PIN_MICROSD_CHIP_SELECT, SD_SCK_MHZ(50))) {
        if (SD.begin(SD_CONFIG)) {

          // if (sd.begin(SD_CONFIG)) {
          Error == false;
          lcd.setCursor(18, 2); lcd.print(":)");
        }
        else {
          Error = true;
          lcd.setCursor(18, 2); lcd.print(";(");
        }
        delay(500);
      }
      lcd.setCursor(0, 3); lcd.print(".....Systems OK.....");
      delay(5000);

    }
    void beginQwiic()
    {
      pinMode(PIN_QWIIC_POWER, OUTPUT);
      digitalWrite(PIN_QWIIC_POWER, HIGH);
      qwiic.begin();
      myGPS.begin(qwiic, address_GPS); // Make sure gps is started with correct address
      myADC.begin(address_ADC, qwiic);
      delay(100);          // need several delays for lcd to start, not sure why!!!
      lcd.begin(qwiic, address_LCD);   // Make sure lcd is started with correct address
      delay(1000);
      lcd.setFastBacklight(255, 255, 255);

    }

    void updateDataFileAccess(SdFile * dataFile)
    {
      myRTC.getTime(); //Get the RTC time so we can use it to update the last modified time
      //Update the file access time
      dataFile->timestamp(T_ACCESS, (myRTC.year + 2000), myRTC.month, myRTC.dayOfMonth, myRTC.hour, myRTC.minute, myRTC.seconds);
      //Update the file write time
      dataFile->timestamp(T_WRITE, (myRTC.year + 2000), myRTC.month, myRTC.dayOfMonth, myRTC.hour, myRTC.minute, myRTC.seconds);
    }

    void MakeMeasDir() {
      FolderName = GPS.getFolderName();
      SD.mkdir(FolderName);
      if (!SD.chdir(FolderName)) {
        Serial.println("Make dir  failed .\n");
        lcd.clear();

        lcd.setCursor(0, 0); lcd.print("Error making folder");
        while (1);
      }
      else
        Serial.println(FolderName);

    }
    //      FolderName = GPS.getFolderName();
    //      sd.mkdir(FolderName);
    //      if (!sd.chdir(FolderName)) {
    //        Serial.println("Make dir  failed .\n");
    //        lcd.clear();
    //
    //        lcd.setCursor(0, 0); lcd.print("Error making folder");
    //        while (1);
    //      }
    //      else
    //        Serial.println(FolderName);
    //
    //    }
    void MakeMeasFile() {
      SD.remove(myFileName);
      //  if (myFile.open(myFileName, O_CREAT | O_WRITE  ) == false)
      //  {
      //    Serial.println(F("Failed to create sensor data file")); while (1);
      //  }
      if (myFile.open(myFileName, O_CREAT | O_WRITE  ) == false)
      {
        Serial.println(F("Failed to create sensor data file")); while (1);
      }
    }
    void stopLogging() {
      detachInterrupt(digitalPinToInterrupt(PIN_STOP_LOGGING)); // Disable the interrupt
      lcd.clear();
      lcd.setFastBacklight(255, 0, 0);
      lcd.setCursor(0, 0); lcd.print("Shuting down!");
      lcd.setCursor(0, 1); lcd.print("please wait..");
      myFile.sync();
      //updateDataFileAccess(&myFile); // Update the file access time & date
      delay(100);
      myFile.close();

      delay(3000); // wait for sd to stop
      lcd.setFastBacklight(255, 255, 255);

      Serial.println("System stopped by user");
      lcd.clear();

      lcd.setCursor(0, 0); lcd.print("System Shutdown!");
      lcd.setCursor(0, 1); lcd.print("Press reset button");
      lcd.setCursor(0, 2); lcd.print("to start again");
      lcd.setCursor(0, 3); lcd.print("(-,-)...zzzzZZZZZ");
      while (1);
    }

    void MakeHeaderFile() {
      if (myHeaderFile.open(myHeaderFileName, O_CREAT | O_WRITE | O_TRUNC  ) == false)
      {
        Serial.println(F("Failed to create Header file")); while (1);
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("Error Header file");
        while (1);
      }
      myHeaderFile.println("EDE: 999");
      myHeaderFile.println("VERSION: Aug 14 2020");
      myHeaderFile.println("TYPE: E");
      myHeaderFile.println("SAMPLECOUNTER: 1");
      myHeaderFile.println("LSB: 2.44140625e-04 mV");

      myHeaderFile.print("GAIN: ");
      myHeaderFile.println(gain);

      myHeaderFile.println("LP: 1");
      myHeaderFile.println(" ");

      float templong = ((float)myGPS.getLongitude()) / 10000000;
      myHeaderFile.print("LONGITUDE: ");
      myHeaderFile.println(templong);

      myHeaderFile.print("LATITUDE: ");
      float templat = ((float)myGPS.getLatitude()) / 10000000;
      myHeaderFile.println(templat);

      myHeaderFile.println("SYSTEMSTARTTIME: 00:00:00");
      myHeaderFile.println("SYSTEMSTARTDATE: 00:00:00");
      myHeaderFile.println("PPSDELAY: 00 us");

      myHeaderFile.print("SAMPLERATE:");
      myHeaderFile.println(Freq);
      myHeaderFile.println("Ex: 1 m");
      myHeaderFile.println("Ey: 1 m");

      myHeaderFile.print("STARTTIME: ");
      myHeaderFile.println(GPS.getStartTime());

      myHeaderFile.print("STARTDate: ");
      myHeaderFile.println(GPS.getStartDate());

      myHeaderFile.println("STOPTIME: 00 00 00 000");
      myHeaderFile.println("STOPDATE: 00 00 0000");

      myHeaderFile.println("STOPFILENUMBER: 001");
      myHeaderFile.println("NSAMPLES: 0000000");

      myHeaderFile.sync();
      // updateDataFileAccess(&myHeaderFile); // Update the file access time & date
      myHeaderFile.close();


    }
    unsigned int WriteETS() {
      //  unsigned int chunkSize = myFile.write((const uint8_t *)&myData, sizeof(myData));
//      return (chunkSize);
    }

};
