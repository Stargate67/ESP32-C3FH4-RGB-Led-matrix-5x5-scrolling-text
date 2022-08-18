// RTC demo for ESP32, that includes TZ and DST adjustments
// Get the POSIX style TZ format string from  https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
// Created by Hardy Maxa
// Complete project details at: https://RandomNerdTutorials.com/esp32-ntp-timezones-daylight-saving/
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include "time.h"
#include "font.h"
#include "credentials.h"

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 5
#define LED_PIN 8
#define LED_COUNT 25

unsigned long currmillis = 0; //used in my function to find the current millis()
unsigned long prevmillis = 0; //used to hold previous value of currmillis
int boolval = 0;    //used to control whether to write the brightness value to the led or not
int timer = 0;     //used in the delay function, difference between currmillis and prevmillis

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t amber = strip.Color(255, 140, 0);
uint32_t white = strip.Color(255, 255, 255);
uint32_t yellow = strip.Color(255, 255, 0);
int i;
struct tm timeinfo;

void timeloop (int interval){ // the delay function
  prevmillis = millis();
  do{
     timer = (millis() - prevmillis); 
  } while(timer < interval); 
}

void DrawPixel(uint32_t colour, uint8_t x, uint8_t y, uint8_t brightness)
{
  uint8_t led = (4 - y) * 5 + (4 - x); //  Calc LED from x,y coords, 0,0 is bottom left pixel
  uint8_t red = ((colour >> 16) & 0xff) * brightness / 100;
  uint8_t green = ((colour >> 8) & 0xff) * brightness / 100;
  uint8_t blue = (colour & 0xff) * brightness / 100;
  strip.setPixelColor(led, red, green, blue);
  // strip.show();
}

// Based on code from https://jared.geek.nz/2014/jan/custom-fonts-for-microcontrollers
void DrawChar(uint32_t colour, char c, uint8_t brightness) {
  uint8_t w, x, y, z;
  uint8_t c1[CHAR_WIDTH];

  // Convert the character to an index
  c = c & 0x7F;
  if (c < ' ')
  {
    c = 0;
  }
  else
  {
    //c = c - ' ';
    c = c - 32;
  }

  // 'font' is a multidimensional array of [96][char_width]
  //const uint8_t *chr = font[c];
  // Draw pixels
  //Serial.println(c);
 
  // Reset matrix before shift bytes
  memmove(c1, font[0], sizeof(c1));

  w=0;
   //Shift bytes to left and draw pixel
  for (w = 0; w < CHAR_WIDTH; w++) {
  //memcpy(c1, chr, sizeof(c1));

  //Shift bytes to left and draw pixel
    c1[0] = c1[1];
    c1[1] = c1[2];
    c1[2] = c1[3];
    c1[3] = c1[4];
    c1[4] = font[c][w];
    
    /* 
    Serial.println(c); 
    Serial.println(c1[0], BIN); 
    Serial.println(c1[1], BIN); 
    Serial.println(c1[2], BIN); 
    Serial.println(c1[3], BIN); 
    Serial.println(c1[4], BIN); 
    Serial.println("Next byte"); 
  */
    for (x = 0; x < CHAR_WIDTH; x++) {
      for (y = 0; y < CHAR_HEIGHT; y++) {
        if (c1[x] >> 2 & (1 << y)) {
          DrawPixel(colour, x, y, brightness);
        }
        strip.show();
      }
    }
    timeloop(200); // a delay of 200 ms using the function
    strip.clear();
   }
}

void DisplayScrollingString(uint32_t colour, String s, uint8_t brightness=80, uint8_t speed=150/*ms*/) {
  
  int iBufIdx;
  uint8_t  x, y, z;
  int w, iPointer;

  uint8_t c1[CHAR_WIDTH];
  
  char c;
  char buffer[s.length() + 1];
  s.toCharArray(buffer, s.length()+1);
  
  // Array of all matrix bytes (columns)
  uint8_t iBuffer[(s.length()+2) * 6]; // roomm for 1 empty character in front and one at the end 

  //Serial.println(s.length()); 
  //Serial.println(sizeof(buffer)); 
  iPointer = 0;
  //Chaine all char bytes 
  iBuffer[iPointer] = 0;
  iPointer++; //+1 to add an empty column
  iBuffer[iPointer] = 0;
  iPointer++; //+1 to add an empty column
  iBuffer[iPointer] = 0;
  iPointer++; //+1 to add an empty column
  iBuffer[iPointer] = 0;
  iPointer++; //+1 to add an empty column
  iBuffer[iPointer] = 0;

  for (iBufIdx=0; iBufIdx < (s.length()+1); iBufIdx++ ) {
    
    // Convert the character to an index
    c = buffer[iBufIdx];
    //Serial.print("c brut="); 
    //Serial.println(c); 
  
    c = c & 0x7F;
    if (c < ' ')
    {
      c = 0;
    }
    else
    {
      //c = c - ' ';
      c = c - 32;
    }

    //Serial.print("c="); 
    //Serial.println(c); 

    //Assemble the scrolling pattern
    //Move the effectives columns from the character to the next assembled columns 
    for (w=0; w < ifont[c][5]; w++ ) {
      iBuffer[iPointer] = ifont[c][w];
      iPointer++;
      //Serial.println(iBuffer[w], HEX); 
    }
    iBuffer[iPointer] = 0;
    iPointer++; //+1 to add an empty column after each character
  
    //Serial.print("iPointer="); 
    //Serial.println(iPointer); 
  }
  iBuffer[iPointer] = 0;
  iPointer++; //+1 to add an empty column
  iBuffer[iPointer] = 0;
  iPointer++; //+1 to add an empty column
  iBuffer[iPointer] = 0;
  iPointer++; //+1 to add an empty column
  iBuffer[iPointer] = 0;
  iPointer++; //+1 to add an empty column
  iBuffer[iPointer] = 0;

  // Print iBuffer
  //Serial.println(sizeof(ifont[0])); 
  /*
  Serial.println("Debut buffer"); 
  for (w=0; w < sizeof(iBuffer); w++ ) {
    Serial.println(iBuffer[w], HEX); 
  }
  Serial.println("fin buffer"); 
  Serial.println(sizeof(iBuffer)); 
  */

  // Reset display matrix before shift bytes
  memmove(c1, ifont[0], sizeof(c1));

  w=0;
   //Shift bytes to the right and draw pixels
  for (w = 0; w < (iPointer-CHAR_WIDTH); w++) {
    /*
    Serial.println(c1[0], HEX); 
    Serial.println(c1[1], HEX); 
    Serial.println(c1[2], HEX); 
    Serial.println(c1[3], HEX); 
    Serial.println(c1[4], HEX); 
    Serial.print(w);
    Serial.println(" Next byte"); 
  */
    for (x = 0; x < CHAR_WIDTH; x++) {
      for (y = 0; y < CHAR_HEIGHT; y++) {
        if (c1[x] >> 2 & (1 << y)) {
          DrawPixel(colour, x, y, brightness);
        }
        strip.show();
      }
    }
    timeloop(100);
    strip.clear();
    //copy next set of 5 bytes shifted from next byte from the right
    memmove(&c1[0], &iBuffer[w], sizeof(c1));
  }
}

void FadeChar(uint32_t colour, char c) {
    DrawChar(colour, c, 80);
}

void FadeString(uint32_t colour, String s) {
  char buffer[s.length() + 1];
  s.toCharArray(buffer, s.length() + 1);

  for (int i = 0; i < s.length() + 1; i++)
    FadeChar(colour, buffer[i]);
}

void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void initTime(String timezone){
  struct tm timeinfo;

  //Serial.println("Setting up time");
  DisplayScrollingString(blue, "Setting up time",30,200);
  configTime(0, 0, "pool.ntp.org");    // First connect to NTP server, with 0 TZ offset
  if(!getLocalTime(&timeinfo)){
    //Serial.println("  Failed to obtain time");
    DisplayScrollingString(red, "Failed to obtain time",30,100);
    return;
  }
  //Serial.println("  Got the time from NTP");
  //DisplayScrollingString(green , "Got the time from NTP",30,100);
  // Now we can set the real timezone
  setTimezone(timezone);
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    DisplayScrollingString(red, "Failed to obtain time",30,100);
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}


void startWifi(){
  WiFi.begin(ssid, password);
  //Serial.println("Connecting Wifi");
  DisplayScrollingString(green, "Connecting Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    DisplayScrollingString(green, ".");
    //delay(500);
  }
  //Serial.print("Wifi RSSI=");
  //DisplayScrollingString(green, "Wifi RSSI=");
  //Serial.println(WiFi.RSSI());
  //long rssi = WiFi.RSSI();
  //DisplayScrollingString(green, String(rssi));
  //Serial.println("");
  //DisplayScrollingString(green, "Connected to ");
  //DisplayScrollingString(green, ssid);
  //DisplayScrollingString(green, "IP address: ");
  //Serial.println(WiFi.localIP());
  //FadeString(green, WiFi.localIP().toString().substring(7,20));
  
  DisplayScrollingString(green, WiFi.localIP().toString(), 30);
  //DisplayScrollingString(blue, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 20);
  DisplayScrollingString(green, "abcdefghijklmnopqrstuvwxyz", 20);
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
  Serial.printf("Setting time: %s", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

void setup(){
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  
  strip.begin();
  strip.setBrightness(20);
  strip.show(); // Initialize all pixels to 'off'

  startWifi();
  //FadeString(amber, "Sync NTP");
  initTime("CET-1CEST,M3.5.0,M10.5.0/3");   // Set for Paris/FR

  printLocalTime();
}

void loop() {
  // put your main code here, to run repeatedly:

  if(!getLocalTime(&timeinfo)){
    DisplayScrollingString(red, "Failed to obtain time 1");
    return;
  }
  
  //FadeString(green, (String(timeinfo.tm_hour).length() > 1 ? String(timeinfo.tm_hour) : "0" + String(timeinfo.tm_hour) )+ ":" + (String(timeinfo.tm_min).length() > 1 ? String(timeinfo.tm_min) : "0" + String(timeinfo.tm_min)));
  DisplayScrollingString(strip.Color(random(0,255), random(0,255), random(0,255)), 
        (String(timeinfo.tm_hour).length() > 1 ? String(timeinfo.tm_hour) : "0" + String(timeinfo.tm_hour) )+ ":" + (String(timeinfo.tm_min).length() > 1 ? String(timeinfo.tm_min) : "0" + String(timeinfo.tm_min)),
        20);

  //Serial.println(String(timeinfo.tm_min % 2));
  //Serial.println(String(i));
  if (timeinfo.tm_min % 2 != i){
    //printLocalTime();
    //FadeString(blue, String(timeinfo.tm_mday) + "/" + String(timeinfo.tm_mon+1)+ "/" + String(timeinfo.tm_year+1900) );
    DisplayScrollingString(blue, String(timeinfo.tm_mday) + "/" + String(timeinfo.tm_mon+1)+ "/" + String(timeinfo.tm_year+1900), 20 );
    //DisplayScrollingString(red, "Yves FUCHS", 100);
    //Serial.println(String(timeinfo.tm_mday) + "/" + String(timeinfo.tm_mon+1)+ "/" + String(timeinfo.tm_year+1900));
    i = timeinfo.tm_min % 2;
  }
}
