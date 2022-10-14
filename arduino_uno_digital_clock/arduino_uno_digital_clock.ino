#include "GravityRtc.h"
#include "Wire.h"
#include "Button.h"
#include "DisplayDigitsHandler5Digits.h"
#include "LedMultiplexer5x8.h"
#include "Buzzer.h"
#include <EEPROM.h>
#include "SuperTimer.h"

// #define ENABLE_SERIAL

#define DELAY_TO_REDUCE_LIGHT_FLICKER_MILLIS 1 // if we iterate too fast through the loop, the display gets refreshed so quickly that it never really settles down. Off time at transistions beats ON time. So, with a dealy, we increase the ON time a tad.

#define DISPLAY_IS_COMMON_ANODE true // check led displays both displays should be of same type   //also set in SevSeg5Digits.h : MODEISCOMMONANODE
#define BRIGHTNESS_LEVELS 3
#define DEFAULT_BRIGHTNESS_LEVEL_INDEX 3

#define EEPROM_VALID_VALUE 66
#define FACTORY_DEFAULT_KITCHEN_TIMER 10
#define FACTORY_DEFAULT_SNOOZE_TIME_MINUTES 9
#define FACTORY_DEFAULT_ALARM_HOUR 7
#define FACTORY_DEFAULT_ALARM_MINUTE 30

#define EEPROM_ADDRESS_EEPROM_VALID 0 // 1 byte
#define EEPROM_ADDRESS_ALARM_HOUR 1               // 1 byte
#define EEPROM_ADDRESS_ALARM_MINUTE 2             // 1 byte
#define EEPROM_ADDRESS_KITCHEN_TIMER_INIT_INDEX 3 // 1 byte
#define EEPROM_ADDRESS_SNOOZE_TIME_MINUTES 4 // 1 byte



#define PIN_DUMMY 66
#define PIN_DUMMY_2 22 // randomly chosen. I've had it set to 67, and at some point, multiple segments were lit up. This STILL is C hey, it's gonna chug on forever!

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
#define PIN_DISPLAY_SEGMENT_G 7  // swapped!
#define PIN_DISPLAY_SEGMENT_DP 4

#define PIN_BUZZER A0

#define PIN_BUTTON_2 A3
#define PIN_BUTTON_1 A2
#define PIN_BUTTON_0 A1
#define PIN_BUTTON_3 2

#define button_down button_0
#define button_up button_1
#define button_enter button_3
#define button_exit button_2

#define button_menu button_0
#define button_brightness button_1
#define button_alarm button_3
#define button_kitchen_timer button_2

#define TIME_UPDATE_DELAY 1000
#define DOT_UPDATE_DELAY 250
#define TIME_HALF_BLINK_PERIOD_MILLIS 250
#define DELAY_ALARM_AUTO_ESCAPE_MILLIS 5000
#define DELAY_KITCHEN_TIMER_AUTO_ESCAPE_MILLIS 5000
#define ALARM_USER_STOP_BUTTON_PRESS_MILLIS 1000
#define KITCHEN_TIMER_ENDED_PERIODICAL_BEEP_SECONDS 60


#define PERIODICAL_EDGES_DELAY 2 // used to delay long press time. 0 = first long press period occurence, e.g. 5  = 5 long press periods delay

DisplayManagement visualsManager;
LedMultiplexer5x8 ledDisplay;
GravityRtc rtc; // RTC Initialization

Button button_2;
Button button_1;
Button button_0;
Button button_3;

Buzzer buzzer;

SuperTimer kitchenTimer;
bool kitchenTimerCountingDownMemory;
bool beep_memory;

long nextTimeUpdateMillis;
long nextBlinkUpdateMillis;
long nextKitchenBlinkUpdateMillis;
long nextDisplayTimeUpdateMillis;
long blink_offset;

bool display_dot_status_memory;

long watchdog_last_button_press_millis;
bool alarm_triggered_memory;

uint8_t alarm_hour;
uint8_t alarm_minute;
uint8_t alarm_hour_snooze;
uint8_t alarm_minute_snooze;
int8_t alarm_snooze_duration_minutes;
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

uint8_t main_menu_item_index;
bool main_menu_display_update;
char main_menu_text_buf[4];

#define MENU_MENU_ITEMS_COUNT 3
const byte menu_item_titles[] PROGMEM = {
    'T', 'S', 'E', 'T',
    'S', 'N', 'O', 'O',
    'B', 'E', 'E', 'P'};
#define MAIN_MENU_ITEM_TIME_SET 0
#define MAIN_MENU_ITEM_SNOOZE_TIME 1
#define MAIN_MENU_ITEM_ENABLE_HOURLY_BEEP 2

enum Time_type : uint8_t
{
    hours = 0,
    minutes = 1,
    seconds = 2
};

enum Main_state : uint8_t
{
    state_display_time = 0,
    state_set_time,
    state_night_mode,
    state_alarm_set,
    state_kitchen_timer,
    state_main_menu
};
Main_state main_state;

enum Kitchen_timer_state : uint8_t
{
    state_stopped = 0,
    state_running,
    state_running_invisible,
    state_stopped_refresh_display,
    state_running_refresh_display,
    state_enter,
    state_exit

};
Kitchen_timer_state kitchen_timer_state;
// Kitchen_timer_state state_mem_tmp;

enum Main_menu_state : uint8_t
{
    state_main_menu_init = 0,
    state_main_menu_exit = 1,
    state_main_menu_display_item = 2,
    state_main_menu_modify_item = 3

};
Main_menu_state main_menu_state;

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

enum Alarm_set_state : uint8_t
{
    state_alarm_init = 0,
    state_alarm_display = 1,
    state_alarm_set_hours = 2,
    state_alarm_set_minutes = 3,
    state_alarm_end = 4
};
Alarm_set_state alarm_set_state;

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

// bool Apps::millis_second_period()
// {
//     return millis() % 1000 > 500;
// }

// bool Apps::millis_quarter_second_period()
// {
//     return millis() % 250 > 125;
// }

// bool Apps::millis_half_second_period()
// {
//     return millis() % 500 > 250;
// }

bool millis_blink_250_750ms()
{
    // true for shorter part of the second
    return (millis() - blink_offset) % 1000 > 750;
}

void set_blink_offset()
{
    // keeps blink return to the same value. e.g. menu blinking between value and text. while dial rotates, we want to only see the value.
    blink_offset = millis() % 1000;
}

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
    // #define button_1 button_0

    button_0.init(PIN_BUTTON_0); // red
    button_1.init(PIN_BUTTON_1); // green
    button_2.init(PIN_BUTTON_2); // white
    button_3.init(PIN_BUTTON_3); // blue

    ledDisplay.Begin(DISPLAY_IS_COMMON_ANODE, PIN_DISPLAY_DIGIT_0, PIN_DISPLAY_DIGIT_1, PIN_DISPLAY_DIGIT_2, PIN_DISPLAY_DIGIT_3, PIN_DISPLAY_DIGIT_BUTTON_LIGHTS, PIN_DISPLAY_SEGMENT_A, PIN_DISPLAY_SEGMENT_B, PIN_DISPLAY_SEGMENT_C, PIN_DISPLAY_SEGMENT_D, PIN_DISPLAY_SEGMENT_E, PIN_DISPLAY_SEGMENT_F, PIN_DISPLAY_SEGMENT_G, PIN_DISPLAY_SEGMENT_DP);
    visualsManager.setMultiplexerBuffer(ledDisplay.getDigits());

    buzzer.setPin(PIN_BUZZER);
    buzzer.setSpeedRatio(2);

    // check eeprom sanity. If new device, set values in eeprom
    if (EEPROM.read(EEPROM_ADDRESS_EEPROM_VALID) != EEPROM_VALID_VALUE){
        EEPROM.write(EEPROM_ADDRESS_SNOOZE_TIME_MINUTES, FACTORY_DEFAULT_SNOOZE_TIME_MINUTES);
        EEPROM.write(EEPROM_ADDRESS_KITCHEN_TIMER_INIT_INDEX, FACTORY_DEFAULT_KITCHEN_TIMER);
        EEPROM.write(EEPROM_ADDRESS_ALARM_HOUR, FACTORY_DEFAULT_ALARM_HOUR);
        EEPROM.write(EEPROM_ADDRESS_ALARM_MINUTE, FACTORY_DEFAULT_ALARM_MINUTE);
        EEPROM.write(EEPROM_ADDRESS_EEPROM_VALID, EEPROM_VALID_VALUE);
        buzzer.addNoteToNotesBuffer(C5_1);
        buzzer.addNoteToNotesBuffer(D5_2);
        buzzer.addNoteToNotesBuffer(E5_4);
        buzzer.addNoteToNotesBuffer(F5_8);
    }
    
    // get eeprom values
    alarm_hour = EEPROM.read(EEPROM_ADDRESS_ALARM_HOUR);
    alarm_minute = EEPROM.read(EEPROM_ADDRESS_ALARM_MINUTE);
    alarm_snooze_duration_minutes = EEPROM.read(EEPROM_ADDRESS_SNOOZE_TIME_MINUTES);
    kitchen_timer_set_time_index = EEPROM.read(EEPROM_ADDRESS_KITCHEN_TIMER_INIT_INDEX);



    nextTimeUpdateMillis = millis();
    nextBlinkUpdateMillis = millis();
    nextKitchenBlinkUpdateMillis = millis();
    nextDisplayTimeUpdateMillis = millis();
    cycleBrightness(true);
    main_state = state_display_time;
    alarm_set_state = state_alarm_init;


    kitchenTimer.setInitCountDownTimeSecs(timeDialDiscreteSeconds[kitchen_timer_set_time_index]);
    // kitchen_timer_state = state_stopped_refresh_display;
    blink_offset = 0;

// #define ALARM_DEBUG
#ifdef ALARM_DEBUG

//   rtc.read();
//   alarm_hour = rtc.hour;
//   alarm_minute = rtc.minute + 1;
#endif
    
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
        brightness_index = DEFAULT_BRIGHTNESS_LEVEL_INDEX;
    }

    // enter night mode screen when zero visibility.
    if (brightness_index == 0)
    {
        visualsManager.setBlankDisplay();
        // Serial.println("dark mode enter.");
        main_state = state_night_mode;
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

// void divider_colon_to_display()
// {
//     // will blink with a two second period
//     rtc.read();
//     visualsManager.setDecimalPointToDisplay(rtc.second % 2, 1);
// }

void divider_colon_to_display(bool active)
{
    visualsManager.setDecimalPointToDisplay(active, 1);
}

void refresh_indicator_dot()
{

    // alarm going off or snoozed has priority
    if (alarm_status_state == state_alarm_status_snoozing || alarm_status_state == state_alarm_status_triggered)
    {
        set_display_indicator_dot((millis() % 250) > 125);
    }
    else if (main_state == state_display_time)
    {
        // set_display_indicator_dot(kitchenTimer.getInFirstGivenHundredsPartOfSecond(500));
        // if (alarm_status_state == state_alarm_status_snoozing)
        // {
        //     set_display_indicator_dot((millis() % 250) > 125);
        // }
        // else
        if (kitchenTimer.getIsStarted())
        {

            if (kitchenTimer.getTimeIsNegative())
            {
                set_display_indicator_dot((millis() % 1000) > 500);
            }
            else
            {
                set_display_indicator_dot((millis() % 500) > 250);
            }
        }
        else if (alarm_status_state == state_alarm_status_is_enabled)
        {
            set_display_indicator_dot(true);
        }
        else
        {
            set_display_indicator_dot(false);
        }
    }
    else if (main_state == state_kitchen_timer)
    {
        if (kitchen_timer_state == state_running)
        {
            if (kitchenTimer.getTimeIsNegative())
            {
                set_display_indicator_dot((millis() % 1000) > 500);
            }
            else
            {
                set_display_indicator_dot((millis() % 500) > 250);
            }
        }
    }
    else if (main_state == state_alarm_set)
    {
        if (alarm_status_state == state_alarm_status_is_enabled)
        {
            set_display_indicator_dot(true);
        }
        else if (alarm_status_state == state_alarm_status_is_not_enabled)
        {
            set_display_indicator_dot(false);
        }
    }
    else
    {
        set_display_indicator_dot(false);
    }
}

void set_display_indicator_dot(bool active)
{
    // only update display if "different"
    if (active != display_dot_status_memory)
    {
        visualsManager.setDecimalPointToDisplay(active, 0);
    }
    display_dot_status_memory = active;
}

void seconds_to_display()
{
    int16_t timeAsNumber;
    // rtc.read();
    // timeAsNumber = (int16_t)rtc.second;
    timeAsNumber = (int16_t)second_now;
    visualsManager.setNumberToDisplay(timeAsNumber, false);
    if (timeAsNumber < 10)
    {
        visualsManager.setCharToDisplay('0', 2); // leading zero if less than ten seconds
    }
    divider_colon_to_display(second_now % 2);
}

void minutes_seconds_to_display()
{
    int16_t timeAsNumber;
    // rtc.read();
    // timeAsNumber = 100 * ((int16_t)rtc.minute) + (int16_t)rtc.second;
    timeAsNumber = 100 * (int16_t)minute_now + (int16_t)second_now;
    visualsManager.setNumberToDisplay(timeAsNumber, false);
    add_leading_zeros(timeAsNumber, true);
    divider_colon_to_display(second_now % 2);
}

void hour_minutes_to_display()
{
    int16_t timeAsNumber;
    // rtc.read();
    // timeAsNumber = 100 * ((int16_t)rtc.hour) + (int16_t)rtc.minute;
    timeAsNumber = 100 * ((int16_t)hour_now) + (int16_t)minute_now;
    visualsManager.setNumberToDisplay(timeAsNumber, false);
    //   if (timeAsNumber < 100) {
    //     visualsManager.setCharToDisplay('0', 1); // leading zero at midnight ("0" hour)
    //     if(rtc.minute<10){
    //         visualsManager.setCharToDisplay('0', 2); // leading zero at midnight for the first nine minutes("0" hour)
    //     }
    //   }
    add_leading_zeros(timeAsNumber, false);
    divider_colon_to_display(second_now % 2);
}

void set_time(Time_type t)
{
    if (button_down.isPressedEdge() || button_up.isPressedEdge() || button_down.getLongPressPeriodicalEdge() || button_up.getLongPressPeriodicalEdge())
    {
        rtc.read();

        if (t == hours)
        {
            nextStepRotate(&rtc.hour, button_up.isPressed(), 0, 23);
            nextBlinkUpdateMillis -= TIME_HALF_BLINK_PERIOD_MILLIS;
            blinker = true;
            hour_minutes_to_display();
            hour_now = rtc.hour;
        }
        else if (t == minutes)
        {
            nextStepRotate(&rtc.minute, button_up.isPressed(), 0, 59);
            nextBlinkUpdateMillis -= TIME_HALF_BLINK_PERIOD_MILLIS;
            blinker = true;
            hour_minutes_to_display();
            minute_now = rtc.minute;
        }
        else if (t == seconds)
        {
            rtc.second = 0;
            nextBlinkUpdateMillis -= TIME_HALF_BLINK_PERIOD_MILLIS;
            blinker = true;
            seconds_to_display();
            second_now = rtc.second;
        }
        rtc.adjustRtc(rtc.year, rtc.month, rtc.day, rtc.week, rtc.hour, rtc.minute, rtc.second);

        // display updated time here. Even in the "off time" of the blinking process, it will still display the change for the remainder of the off-time. This is good.
    }

    if (millis() > nextBlinkUpdateMillis)
    {

        nextBlinkUpdateMillis = millis() + TIME_HALF_BLINK_PERIOD_MILLIS;

        // rtc.read();
        // divider_colon_to_display(now_second % 2);

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

        hour_minutes_to_display();
    }

    if (button_menu.isPressedEdge())
    {
        // main_state = static_cast<Main_state>(static_cast<int>(main_state) + 1);;
        main_state = state_set_time;
    }

    if (button_kitchen_timer.isPressedEdge())
    {
        // kitchen_timer_refresh_display();
        kitchen_timer_state = state_enter;
        main_state = state_kitchen_timer;
    }

    if (button_alarm.isPressedEdge())
    {
        main_state = state_alarm_set;
    }

    if (button_brightness.isPressedEdge())
    {
        cycleBrightness(false);
    }
}

void main_menu_state_refresh()
{
    switch (main_menu_state)
    {
    case (state_main_menu_init):
    {
        main_menu_state = state_main_menu_display_item;
        main_menu_item_index = 0;
        main_menu_display_update = true;
    }
    break;
    case (state_main_menu_exit):
    {
        main_menu_state = state_main_menu_init;
        main_state = state_display_time;
    }
    break;

    case (state_main_menu_display_item):
    {

        if (button_enter.isPressedEdge())
        {

            main_menu_state = state_main_menu_modify_item;
        }
        if (button_exit.isPressedEdge())
        {
            main_menu_state = state_main_menu_exit;
        }
        if (button_up.isPressedEdge() || button_down.isPressedEdge())
        {
            nextStepRotate(&main_menu_item_index, button_down.isPressed(), 0, MENU_MENU_ITEMS_COUNT - 1);
            main_menu_display_update = true;
        }

        if (main_menu_display_update)
        {
            for (uint8_t i = 0; i < 4; i++)
            {
                main_menu_text_buf[i] = pgm_read_byte_near(menu_item_titles + main_menu_item_index * 4 + i);
            }
            main_menu_display_update = false;
        }

        if (millis_blink_250_750ms())
        {
            switch (main_menu_item_index)
            {
            case (MAIN_MENU_ITEM_TIME_SET):
            {
                hour_minutes_to_display();
            }
            break;
            case (MAIN_MENU_ITEM_SNOOZE_TIME):
            {
                visualsManager.setNumberToDisplay(alarm_snooze_duration_minutes, false);
            }
            break;
            case (MAIN_MENU_ITEM_ENABLE_HOURLY_BEEP):
            {
                visualsManager.setBoolToDisplay(false);
            }
            break;
            default:
            {
                visualsManager.setNumberToDisplay(666, false);
            };
            }
        }
        else
        {
            visualsManager.setTextBufToDisplay(main_menu_text_buf);
            // visualsManager.setBoolToDisplay(true);
        }
    }
    break;
    case (state_main_menu_modify_item):
    {
        if (button_exit.isPressedEdge())
        {
            main_menu_state = state_main_menu_display_item;

            if (alarm_snooze_duration_minutes != EEPROM.read(EEPROM_ADDRESS_SNOOZE_TIME_MINUTES))
            {
                EEPROM.write(EEPROM_ADDRESS_SNOOZE_TIME_MINUTES, alarm_snooze_duration_minutes);
            }
            
        }

        switch (main_menu_item_index)
        {
        case (MAIN_MENU_ITEM_TIME_SET):
        {
        }
        break;
        case (MAIN_MENU_ITEM_SNOOZE_TIME):
        {
            // if (button_enter.isPressedEdge())
            // {
            //     main_menu_state = state_main_menu_exit;
            // }
            visualsManager.setNumberToDisplay(alarm_snooze_duration_minutes, false);
            if (button_up.isPressedEdge() || button_down.isPressedEdge())
            {
                nextStep(&alarm_snooze_duration_minutes, button_up.isPressed(), 0, 127, false);
            }
        }
        break;
        case (MAIN_MENU_ITEM_ENABLE_HOURLY_BEEP):
        {
        }
        break;
        default:
        {
        };
        }

    }
    break;
    default:
    {
    }
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
        main_state = state_display_time;
        // Serial.println("at time state: END");
    }
    break;
    default:
    {
    }
    break;
    }

    if (button_enter.isPressedEdge())
    {
        // cycle trough states in sequence
        set_time_state = static_cast<Set_time_state>(static_cast<int>(set_time_state) + 1);
        ;
        // Serial.println("State set to:");
        // Serial.println(set_time_state);
    }
    if (button_exit.isPressedEdge())
    {
        set_time_state = state_set_time_end;
    }
}

void alarm_set_state_refresh()
{

    if (button_exit.isPressedEdge() || millis() > watchdog_last_button_press_millis + DELAY_ALARM_AUTO_ESCAPE_MILLIS)
    {
        alarm_set_state = state_alarm_end;
    }

    switch (alarm_set_state)
    {
    case state_alarm_init:
    {
        alarm_set_state = state_alarm_display;
        display_alarm();
    }
    break;
    case state_alarm_display:
    {
        if (button_down.isPressedEdge() || button_up.isPressedEdge() || ((button_alarm.getLongPressCount() == PERIODICAL_EDGES_DELAY) && button_alarm.getLongPressPeriodicalEdge()))
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
        if (button_alarm.isPressedEdge())
        {
            alarm_set_state = state_alarm_set_hours;
        }
        // if (button_alarm.isPressedEdge())
        // {
        //     alarm_set_state = state_alarm_end;
        // }
    }
    break;
    case state_alarm_set_hours:
    {
        if (button_up.isPressedEdge() || button_down.isPressedEdge() || button_up.getLongPressPeriodicalEdge() || button_down.getLongPressPeriodicalEdge())
        {
            nextStepRotate(&alarm_hour, button_up.isPressed(), 0, 23);
            nextBlinkUpdateMillis -= TIME_HALF_BLINK_PERIOD_MILLIS;
            blinker = true;
        }
        if (button_alarm.isPressedEdge())
        {
            alarm_set_state = state_alarm_set_minutes;
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
        if (button_up.isPressedEdge() || button_down.isPressedEdge() || button_up.getLongPressPeriodicalEdge() || button_down.getLongPressPeriodicalEdge())
        {
            nextStepRotate(&alarm_minute, button_up.isPressed(), 0, 59);
            display_alarm();
            nextBlinkUpdateMillis -= TIME_HALF_BLINK_PERIOD_MILLIS;
            blinker = true;
            rtc.setAlarm(alarm_hour, alarm_minute);
        }

        if (button_alarm.isPressedEdge())
        {
            display_alarm();
            alarm_set_state = state_alarm_display;
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
        alarm_set_state = state_alarm_init; // prepare for the next time.
        main_state = state_display_time;

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
        main_state = state_display_time;
    }
    break;
    }
}

void alarm_status_refresh()
{
    switch (alarm_status_state)
    {
    case (state_alarm_status_disable):
    {

        // alarm_user_toggle_action = false;
        buzzer.addNoteToNotesBuffer(G4_4);
        buzzer.addNoteToNotesBuffer(REST_1_8);
        buzzer.addNoteToNotesBuffer(C4_2);
        buzzer.addNoteToNotesBuffer(REST_8_8);
        alarm_status_state = state_alarm_status_is_not_enabled;
        // set_display_indicator_dot(false);
        snooze_count = 0;
        // Serial.println("Disable alarm");
    }
    break;

    case (state_alarm_status_enable):
    {
        // alarm_user_toggle_action = false;
        // buzzer.addRandomSoundToNotesBuffer(0,255);
        buzzer.addNoteToNotesBuffer(C4_4);
        buzzer.addNoteToNotesBuffer(REST_1_8);
        buzzer.addNoteToNotesBuffer(G4_2);
        buzzer.addNoteToNotesBuffer(REST_8_8);
        alarm_status_state = state_alarm_status_is_enabled;
        // set_display_indicator_dot(true);
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
            switch (main_state)
            {
            case state_set_time:
            {
                if (set_time_state != state_set_time_hours &&
                    set_time_state != state_set_time_minutes &&
                    set_time_state != state_set_time_seconds)
                {
                    set_time_state = state_set_time_init;
                    main_state = state_display_time;
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
                if (alarm_set_state != state_alarm_set_hours && alarm_set_state != state_alarm_set_minutes)
                {
                    alarm_set_state = state_alarm_init;
                    main_state = state_display_time;
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
        if (button_1.getValueChanged() ||
            button_2.getValueChanged() ||
            button_0.getValueChanged())
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
        if (button_alarm.isPressed() &&
            millis() > (button_alarm.getLastStateChangeMillis() + ALARM_USER_STOP_BUTTON_PRESS_MILLIS))
        {
            snooze_count = 0;
            alarm_status_state = state_alarm_status_disable;
        }

        // set_display_indicator_dot((millis() % 500) > 250);
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

    // if (kitchen_timer_state != state_mem_tmp)
    // {
    //     Serial.println("kitchen_timer_state");
    //     Serial.println(kitchen_timer_state);
    // }
    // state_mem_tmp = kitchen_timer_state;

    if (millis() > watchdog_last_button_press_millis + DELAY_KITCHEN_TIMER_AUTO_ESCAPE_MILLIS)
    {

        if (kitchen_timer_state == state_stopped)
            kitchen_timer_state = state_exit;
    }

    switch (kitchen_timer_state)
    {
        // todo refresh display and restore "saved state"

    case (state_enter):
    {
        if (kitchenTimer.getIsStarted())
        {
            kitchen_timer_state = state_running_refresh_display;
        }
        else
        {
            kitchen_timer_state = state_stopped_refresh_display;
        }
    }
    break;
    case (state_stopped_refresh_display):
    {

        char *disp;
        disp = visualsManager.getDisplayTextBufHandle();
        if (((millis() - blink_offset) % 1000) < 500)
        {
            kitchenTimer.getTimeString(disp);
        }
        else
        {
            disp[0] = ' ';
            visualsManager.blanksToBuf(disp);
        }
        divider_colon_to_display(true);
        kitchen_timer_state = state_stopped;
    }
    break;
    case (state_stopped):
    {
        if (button_enter.isPressedEdge() || ((button_3.getLongPressCount() == PERIODICAL_EDGES_DELAY) && button_kitchen_timer.getLongPressPeriodicalEdge()))
        {
            kitchen_timer_state = state_running;
            kitchenTimer.start();
            buzzer.addNoteToNotesBuffer(G6_4);
        }
        if (millis() > nextKitchenBlinkUpdateMillis)
        {
            nextKitchenBlinkUpdateMillis = millis() + TIME_HALF_BLINK_PERIOD_MILLIS;
            kitchen_timer_state = state_stopped_refresh_display;
        }

        if (kitchenTimer.getIsStarted())
        {
            kitchen_timer_state = state_running_refresh_display;
        }

        if (button_kitchen_timer.isPressedEdge())
        {
            kitchen_timer_state = state_exit;
        }

        if (button_up.isPressedEdge() || button_down.isPressedEdge() || button_up.getLongPressPeriodicalEdge() || button_down.getLongPressPeriodicalEdge())
        {
            nextStep(&kitchen_timer_set_time_index, button_up.isPressed(), 0, 90, false);
            kitchenTimer.setInitCountDownTimeSecs(timeDialDiscreteSeconds[kitchen_timer_set_time_index]);
            kitchen_timer_state = state_stopped_refresh_display;
            set_blink_offset();
        }
    }
    break;
    case (state_running):
    {
        if (kitchenTimer.getEdgeSinceLastCallFirstGivenHundredsPartOfSecond(500, true, true))
        {
            kitchen_timer_state = state_running_refresh_display;
        }
        else if (button_kitchen_timer.isPressedEdge())
        {
            main_state = state_display_time;
        }
        else if (button_enter.isPressedEdge() || ((button_kitchen_timer.getLongPressCount() == PERIODICAL_EDGES_DELAY) && button_kitchen_timer.getLongPressPeriodicalEdge()))
        {
            kitchenTimer.reset();
            kitchen_timer_state = state_stopped_refresh_display;
            buzzer.addNoteToNotesBuffer(C6_4);
        }
        else if (button_up.isPressedEdge() || button_down.isPressedEdge())
        {
            kitchenTimer.setOffsetInitTimeMillis((1 - 2 * button_up.isPressed()) * 60000);
            kitchen_timer_state = state_running_refresh_display;
        }
    }
    break;
    case (state_running_refresh_display):
    {
        char *disp;
        disp = visualsManager.getDisplayTextBufHandle();
        kitchenTimer.getTimeString(disp);
        divider_colon_to_display(kitchenTimer.getInFirstGivenHundredsPartOfSecond(500));

        kitchen_timer_state = state_running;
    }
    break;
    case (state_exit):
    {
        if (kitchen_timer_set_time_index != EEPROM.read(EEPROM_ADDRESS_KITCHEN_TIMER_INIT_INDEX))
        {
            EEPROM.write(EEPROM_ADDRESS_KITCHEN_TIMER_INIT_INDEX, kitchen_timer_set_time_index);
        }
        main_state = state_display_time;
    }
    break;

    default:
    {
    }
    break;
    }
}

void alarm_kitchen_timer_refresh()
{
    long kitchen_timer_beep = (kitchenTimer.getTimeSeconds() % KITCHEN_TIMER_ENDED_PERIODICAL_BEEP_SECONDS) == 0;
    if (!kitchenTimer.getTimeIsNegative() && kitchenTimerCountingDownMemory)
    {
        uint8_t song_happy_dryer[] = {A6_2, REST_6_8,
                                      Cs7_2, REST_6_8,
                                      E7_4, REST_3_8,
                                      Cs7_4, REST_3_8,
                                      E7_4, REST_3_8,
                                      A7_1, A7_1};
        for (uint8_t i = 0; i < 12; i++)
        {
            buzzer.addNoteToNotesBuffer(song_happy_dryer[i]);
        }
    }
    else if (!kitchenTimer.getTimeIsNegative())
    {
        if (kitchen_timer_beep && !beep_memory)
        {
            buzzer.addNoteToNotesBuffer(Cs7_2);
        }
    }
    kitchenTimerCountingDownMemory = kitchenTimer.getTimeIsNegative();
    beep_memory = kitchen_timer_beep;
}

void dark_mode_refresh()
{

    if (button_brightness.isPressedEdge())
    {
        // Serial.println("back to state display time");
        main_state = state_display_time;
        // hour_minutes_to_display();
        // set_display_indicator_dot((alarm_status_state == state_alarm_status_is_enabled));
        //  Serial.println(brightness);
    }
    else if (button_0.isPressedEdge() || button_2.isPressedEdge() || button_3.isPressedEdge())
    {
        // press button, time is displayed
        // rtc.read();
        // divider_colon_to_display(rtc.second % 2);
        hour_minutes_to_display();
        divider_colon_to_display(true); // have a static time indidation.
        // set_display_indicator_dot((alarm_status_state == state_alarm_status_is_enabled));
    }
    else if (button_0.isUnpressedEdge() || button_2.isUnpressedEdge() || button_3.isUnpressedEdge())
    {
        // release button, clock light off
        visualsManager.setBlankDisplay();
    }
}

void refresh_main_state()
{

    // alarm FSM runs independently
    alarm_status_refresh();
    alarm_kitchen_timer_refresh();

    switch (main_state)
    {
    case state_display_time:
    {
        display_time_state_refresh();
    }
    break;
    case state_set_time:
    {
        main_menu_state_refresh();
    }
    break;
    // case state_set_time:
    // {
    //     set_time_state_refresh();
    // }
    // break;
    case state_night_mode:
    {
        // todo rework. this is not a good way. Dark mode should just skip visuals update, and if a button pressed, allow visual update until depressed.
        dark_mode_refresh();
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
        main_state = state_display_time;
    }
    break;
    }
}

void checkWatchDog()
{
    if (
        button_1.getValueChanged() ||
        button_2.getValueChanged() ||
        button_0.getValueChanged() ||
        button_3.getValueChanged() ||
        button_1.getLongPressPeriodicalEdge() ||
        button_2.getLongPressPeriodicalEdge() ||
        button_0.getLongPressPeriodicalEdge() ||
        button_3.getLongPressPeriodicalEdge())
    {
        watchdog_last_button_press_millis = millis();
    }
}

void updateTimeNow()
{
    if (millis() > nextTimeUpdateMillis)
    {
        nextTimeUpdateMillis = millis() + TIME_UPDATE_DELAY;

        // limit calls to peripheral by only loading time periodically
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
    button_2.refresh();
    button_1.refresh();
    button_0.refresh();
    button_3.refresh();

    // process
    updateTimeNow();
    checkWatchDog();
    refresh_main_state();
    refresh_indicator_dot();

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
