#include "GravityRtc.h"
#include "Wire.h"  
#include "Button.h"
#include "DisplayDigitsHandler5Digits.h"
#include "LedMultiplexer5x8.h"

#define DISPLAY_IS_COMMON_ANODE true //check led displays both displays should be of same type   //also set in SevSeg5Digits.h : MODEISCOMMONANODE

#define PIN_DUMMY 66
#define PIN_DUMMY_2 22 // randomly chosen. I've had it set to 67, and at some point, multiple segments were lit up. This STILL is C hey, it's gonna chug on forever!

#define PIN_DISPLAY_DIGIT_0 11 // swapped
#define PIN_DISPLAY_DIGIT_1 10
#define PIN_DISPLAY_DIGIT_2 9
#define PIN_DISPLAY_DIGIT_3 6 // swapped!
#define PIN_DISPLAY_DIGIT_BUTTON_LIGHTS A0

#define PIN_DISPLAY_SEGMENT_A 12  
#define PIN_DISPLAY_SEGMENT_B 8
#define PIN_DISPLAY_SEGMENT_C 5
#define PIN_DISPLAY_SEGMENT_D 3
#define PIN_DISPLAY_SEGMENT_E 2
#define PIN_DISPLAY_SEGMENT_F 13 // swapped!
#define PIN_DISPLAY_SEGMENT_DP 4 
#define PIN_DISPLAY_SEGMENT_G 7 // swapped!

#define PIN_BUTTON_TIME_UP A3
#define TIME_UPDATE_DELAY 1000


DisplayManagement visualsManager;
LedMultiplexer5x8 ledDisplay;
GravityRtc rtc;     //RTC Initialization

Button button_time_up;

long nextTimeUpdateMillis;

bool displayHourMinutesElseMinutesSeconds;
uint8_t brightness;

void setup() {
  
  Serial.begin(9600);
  
  
  rtc.setup();

  //Set the RTC time automatically: Calibrate RTC time by your computer time
  //rtc.adjustRtc(F(__DATE__), F(__TIME__)); 
  
  //Set the RTC time manually
  //rtc.adjustRtc(2017,6,19,1,12,7,0);  //Set time: 2017/6/19, Monday, 12:07:00


  button_time_up.init(PIN_BUTTON_TIME_UP);


  ledDisplay.Begin(DISPLAY_IS_COMMON_ANODE, PIN_DISPLAY_DIGIT_0, PIN_DISPLAY_DIGIT_1, PIN_DISPLAY_DIGIT_2, PIN_DISPLAY_DIGIT_3, PIN_DISPLAY_DIGIT_BUTTON_LIGHTS, PIN_DISPLAY_SEGMENT_A, PIN_DISPLAY_SEGMENT_B, PIN_DISPLAY_SEGMENT_C, PIN_DISPLAY_SEGMENT_D, PIN_DISPLAY_SEGMENT_E, PIN_DISPLAY_SEGMENT_F, PIN_DISPLAY_SEGMENT_G, PIN_DISPLAY_SEGMENT_DP);
  visualsManager.setMultiplexerBuffer(ledDisplay.getDigits());

  nextTimeUpdateMillis = millis();
  displayHourMinutesElseMinutesSeconds = true;
  toggleBrightness(true);
}

void toggleBrightness(bool init){
    brightness++;
    if (init){
      brightness = 1;
    }
    
    if (brightness>7){
      brightness = 0; // zero is dark. but, maybe you want that...
    }
    ledDisplay.setBrightness(brightness*brightness*brightness, false);
}

void loop() {
  
  button_time_up.refresh();
  rtc.read();

  if(!button_time_up.getValue()){
    delay(1);
    
  }
  
  if (button_time_up.getEdgeDown()){
    toggleBrightness(false);
//    visualsManager.setDecimalPointsToDisplay(0xFF);
    nextTimeUpdateMillis = millis(); // make sure to refresh display
    
    if (button_time_up.getToggleValue()){
      Serial.println("seconds minute ON");
      displayHourMinutesElseMinutesSeconds = false;
      
    }else{
      Serial.println("OFF");
      displayHourMinutesElseMinutesSeconds = true;
    }
  }
  visualsManager.refresh();
  ledDisplay.refresh();

  if (millis() > nextTimeUpdateMillis ){

    if (rtc.second % 2){
      visualsManager.setDecimalPointToDisplay(true,1);  
    }else{
      visualsManager.setDecimalPointToDisplay(false,1);  
    }
    
    int16_t timeAsNumber;
    nextTimeUpdateMillis = millis() + TIME_UPDATE_DELAY;
    if (displayHourMinutesElseMinutesSeconds){
      timeAsNumber = 100 * ((int16_t)rtc.hour)  + (int16_t)rtc.minute;
    }else{
      //timeAsNumber = 100 * ((int16_t)rtc.minute)  + (int16_t)rtc.second;
      timeAsNumber = (int16_t)rtc.second;     
    }
    
    visualsManager.setNumberToDisplay(timeAsNumber,false);
  }
  
//  rtc.read();
//  //*************************Time********************************
//  Serial.print("   Year = ");//year
//  Serial.print(rtc.year);
//  Serial.print("   Month = ");//month
//  Serial.print(rtc.month);
//  Serial.print("   Day = ");//day
//  Serial.print(rtc.day);
//  Serial.print("   Week = ");//week
//  Serial.print(rtc.week);
//  Serial.print("   Hour = ");//hour
//  Serial.print(rtc.hour);
//  Serial.print("   Minute = ");//minute
//  Serial.print(rtc.minute);
//  Serial.print("   Second = ");//second
//  Serial.println(rtc.second);
//  delay(1000);


}
