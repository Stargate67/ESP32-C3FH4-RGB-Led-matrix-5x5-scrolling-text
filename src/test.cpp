// Adafruit_NeoMatrix example for single NeoPixel Shield.
// Scrolls 'Howdy' across the matrix in a portrait (vertical) orientation.

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include "time.h"
#include "credentials.h"
#include <ModbusIP_ESP8266.h>

#ifndef PSTR
#define PSTR // Make Arduino Due happy
#endif

#define PIN 14

// MATRIX DECLARATION:
// Parameter 1 = width of NeoPixel matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_GRBW    Pixels are wired for GRBW bitstream (RGB+W NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

// Example for NeoPixel Shield.  In this application we'd like to use it
// as a 8x8 tall matrix, with the USB port positioned at the top of the
// Arduino.  When held that way, the first pixel is at the top right, and
// lines are arranged in columns, progressive order.  The shield uses
// 800 KHz (v2) pixels that expect GRB color data.
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, PIN,
  NEO_MATRIX_BOTTOM  + NEO_MATRIX_RIGHT +
  NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB            + NEO_KHZ800);

const uint16_t colors[] = { matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255) };

bool bSerialDebug = false;
String sPrintToMatrix;
unsigned long currmillis = 0; //used in my function to find the current millis()
unsigned long prevmillis = 0; //used to hold previous value of currmillis
unsigned long prevmillis1 = 0; //used to hold previous value of currmillis
int boolval = 0;    //used to control whether to write the brightness value to the led or not
int timer = 0;     //used in the delay function, difference between currmillis and prevmillis
int i;
struct tm timeinfo;
int  pixelPerChar = 6;
int  maxDisplacement;
int iState = 0;
String sPrintdate;
String sPrintShortdate;
String sPrintClock;
String sLocalIP;
String sTempExt;
String sTrend;
float rAvgTempExt = 0;
float rTmp = 0;
unsigned long time_now = 0;
unsigned long time1_now = 0;

const int numReadings = 10;
float readings [numReadings];
int iReadIndex = 0;
int iStartIndex = 0;
float total = 0;
int aisVal = 0;

WiFiClient client;
ModbusIP mb;  //ModbusIP object
//IPAddress MBremote(192, 168, 0, 105);  // Address of Modbus Slave device
IPAddress MBremote(77,204,15,6);  // Address of Internet Box 
const int START_REG = 12689;           // Starting holding register
const int NUM_REGS = 10;           // Number of holding registers to read
const int INTERVAL = 5000;         // Interval between reads (in milliseconds)

uint16_t MBresult[NUM_REGS];
uint8_t show = NUM_REGS;  // Counter for displaying values
uint32_t LastModbusRequest = 0;  // Variable to track the last Modbus request time
float rTempExt;


float smooth(float fInput) { /* function smooth */
  //Perform average on sensor readings
  float average;
  // subtract the last reading:
  total = total - readings[iReadIndex];
  // read the sensor:
  readings[iReadIndex] = fInput;
  // add value to total:
  total = total + readings[iReadIndex];
  // handle index
  iReadIndex = iReadIndex + 1;
  if (iReadIndex >= numReadings) {
    iReadIndex = 0;
  }
  if (iStartIndex < numReadings) {
    iStartIndex = iStartIndex + 1;
  }

  // calculate the average:
  average = total / numReadings;

  if (iStartIndex >= numReadings) {
    return average;
  } else {
    return fInput;
  }
}

void ReadModbus() {
  mb.task();
  if (mb.isConnected(MBremote)) {  
    if (bSerialDebug) Serial.println(String(iState));
    switch (iState)
      {
      case 0:
      {
        // Read holding registers from Modbus Slave
        uint8_t transaction = mb.readHreg(MBremote, START_REG, MBresult, NUM_REGS, nullptr, 1);        
        //if (mb.isTransaction(transaction)) {
          prevmillis1 = millis();
          iState = 10;
        ///} else {
        //  mb.disconnect(MBremote);
        //}
       } 
       break;
      case 10:
      {
        // Wait for the transaction to complete
        if (millis() >= prevmillis1 + 50){ //Process MB client request each second
          prevmillis1 = millis();
//          mb.task();
          iState = 20;
        }
      }
      break;
      case 20:
      {
        mb.disconnect(MBremote);

        rTmp = (MBresult[7] * 100.0 / 32764.0) - 50.0; // Mise a l'echelle
        rTempExt = round(rTmp * 100.0)/100.0;
        //if (bSerialDebug) Serial.print("T.Ext. = ");
        //if (bSerialDebug) Serial.println(rTempExt);
        sTempExt = "T.Ext:" + String(rTempExt) + " C";
        // Calcul la moyenne 
        rAvgTempExt = round(smooth(rTempExt)*100.0)/100.0;
        sTrend = String(" =");
        if (rTempExt > rAvgTempExt) {
          sTrend = String(" Up");
        } else if (rTempExt < rAvgTempExt){
          //sTrend = String(" Dn");
          sTrend = String(" Dn");
        }

        if (bSerialDebug) { 
          Serial.print("T.Ext. = ");
          Serial.println(String(rTempExt));
          Serial.print("Avg T.Ext. = ");
          Serial.print(String(rAvgTempExt));
          Serial.println(sTrend);
        }
        LastModbusRequest = millis();
        
        iState = 30;
        
        if (bSerialDebug) {
        // Print holding register values
          Serial.println("Holding Register Values:");
          for (int i = 0; i < NUM_REGS; i++) {
            Serial.print("Register ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(MBresult[i]);
          }
        }
      }
      break;
      case 30:
      {     // Wait 5 sec
        if (millis() - LastModbusRequest >= INTERVAL) {
          LastModbusRequest = millis();
          iState = 0;
        }
      }
      break;
      default:
      {
        iState = 0;
      }
      break;
      }

  } else {
    // If not connected, try to connect
    mb.connect(MBremote);

    //sTempExt = "ModBus Connecting to " + String(MBremote);
  }
}

void timeloop (int interval){ // the delay function
  prevmillis = millis();
  do {
     timer = (millis() - prevmillis); 
  } while(timer < interval); 
}

void setTimezone(String timezone){
  if (bSerialDebug) Serial.printf(" Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void initTime(String timezone){
  struct tm timeinfo;

  if (bSerialDebug) Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org");    // First connect to NTP server, with 0 TZ offset
  if (!getLocalTime(&timeinfo)) {
    if (bSerialDebug) Serial.println(" Failed to obtain time");
    return;
  }
  if (bSerialDebug) Serial.println(" Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(timezone);
}

void printLocalTime(){
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }
  if (bSerialDebug) Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}

void startWifi(){
  WiFi.begin(ssid, password);
  //if (bSerialDebug) Serial.println("Connecting Wifi");
  //DisplayScrollingString(green, "Connecting Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    //DisplayScrollingString(green, ".");
    //delay(500);
  }
  Serial.print("Wifi RSSI=");
  Serial.println(WiFi.RSSI());
  long rssi = WiFi.RSSI();
  Serial.println("");
  Serial.println(WiFi.localIP());
  sLocalIP = WiFi.localIP().toString();
}

void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  struct tm tm;

  tm.tm_year = yr - 1900;   // Set date
  tm.tm_mon = month-1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;      // Set time
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;  // 1 or 0
  time_t t = mktime(&tm);
  if (bSerialDebug) Serial.printf("Setting time: %s", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

void setup() {  
  if (bSerialDebug) Serial.begin(115200);

  startWifi();
  //FadeString(amber, "Sync NTP");
  initTime("CET-1CEST,M3.5.0,M10.5.0/3");   // Set for Paris/FR

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(20);
  matrix.setTextColor(matrix.Color(50,0,100));

  mb.client();
  time1_now = millis();
}

int x    = matrix.width();
int pass = 0;
uint16_t MbResult = 0;

void loop() {

  if (!getLocalTime(&timeinfo)) {
    sPrintToMatrix = "Failed to obtain time 1";
    return;
  }

  // Display clock
  sPrintClock = (String(timeinfo.tm_hour).length() > 1 ? String(timeinfo.tm_hour) : "0" + String(timeinfo.tm_hour))+ ":" + (String(timeinfo.tm_min).length() > 1 ? String(timeinfo.tm_min) : "0" + String(timeinfo.tm_min));
  
  // Display Date 
  if (timeinfo.tm_min % 2 != i) {
      //printLocalTime();
      sPrintdate = (String(timeinfo.tm_mday).length() > 1 ? String(timeinfo.tm_mday) : "0" + String(timeinfo.tm_mday)) + "/" + (String(timeinfo.tm_mon+1).length() > 1 ? String(timeinfo.tm_mon+1) : "0" + String(timeinfo.tm_mon+1))+ "/" + String(timeinfo.tm_year+1900);

      sPrintShortdate = (String(timeinfo.tm_mday).length() > 1 ? String(timeinfo.tm_mday) : "0" + String(timeinfo.tm_mday)) + "/" + (String(timeinfo.tm_mon+1).length() > 1 ? String(timeinfo.tm_mon+1) : "0" + String(timeinfo.tm_mon+1));

      //if (bSerialDebug) Serial.println(sPrintdate);
      i = timeinfo.tm_min % 2;
  }

 // sPrintdate = (String(timeinfo.tm_mday).length() > 1 ? String(timeinfo.tm_mday) : "0" + String(timeinfo.tm_mday)) + "/" + (String(timeinfo.tm_mon+1).length() > 1 ? String(timeinfo.tm_mon+1) : "0" + String(timeinfo.tm_mon+1))+ "/" + String(timeinfo.tm_year+1900);

  matrix.fillScreen(0);
  matrix.setCursor(x, 0);
  //sPrintToMatrix = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  // On n'affiche plus l'adresse IP après 10s
  if (millis() - time1_now >= 10000) {
    sLocalIP = "";
  }

  sPrintToMatrix =sLocalIP + " " + sPrintShortdate + " " + sPrintClock + " " + sTempExt + sTrend;

  //if (bSerialDebug) Serial.println(sPrintToMatrix);
  matrix.print(sPrintToMatrix);

  //maxDisplacement = sPrintToMatrix.length() * pixelPerChar + matrix.width();
  maxDisplacement = sPrintToMatrix.length() * pixelPerChar;
  if (--x < -maxDisplacement) {
    x = matrix.width();
    if (++pass >= 3) pass = 0;
    //matrix.setBrightness(50);
    //matrix.setTextColor(colors[pass]);
    matrix.setTextColor(matrix.Color(100,0,100));
    //matrix.setBrightness(10);
  }

  matrix.show();
  //timeloop(70);
  delay(50);
  
  ReadModbus();
}
