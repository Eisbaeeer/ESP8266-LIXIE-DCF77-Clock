#include <Arduino.h>
#include <Wire.h>
#include "LittleFS.h"
#include "WiFiManager.h"
#include "webServer.h"
#include "updater.h"
#include "fetch.h"
#include "configManager.h"
#include "dashboard.h"
#include "timeSync.h"
#include <TimeLib.h>
#include <TZ.h>
#include <ESP8266httpUpdate.h>     // Web Updater online
#include "DCF77.h"
#include "TimeLib.h"
#include <FastLED.h>

// FASTLED WS2812 definitions
#define NUM_LEDS 30
#define DATA_PIN 3  
// Define the array of leds
CRGB leds[NUM_LEDS];
// Colorpalette
CRGBPalette16 currentPalette;
TBlendType    currentBlending;
uint8_t colorIndex;
uint8_t startIndex = 0;

// Update client
WiFiClient updateclient;

// Tasks
struct task
{    
    unsigned long rate;
    unsigned long previous;
};

task taskA = { .rate = 1000, .previous = 0 };     // 1 second
task taskB = { .rate = 60000, .previous = 0 };    // 1 minute
task taskC = { .rate = 50, .previous = 0 };      // 50 ms

// Global definitions
//*************************************************************************************
#define BUILTIN_LED 2
int seconds;
bool isCaptive;
uint8_t intensity;
bool secondBlink;

// DCF77
#define DCF_PIN 14                          // Connection pin to DCF 77 device
#define DCF_ENABLE 13                       // 3.3V for DCF Module
#define DCF_GND 12                          // GND for DCF Module
#define DCF_INTERRUPT 14                    // Interrupt number associated with pin
#define LED_PIN 2                           // DCF Signal visualization
#define DCF_SYNC_TIME 1440                  // DCF Signal lost for more then 60 minutes
DCF77 DCF = DCF77(LED_PIN,DCF_PIN,DCF_INTERRUPT);   // Interrupt DCF77


// SUBROUTINES
//*************************************************************************************

// function to crate HTML Colour
void array_to_string(byte array[], unsigned int len, char buffer[]) {
    for (unsigned int i = 0; i < len; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
        buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
    }
    buffer[len*2] = '\0';
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(F(" "));
  Serial.print(day());
  Serial.print(F(" "));
  Serial.print(month());
  Serial.print(F(" "));
  Serial.print(year()); 
  Serial.println(); 
  String strtime = String(hour());
          strtime += ":";
          strtime += String(minute());
          strtime += ":";
          strtime += String(second());
    int str_len = strtime.length() + 1;
    strtime.toCharArray(dash.data.Time,str_len);

    String strdate = String(day());
          strdate += ".";
          strdate += String(month());
          strdate += ".";
          strdate += String(year());
    str_len = strdate.length() + 1;
    strdate.toCharArray(dash.data.Date,str_len);
  
}

void displayTime() {
  // Display time on LIXIE
  
  // LEDs aus 
  for(uint8_t i=0; i<30; i++) { // For each pixel...
          leds[i] = CRGB::Black;
        }

  int hours = hour();
  
  byte upperHours = (hour() / 10) % 10;
  byte lowerHours = hour() % 10;
  byte upperMinutes = (minute() / 10) % 10;
  byte lowerMinutes = minute() % 10;

  switch (upperHours) {

        case 0: 
            if (!configManager.data.rainbow) { 
              leds[1].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[1] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 1:
            if (!configManager.data.rainbow) { 
              leds[2].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[2] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 2:
            if (!configManager.data.rainbow) { 
              leds[0].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[0] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
  }

  switch (lowerHours) {
        case 0:
            if (!configManager.data.rainbow) { 
              leds[7].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[7] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            } 
            break;
        case 1:
            if (!configManager.data.rainbow) { 
              leds[8].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[8] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 2:
            if (!configManager.data.rainbow) { 
              leds[6].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[6] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 3:
            if (!configManager.data.rainbow) { 
              leds[9].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[9] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 4:
            if (!configManager.data.rainbow) { 
              leds[5].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[5] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 5:
            if (!configManager.data.rainbow) { 
              leds[10].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[10] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 6:
            if (!configManager.data.rainbow) { 
              leds[4].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[4] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 7:
            if (!configManager.data.rainbow) { 
              leds[11].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[11] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 8:
            if (!configManager.data.rainbow) { 
              leds[3].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[3] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 9:
            if (!configManager.data.rainbow) { 
              leds[12].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[12] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
    }

    switch (upperMinutes) {
        case 0: 
            if (!configManager.data.rainbow) { 
              leds[16].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[16] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 1:
            if (!configManager.data.rainbow) { 
              leds[17].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[17] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 2:
            if (!configManager.data.rainbow) { 
              leds[15].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[15] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 3:
            if (!configManager.data.rainbow) { 
              leds[18].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[18] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 4:
            if (!configManager.data.rainbow) { 
              leds[14].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[14] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 5:
            if (!configManager.data.rainbow) { 
              leds[19].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[19] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
    }

    switch (lowerMinutes) {
        case 0: 
            if (!configManager.data.rainbow) { 
              leds[24].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[24] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 1:
            if (!configManager.data.rainbow) { 
              leds[25].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[25] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 2:
            if (!configManager.data.rainbow) { 
              leds[23].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[23] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 3:
            if (!configManager.data.rainbow) { 
              leds[26].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[26] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 4:
            if (!configManager.data.rainbow) { 
              leds[22].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[22] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 5:
            if (!configManager.data.rainbow) { 
              leds[27].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[27] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 6:
            if (!configManager.data.rainbow) { 
              leds[21].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[21] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 7:
            if (!configManager.data.rainbow) { 
              leds[28].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[28] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 8:
            if (!configManager.data.rainbow) { 
              leds[20].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[20] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
        case 9:
            if (!configManager.data.rainbow) { 
              leds[29].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[29] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
            break;
    }
    // Blinking indicator control
    /***
     / Blink separator
      if (dash.data.DCF77_Sync && configManager.data.DCF77Indicator && configManager.data.DCF77) {
        if (secondBlink) {
          secondBlink = false;
          leds[13] = CRGB::Black;
        } else {
          secondBlink = true;
          if (!configManager.data.rainbow) { 
              leds[13].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configsecondBlinkManager.data.ledColour[2]);
            } else {
              leds[13] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
        }
      } 
      if (!configManager.data.DCF77Indicator) {
        if (secondBlink) {
          secondBlink = false;
          leds[13] = CRGB::Black;
        } else {
          secondBlink = true;
          if (!configManager.data.rainbow) { 
              leds[13].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[13] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
        }
      }
    
  FastLED.show();
    */
    if (secondBlink) {
      if (dash.data.DCF77_Sync && configManager.data.DCF77Indicator && configManager.data.DCF77) {
         if (!configManager.data.rainbow) { 
              leds[13].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[13] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
      } else {
        leds[13] = CRGB::Black;
      }
    if (!configManager.data.DCF77Indicator) {
      if (!configManager.data.rainbow) { 
              leds[13].setRGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
            } else {
              leds[13] = ColorFromPalette( RainbowColors_p, colorIndex, configManager.data.matrixIntensity, LINEARBLEND);
            }
      } else {
        leds[13] = CRGB::Black;
      }
    } 
    
    FastLED.setBrightness(configManager.data.matrixIntensity);
    FastLED.show();
}
//*** END DCF77 SUBS

void saveCallback() {
    intensity = configManager.data.matrixIntensity;
    FastLED.setBrightness(configManager.data.matrixIntensity);
    if (configManager.data.rainbow) {
        currentPalette = RainbowColors_p;
        currentBlending = LINEARBLEND;  
    }
    displayTime();
}

void syncTime() {
  if (timeSync.isSynced()) {
        dash.data.NTP_Sync = true;
        time_t now = time(nullptr);
        Serial.print(PSTR("[INFO] Current time in "));
        Serial.print(configManager.data.Time_Zone);
        Serial.print(PSTR(": "));
        Serial.println(asctime(localtime(&now)));
       
        struct tm * timeinfo;
        time(&now);
        timeinfo = localtime(&now);
        int hr = timeinfo->tm_hour;
        int mi = timeinfo->tm_min;
        int se = timeinfo->tm_sec;
        int da = timeinfo->tm_mday;
        int mo = timeinfo->tm_mon + 1;
        int ye = timeinfo->tm_year + 1900;

        setTime(hr,mi,se,da,mo,ye); // Another way to set the time
    }
    else 
    {
      dash.data.NTP_Sync = false;
        Serial.println(F("[ERROR] Timeout while receiving the time"));
    }
}

void setup() {
  Serial.begin(115200);

  LittleFS.begin();
  GUI.begin();
  configManager.begin();
  WiFiManager.begin(configManager.data.projectName);
  configManager.setConfigSaveCallback(saveCallback);
  dash.begin(500);

    // WiFi
  WiFi.hostname(configManager.data.wifi_hostname);
  WiFi.begin();
  WiFi.setAutoReconnect(true);
  
  //Onboard LED & analog port, etc
  pinMode(DCF_ENABLE,OUTPUT);                   // 3.3V pin of DCF module
  digitalWrite(DCF_ENABLE,HIGH);                // Take it high = 3.3V
  pinMode(DCF_GND,OUTPUT);                      // GND pin of DCF module
  digitalWrite(DCF_GND,LOW);                    // Take it LOW = 0V
  pinMode(BUILTIN_LED,OUTPUT);
  digitalWrite(BUILTIN_LED,HIGH);


  // FastLED SETUP
  // Uncomment/edit one of the following lines for your leds arrangement.
    // ## Clockless types ##
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed
    // FastLED.addLeds<SM16703, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<TM1829, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<TM1812, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<TM1809, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<TM1804, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<TM1803, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<UCS1903, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<UCS1903B, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<UCS1904, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<UCS2903, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical
    // FastLED.addLeds<WS2852, DATA_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical
    // FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical
    // FastLED.addLeds<GS1903, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<SK6812, DATA_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical
    // FastLED.addLeds<SK6822, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<APA106, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<PL9823, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<SK6822, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<WS2813, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<APA104, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<WS2811_400, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<GE8822, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<GW6205, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<GW6205_400, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<LPD1886, DATA_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<LPD1886_8BIT, DATA_PIN, RGB>(leds, NUM_LEDS);
    // ## Clocked (SPI) types ##
    // FastLED.addLeds<LPD6803, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical
    // FastLED.addLeds<LPD8806, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical
    // FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<WS2803, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<SM16716, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
    // FastLED.addLeds<P9813, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);  // BGR ordering is typical
    // FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);  // BGR ordering is typical
    // FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);  // BGR ordering is typical
    // FastLED.addLeds<SK9822, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);  // BGR ordering is typical

        leds[0] = CRGB::Blue;
        FastLED.show();
        delay(500);
        // Now turn the LED off, then pause
        leds[0] = CRGB::Black;
        FastLED.show();
        delay(500);
        
        // Set brightness
        FastLED.setBrightness(configManager.data.matrixIntensity);
        currentPalette = RainbowColors_p;
        currentBlending = LINEARBLEND;   
    

    // DCF77
    DCF.Start();
    Serial.println("Waiting for DCF77 time ... ");
    Serial.println("It will take at least 2 minutes before a first time update.");
    dash.data.DCF77_Sync = false;

  // Timesync
  timeSync.begin(configManager.data.Time_Zone);
  //Wait for connection
  timeSync.waitForSyncResult(10000);
  syncTime();
   
}

void loop() {

 // framework things
  yield();
  WiFiManager.loop();
  updater.loop();
  configManager.loop();
  dash.loop();

  // Color Effekts
  startIndex = startIndex + 2; /* motion speed higher=longer*/
  
  if (seconds != second()) {
    seconds = second();
    secondBlink = !secondBlink;
    }


  // Tasks routines
    //tasks
    if (taskA.previous == 0 || (millis() - taskA.previous > taskA.rate)) {
        taskA.previous = millis();

  isCaptive = WiFiManager.isCaptivePortal();
        int rssi = 0;
        rssi = WiFi.RSSI();
        sprintf(dash.data.Wifi_RSSI, "%ld", rssi) ;
        dash.data.WLAN_RSSI = WiFi.RSSI();
      
  // DCF77
  if (configManager.data.DCF77) {
    time_t DCFtime = DCF.getTime(); // Check if new DCF77 time is available
    if (DCFtime!=0) {
      Serial.println(F("Time is updated"));
      setTime(DCFtime);
      dash.data.Last_Sync = 0;
      dash.data.DCF77_Sync = true;
    }
  }
  
    // upper color index every second
    colorIndex += 1;

  digitalClockDisplay();      // printout time info on serial
} // TASK A END

  // TASK B
    if (taskB.previous == 0 || (millis() - taskB.previous > taskB.rate)) {
        taskB.previous = millis();

  syncTime();
   
    // DCF77 check if DCF signal is OK
    if (configManager.data.DCF77 == true) {
      dash.data.Last_Sync++;
      Serial.print(F("Minutes since last DCF Sync: "));
      Serial.println(dash.data.Last_Sync);
      if (dash.data.Last_Sync > DCF_SYNC_TIME) {
      dash.data.DCF77_Sync = false;
      }
    }
  }

  //tasks C
    if (taskC.previous == 0 || (millis() - taskC.previous > taskC.rate)) {
        taskC.previous = millis();
    
    // DCF77
    if (configManager.data.DCF77 == true) {
      bool signal = DCF.getSignal(); // Get DCF77 signal for LED
      if (signal) {
        digitalWrite(BUILTIN_LED,LOW);
      } else {
        digitalWrite(BUILTIN_LED,HIGH);
      }

      // show signal on stripe during sync
      if (!dash.data.DCF77_Sync && configManager.data.DCF77Indicator) {
        if (signal) {
          leds[13] = CRGB::Red;          // red
        } else {
          leds[13] = CRGB::Black;
        }
      FastLED.show();
      }
    }

    // Print out clock to LED´s
    displayTime();              // printout time on display

  }
}
