#include "Button.h"

Button::Button()
{
}

void Button::init(int pin)
{
    this->pin = pin;
    pinMode(this->pin, INPUT_PULLUP); // pull down resistors are not available...
    // val_debounced = digitalRead(this->pin);
    setValue(digitalRead(this->pin));
    is_debounced = true;
    button_value_memory = false;
    long_press_edge_detected = false;
    //   button_longpress_edge_memory = false;
}

bool Button::isPressed()
{
    return !getValue();
}
bool Button::isPressedEdge()
{
    return getEdgeDown();
}
bool Button::isUnpressedEdge()
{
    return getEdgeUp();
}

long Button::getLastStateChangeMillis()
{
    // can be used for long button pressed, check how long a button is in a certain state.
    return debounce_start;
}

int16_t Button::getLongPressCount()
{
    return longPressEdgeCount;
}

void Button::setLongPressCount(uint16_t fakeLongPressCountPeriods)
{
    longPressEdgeCount = fakeLongPressCountPeriods;
}

bool Button::getLongPressPeriodicalEdge()
{

    //   bool edge_detected;
    //   edge_detected = long_press_edge_detected;
    //   long_press_edge_detected = false;
    //   return edge_detected;
    // bool tmp = long_press_edge_detected;
    // long_press_edge_detected =false;
    // return tmp;
    return long_press_edge_detected;
}

void Button::refresh()
{
    refreshEdges();
    bool val = digitalRead(this->pin);

    if (val != button_value_memory)
    {
        debounce_start = millis();
        // Serial.println("debounce start");
        is_debounced = false;
    }

    if ((millis() - debounce_start > DEBOUNCE_TIME) && !is_debounced)
    {
        setValue(val);
        is_debounced = true;
        // #ifdef ENABLE_SERIAL
        // Serial.println("bb");
        // #endif

        longPressStartMillis = millis();
        longPressPeriodicalStartMillis = millis();
        longPressEdgeCount = 0;
    }

    if (isPressed())
    {
        // if (millis() - longPressStartMillis > BUTTON_LONG_PRESS_INITIAL_DELAY_MILLIS)
        // {
        //     bool delay_expired = millis() - longPressStartMillis > (BUTTON_LONG_PRESS_INITIAL_DELAY_MILLIS + longPressEdgeCount * BUTTON_LONG_PRESS_FAKE_EDGE_PERIOD_MILLIS);

        //     if (delay_expired && !long_press_edge_detected)
        //     {
        //         longPressEdgeCount++;
        //         // Serial.println("long p[ress edge");
        //         // Serial.println(longPressEdgeCount);
        //         long_press_edge_detected = true;
        //     }
        //     else
        //     {
        //         long_press_edge_detected = false;
        //     }
        //     // button_longpress_edge_memory = delay_expired;
        // }

        if (millis() - longPressStartMillis > BUTTON_LONG_PRESS_INITIAL_DELAY_MILLIS)
        {

            if (millis() - longPressPeriodicalStartMillis > BUTTON_LONG_PRESS_FAKE_EDGE_PERIOD_MILLIS)
            {
                long_press_edge_detected = true;
                longPressEdgeCount++;
                longPressPeriodicalStartMillis = millis();
                // #ifdef ENABLE_SERIAL
                // Serial.println("LOONG");
                // #endif
            }
            else
            {
                long_press_edge_detected = false;
            }
            // bool delay_expired = millis() - longPressStartMillis > (BUTTON_LONG_PRESS_INITIAL_DELAY_MILLIS + longPressEdgeCount * BUTTON_LONG_PRESS_FAKE_EDGE_PERIOD_MILLIS);

            // if (delay_expired && !long_press_edge_detected)
            // {
            //     longPressEdgeCount++;
            //     // Serial.println("long p[ress edge");
            //     // Serial.println(longPressEdgeCount);
            //     long_press_edge_detected = true;
            // }
            // else
            // {
            //     long_press_edge_detected = false;
            // }
            // // button_longpress_edge_memory = delay_expired;
        }
    }

    button_value_memory = val;
}
