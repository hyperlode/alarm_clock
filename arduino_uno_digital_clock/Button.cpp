#include "Button.h"

Button::Button(){
  
}

void Button::init(int pin){
  this->pin = pin;
  pinMode(this->pin,INPUT_PULLUP); // pull down resistors are not available...
  //val_debounced = digitalRead(this->pin);
  setValue(digitalRead(this->pin));
  is_debounced= true;
  button_value_memory =false;
  button_longpress_edge_memory = false;
}


bool Button::isPressed(){
    return !getValue();
}
bool Button::isPressedEdge(){
    return getEdgeDown();
}

long Button::getLastStateChangeMillis(){
    // can be used for long button pressed, check how long a button is in a certain state.
    return debounce_start;
}

bool Button::getLongPressPeriodicalEdge(){
  bool edge_detected;
  edge_detected = false;
  if (isPressed()){
    if (millis() > (longPressStartMillis + BUTTON_LONG_PRESS_INITIAL_DELAY_MILLIS )){
        bool delay_expired = millis() > (longPressStartMillis + BUTTON_LONG_PRESS_INITIAL_DELAY_MILLIS + longPressEdgeCount* BUTTON_LONG_PRESS_FAKE_EDGE_PERIOD_MILLIS);
        if (delay_expired && button_longpress_edge_memory )
            {
            longPressEdgeCount++;
            //Serial.println("long p[ress edge");
            edge_detected = true;
        }
        button_longpress_edge_memory = delay_expired;
    }
  }
  return edge_detected;
}

void Button::refresh(){
  refreshEdges(); 
  bool val = digitalRead(this->pin);

  if (val != button_value_memory){
    debounce_start = millis();
    //Serial.println("debounce start");
    is_debounced= false;
  }

  if ((millis() > (debounce_start + DEBOUNCE_TIME)) && !is_debounced){
    setValue(val);
    is_debounced= true;

    longPressStartMillis = millis();
    longPressEdgeCount = 0;
  }
  

  
  button_value_memory = val;
  
}
