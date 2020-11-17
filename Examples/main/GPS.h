class GPS {
    // properties
  private:
    float Lat ; float Long ; int Alt = 0 ; int Fix = 0 ; int SIV = 0;
    int year = 2000; int month  = 1; int day    = 1;
    int hour = 0; int minute = 0; int second = 0; int milliseconds = 0;
    bool datevalid, timevalid, dataLogging = false;

    uint8_t  tpIdx, Version;
    int16_t  antCableDelay, rfGroupDelay;
    uint32_t freqPeriod = 5, freqPeriodLock, pulseLenRatio, pulseLenRatioLock;
    int32_t  userConfigDelay;
    byte     flags;
    struct StartTime {
      int year;
      int month;
      int day;
      int hour;
      int minute;
      int second;
      int milli;
    };
  public:

    StartTime StartTime;

    //Methods
    void WaitMaxG(int Max_G) {
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("SparkFun logger v1.0");
      lcd.setCursor(0, 1); lcd.print("Waiting for GPS...");
      lcd.setCursor(0, 2); lcd.print("SAT:");
      lcd.setCursor(4, 2); lcd.print(SIV);
      lcd.setCursor(8, 2); lcd.print("FIX:");
      lcd.setCursor(13, 2); lcd.print(Fix);
      lcd.setCursor(18, 2); lcd.print(");");
      delay(100);

      while (Fix < Max_G)   // wait for Fix 3 and proceed
      {
        getUbloxCoord(Lat, Long, Alt, SIV, Fix);    // Get the positon from GPS

        lcd.setCursor(4, 2);  lcd.print(SIV);     // display sat and fix only at screen
        lcd.setCursor(13, 2); lcd.print(Fix);
        delay(1000);                              //update once a second not to case traffic

        if (Fix == Max_G) {                        // if fix is 3 clear screen and proceed

          lcd.setCursor(18, 2); lcd.print(":)");
          lcd.setCursor(0, 3); lcd.print("....Synchronized....");
          delay(3000);                      // wait to put a smile on the user face :)
          //lcd.clear();
        }
      }
    }

    void WaitFullMinute() { // maybe it is best to use RTC instead !!
      int s  = myGPS.getSecond();
      int si  = s;
      int mi = myGPS.getMillisecond();
      int h  = myGPS.getHour();
      int m  = myGPS.getMinute();
      char Time[13];

      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("Ready for recording...");
      lcd.setCursor(0, 1); lcd.print("Wait full minute");
      lcd.setCursor(0, 2); lcd.print("Time: ");

      while (s != 0 || s > si )
      {
        s = myGPS.getSecond();
        mi = myGPS.getMillisecond();
        m  = myGPS.getMinute();
        h  = myGPS.getHour();
        sprintf(Time, "%02d:%02d:%02d.%03d", h, m, s, mi);
        lcd.setCursor(5, 2); lcd.print(Time);
         Serial.println(Time);

      }
    }


    void getUbloxDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second, int &millisecond, bool & dateValid, bool & timeValid)
    {
      year        = myGPS.getYear();
      month       = myGPS.getMonth();
      day         = myGPS.getDay();
      hour        = myGPS.getHour();
      minute      = myGPS.getMinute();
      second      = myGPS.getSecond();
      dateValid   = myGPS.getDateValid();
      timeValid   = myGPS.getTimeValid();
      millisecond = myGPS.getMillisecond();



    }
    void getUbloxCoord(float & Lat, float & Long, int &Alt, int &SIV, int &Fix)
    {
      Lat  = myGPS.getLatitude();
      Long = myGPS.getLongitude();
      Alt  = myGPS.getAltitude();
      SIV  = myGPS.getSIV();
      Fix  = myGPS.getFixType();
    }

    void UpdateRTC() {
      getUbloxDateTime(year, month, day, hour, minute, second, milliseconds, datevalid, timevalid); // get gps time
      myRTC.setTime(hour, minute, second, (milliseconds / 10), day, month, (year - 2000)); //Manually set RTC

    }

    String gpsTime() {
      getUbloxDateTime(year, month, day, hour, minute, second, milliseconds, datevalid, timevalid); // get gps time
      String myTime = "";
      char tempTime[13]; //09:14:37.412
      sprintf(tempTime, "%02d:%02d:%02d.%03d", hour, minute, second, milliseconds % 1000);
      myTime += String(tempTime);
      return (myTime);

    }
    char* getFolderName() {
      getUbloxDateTime(year, month, day, hour, minute, second, milliseconds, datevalid, timevalid); // get gps time
      myRTC.setTime(hour, minute, second, (milliseconds / 10), day, month, (year - 2000)); //Manually set RTC
      StartTime.hour = hour; StartTime.minute = minute+1; StartTime.second = 0; StartTime.milli =  0;

      StartTime.year = year; StartTime.day = day; StartTime.month = month;
      static char tempName[30]; //09:14:37
      sprintf(tempName, "meas_%04d-%02d-%02d_%02d-%02d-%02d", year, month, day, hour, minute + 1, 0);
      return (tempName);

    }

    char* getStartTime() {

      static char tempName[14]; //09:14:37
      sprintf(tempName, "%02d %02d %02d %03d",  StartTime.hour ,  StartTime.minute,  StartTime.second, StartTime.milli  );
      return (tempName);

    }
    char* getStartDate() {
      static char tempName[10]; //09:14:37
      sprintf(tempName, "%02d %02d %02d",  StartTime.day ,  StartTime.month,  StartTime.year);
      return (tempName);

    }

    String gpsTimeDate() {
      getUbloxDateTime(year, month, day, hour, minute, second, milliseconds, datevalid, timevalid); // get gps time
      String myTime = "";
      char tempTime[9]; //09:14:37
      sprintf(tempTime, "%02d-%02d-%02d", hour, minute, second, milliseconds % 1000);
      char tempDate[12]; //10/12/2019,
      sprintf(tempDate, "%04d-%02d-%02d", year, month, day);

      myTime += String(tempDate);
      myTime += "_";
      myTime += String(tempTime);
      return (myTime);

    }
    void setNavFreq(int _freq) {
      myGPS.setNavigationFrequency(_freq);
    }

    void setPPSfreq(int _freq, int _width)
    {
      uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes
      uint16_t maxWait       = 1100;          // Wait for up to 250ms (Serial may need a lot longer e.g. 1100)
      int max_ind = 0;

      // The next line creates and initialises the packet information which wraps around the payload
      ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

      customCfg.cls           = UBX_CLASS_CFG; // This is the message Class
      customCfg.id           = UBX_CFG_TP5;    // This is the message ID
      customCfg.len          = 0;              // Setting the len (length) to zero let's us poll the current settings
      customCfg.startingSpot = 0;              // Always set the startingSpot to zero (unless you really know what you are doing)

      while (max_ind <= 10) {
        if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
        {
          max_ind++; delay(100);
          if (max_ind == 10) {
            lcd.clear();
            lcd.setCursor(0, 0); lcd.print("Error updating pps");
            lcd.setCursor(0, 1); lcd.print("Please restart or");
            lcd.setCursor(0, 2); lcd.print("change pps parameter");

            Serial.print("Error updating pps1"); while (1);
          }
        }
        else {
          max_ind = 0;
          break;
        }
      }
      //............. start setting flages.............
      ////the flags are set in byte 28 (see ublox protocal page 224)
      flags = customPayload[28];
      bitWrite(flags, 3, 1); // change frequency unit from period to frequency
      //bitWrite(Flags, 4, 0); // change duration unit from lenghth to ratio
      customPayload[28] = flags;
      //............. Done setting flags.............

      //............. start setting frequency.............

      //update freqPeriodLock
      customPayload[12] = _freq ; // Store it in the payload
      customPayload[13] = _freq >>  8;
      customPayload[14] = _freq >> 8 * 2;
      customPayload[15] = _freq >>  8 * 3;

      //update freqPeriod (we use only freq lock but update this just in case)
      customPayload[8]  = _freq ; // Store it in the payload
      customPayload[9]  = _freq >>  8;
      customPayload[10] = _freq >> 8 * 2;
      customPayload[11] = _freq >>  8 * 3;

      //............. Done setting frequency.............


      //............. start setting pulse ratio.............

      //  //update pulseLensRationLock
      //  int ratio = 1000000; // set default ratio to 1000 50%

      customPayload[20] = _width; // Store it in the payload
      customPayload[21] = _width >>  8;
      customPayload[22] = _width >> 8 * 2;
      customPayload[23] = _width >>  8 * 3;
      //
      //  //update pulseLensRation (we use only pulse ratio lock but update this just in case)
      //  customPayload[16]  = _width & 0xFF; // Store it in the payload
      //  customPayload[17]  = _width >>  8;
      //  customPayload[18]  = _width >> 8 * 2;
      //  customPayload[19]  = _width >>  8 * 3;

      //............. Done  setting pulse ratio.............
      while (max_ind <= 10) {
        if (myGPS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_SENT) // We are expecting data and an ACK
        {
          max_ind++; delay(100);
          if (max_ind == 10) {
            lcd.clear();
            lcd.setCursor(0, 0); lcd.print("Error updating pps");
            lcd.setCursor(0, 1); lcd.print("Please restart or");
            lcd.setCursor(0, 2); lcd.print("change pps parameter");

            Serial.print("Error updating pps2"); while (1);
          }
        }
        else {
          break;
        }
      }
    }

    uint64_t rtcMillis()
    {
      myRTC.getTime();
      uint64_t millisToday = 0;
      int dayOfYear = calculateDayOfYear(myRTC.dayOfMonth, myRTC.month, myRTC.year + 2000);
      millisToday += ((uint64_t)dayOfYear * 86400000ULL);
      millisToday += ((uint64_t)myRTC.hour * 3600000ULL);
      millisToday += ((uint64_t)myRTC.minute * 60000ULL);
      millisToday += ((uint64_t)myRTC.seconds * 1000ULL);
      millisToday += ((uint64_t)myRTC.hundredths * 10ULL);

      return (millisToday);
    }

    //Returns the day of year
    //https://gist.github.com/jrleeman/3b7c10712112e49d8607
    int calculateDayOfYear(int day, int month, int year)
    {
      // Given a day, month, and year (4 digit), returns
      // the day of year. Errors return 999.

      int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

      // Verify we got a 4-digit year
      if (year < 1000) {
        return 999;
      }

      // Check if it is a leap year, this is confusing business
      // See: https://support.microsoft.com/en-us/kb/214019
      if (year % 4  == 0) {
        if (year % 100 != 0) {
          daysInMonth[1] = 29;
        }
        else {
          if (year % 400 == 0) {
            daysInMonth[1] = 29;
          }
        }
      }

      // Make sure we are on a valid day of the month
      if (day < 1)
      {
        return 999;
      } else if (day > daysInMonth[month - 1]) {
        return 999;
      }

      int doy = 0;
      for (int i = 0; i < month - 1; i++) {
        doy += daysInMonth[i];
      }

      doy += day;
      return doy;
    }


};
