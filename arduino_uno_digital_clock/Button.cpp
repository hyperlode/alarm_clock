#include "Button.h"

Button::Button(){
  
}

void Button::init(int pin){
  this->pin = pin;
  pinMode(this->pin,INPUT_PULLUP); // pull down resistors are not available...
  //val_debounced = digitalRead(this->pin);
  setValue(digitalRead(this->pin));
  is_debounced= true;
  memory =false;
}

long Button::getLastStateChangeMillis(){
    // can be used for long button pressed, check how long a button is in a certain state.
    return debounce_start;
}

void Button::refresh(){
  refreshEdges(); 
  bool val = digitalRead(this->pin);

  if (val != memory){
    debounce_start = millis();
    //Serial.println("debounce start");
    is_debounced= false;
  }

  if (millis() > debounce_start + DEBOUNCE_TIME && !is_debounced){
    //val_debounced = val;
    setValue(val);
    //Serial.println("button edge");
    //debounce_start = millis() + 100000;
    is_debounced= true;
  }
  
  memory = val;
  
}
