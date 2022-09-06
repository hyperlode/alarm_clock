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
#define PIN_BUTTON_TIME_DOWN A2
#define PIN_BUTTON_MENU A1
#define TIME_UPDATE_DELAY 1000


DisplayManagement visualsManager;
LedMultiplexer5x8 ledDisplay;
GravityRtc rtc;     //RTC Initialization

Button button_time_up;
Button button_time_down;
Button button_menu;

long nextTimeUpdateMillis;

bool displayHourMinutesElseMinutesSeconds;
uint8_t brightness;


enum clock_state : uint8_t
{
  state_display_time = 0,
  state_set_time
};
clock_state clock_state;

void setup() {

  Serial.begin(9600);


  rtc.setup();

  //Set the RTC time automatically: Calibrate RTC time by your computer time
  //rtc.adjustRtc(F(__DATE__), F(__TIME__));

  //Set the RTC time manually
  //rtc.adjustRtc(2017,6,19,1,12,7,0);  //Set time: 2017/6/19, Monday, 12:07:00


  button_time_up.init(PIN_BUTTON_TIME_UP);
  button_time_down.init(PIN_BUTTON_TIME_DOWN);
  button_menu.init(PIN_BUTTON_MENU);

  ledDisplay.Begin(DISPLAY_IS_COMMON_ANODE, PIN_DISPLAY_DIGIT_0, PIN_DISPLAY_DIGIT_1, PIN_DISPLAY_DIGIT_2, PIN_DISPLAY_DIGIT_3, PIN_DISPLAY_DIGIT_BUTTON_LIGHTS, PIN_DISPLAY_SEGMENT_A, PIN_DISPLAY_SEGMENT_B, PIN_DISPLAY_SEGMENT_C, PIN_DISPLAY_SEGMENT_D, PIN_DISPLAY_SEGMENT_E, PIN_DISPLAY_SEGMENT_F, PIN_DISPLAY_SEGMENT_G, PIN_DISPLAY_SEGMENT_DP);
  visualsManager.setMultiplexerBuffer(ledDisplay.getDigits());

  nextTimeUpdateMillis = millis();
  displayHourMinutesElseMinutesSeconds = true;
  cycleBrightness(true);
  clock_state = state_display_time;
}

void cycleBrightness(bool init) {

  uint8_t brightness_settings [] = {1, 10, 80, 255, 0};
  brightness++;
  if (init) {
    brightness = 3;
  }

  if (brightness > 4) {
    brightness = 0; // zero is dark. but, maybe you want that... e.g. alarm active without display showing.
  }
  //ledDisplay.setBrightness(brightness*brightness*brightness*brightness, false);
  ledDisplay.setBrightness(brightness_settings[brightness], false);
}



void display_time() {

  if (millis() > nextTimeUpdateMillis ) {
    nextTimeUpdateMillis = millis() + TIME_UPDATE_DELAY;

    divider_colon_to_display();

    int16_t timeAsNumber;

    if (displayHourMinutesElseMinutesSeconds) {
      hour_minutes_to_display();
    } else {
      seconds_to_display();
    }


  }

  if (button_time_up.getEdgeDown()) {

    //    visualsManager.setDecimalPointsToDisplay(0xFF);
    nextTimeUpdateMillis = millis(); // make sure to refresh display

    if (button_time_up.getToggleValue()) {
      Serial.println("seconds minute ON");
      displayHourMinutesElseMinutesSeconds = false;

    } else {
      Serial.println("OFF");
      displayHourMinutesElseMinutesSeconds = true;
    }
  }

  if (button_time_down.getEdgeDown()) {
    cycleBrightness(false);

  }

}

void divider_colon_to_display(){
  //rtc.read();
  if (rtc.second % 2) {
      visualsManager.setDecimalPointToDisplay(true, 1);
    } else {
      visualsManager.setDecimalPointToDisplay(false, 1);
    }
}
void seconds_to_display() {
  int16_t timeAsNumber;
  rtc.read();
  timeAsNumber = (int16_t)rtc.second;
  visualsManager.setNumberToDisplay(timeAsNumber, false);
  if (timeAsNumber < 10) {
    visualsManager.setCharToDisplay('0', 2); // leading zero if less than ten seconds
  }
}

void hour_minutes_to_display() {
  int16_t timeAsNumber;
  rtc.read();
  timeAsNumber = 100 * ((int16_t)rtc.hour)  + (int16_t)rtc.minute;
  visualsManager.setNumberToDisplay(timeAsNumber, false);
  if (timeAsNumber < 100) {
    visualsManager.setCharToDisplay('0', 1); // leading zero at midnight ("0" hour)
  }
}

void set_time() {

}

void refresh_clock_state() {
  switch (clock_state)
  {
    case state_display_time:
      {
        display_time();
      }
      break;
    case state_set_time:
      {
        set_time();
      }
      break;
    default:
      break;
  }
}


void loop() {

  button_time_up.refresh();
  button_time_down.refresh();
  button_menu.refresh();

  refresh_clock_state();

  if (!button_menu.getValue()) {
    delay(10);
  }




  visualsManager.refresh();
  ledDisplay.refresh();


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
