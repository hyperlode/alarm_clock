
#ifndef Button_h
#define Button_h

#include "Arduino.h"
//#include "BinaryInput.cpp"

#define DEBOUNCE_TIME 50

//class Button: public BinaryInput{
class Button{
   public:
    void init(int pin); 
    void refresh();
    
    
   private:
    int pin;
    bool memory;
    bool val_debounced;
    long debounce_start;
    bool is_debounced;
  		
};


#endif
