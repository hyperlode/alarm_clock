#include "GravityRtc.h"
#include "Wire.h"  

GravityRtc rtc;     //RTC Initialization
int pin_button_time_up = 3;
bool button_time_up_memory;
bool button_time_up_debounced;
long debounce_start;
bool debounced;

#define DEBOUNCE_TIME 50

void setup() {
  pinMode(pin_button_time_up,INPUT_PULLUP);
  button_time_up_debounced = digitalRead(pin_button_time_up);
  
  button_time_up_memory =false;
  
  Serial.begin(9600);
  
  
  rtc.setup();

  //Set the RTC time automatically: Calibrate RTC time by your computer time
  //rtc.adjustRtc(F(__DATE__), F(__TIME__)); 
  
  //Set the RTC time manually
  //rtc.adjustRtc(2017,6,19,1,12,7,0);  //Set time: 2017/6/19, Monday, 12:07:00
}


void refresh_button_time_up(){

  bool button_time_up_val = digitalRead(pin_button_time_up);

  if (button_time_up_val != button_time_up_memory){
    debounce_start = millis();
    Serial.println("debounce start");
    debounced = false;
  }

  if (millis() > debounce_start + DEBOUNCE_TIME && !debounced){
    button_time_up_debounced = button_time_up_val;
    Serial.println("button edge");
    //debounce_start = millis() + 100000;
    debounced = true;
  }
  
  button_time_up_memory = button_time_up_val;
  
}

void loop() {
  refresh_button_time_up();
  
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
