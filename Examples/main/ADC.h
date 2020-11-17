class adc {
    // properties
  private:
    union raw_voltage_union {
      int32_t INT32;
      uint32_t UINT32;
    };
  public:

    void Start() {

      myADC.configureADCmode(ADS122C04_RAW_MODE); // Configure the PT100 for raw mode
      //  myADC.setDataRate(ADS122C04_DATA_RATE_600SPS); // Set the data rate (samples per second) to 600
      myADC.setDataRate(ADS122C04_DATA_RATE_1000SPS); // Set the data rate (samples per second) to 600

      // myADC.setConversionMode(ADS122C04_CONVERSION_MODE_SINGLE_SHOT); // Use single shot mode
      myADC.setVoltageReference(ADS122C04_VREF_INTERNAL); // Use the internal 2.048V reference
      myADC.enableInternalTempSensor(ADS122C04_TEMP_SENSOR_OFF); // Disable the temperature sensor
      myADC.setDataCounter(ADS122C04_DCNT_DISABLE); // Disable the data counter (Note: the library does not currently support the data count)
      myADC.setDataIntegrityCheck(ADS122C04_CRC_DISABLED); // Disable CRC checking (Note: the library does not currently support data integrity checking)
      myADC.setBurnOutCurrent(ADS122C04_BURN_OUT_CURRENT_OFF); // Disable the burn-out current
      myADC.setIDACcurrent(ADS122C04_IDAC_CURRENT_OFF); // Disable the IDAC current
      myADC.setIDAC1mux(ADS122C04_IDAC1_DISABLED); // Disable IDAC1
      myADC.setIDAC2mux(ADS122C04_IDAC2_DISABLED); // Disable IDAC2

      myADC.setConversionMode(ADS122C04_CONVERSION_MODE_CONTINUOUS  ); // Use continuos mode
      myADC.setOperatingMode (ADS122C04_OP_MODE_TURBO); // enable turbo mode


      //myADC.setOperatingMode (ADS122C04_OP_MODE_NORMAL); // Disable turbo mode
      myADC.start(); // Start the first conversion

    }

    void setGain(int _gain) {
      if (_gain == 128)
      {
        myADC.setGain(ADS122C04_GAIN_128); // Set the gain to 128
        myADC.enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
      }
      else if (_gain == 64)
      {
        myADC.setGain(ADS122C04_GAIN_64); // Set the gain to 64
        myADC.enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
      }
      else if (_gain == 32)
      {
        myADC.setGain(ADS122C04_GAIN_32); // Set the gain to 32
        myADC.enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
      }
      else if (_gain == 16)
      {
        myADC.setGain(ADS122C04_GAIN_16); // Set the gain to 16
        myADC.enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
      }
      else if (_gain == 8)
      {
        myADC.setGain(ADS122C04_GAIN_8); // Set the gain to 8
        myADC.enablePGA(ADS122C04_PGA_ENABLED); // Enable the Programmable Gain Amplifier
      }
      else if (_gain == 4)
      {
        myADC.setGain(ADS122C04_GAIN_4); // Set the gain to 4
        myADC.enablePGA(ADS122C04_PGA_DISABLED); // Disable the Programmable Gain Amplifier
      }
      else if (_gain == 2)
      {
        myADC.setGain(ADS122C04_GAIN_2); // Set the gain to 2
        myADC.enablePGA(ADS122C04_PGA_DISABLED); // Disable the Programmable Gain Amplifier
      }
      else
      {
        myADC.setGain(ADS122C04_GAIN_1); // Set the gain to 1
        myADC.enablePGA(ADS122C04_PGA_DISABLED); // Disable the Programmable Gain Amplifier
      }
    }

    int32_t ReadData_Raw() {

      raw_voltage_union raw_v;
      raw_v.UINT32 = myADC.readADC();
      if ((raw_v.UINT32 & 0x00800000) == 0x00800000)
        raw_v.UINT32 |= 0xFF000000;
      return (raw_v.INT32);
    }

    bool getDrdy() {
      int d = myADC.checkDataReady();
      if (d == 0)
        return (false);
      else
        return (true);
    }


};
