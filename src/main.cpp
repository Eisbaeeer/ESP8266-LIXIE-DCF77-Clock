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

// Global definitions
//*************************************************************************************
#define BUILTIN_LED 2             // On board LED
int seconds;
bool isCaptive;
bool updatePending;
bool reboot;
uint8_t intensity;

// DCF77
#define DCF_PIN 13 // Connection pin to DCF 77 device
#define DCF_ENABLE 15 // Ground pin of module
#define DCF_INTERRUPT 13  // Interrupt number associated with pin
DCF77 DCF = DCF77(DCF_PIN,DCF_INTERRUPT);



// SUBROUTINES
//*************************************************************************************

// NEOPIXELBUS ANIMATION BEGIN
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

void saveCallback() {
    intensity = configManager.data.matrixIntensity;
    strip.SetBrightness(intensity);
}

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
//*** END DCF77 SUBS

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
  
  // Timesync
  timeSync.begin(configManager.data.Time_Zone);

  //Onboard LED & analog port, etc
  pinMode(BUILTIN_LED,OUTPUT);                 // LED
  digitalWrite(BUILTIN_LED,1);                 // LED off
  pinMode(DCF_ENABLE,OUTPUT);                  // Ground pin of DCF module
  digitalWrite(DCF_ENABLE,LOW);


  // NeoPixelBus SETUP
  // this resets all the neopixels to an off state
    strip.Begin();
    uint8_t intensity = configManager.data.matrixIntensity;
    strip.SetBrightness(intensity);
    SetupAnimations();
    //colorWipe(0,0,255);
    strip.Show();

    // DCF77
  DCF.Start();
  Serial.println("Waiting for DCF77 time ... ");
  Serial.println("It will take at least 2 minutes before a first time update.");
  dash.data.DCF77_Sync = false;
   
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
     
      // Check if reboot flag is set
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

  // reboot pending
    if (reboot) {
      reboot = false;
      ESP.reset();
    }

  // DCF77
  time_t DCFtime = DCF.getTime(); // Check if new DCF77 time is available
  if (DCFtime!=0) {
    Serial.println("Time is updated");
    setTime(DCFtime);
    dash.data.DCF77_Sync = true;
  }
  digitalClockDisplay();  
    
}

    if (taskB.previous == 0 || (millis() - taskB.previous > taskB.rate)) {
        taskB.previous = millis();
    
  // Reboot from dashboard 
    if (dash.data.Reboot) {
        dash.data.Reboot = false;
        dash.loop();
        reboot = true;
    }
  }
}

