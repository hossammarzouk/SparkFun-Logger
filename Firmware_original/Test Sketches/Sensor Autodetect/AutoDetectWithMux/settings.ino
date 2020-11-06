
// Check for extra characters in field or find minus sign.
char* skipSpace(char* str) {
  while (isspace(*str)) str++;
  return str;
}


void beginSD()
{
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected

  if (true == true)
  {
    microSDPowerOn();

    //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
    //Max current is 200mA average across 1s, peak 300mA
    delay(10);

    if (sd.begin(SD_CONFIG) == false) //Slightly Faster SdFat Beta (we don't have dedicated SPI)
    {
      delay(250); //Give SD more time to power up, then try again
      if (sd.begin(SD_CONFIG) == false) //Slightly Faster SdFat Beta (we don't have dedicated SPI)
      {
        Serial.println("SD init failed. Is card present? Formatted?");
        digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
        //online.microSD = false;
        return;
      }
    }
    //    }

    //Change to root directory. All new file creation will be in root.
    if (sd.chdir() == false)
    {
      Serial.println("SD change directory failed");
      //online.microSD = false;
      return;
    }

    //online.microSD = true;
  }
  else
  {
    microSDPowerOff();
    //online.microSD = false;
  }
}
