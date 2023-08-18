// BASE CODE CREDIT TO ELI THE COMPUTER GUY: https://www.elithecomputerguy.com/2020/06/arduino-arducam-5mp-time-lapse-camera/

//Modification of Example: ArduCAM_Mini_5MP_Plus_Multi_Capture2SD

#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <SD.h>
#include "memorysaver.h"
#include <TimeLib.h>
#include <DS1307RTC.h>
//This demo can only work on OV5640_MINI_5MP_PLUS or OV5642_MINI_5MP_PLUS platform.
#if !(defined (OV5640_MINI_5MP_PLUS)||defined (OV5642_MINI_5MP_PLUS))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif

//ETCG Notes -- Change Number of Pictures Taken Per Function Call
#define   FRAMES_NUM    0x00
// set pin 7 as the slave select for the digital port for ArduCam:
const int CS = 7;

//ETCG Notes -- Change Digital Pin for SD Module/ Shield
#define SD_CS 10

//ETCG Note -- RTC/ Real Time Clock Code
//#include <DS3231.h>
//DS3231 rtc(SDA, SCL);
tmElements_t tm;


long timeLapseLength = 20;
long lapseTimeStamp;

//ETCG Note -- Variable for Filename
String fileName;

bool is_header = false;
int total_time = 0;
//#if defined (OV5640_MINI_5MP_PLUS)
//ArduCAM myCAM(OV5640, CS);
//#else
ArduCAM myCAM(OV5642, CS);
//#endif
uint8_t read_fifo_burst(ArduCAM myCAM);
void setup() {
  // put your setup code here, to run once:
  uint8_t vid, pid;
  uint8_t temp;
#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  Serial.begin(115200);
  Serial.println(F("ArduCAM Start!"));

  while (!Serial) ; // wait for serial
  delay(200);
  Serial.println("DS1307RTC Read Test");
  Serial.println("-------------------");

  // set the CS as an output:
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  //Serial.println(F("before SPI initial"));

  // initialize SPI:
  SPI.begin();
  //Serial.println(F("SPI begin ran"));

  //Reset the CPLD
  myCAM.write_reg(0x07, 0x80);
  //Serial.println(F("0x80 ran"));

  delay(100);

  //Serial.println(F("delay ran"));

  myCAM.write_reg(0x07, 0x00);
  delay(100);

  //Serial.println(F("after SPI initial"));

  while (1) {
    //Check if the ArduCAM SPI bus is OK
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);
    if (temp != 0x55)
    {
      Serial.println(F("SPI interface Error!"));
      delay(1000); continue;
    } else {
      Serial.println(F("SPI interface OK.")); break;
    }
  }
#if defined (OV5640_MINI_5MP_PLUS)
  while (1) {
    //Check if the camera module type is OV5640
    myCAM.rdSensorReg16_8(OV5640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg16_8(OV5640_CHIPID_LOW, &pid);
    if ((vid != 0x56) || (pid != 0x40)) {
      Serial.println(F("Can't find OV5640 module!"));
      delay(1000); continue;
    } else {
      Serial.println(F("OV5640 detected.")); break;
    }
  }
#else
  while (1) {
    //Check if the camera module type is OV5642
    myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
    if ((vid != 0x56) || (pid != 0x42)) {
      Serial.println(F("Can't find OV5642 module!"));
      delay(1000); continue;
    } else {
      Serial.println(F("OV5642 detected.")); break;
    }
  }
#endif
  //Initialize SD Card
  while (!SD.begin(SD_CS))
  {
    Serial.println(F("SD Card Error!")); delay(1);
  }
  Serial.println(F("SD Card detected."));
  //Change to JPEG capture mode and initialize the OV5640 module
  myCAM.set_format(JPEG);
//  Serial.println(F("JPEG ran"));
  myCAM.InitCAM();
 Serial.println(F("initcam ran"));


  myCAM.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
  //Serial.println(F("setbit ran"));

  myCAM.clear_fifo_flag();
//  Serial.println(F("fifoflag ran"));

  myCAM.write_reg(ARDUCHIP_FRAMES, FRAMES_NUM);
  //Serial.println(F("writereg ran"));

}

void loop() {

  //ETCG Note -- RTC/ Real Time Clock Code
  //rtc.begin();
  tmElements_t tm2;
  RTC.read(tm2);

  // put your main code here, to run repeatedly:
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();

  //ETCG Notes -- Change Resolution in set_JPEG_size
#if defined (OV5640_MINI_5MP_PLUS)
  myCAM.OV5640_set_JPEG_size(OV5640_2592x1944); delay(1000);
#else
  myCAM.OV5642_set_JPEG_size(OV5642_2592x1944); delay(1000);
#endif

  //int timeinSec = (tm.Hour*3600 + tm.Minute*60 + tm.Second);

  //ETCG Notes -- Check if Time Lapse has been long enough and if So Take Picture
  
  long elapsedTime = ((long)tm2.Hour*3600) + (tm2.Minute*60) + (tm2.Second) - lapseTimeStamp;
  if (elapsedTime >= timeLapseLength) {
    lapseTimeStamp = ((long)tm2.Hour*3600) + (tm2.Minute*60) + (tm2.Second);
    elapsedTime = ((long)tm2.Hour*3600) + (tm2.Minute*60) + (tm2.Second) - lapseTimeStamp;

    //Start capture
    myCAM.start_capture();
    Serial.println(F("start capture."));
    total_time = millis();
    while ( !myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
    Serial.println(F("CAM Capture Done."));
    total_time = millis() - total_time;
    Serial.print(F("capture total_time used (in miliseconds):"));
    Serial.println(total_time, DEC);
    total_time = millis();
    read_fifo_burst(myCAM);
    total_time = millis() - total_time;
    Serial.print(F("save capture total_time used (in miliseconds):"));
    Serial.println(total_time, DEC);
    //Clear the capture done flag
    myCAM.clear_fifo_flag();
    delay(5000);
  } else {
    Serial.print("Waiting: ");
    //Serial.print(print2digits(tm2.Second));
    //Serial.println(lapseTimeStamp);
    Serial.println(timeLapseLength - elapsedTime);
    //Serial.println(elapsedTime);
    //Serial.println(lapseTimeStamp);
    //Serial.println(tm2.Hour);
    //Serial.println(tm2.Minute);
    //Serial.println(tm2.Second);
  }

}

uint8_t read_fifo_burst(ArduCAM myCAM)
{
  uint8_t temp = 0, temp_last = 0;
  uint32_t length = 0;
  static int i = 0;
  static int k = 0;
  char str[8];
  File outFile;
  byte buf[256];
  length = myCAM.read_fifo_length();
  Serial.print(F("The fifo length is :"));
  Serial.println(length, DEC);
  if (length >= MAX_FIFO_SIZE) //8M
  {
    Serial.println("Over size.");
    return 0;
  }
  if (length == 0 ) //0 kb
  {
    Serial.println(F("Size is 0."));
    return 0;
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();//Set fifo burst mode
  i = 0;
  while ( length-- )
  {
    temp_last = temp;
    temp =  SPI.transfer(0x00);
    //Read JPEG data from FIFO
    if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
    {
      buf[i++] = temp;  //save the last  0XD9
      //Write the remain bytes in the buffer
      myCAM.CS_HIGH();
      outFile.write(buf, i);
      //Close the file
      outFile.close();
      Serial.println(F("OK"));
      is_header = false;
      myCAM.CS_LOW();
      myCAM.set_fifo_burst();
      i = 0;
    }
    if (is_header == true)
    {
      //Write image data to buffer if not full
      if (i < 256)
        buf[i++] = temp;
      else
      {
        //Write 256 bytes image data to file
        myCAM.CS_HIGH();
        outFile.write(buf, 256);
        i = 0;
        buf[i++] = temp;
        myCAM.CS_LOW();
        myCAM.set_fifo_burst();
      }
    }
    else if ((temp == 0xD8) & (temp_last == 0xFF))
    {
      is_header = true;
      myCAM.CS_HIGH();

      //ETCG Note -- Create Filename for Image
      //long timeStamp = rtc.getUnixTime(rtc.getTime());
      //String timeStampNew = String(timeStamp % 100000000);
      RTC.read(tm);
      //fileName = String(tm.Day) + "_" + String(tm.Month) + "_" + String(tmYearToCalendar(tm.Year)) + ".jpg";

      fileName = String(tm.Hour) + "_" + String(tm.Minute) + "_" + String(tm.Second) + ".jpg";
      //outFile = SD.open(fileName, O_WRITE | O_CREAT | O_TRUNC);
      outFile = SD.open(fileName, FILE_WRITE);

      Serial.println(fileName);
      if (! outFile)
      {
        Serial.println(F("File open failed"));
        while (1);
      }
      myCAM.CS_LOW();
      myCAM.set_fifo_burst();
      buf[i++] = temp_last;
      buf[i++] = temp;
    }
  }
  myCAM.CS_HIGH();
  return 1;

}
/*
long print2digits(long number) {
  if (number >= 0 && number < 10) {
    return 0;
  }
  return number;
}
*/
