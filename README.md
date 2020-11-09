# SparkFun-Logger
Hossam Marzouk
05, November 2020
 

##	Logger
The SparkFun OpenLog Artemis is an open-source data logger that comes preprogrammed to automatically log IMU, GPS, serial data, and various pressure, humidity, and distance sensors.

###	Main features
*	Operating voltage: 
*	5V with USB type C 
*	3.6V to 4.2V with LiPo battery
*	Built-in MCP73831 single cell LiPo charger
*	Current consumption ~33mA (running with GPS and ADC) and ~50mA with LCD lights on
*	Built-in 4 channels ADC with 14-bit resolution and up to 1900Hz sampling rate
*	SD card socket, Support FAT32 up to 32GB
*	RTC with 1mAhr battery backup
*	9-axis IMU logging up to 250Hz

A detailed list of features and documentation can be found in the following links:

Main page	https://www.sparkfun.com/products/16832

Schematic	https://cdn.sparkfun.com/assets/1/4/e/1/7/OpenLog_Artemis_9DoF_IMU_Schematic_v1_0_latest.pdf

Processor datasheet	https://cdn.sparkfun.com/assets/d/e/8/b/4/Apollo3_Blue_MCU_Data_Sheet_v0_12_1_rZ9Akgo.pdf

Hookup Guide	https://learn.sparkfun.com/tutorials/openlog-artemis-hookup-guide?_ga=2.77576021.1550399205.1604572944-63414889.1601900928


## Firmware
The SparkFun logger comes preprogrammed with a firmware, current version 1.7,  ready to auto-detect sensors, and start logging automatically to the SD card. The logging parameters can be configured by connecting to the logger with a USB cable and using a terminal window. Full description of firmware, up to date firmware version, and installation procedures can be found in the hookup guide link in Table 1.


###	Logger/firmware disadvantages
The original firmware gives a wide range of applications to use, but it is not very useful in high-frequency time precision applications such as telluric recording for the following reasons:

-	The logging starts immediately after power-on regardless of the GPS fix.
-	The GPS time is only used to update the RTC clock once and only upon booting.
-	The logging frequency is controlled by the RTC only.
-	No support to the LCD display.
-	The firmware states that the maximum achievable sample rate is 156Hz, but in reality, I couldn’t get it to record more than 10 Hz due to a GPS update bug in the firmware.
-	Although they offer the option to log from an external trigger(i.e GPS), there is no way to change the default time pulses of the GPS from 1 PPS.
-	No gain control in the up to date version, but available in older versions!
-	 The firmware uses built-in functions for sensors only, which limits the ADC to 20Hz if it is logging raw voltage. But it can go higher if it logs temperature!.
-	 The durability of the SD card is very weak, it broke after a few times of removal to download the data and it is not repairable. 
-	Editing the source code firmware is possible, but compiling takes a long time ~40 minutes.

###	Firmware Modification
 I have started to fix most of the issues in Section ‎1.4 by doing the following edits to the firmware:
-	Decreased compiling time to ~2 minutes by removing all unused libraries and functions from the source code.
-	Forced the logger to wait for GPS fix before recording.
-	Display time, satellites, and fix type on the LCD.
-	Control the GPS time pule frequency and width (Missy code, but works)

I have stopped modification to that limit because I couldn’t increase the max rate more than 10Hz and it's better to make new firmware instead of making ugly modifications to the original firmware.
 
##	GPS
###	Main features
* Internal antenna
*	72-Channel GNSS Receiver
*	18Hz Max Update Rate
*	Time-To-First-Fix:	Cold: 26s,	Hot: 1s
*	Max G: ≤4
*	Time Pulse Accuracy: 30ns

SparkFun Ublox SAM M8Q GPS documentation links
Main page	https://www.sparkfun.com/products/15210

Schematic	https://cdn.sparkfun.com/assets/3/c/2/2/e/SparkFun_Ublox_SAM-M8Q.pdf

Datasheet	https://cdn.sparkfun.com/assets/4/e/b/9/f/SAM-M8Q_DataSheet__UBX-16012619_.pdf

u-blox Protocol Specification	https://cdn.sparkfun.com/assets/0/b/0/f/7/u-blox8-M8_ReceiverDescrProtSpec__UBX-13003221__Public.pdf

Hookup Guide	https://learn.sparkfun.com/tutorials/sparkfun-gps-breakout-zoe-m8q-and-sam-m8q-hookup-guide?_ga=2.114593863.1550399205.1604572944-63414889.1601900928


###	Advantages
*	Powerful antenna, it can catch more than 7 satellites and fix of 3 indoors.
*	Quick synchronization, max wait indoors 1 min while outdoors 20 seconds wait.
*	Accurate time pulses.
*	Back up internal oscillator for time pulse in case, the GPS lost sync.
*	Different time grids to align time pules, UTC, GPS, GLONASS, BeiDou, and Galileo.
 
###	Disadvantages
*	No build-in function to easily control the time pulse parameters, so I had to make my own functions that write/read binary commands to the GPS board (…\Examples\PPS_control_Blink\PPS_ReadWrite.ino).
*	The documentation states that it can get a max G of 4, but in reality, I can only get what is called fix type of 3. Not sure if this is interpolated as G 4 or not. According to the example code provided by SparkFun

 
##	ADC
###	Main features
*	Measurement Type: Two Differential (Default) or Four Single-Ended
*	Programmable Gain Amplifier: x1 to x128
*	Programmable Voltage Reference
*	Internal Precision 2.048V Reference
*	24-bits (up to 20 bits effective resolution)
*	Sample Rate: 20Hz to 2.0kHz
*	At 20Hz the built-in digital filter provides simultaneous 50Hz and 60Hz noise rejection for industrial applications
*	Current Consumption (Typical):
*	250μA to 955μA (depending on the selected mode)

SparkFun ADS122C04 ADC documentation links
Main page	https://www.sparkfun.com/products/16770

Schematic	https://cdn.sparkfun.com/assets/9/b/6/d/8/Qwiic_PT100-Schematic.pdf

Datasheet	https://cdn.sparkfun.com/assets/7/4/e/1/4/ads122c04_datasheet.pdf


###	Advantages
*	Different acquisition modes (2 wire,3 wire, 4 wire, and raw modes) to internally compensate for the cable resistance or to read the raw voltage value.
*	Two operation modes(normal and turbo) to double the sample rate if needed.
*	Two conversion modes (single and continuous) to control ADC conversion.
*	Predefined sample rates functions of 20, 45, 90, 175, 330, 600, and 1000 Hz. If used in turbo mode the sample rate will be doubled.
*	Predefined gain setting functions of 1, 2, 4, 8, 16, 32, 64, 128.

###	Disadvantages
*	Unable to set the ADC to record between 2 differential channels at the same time, i.e the input channels can be 2 and 3 or 1 and 4 but not both as there are hardware jumpers must be switched to use each channel. It is hardware not possible. 
*	Unable to activate the internal sample counter.	
*	Not sure if I can change the sampling frequency to be as we want i.e 256 instead of 300.
*	No ability to use external trigger directly to the ADC. i.e I have to trigger the logger which in terms triggers the ADC indirectly.
*	Effective resolution is not great for high sample rate and gain, below is the effective resolution table from the datasheet(pages 18,19).
  
 
## Firmware Modification Guide
The SparkFun logger is a very powerful microcontroller but the main disadvantage that it comes with predefined firmware and according to the SparkFun staff it is NOT meant to be modified or edited by any other tool.

Despite the logger not meant to be edited, it is still an open-source firmware and some workarounds can be made to edit or make new firmware.

In the following sections, I will make a step by step guide on how to install the right tools to start editing and how to program each component to work separately and combined with other components. I will also provide some examples programmed in Arduino IDE.

###	Installation guide
The first and most important tool needed in programming microcontrollers is the right compiler, in this case, we will use the Arduino IDE compiler.

Most of the following instructions are from this page https://learn.sparkfun.com/tutorials/artemis-development-with-the-arduino-ide/all, but unfortunately, not everything is compatible with this specific logger so, I will make my own detailed instructions.

### Install Arduino IDE
According to the SparkFun documentation, it is best to use the Arduino IDE version 1.8.12, but I found no problem at all using the up to date version 1.8.13 which can be downloaded from here: https://www.arduino.cc/en/software

### Install SparkFun Apollo3 Boards
Users will need to install the SparkFun Apollo3 Boards board definitions in the Arduino IDE for our development boards with an Artemis module
-	From File>Preferences, copy the following URL to the Additional boards manager
https://raw.githubusercontent.com/sparkfun/Arduino_Boards/master/IDE_Board_Manager/package_sparkfun_index.json
-	Tools>Boards>Boards Manager; Type in search Apollo3 then install SparkFun Apollo3 boards. Make sure the version is 1.2.0 or older, NEVER install new versions
-	Tools>Boards>SparFun Apollo3; choose SparkFun RedBoard Artemis ATP, NEVER choose another board.
-	Tools>Boards>BootLoader; choose SparkFun Variable Loader, NEVER choose another loader. 
-	Finally, don’t forget to set the Port and baud rate according to your configuration.

###	Install Libraries
All the required libraries for the SparkFun logger to work can be found in this link:
https://github.com/hossammarzouk/SparkFun-Logger/blob/main/libraries.zip 

Just download all the libraries and copy to the directory “....\Documents\Arduino\libraries”.

I have included a lot of unused libraries in the link, in case someone compiles the original firmware without deleting unused functions.
 
## Examples
In the following section, I have provided some examples to use with the SparkFun from a simple blink example to read ADC and GPS data.

I will not be able to write all examples here as some require multiple pages, But I will add some remarks here on how some codes work and usefull tricks.

All the example codes and maybe can be found in the following GitHub link: 
https://github.com/hossammarzouk/SparkFun-Logger 

###	Blinking
The purpose of this example to make sure we had established a connection between logger and PC by turning the power and status LEDs on and off.
Note: Most of the predefined blink example will not work on the SparFun logger because it doesn’t have LED_BUILTIN defined so, the predefinition of the LEDs pin fix this issue.
  
###	LCD HelloWorld
In this example, we test the I2C connection between one or multiple devices, which we will use this sequence with any device attached to the logger.
*	Most of the I2C communication uses the library <Wire.h>, but it is not compatible with the logger because the logger uses Wire1.h. The workaround is to define TwoWire qwiic(1) at the start and call it from now on instead of Wire.
*	All the I2C devices are connected together through qwiic connector started in 3 stages :
-	Power up everything,  (pinMode(18, OUTPUT);  digitalWrite(18, HIGH);)
-	Start all qwiic channels (qwiic.begin();)
-	Start communicating with specific device (lcd.begin(qwiic);)
 
###	GPS get time and position
We use the same previous method in initializing the sensor:

*	Define connection library, TwoWire qwiic(1);
*	Power up everything,  (pinMode(18, OUTPUT);  digitalWrite(18, HIGH);)
*	Start all qwiic channels (qwiic.begin();)
*	Start communicating with specific device (myGPS.begin(qwiic);)

 ###	ADC read raw voltage
Again, the same starting sequence but this time we need to specify the ADC I2c address. The default address is 0x45, then we can begin the sensor by (mySensor.begin(address_ADC, qwiic)).
We must also configure the working mode of ADC between raw,2wire,3wrire and 4wire. In this example we use the raw mode to read raw voltage values.
Finally, a conversion must be made to convert the raw values to volts by dividing the value to the LSB. The LSB is 2.048 / 2^23 = 0.24414 uV (0.24414 microvolts).
 
### PPS Configuration
There is no preset functions to control the PPS in the GPS modeule so, we have to send commands to the GPS in binary to change the default configuration.

According to the U-blox8 protocol page 67 (link in section ‎3.1), the following parameters can be used to control PPS:

*	time pulse index - Index of time pulse output pin to be configured
*	antenna cable delay - Signal delay due to the cable between antenna and receiver.
*	RF group delay - Signal delay in the RF module of the receiver (read-only).
*	pulse frequency/period - Frequency or period time of the pulse when locked mode is not configured or active.
*	pulse frequency/period lock - Frequency or period time of the pulse, as soon as receiver has calculated a valid time from a received signal. Only used if the corresponding flag is set to use another setting in locked mode.
*	pulse length/ratio - Length or duty cycle of the generated pulse, either specifies a time or ratio for the pulse to be on/off.
*	pulse length/ratio lock - Length or duty cycle of the generated pulse, as soon as receiver has calculated a valid time from a received signal. Only used if the corresponding flag is set to use another setting in locked mode.
*	user delay - The cable delay from the receiver to the user device plus signal delay of any user application.
*	active - time pulse will be active if this bit is set.
*	lock to gps freq - Use frequency gained from GPS signal information rather than local oscillator's frequency if flag is set.
*	lock to gnss freq - Use frequency gained from GNSS signal information rather than local oscillator's frequency if flag is set.
*	locked other setting - If this bit is set, as soon as the receiver can calculate a valid time, the alternative setting is used. This mode can be used for example to disable time pulse if time is not locked, or indicate lock with different duty cycles.
*	is frequency - Interpret the 'Frequency/Period' field as frequency rather than period if flag is set.
*	is length - Interpret the 'Length/Ratio' field as length rather than ratio if flag is set.
*	align to TOW - If this bit is set, pulses are aligned to the top of a second.
*	polarity - If set, the first edge of the pulse is a rising edge (Pulse Mode: Rising).
*	grid UTC/GPS - Selection between UTC (0) or GPS (1) timegrid. 
*	grid UTC/GNSS - Selection between UTC (0), GPS (1), GLONASS (2) and Beidou (3) timegrid. 

According to the U-blox8 protocol page 222, the following are the bytes index and format of each parameter:  
    
For the example code to read the default configuration of the PPS we need to do the following:
-	Declare empty array to hold configuration
-	Read the binary configuration
-	Decode each parameter in its index bytes according to its format

The multi bytes parametres need to be decoded by, shifting all the adjancent bits to be aligned with the first 8 bits then sum up. For example, the freqperiod parameters which is 4 bytes long from(8:11), we read the last byte shift it by 3 bytes (8*3 bits), then read the second to last bytes shift it by 8*2 bits, then read second bytes and shift 1 byte(8 bits) and finally read the first byte with no shifting and sum all bits together.
 
The following example code shows how to change the default PPS paramertes after reading the configuration:
-	Declare empty array to hold configuration
-	Read the default configuration
-	Change desired parameter in the same way that we used in reading
-	Write all configuration back to GPS

 
