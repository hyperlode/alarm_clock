#include "GravityRtc.h"
#include "Wire.h"
#include "Button.h"
#include "DisplayDigitsHandler5Digits.h"
#include "LedMultiplexer5x8.h"
#include "Buzzer.h"
#include <EEPROM.h>
#include "SuperTimer.h"

#define ENABLE_SERIAL

#define DELAY_TO_REDUCE_LIGHT_FLICKER_MILLIS 1 // if we iterate too fast through the loop, the display gets refreshed so quickly that it never really settles down. Off time at transistions beats ON time. So, with a dealy, we increase the ON time a tad.
#define DISPLAY_IS_COMMON_ANODE true           // check led displays both displays should be of same type   //also set in SevSeg5Digits.h : MODEISCOMMONANODE
#define DEFAULT_BRIGHTNESS 2
#define PIN_DUMMY 66
#define PIN_DUMMY_2 22 // randomly chosen. I've had it set to 67, and at some point, multiple segments were lit up. This STILL is C hey, it's gonna chug on forever!

#define EEPROM_ADDRESS_ALARM_HOUR 0
#define EEPROM_ADDRESS_ALARM_MINUTE 1

// DO NOT USE pin 3 and 11 for PWM if working with tone
// the common anode pins work with pwm. pwm and tone libraries interfere.
#define PIN_DISPLAY_DIGIT_0 5 // swapped
#define PIN_DISPLAY_DIGIT_1 10
#define PIN_DISPLAY_DIGIT_2 9
#define PIN_DISPLAY_DIGIT_3 6 // swapped!
#define PIN_DISPLAY_DIGIT_BUTTON_LIGHTS PIN_DUMMY

#define PIN_DISPLAY_SEGMENT_A 12
#define PIN_DISPLAY_SEGMENT_B 8
#define PIN_DISPLAY_SEGMENT_C 11 // swapped
#define PIN_DISPLAY_SEGMENT_D 3

#ifdef ENABLE_SERIAL

#define PIN_DISPLAY_SEGMENT_E PIN_DUMMY
#else

#define PIN_DISPLAY_SEGMENT_E 0
#endif

#define PIN_DISPLAY_SEGMENT_F 13 // swapped!
#define PIN_DISPLAY_SEGMENT_DP 4
#define PIN_DISPLAY_SEGMENT_G 7 // swapped!

#define PIN_BUZZER A0
#define PIN_button_up A3
#define PIN_button_down A2
#define PIN_BUTTON_MENU A1
#define PIN_BUTTON_EXTRA 2

#define TIME_UPDATE_DELAY 1000
#define TIME_HALF_BLINK_PERIOD_MILLIS 250
#define DELAY_ALARM_AUTO_ESCAPE_MILLIS 5000
#define ALARM_USER_STOP_BUTTON_PRESS_MILLIS 1000

DisplayManagement visualsManager;
LedMultiplexer5x8 ledDisplay;
GravityRtc rtc; // RTC Initialization

Button button_up;
Button button_down;
Button button_menu;
Button button_extra;

Buzzer buzzer;

SuperTimer kitchenTimer;

long nextTimeUpdateMillis;
long nextBlinkUpdateMillis;
long nextDisplayTimeUpdateMillis;

long watchdog_last_button_press_millis;
bool alarm_triggered_memory;

uint8_t alarm_hour;
uint8_t alarm_minute;
uint8_t alarm_hour_snooze;
uint8_t alarm_minute_snooze;
uint8_t alarm_snooze_duration_minutes;
uint8_t snooze_count;

// bool alarm_user_toggle_action;

uint8_t brightness_index;
bool blinker;

uint8_t hour_now;
uint8_t minute_now;
uint8_t second_now;

int8_t kitchen_timer_set_time_index;
const uint16_t timeDialDiscreteSeconds[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 15, 20, 25, 30, 45, 60, 75, 90, 120,
    150, 180, 210, 240, 270, 300, 330, 360, 390, 420,
    450, 480, 510, 540, 570, 600, 660, 720, 780, 840,
    900, 960, 1020, 1080, 1140, 1200, 1260, 1320, 1380, 1440,
    1500, 1560, 1620, 1680, 1740, 1800, 2100, 2400, 2700, 3000,
    3300, 3600, 3900, 4200, 4500, 4800, 5100, 5400, 6000, 6600,
    7200, 7800, 8400, 9000, 9600, 10200, 10800, 12600, 14400, 16200,
    18000, 19800, 21600, 23400, 25200, 27000, 28800, 30600, 32400, 34200,
    36000};

enum Time_type : uint8_t
{
    hours = 0,
    minutes = 1,
    seconds = 2
};

enum Clock_state : uint8_t
{
    state_display_time = 0,
    state_set_time,
    state_night_mode,
    state_alarm_set,
    state_kitchen_timer
};
Clock_state clock_state;

enum Kitchen_timer_state : uint8_t
{
    state_stopped = 0,
    state_running,
    state_running_invisible

};
Kitchen_timer_state kitchen_timer_state;

enum Set_time_state : uint8_t
{
    state_set_time_init = 0,
    state_display_minutes_seconds = 1,
    state_set_time_hours = 2,
    state_set_time_minutes = 3,
    state_set_time_seconds = 4,
    state_set_time_end = 5
};
Set_time_state set_time_state;

enum Alarm_state : uint8_t
{
    state_alarm_init = 0,
    state_alarm_display = 1,
    state_alarm_set_hours = 2,
    state_alarm_set_minutes = 3,
    state_alarm_end = 4
};
Alarm_state alarm_state;

enum Alarm_status_state : uint8_t
{
    state_alarm_status_is_not_enabled = 0,
    state_alarm_status_triggered = 1,
    state_alarm_status_snoozing = 2,
    state_alarm_status_stop = 3,
    state_alarm_status_toggle,
    state_alarm_status_is_enabled,
    state_alarm_status_enable,
    state_alarm_status_disable
};
Alarm_status_state alarm_status_state;

void setup()
{

#ifdef ENABLE_SERIAL
    Serial.begin(9600);
#endif
    blinker = false;

    rtc.setup();

    // Set the RTC time automatically: Calibrate RTC time by your computer time
    // rtc.adjustRtc(F(__DATE__), F(__TIME__));

    // Set the RTC time manually
    // rtc.adjustRtc(2017,6,19,1,12,7,0);  //Set time: 2017/6/19, Monday, 12:07:00

    button_up.init(PIN_button_up);
    button_down.init(PIN_button_down);
    button_menu.init(PIN_BUTTON_MENU);
    button_extra.init(PIN_BUTTON_EXTRA);

    ledDisplay.Begin(DISPLAY_IS_COMMON_ANODE, PIN_DISPLAY_DIGIT_0, PIN_DISPLAY_DIGIT_1, PIN_DISPLAY_DIGIT_2, PIN_DISPLAY_DIGIT_3, PIN_DISPLAY_DIGIT_BUTTON_LIGHTS, PIN_DISPLAY_SEGMENT_A, PIN_DISPLAY_SEGMENT_B, PIN_DISPLAY_SEGMENT_C, PIN_DISPLAY_SEGMENT_D, PIN_DISPLAY_SEGMENT_E, PIN_DISPLAY_SEGMENT_F, PIN_DISPLAY_SEGMENT_G, PIN_DISPLAY_SEGMENT_DP);
    visualsManager.setMultiplexerBuffer(ledDisplay.getDigits());

    buzzer.setPin(PIN_BUZZER);

    nextTimeUpdateMillis = millis();
    nextBlinkUpdateMillis = millis();
    nextDisplayTimeUpdateMillis = millis();
    cycleBrightness(true);
    clock_state = state_display_time;
    alarm_state = state_alarm_init;

    kitchen_timer_set_time_index = 15;

// #define ALARM_DEBUG
#ifdef ALARM_DEBUG

//   rtc.read();
//   alarm_hour = rtc.hour;
//   alarm_minute = rtc.minute + 1;
#else

    alarm_hour = EEPROM.read(EEPROM_ADDRESS_ALARM_HOUR);
    alarm_minute = EEPROM.read(EEPROM_ADDRESS_ALARM_MINUTE);
    if (alarm_hour == 0xff)
    {
        alarm_hour = 7;
        alarm_minute = 0;
    }
#endif

    alarm_snooze_duration_minutes = 1;
    snooze_count = 0;

    //   alarm_user_toggle_action = false;
    alarm_status_state = state_alarm_status_is_not_enabled;

    uint8_t test[32];
    rtc.readMemory(test);
    //   for (uint8_t i=0;i<32;i++){
    //     Serial.println(test[i]);
    //   }
}

void cycleBrightness(bool init)
{

#define BRIGHTNESS_LEVELS 3
    //#define CYCLING_GOES_BRIGHTER

#if (BRIGHTNESS_LEVELS == 4)
    uint8_t brightness_settings[] = {0, 1, 10, 80, 254}; // do not use 255, it creates an after glow once enabled. (TODO: why?!) zero is dark. but, maybe you want that... e.g. alarm active without display showing.
#elif (BRIGHTNESS_LEVELS == 3)
    uint8_t brightness_settings[] = {0, 1, 10, 254}; // do not use 255, it creates an after glow once enabled. (TODO: why?!) zero is dark. but, maybe you want that... e.g. alarm active without display showing.
#endif

#ifdef CYCLING_GOES_BRIGHTER
    // if cycling goes brighter
    brightness++;
    if (brightness > BRIGHTNESS_LEVELS)
    {
        brightness = 0; // zero is dark. but, maybe you want that... e.g. alarm active without display showing.
    }
#else
    // cycling goes darker
    if (brightness_index == 0)
    {
        brightness_index = BRIGHTNESS_LEVELS;
    }
    else
    {
        brightness_index--;
    }
#endif

    if (init)
    {
        brightness_index = DEFAULT_BRIGHTNESS;
    }

    // enter night mode screen when zero visibility.
    if (brightness_index == 0)
    {
        visualsManager.setBlankDisplay();
        // Serial.println("dark mode enter.");
        clock_state = state_night_mode;
        brightness_index = BRIGHTNESS_LEVELS;
    }
    ledDisplay.setBrightness(brightness_settings[brightness_index], false);
}

void display_alarm()
{
    int16_t timeAsNumber;
    timeAsNumber = 100 * alarm_hour + alarm_minute;
    visualsManager.setNumberToDisplay(timeAsNumber, false);
    add_leading_zeros(timeAsNumber, false);
    divider_colon_to_display(true);
}

void add_leading_zeros(int16_t number, bool leading_zeros_for_most_left_digit)
{
    if (number < 1000)
    {
        if (leading_zeros_for_most_left_digit)
        {
            visualsManager.setCharToDisplay('0', 0);
        }
        if (number < 100)
        {
            visualsManager.setCharToDisplay('0', 1);
            if (number < 10)
            {
                visualsManager.setCharToDisplay('0', 2);
            }
        }
    }
}

void divider_colon_to_display()
{
    // will blink with a two second period
    rtc.read();
    visualsManager.setDecimalPointToDisplay(rtc.second % 2, 1);
}
void divider_colon_to_display(bool active)
{
    visualsManager.setDecimalPointToDisplay(active, 1);
}

void alarm_activated_to_display(bool active)
{
    visualsManager.setDecimalPointToDisplay(active, 0);
}

void seconds_to_display()
{
    int16_t timeAsNumber;
    rtc.read();
    timeAsNumber = (int16_t)rtc.second;
    visualsManager.setNumberToDisplay(timeAsNumber, false);
    if (timeAsNumber < 10)
    {
        visualsManager.setCharToDisplay('0', 2); // leading zero if less than ten seconds
    }
}

void minutes_seconds_to_display()
{
    int16_t timeAsNumber;
    rtc.read();
    timeAsNumber = 100 * ((int16_t)rtc.minute) + (int16_t)rtc.second;
    visualsManager.setNumberToDisplay(timeAsNumber, false);
    add_leading_zeros(timeAsNumber, true);
}

void hour_minutes_to_display()
{
    int16_t timeAsNumber;
    rtc.read();
    timeAsNumber = 100 * ((int16_t)rtc.hour) + (int16_t)rtc.minute;
    visualsManager.setNumberToDisplay(timeAsNumber, false);
    //   if (timeAsNumber < 100) {
    //     visualsManager.setCharToDisplay('0', 1); // leading zero at midnight ("0" hour)
    //     if(rtc.minute<10){
    //         visualsManager.setCharToDisplay('0', 2); // leading zero at midnight for the first nine minutes("0" hour)
    //     }
    //   }
    add_leading_zeros(timeAsNumber, false);
}

void set_time(Time_type t)
{
    if (button_up.getEdgeDown() || button_down.getEdgeDown())
    {
        rtc.read();

        if (t == hours)
        {
            nextStepRotate(&rtc.hour, button_up.getEdgeDown(), 0, 23);
        }
        else if (t == minutes)
        {
            nextStepRotate(&rtc.minute, button_up.getEdgeDown(), 0, 59);
        }
        else if (t == seconds)
        {
            // nextStepRotate(&rtc.minute, button_up.getEdgeDown(), 0, 59);
            rtc.second = 0;
        }
        rtc.adjustRtc(rtc.year, rtc.month, rtc.day, rtc.week, rtc.hour, rtc.minute, rtc.second);

        // display updated time here. Even in the "off time" of the blinking process, it will still display the change for the remainder of the off-time. This is good.
        if (t == hours)
        {
            hour_minutes_to_display();
        }
        else if (t == minutes)
        {
            hour_minutes_to_display();
        }
        else if (t == seconds)
        {
            seconds_to_display();
        }
    }

    if (millis() > nextBlinkUpdateMillis)
    {

        nextBlinkUpdateMillis = millis() + TIME_HALF_BLINK_PERIOD_MILLIS;
        rtc.read();
        divider_colon_to_display(rtc.second % 2);

        if (t == hours)
        {
            hour_minutes_to_display();
        }
        else if (t == minutes)
        {
            hour_minutes_to_display();
        }
        else if (t == seconds)
        {
            seconds_to_display();
        }

        blinker = !blinker;
        if (blinker)
        {
            if (t == hours)
            {
                visualsManager.setCharToDisplay(' ', 1);
                visualsManager.setCharToDisplay(' ', 0);
            }
            else if (t == minutes)
            {
                visualsManager.setCharToDisplay(' ', 2);
                visualsManager.setCharToDisplay(' ', 3);
            }
            else if (t == seconds)
            {
                visualsManager.setCharToDisplay(' ', 2);
                visualsManager.setCharToDisplay(' ', 3);
            }
        }
    }
}

void display_time_state_refresh()
{

    //   if (millis() > nextTimeUpdateMillis ) {
    //     nextTimeUpdateMillis = millis() + TIME_UPDATE_DELAY;
    //     rtc.read();
    //     hour_now = rtc.hour;
    //     minute_now = rtc.minute;
    //     second_now = rtc.second;
    //
    //   }

    if (millis() > nextDisplayTimeUpdateMillis)
    {
        nextDisplayTimeUpdateMillis = millis() + TIME_UPDATE_DELAY;
        divider_colon_to_display(second_now % 2);
        hour_minutes_to_display();
    }

    if (button_menu.getEdgeDown())
    {
        // clock_state = static_cast<Clock_state>(static_cast<int>(clock_state) + 1);;
        clock_state = state_set_time;
    }
    if (button_extra.getEdgeDown())
    {

        clock_state = state_kitchen_timer;
    }

    if (button_up.getEdgeDown())
    {
        clock_state = state_alarm_set;
    }

    if (button_down.getEdgeDown())
    {
        cycleBrightness(false);
    }
}

void set_time_state_refresh()
{
    switch (set_time_state)
    {
    case state_set_time_init:
    {
        set_time_state = state_display_minutes_seconds;
    }
    break;
    case state_display_minutes_seconds:
    {
        if (millis() > nextBlinkUpdateMillis)
        {
            nextBlinkUpdateMillis = millis() + TIME_HALF_BLINK_PERIOD_MILLIS;
            minutes_seconds_to_display();
        }

        if (button_up.getEdgeDown() || button_down.getEdgeDown())
        {
            set_time_state = state_set_time_end;
        }
    }
    break;
    case state_set_time_hours:
    {
        set_time(hours);
    }
    break;
    case state_set_time_minutes:
    {
        set_time(minutes);
    }
    break;
    case state_set_time_seconds:
    {
        set_time(seconds);
    }
    break;
    case state_set_time_end:
    {
        set_time_state = state_set_time_init;
        clock_state = state_display_time;
        // Serial.println("at time state: END");
    }
    break;
    default:
    {
    }
    break;
    }
    if (button_menu.getEdgeDown())
    {
        // cycle trough states in sequence
        set_time_state = static_cast<Set_time_state>(static_cast<int>(set_time_state) + 1);
        ;
        // Serial.println("State set to:");
        // Serial.println(set_time_state);
    }
}

void alarm_state_refresh()
{
    switch (alarm_status_state)
    {
    case (state_alarm_status_disable):
    {
        // alarm_user_toggle_action = false;
        buzzer.addNoteToNotesBuffer(G4_2);
        buzzer.addNoteToNotesBuffer(REST_4_8);
        buzzer.addNoteToNotesBuffer(C4_1);
        buzzer.addNoteToNotesBuffer(C4_1);
        buzzer.addNoteToNotesBuffer(REST_4_8);
        alarm_status_state = state_alarm_status_is_not_enabled;
        alarm_activated_to_display(false);
        snooze_count = 0;
        // Serial.println("Disable alarm");
    }
    break;

    case (state_alarm_status_enable):
    {
        // alarm_user_toggle_action = false;
        // buzzer.addRandomSoundToNotesBuffer(0,255);
        buzzer.addNoteToNotesBuffer(C4_2);
        buzzer.addNoteToNotesBuffer(REST_4_8);
        buzzer.addNoteToNotesBuffer(G4_1);
        buzzer.addNoteToNotesBuffer(G4_1);
        buzzer.addNoteToNotesBuffer(REST_4_8);
        alarm_status_state = state_alarm_status_is_enabled;
        alarm_activated_to_display(true);
        snooze_count = 0;
        // Serial.println("Enable alarm");
    }
    break;

    case (state_alarm_status_is_not_enabled):
    {
    }
    break;

    case (state_alarm_status_is_enabled):
    {
        // Serial.println(hour_now);
        if (hour_now == alarm_hour && minute_now == alarm_minute && second_now == 0)
        {
            // // make sure there is only one trigger edge at an alarm occurence

            // alarm going off will interrupt ongoing user operations.
            switch (clock_state)
            {
            case state_set_time:
            {
                if (set_time_state != state_set_time_hours &&
                    set_time_state != state_set_time_minutes &&
                    set_time_state != state_set_time_seconds)
                {
                    set_time_state = state_set_time_init;
                    clock_state = state_display_time;
                    alarm_status_state = state_alarm_status_triggered;
                    // Serial.println("alarm triggered...(exit time setting)");
                }
                else
                {
                    // DO NOT PRINT STUFF HERE, it will hang and crash.
                    // Serial.println("Do not trigger alarm while setting time");
                    // delay(1000);
                }
            }
            break;
            case state_night_mode:
            {
                alarm_status_state = state_alarm_status_triggered;
            }
            break;
            case state_alarm_set:
            {
                // dont trigger alarm while setting the alarm time
                if (alarm_state != state_alarm_set_hours && alarm_state != state_alarm_set_minutes)
                {
                    alarm_state = state_alarm_init;
                    clock_state = state_display_time;
                    // Serial.println("alarm triggered...(exit alarm setting)");
                    alarm_status_state = state_alarm_status_triggered;
                }
                else
                {
                    // DO NOT PRINT STUFF HERE, it will hang and crash.
                    // Serial.println("alarm triggered...(will do nothing because we're setting the time)");
                }
            }
            break;
            case state_display_time:
            {
                alarm_status_state = state_alarm_status_triggered;
            }
            break;
            default:
            {
                // Serial.println("alarm triggered...");
            }
            break;
            }
            // if alarm is triggered while in a different state, these states should be reset.
        }
    }
    break;

    case (state_alarm_status_triggered):
    {
        if (button_down.getValueChanged() ||
            button_up.getValueChanged() ||
            button_menu.getValueChanged())
        {
            // set snooze time
            snooze_count++;
            long time_stamp_minutes = 60 * alarm_hour + alarm_minute;
            time_stamp_minutes += (alarm_snooze_duration_minutes * snooze_count);
            time_stamp_minutes %= 24 * 60;
            alarm_hour_snooze = time_stamp_minutes / 60;
            alarm_minute_snooze = time_stamp_minutes % 60;

            // alarm_sounding = false;
            alarm_status_state = state_alarm_status_snoozing;
        }

        // add notes when buffer empty and alarm ringing
        if (buzzer.getBuzzerNotesBufferEmpty())
        {
            buzzer.addNoteToNotesBuffer(C6_1);
            buzzer.addNoteToNotesBuffer(REST_2_8);
        }
    }
    break;

    case (state_alarm_status_snoozing):
    {
        if (hour_now == alarm_hour_snooze && minute_now == alarm_minute_snooze && second_now == 0)
        {
            alarm_status_state = state_alarm_status_triggered;
        }
        if (!button_up.getValue() &&
            millis() > (button_up.getLastStateChangeMillis() + ALARM_USER_STOP_BUTTON_PRESS_MILLIS))
        {
            snooze_count = 0;
            alarm_status_state = state_alarm_status_disable;
        }

        alarm_activated_to_display((millis() % 500) > 250);
    }
    break;

    default:
    {
    }
    break;
    }
}

void kitchen_timer_state_refresh()
{
    switch (kitchen_timer_state)
    {
    case (state_stopped):
    {
        char *disp;
        disp = visualsManager.getDisplayTextBufHandle();
        kitchenTimer.getTimeString(disp);
        if (button_extra.getEdgeDown())
        {
            clock_state = state_display_time;
        }

        if (button_down.getEdgeDown() || button_up.getEdgeDown())
        {

            // #ifdef ENABLE_SERIAL
            //             Serial.println(kitchen_timer_set_time_index);
            // #endif

            nextStep(&kitchen_timer_set_time_index, button_up.getEdgeDown(), 0, 90, false);
            kitchenTimer.setInitCountDownTimeSecs(timeDialDiscreteSeconds[kitchen_timer_set_time_index]);
        }

        if (button_menu.getEdgeDown())
        {
            kitchen_timer_state = state_running;
        }
    }
    break;
    default:
    {
    }
    break;
    }
}

void alarm_set_state_refresh()
{

    if (millis() > watchdog_last_button_press_millis + DELAY_ALARM_AUTO_ESCAPE_MILLIS)
    {
        alarm_state = state_alarm_end;
        // Serial.println("yeooie");
        // Serial.println(watchdog_last_button_press_millis);
    }

    switch (alarm_state)
    {
    case state_alarm_init:
    {
        alarm_state = state_alarm_display;
        display_alarm();
    }
    break;
    case state_alarm_display:
    {
        if (button_up.getEdgeDown())
        {

            // state_alarm_status = state_alarm_status_toggle_active;
            // alarm_user_toggle_action = true;

            switch (alarm_status_state)
            {
            case state_alarm_status_is_not_enabled:
            {
                alarm_status_state = state_alarm_status_enable;
            }
            break;

            case state_alarm_status_is_enabled:
            {
                alarm_status_state = state_alarm_status_disable;
            }
            break;
            case state_alarm_status_snoozing:
            {
                alarm_status_state = state_alarm_status_disable;
            }
            break;
            default:
            {
                // Serial.println("ASSERT ERROR: alarm state not expected");
            }
            break;
            }
        }
        if (button_menu.getEdgeDown())
        {
            alarm_state = state_alarm_set_hours;
        }
        if (button_down.getEdgeDown())
        {
            alarm_state = state_alarm_end;
        }
    }
    break;
    case state_alarm_set_hours:
    {
        if (button_down.getEdgeDown())
        {
            nextStepRotate(&alarm_hour, 0, 0, 23);
            display_alarm();
            rtc.setAlarm(alarm_hour, alarm_minute);
        }
        if (button_up.getEdgeDown())
        {
            nextStepRotate(&alarm_hour, 1, 0, 23);
            display_alarm();
            rtc.setAlarm(alarm_hour, alarm_minute);
        }
        if (button_menu.getEdgeDown())
        {
            alarm_state = state_alarm_set_minutes;
            display_alarm();
        }

        if (millis() > nextBlinkUpdateMillis)
        {
            nextBlinkUpdateMillis = millis() + TIME_HALF_BLINK_PERIOD_MILLIS;
            display_alarm();
            blinker = !blinker;
            if (blinker)
            {
                visualsManager.setCharToDisplay(' ', 1);
                visualsManager.setCharToDisplay(' ', 0);
            }
        }
    }
    break;
    case state_alarm_set_minutes:
    {
        if (button_down.getEdgeDown())
        {
            nextStepRotate(&alarm_minute, 0, 0, 59);
            display_alarm();
            rtc.setAlarm(alarm_hour, alarm_minute);
        }
        if (button_up.getEdgeDown())
        {
            nextStepRotate(&alarm_minute, 1, 0, 59);
            display_alarm();
            rtc.setAlarm(alarm_hour, alarm_minute);
        }
        if (button_menu.getEdgeDown())
        {
            display_alarm();
            alarm_state = state_alarm_display;
        }
        if (millis() > nextBlinkUpdateMillis)
        {
            nextBlinkUpdateMillis = millis() + TIME_HALF_BLINK_PERIOD_MILLIS;
            display_alarm();
            blinker = !blinker;
            if (blinker)
            {
                visualsManager.setCharToDisplay(' ', 2);
                visualsManager.setCharToDisplay(' ', 3);
            }
        }
    }
    break;
    case state_alarm_end:
    {
        alarm_state = state_alarm_init; // prepare for the next time.
        clock_state = state_display_time;

        // eeprom only write when changed.
        if (EEPROM.read(EEPROM_ADDRESS_ALARM_HOUR) != alarm_hour)
        {
            EEPROM.write(EEPROM_ADDRESS_ALARM_HOUR, alarm_hour);
            // Serial.println("write eerprom");
        }
        if (EEPROM.read(EEPROM_ADDRESS_ALARM_MINUTE) != alarm_minute)
        {
            EEPROM.write(EEPROM_ADDRESS_ALARM_MINUTE, alarm_minute);
        }
    }
    break;
    default:
    {
        // Serial.println("ASSERT ERROR: unknown alarm set state");
        clock_state = state_display_time;
    }
    break;
    }
}

void display_on_touch_state_refresh()
{

    if (button_down.getEdgeDown())
    {
        // Serial.println("back to state display time");
        clock_state = state_display_time;
        hour_minutes_to_display();
        alarm_activated_to_display((alarm_status_state == state_alarm_status_is_enabled));
        // Serial.println(brightness);
    }
    else if (button_menu.getEdgeDown() || button_up.getEdgeDown())
    {
        // press button, time is displayed
        rtc.read();
        divider_colon_to_display(rtc.second % 2);
        hour_minutes_to_display();
        alarm_activated_to_display((alarm_status_state == state_alarm_status_is_enabled));
    }
    else if (button_menu.getEdgeUp() || button_up.getEdgeUp())
    {
        // release button, clock light off
        visualsManager.setBlankDisplay();
    }
}

void refresh_clock_state()
{

    // alarm FSM runs independently
    alarm_state_refresh();

    switch (clock_state)
    {
    case state_display_time:
    {
        display_time_state_refresh();
    }
    break;
    case state_set_time:
    {
        set_time_state_refresh();
    }
    break;
    case state_night_mode:
    {
        display_on_touch_state_refresh();
    }
    break;
    case state_alarm_set:
    {
        alarm_set_state_refresh();
    }
    break;
    case state_kitchen_timer:
    {
        kitchen_timer_state_refresh();
    }
    break;
    default:
    {
        clock_state = state_display_time;
    }
    break;
    }
}

void checkWatchDog()
{
    if (button_down.getValueChanged() ||
        button_up.getValueChanged() ||
        button_menu.getValueChanged())
    {
        watchdog_last_button_press_millis = millis();
    }
}

void updateTimeNow()
{
    if (millis() > nextTimeUpdateMillis)
    {
        nextTimeUpdateMillis = millis() + TIME_UPDATE_DELAY;

        // limit calls to peripheral by only loading time once every
        rtc.read();
        hour_now = rtc.hour;
        minute_now = rtc.minute;
        second_now = rtc.second;
        // Serial.println(second_now);
    }
}

void loop()
{

    // input
    button_up.refresh();
    button_down.refresh();
    button_menu.refresh();
    button_extra.refresh();

    // process
    updateTimeNow();
    checkWatchDog();
    refresh_clock_state();

    // output
    buzzer.checkAndPlayNotesBuffer();
    visualsManager.refresh();
    ledDisplay.refresh();
    delay(DELAY_TO_REDUCE_LIGHT_FLICKER_MILLIS);

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
