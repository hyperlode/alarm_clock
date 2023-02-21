
#ifndef Button_h
#define Button_h

#include "Arduino.h"
#include "BinaryInput.h"

// Simple button connected straight to an arduino uno pin.
// pulled up. So, connect the push button to ground at NO(normal open).
// use the binaryinput class methods to handle the button states.

#define DEBOUNCE_TIME 50

#define BUTTON_LONG_PRESS_INITIAL_DELAY_MILLIS 300
#define BUTTON_LONG_PRESS_FAKE_EDGE_PERIOD_MILLIS 100

class Button : public BinaryInput
{
    // class Button{
public:
    Button();
    void init(int pin);
    void refresh();
    long getLastStateChangeMillis();
    bool isPressed();
    bool isPressedEdge();
    bool isUnpressedEdge();
    bool getLongPressPeriodicalEdge();
    int16_t getLongPressCount();
    void setLongPressCount(uint16_t fakeLongPressCountPeriods);

    unsigned long longPressStartMillis;
    unsigned long longPressPeriodicalStartMillis;
    int16_t longPressEdgeCount;
    bool long_press_edge_detected = false;

private:
    int pin;
    bool button_value_memory;
    bool button_longpress_edge_memory;
    //    bool val_debounced;
    long debounce_start;
    bool is_debounced;
};

#endif
