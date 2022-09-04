#include "Button.h"

void Button::init(int pin){
  this->pin = pin;
  pinMode(this->pin,INPUT_PULLUP);
  val_debounced = digitalRead(this->pin);
  is_debounced= true;
  memory =false;
}

void Button::refresh(){
  bool val = digitalRead(this->pin);

  if (val != memory){
    debounce_start = millis();
    Serial.println("debounce start");
    is_debounced= false;
  }

  if (millis() > debounce_start + DEBOUNCE_TIME && !is_debounced){
    val_debounced = val;
    Serial.println("button edge");
    //debounce_start = millis() + 100000;
    is_debounced= true;
  }
  
  memory = val;
}
