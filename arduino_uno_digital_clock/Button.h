
#ifndef Button_h
#define Button_h

#include "Arduino.h"
#include "BinaryInput.h"

// Simple button connected straight to an arduino uno pin.
// pulled up. So, connect the push button to ground at NO(normal open).
// use the binaryinput class methods to handle the button states.


#define DEBOUNCE_TIME 50

class Button: public BinaryInput{
//class Button{
  public:
    Button();
    void init(int pin); 
    void refresh();
    long getLastStateChangeMillis();
    
    
  private:
    int pin;
    bool memory;
//    bool val_debounced;
    long debounce_start;
    bool is_debounced;
  		
};


#endif
