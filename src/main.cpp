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
#include <NeoPixelBrightnessBus.h> // instead of NeoPixelBus.h
#include <NeoPixelAnimator.h>
#include <ESP8266httpUpdate.h>     // Web Updater online
#include "DCF77.h"
#include "TimeLib.h"

// WS2812 definitions
  const uint16_t PixelCount = 40; // this example assumes 4 pixels, making it smaller will cause a failure
  const uint8_t PixelPin = 3;  // make sure to set this to the correct pin, ignored for Esp8266
  //const RgbColor CylonEyeColor(HtmlColor(0x7f0000));
  const RgbColor CylonEyeColor(0,0,255);
#define colorSaturation 255 // saturation of color constants
RgbColor blue(0, 0, colorSaturation);

NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
//NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod>* strip = NULL;
NeoPixelAnimator animations(2); // only ever need 2 animations

uint16_t lastPixel = 0; // track the eye position
int8_t moveDir = 1; // track the direction of movement

// uncomment one of the lines below to see the effects of
// changing the ease function on the movement animation
AnimEaseFunction moveEase =
//      NeoEase::Linear;
//      NeoEase::QuadraticInOut;
//      NeoEase::CubicInOut;
        NeoEase::QuarticInOut;
//      NeoEase::QuinticInOut;
//      NeoEase::SinusoidalInOut;
//      NeoEase::ExponentialInOut;
//      NeoEase::CircularInOut;

// NeoPixelBus TEST ENDE

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
bool updatePending;
uint8_t intensity;
bool secondBlink;

// DCF77
#define DCF_PIN 13                          // Connection pin to DCF 77 device
#define DCF_ENABLE 15                       // Ground pin of module
#define DCF_INTERRUPT 13                    // Interrupt number associated with pin
#define LED_PIN 2                           // DCF Signal visualization
#define DCF_SYNC_TIME 60                    // DCF Signal lost for more then 60 minutes
DCF77 DCF = DCF77(LED_PIN,DCF_PIN,DCF_INTERRUPT);   // Interrupt DCF77


// SUBROUTINES
//*************************************************************************************

// NEOPIXELBUS ANIMATION BEGIN
void colorWipe(uint8_t R, uint8_t G, uint8_t B) {
  RgbColor RGB(R,G,B);
    for(uint8_t i=0; i<PixelCount; i++) { // For each pixel...
      strip.SetPixelColor(i, RGB);
      strip.Show();
      delay(20);
    }
}

void FadeAll(uint8_t darkenBy)
{
    RgbColor color;
    for (uint16_t indexPixel = 0; indexPixel < 30; indexPixel++)
    {
        color = strip.GetPixelColor(indexPixel);
        color.Darken(darkenBy);
        strip.SetPixelColor(indexPixel, color);
    }
}

void FadeAnimUpdate(const AnimationParam& param)
{
    if (param.state == AnimationState_Completed)
    {
        FadeAll(10);
        animations.RestartAnimation(param.index);
    }
}

void MoveAnimUpdate(const AnimationParam& param)
{
    // apply the movement animation curve
    float progress = moveEase(param.progress);

    // use the curved progress to calculate the pixel to effect
    uint16_t nextPixel;
    if (moveDir > 0)
    {
        nextPixel = progress * 30;
    }
    else
    {
        nextPixel = (1.0f - progress) * 30;
    }

    // if progress moves fast enough, we may move more than
    // one pixel, so we update all between the calculated and
    // the last
    if (lastPixel != nextPixel)
    {
        for (uint16_t i = lastPixel + moveDir; i != nextPixel; i += moveDir)
        {
            strip.SetPixelColor(i, CylonEyeColor);
        }
    }
    strip.SetPixelColor(nextPixel, CylonEyeColor);

    lastPixel = nextPixel;

    if (param.state == AnimationState_Completed)
    {
        // reverse direction of movement
        moveDir *= -1;

        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);
    }
}

void SetupAnimations()
{
    // fade all pixels providing a tail that is longer the faster
    // the pixel moves.
    animations.StartAnimation(0, 5, FadeAnimUpdate);

    // take several seconds to move eye fron one side to the other
    animations.StartAnimation(1, 2000, MoveAnimUpdate);
}

// NEOPIXELBUS END

// function to crate HTML Colour
void array_to_string(byte array[], unsigned int len, char buffer[])
{
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
  
  // LEDs aus bis auf Doppelpunkt
  for(uint8_t i=0; i<12; i++) { // For each pixel...
          RgbColor AUS(0,0,0);
          strip.SetPixelColor(i, AUS);
        }
  for(uint8_t i=14; i<29; i++) { // For each pixel...
          RgbColor AUS(0,0,0);
          strip.SetPixelColor(i, AUS);
        }

  RgbColor RGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
  int hours = hour();
  
  byte upperHours = (hour() / 10) % 10;
  byte lowerHours = hour() % 10;
  byte upperMinutes = (minute() / 10) % 10;
  byte lowerMinutes = minute() % 10;

  switch (upperHours) {

        case 0: 
            strip.SetPixelColor(1, RGB);
            break;
        case 1:
            strip.SetPixelColor(2, RGB);
            break;
        case 2:
            strip.SetPixelColor(0, RGB);
            break;
  }

  switch (lowerHours) {
        case 0: 
            strip.SetPixelColor(7, RGB);
            break;
        case 1:
            strip.SetPixelColor(8, RGB);
            break;
        case 2:
            strip.SetPixelColor(6, RGB);
            break;
        case 3:
            strip.SetPixelColor(9, RGB);
            break;
        case 4:
            strip.SetPixelColor(5, RGB);
            break;
        case 5:
            strip.SetPixelColor(10, RGB);
            break;
        case 6:
            strip.SetPixelColor(4, RGB);
            break;
        case 7:
            strip.SetPixelColor(11, RGB);
            break;
        case 8:
            strip.SetPixelColor(3, RGB);
            break;
        case 9:
            strip.SetPixelColor(12, RGB);
            break;
    }

    switch (upperMinutes) {
        case 0: 
            strip.SetPixelColor(16, RGB);
            break;
        case 1:
            strip.SetPixelColor(17, RGB);
            break;
        case 2:
            strip.SetPixelColor(15, RGB);
            break;
        case 3:
            strip.SetPixelColor(18, RGB);
            break;
        case 4:
            strip.SetPixelColor(14, RGB);
            break;
        case 5:
            strip.SetPixelColor(19, RGB);
            break;
    }

    switch (lowerMinutes) {
        case 0: 
            strip.SetPixelColor(24, RGB);
            break;
        case 1:
            strip.SetPixelColor(25, RGB);
            break;
        case 2:
            strip.SetPixelColor(23, RGB);
            break;
        case 3:
            strip.SetPixelColor(26, RGB);
            break;
        case 4:
            strip.SetPixelColor(22, RGB);
            break;
        case 5:
            strip.SetPixelColor(27, RGB);
            break;
        case 6:
            strip.SetPixelColor(21, RGB);
            break;
        case 7:
            strip.SetPixelColor(28, RGB);
            break;
        case 8:
            strip.SetPixelColor(20, RGB);
            break;
        case 9:
            strip.SetPixelColor(29, RGB);
            break;
    }
    strip.Show();
}
//*** END DCF77 SUBS

void saveCallback() {
    intensity = configManager.data.matrixIntensity;
    strip.SetBrightness(intensity);
    displayTime();
}

void syncTime() {
  if (timeSync.isSynced())
    {
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
  pinMode(DCF_ENABLE,OUTPUT);                  // Ground pin of DCF module
  digitalWrite(DCF_ENABLE,LOW);
  pinMode(BUILTIN_LED,OUTPUT);
  digitalWrite(BUILTIN_LED,HIGH);


  // NeoPixelBus SETUP
  // this resets all the neopixels to an off state
    strip.Begin();
    uint8_t intensity = configManager.data.matrixIntensity;
    strip.SetBrightness(intensity);
    SetupAnimations();
    strip.Show();
    colorWipe(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
    delay(1000);
    colorWipe(0,0,0);

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

    //tasks
    if (taskA.previous == 0 || (millis() - taskA.previous > taskA.rate)) {
        taskA.previous = millis();

      seconds++;

      isCaptive = WiFiManager.isCaptivePortal();

        int rssi = 0;
        rssi = WiFi.RSSI();
        sprintf(dash.data.Wifi_RSSI, "%ld", rssi) ;
        dash.data.WLAN_RSSI = WiFi.RSSI();
      
      
  // update pending
    if (updatePending) {
      updatePending = false;
      //updateFirmware();
    }

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
  
    // Blink separator
      if (dash.data.DCF77_Sync && configManager.data.DCF77 ) {
        if (secondBlink) {
          secondBlink = false;
          RgbColor AUS(0,0,0);
          strip.SetPixelColor(13, AUS);
        } else {
          secondBlink = true;
          RgbColor RGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
          strip.SetPixelColor(13, RGB);
        }
      } 
      if (!configManager.data.DCF77) {
        if (secondBlink) {
          secondBlink = false;
          RgbColor AUS(0,0,0);
          strip.SetPixelColor(13, AUS);
        } else {
          secondBlink = true;
          RgbColor RGB(configManager.data.ledColour[0],configManager.data.ledColour[1],configManager.data.ledColour[2]);
          strip.SetPixelColor(13, RGB);
        }
      }
    
  strip.Show();
  digitalClockDisplay();      // printout time info on serial
  displayTime();              // printout time on display
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
      if (!dash.data.DCF77_Sync) {
        if (signal) {
          RgbColor RGB(255,0,0);          // red
          strip.SetPixelColor(13, RGB);
        } else {
          RgbColor AUS(0,0,0);
          strip.SetPixelColor(13, AUS);
        }
      strip.Show();
      }
    }

  }
}

