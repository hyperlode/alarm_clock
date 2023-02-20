
// #define ENABLE_SERIAL
// #define ENABLE_MEASURE_CYCLE_TIME
// #define PROTOTYPE_BIG_BOX
// #define PROTOTYPE_GRAVITY_RTC

#ifdef PROTOTYPE_GRAVITY_RTC
#include "GravityRtc.h"
#else
#include "DS3231.h"
#endif

#include "Wire.h"
#include "Button.h"
#include "DisplayDigitsHandler5Digits.h"
#include "LedMultiplexer5x8.h"
#include "Buzzer.h"
#include <EEPROM.h>
#include "SuperTimer.h"

#define DELAY_TO_REDUCE_LIGHT_FLICKER_MILLIS 1 // if we iterate too fast through the loop, the display gets refreshed so quickly that it never really settles down. Off time at transistions beats ON time. So, with a dealy, we increase the ON time a tad.
#define DISPLAY_IS_COMMON_ANODE true           // check led displays both displays should be of same type   //also set in SevSeg5Digits.h : MODEISCOMMONANODE
#define BRIGHTNESS_LEVELS 3
#define DEFAULT_BRIGHTNESS_LEVEL_INDEX 3

#define EEPROM_VALID_VALUE 67
#define FACTORY_DEFAULT_KITCHEN_TIMER 10
#define FACTORY_DEFAULT_SNOOZE_TIME_MINUTES 9
#define FACTORY_DEFAULT_ALARM_HOUR 7
#define FACTORY_DEFAULT_ALARM_MINUTE 30
#define FACTORY_DEFAULT_HOURLY_BEEP_ENABLED 0
#define FACTORY_DEFAULT_ALARM_SET_MEMORY 0
#define FACTORY_DEFAULT_ALARM_IS_SNOOZING 0
#define FACTORY_DEFAULT_ALARM_ENABLE_SNOOZE_TIME_DECREASE 0
#define FACTORY_DEFAULT_ALARM_TUNE 0

#define EEPROM_ADDRESS_EEPROM_VALID 0                      // 1 byte
#define EEPROM_ADDRESS_ALARM_HOUR 1                        // 1 byte
#define EEPROM_ADDRESS_ALARM_MINUTE 2                      // 1 byte
#define EEPROM_ADDRESS_KITCHEN_TIMER_INIT_INDEX 3          // 1 byte
#define EEPROM_ADDRESS_SNOOZE_TIME_MINUTES 4               // 1 byte
#define EEPROM_ADDRESS_HOURLY_BEEP_ENABLED 5               // 1 byte
#define EEPROM_ADDRESS_ALARM_SET_MEMORY 6                  // 1 byte
#define EEPROM_ADDRESS_ALARM_IS_SNOOZING 7                 // 1 byte
#define EEPROM_ADDRESS_ALARM_ENABLE_SNOOZE_TIME_DECREASE 8 // 1 byte
#define EEPROM_ADDRESS_ALARM_TUNE 9                        // 1 byte

#define PIN_DUMMY 66
#define PIN_DUMMY_2 22 // randomly chosen. I've had it set to 67, and at some point, multiple segments were lit up. This STILL is C hey, it's gonna chug on forever!

// DO NOT USE pin 3 and 11 for PWM if working with tone
// the common anode pins work with pwm. pwm and tone libraries interfere.

#ifdef PROTOTYPE_BIG_BOX
#define PIN_DISPLAY_DIGIT_0 5 // swapped
#define PIN_DISPLAY_DIGIT_1 10
#define PIN_DISPLAY_DIGIT_2 9
#define PIN_DISPLAY_DIGIT_3 6 // swapped!
#else
#define PIN_DISPLAY_DIGIT_0 6
#define PIN_DISPLAY_DIGIT_1 9
#define PIN_DISPLAY_DIGIT_2 10
#define PIN_DISPLAY_DIGIT_3 5
#endif

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

#define PIN_BUTTON_0 A1
#define PIN_BUTTON_1 A2

#ifdef PROTOTYPE_BIG_BOX
#define PIN_BUTTON_2 2
#define PIN_BUTTON_3 A3
#else
#define PIN_BUTTON_2 A3
#define PIN_BUTTON_3 2

#endif

#define button_down button_2
#define button_up button_3
#define button_exit button_1
#define button_enter button_0

#define button_menu button_3
#define button_brightness button_2
#define button_kitchen_timer button_1
#define button_alarm button_0

#define TIME_UPDATE_DELAY 1000
#define DOT_UPDATE_DELAY 250
#define TIME_HALF_BLINK_PERIOD_MILLIS 250
#define DELAY_ALARM_AUTO_ESCAPE_MILLIS 5000
#define DELAY_KITCHEN_TIMER_AUTO_ESCAPE_MILLIS 5000
#define MAIN_MENU_DISPLAY_ITEMS_AUTO_ESCAPE_MILLIS 8000
#define MAIN_MENU_MODIFY_ITEMS_AUTO_ESCAPE_MILLIS 4000
#define ALARM_USER_STOP_BUTTON_PRESS_MILLIS 1000
#define KITCHEN_TIMER_ENDED_PERIODICAL_BEEP_SECONDS 60
#define PERIODICAL_EDGES_DELAY 2 // used to delay long press time. 0 = first long press period occurence, e.g. 5  = 5 long press periods delay

DisplayManagement visualsManager;
LedMultiplexer5x8 ledDisplay;

#ifdef PROTOTYPE_GRAVITY_RTC
GravityRtc rtc; // RTC Initialization
#else
DS3231 rtcDS3231;
#endif

Button button_2;
Button button_1;
Button button_0;
Button button_3;

Buzzer buzzer;
SuperTimer kitchenTimer;
bool kitchenTimerCountingDownMemory;
bool beep_memory;

long alarm_started_millis;

long nextTimeUpdateMillis;
long nextBlinkUpdateMillis;
long nextKitchenBlinkUpdateMillis;
long nextDisplayTimeUpdateMillis;
long blink_offset;

bool display_dot_status_memory;

#define CYCLE_TIMES_WINDOW_SIZE 100
uint8_t cycle_time_counter;
long cycle_times_micros[CYCLE_TIMES_WINDOW_SIZE + 1];
long max_cycle;
long min_cycle;

long watchdog_last_button_press_millis;
bool hourly_beep_done_memory;
uint8_t alarm_hour;
uint8_t alarm_minute;
uint8_t alarm_hour_snooze;
uint8_t alarm_minute_snooze;
int8_t alarm_snooze_duration_minutes;
uint8_t snooze_count;
uint16_t total_snooze_time_minutes;
int8_t alarm_tune_index;
char tune_name_buf[4];

uint8_t brightness_index;
bool blinker;
bool hourly_beep_enabled;
bool enable_snooze_time_decrease;

int8_t time_set_index_helper;
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

#define MENU_MENU_ITEMS_COUNT 5
const byte menu_item_titles[] PROGMEM = {
    'T', 'S', 'E', 'T',
    'S', 'N', 'O', 'O',
    'B', 'E', 'E', 'P',
    'D', 'E', 'C', 'R',
    'T', 'U', 'N', 'E'};
#define MAIN_MENU_ITEM_TIME_SET 0
#define MAIN_MENU_ITEM_SNOOZE_TIME 1
#define MAIN_MENU_ITEM_ENABLE_HOURLY_BEEP 2
#define MAIN_MENU_ITEM_ENABLE_SNOOZE_TIME_DECREASE 3
#define MAIN_MENU_ITEM_TUNE 4

#define TUNES_COUNT 6

#define LEN_TUNE_DRYER_HAPPY 12
#define LEN_TUNE_ATTACK 13
#define LEN_TUNE_DRYER_UNHAPPY 11
#define LEN_TUNE_RETREAT 14
#define LEN_TUNE_ALPHABET 77
// uint8_t tune_lengths[] = {LEN_TUNE_DRYER_HAPPY, LEN_TUNE_ATTACK, LEN_TUNE_DRYER_UNHAPPY, LEN_TUNE_RETREAT, LEN_TUNE_ALPHABET};

const byte menu_item_tune_names[] PROGMEM = {
    'F', 'U', 'N', ' ',
    'A', 'T', 'A', 'C',
    'B', 'L', 'A', 'H',
    'R', 'E', 'T', 'R',
    'A', 'L', 'F', 'A',
    'B', 'I', 'L', 'D'};

#define TUNE_DRYER_HAPPY 0
#define TUNE_ATTACK 1
#define TUNE_DRYER_UNHAPPY 2
#define TUNE_RETREAT 3
#define TUNE_ALPHABET 4
#define TUNE_BUILDUP 5

// https://forum.arduino.cc/t/multi-dimensional-array-in-progmem-arduino-noob/45822/2
// const uint8_t tunes[][100] PROGMEM = {
//     {,,},
//     {,,}
// };
// byte firstnoteof song = pgm_read_byte(&(tunes[song_index][0]));

// one big library. tune lengths in separate array. This is the easiest option.
const uint8_t tune_dryer_happy[] PROGMEM = {
    // happy dryer
    A6_2, REST_6_8,
    Cs7_2, REST_6_8,
    E7_4, REST_3_8,
    Cs7_4, REST_3_8,
    E7_4, REST_3_8,
    A7_1, A7_1};

const uint8_t tune_attack[] PROGMEM = {
    // aaanvallueeeeee!
    Gs6_2, REST_2_8, Gs6_2, REST_2_8, Gs6_2, REST_2_8, Cs7_2, REST_8_8, Gs6_2, REST_2_8, Cs7_1, Cs7_1, Cs7_1};

const uint8_t tune_dryer_unhappy[] PROGMEM = {
    // unhappy dryer
    A6_1, REST_4_8, Cs7_1, REST_4_8, E7_2, REST_2_8, Cs7_2, REST_2_8, B6_2, REST_2_8, A6_1};

const uint8_t tune_retreat[] PROGMEM = {
    // retreat tune
    Gs6_2, REST_2_8, Gs6_2, REST_2_8, Gs6_2, REST_2_8, Gs6_2, REST_4_8, REST_4_8, Gs6_2, REST_2_8, Cs6_1, Cs6_1, Cs6_1};

const uint8_t tune_alphabet[] PROGMEM = {
    // alphabet tune
    C7_2, REST_12_8,
    C7_2, REST_12_8,
    G7_2, REST_12_8,
    G7_2, REST_12_8,
    A7_2, REST_12_8,
    A7_2, REST_12_8,
    G7_1, G7_1, REST_15_8,
    F7_2, REST_12_8, F7_2, REST_12_8, E7_2, REST_12_8, E7_2, REST_12_8, D7_2, REST_4_8, D7_2, REST_4_8, D7_2, REST_4_8, D7_2, REST_4_8, C7_1, C7_1, REST_15_8,
    G7_2, REST_12_8, G7_2, REST_12_8, F7_2, REST_12_8, F7_2, REST_12_8, E7_2, REST_12_8, E7_2, REST_12_8, D7_1, D7_1, REST_15_8, C7_2, REST_12_8,
    C7_2, REST_12_8, G7_2, REST_12_8, G7_2, REST_12_8, A7_1, A7_1, REST_15_8, G7_2, REST_13_8, REST_15_8,
    F7_2, REST_12_8, F7_2, REST_12_8, E7_2, REST_12_8, E7_2, REST_12_8, D7_2, REST_12_8, G7_2, REST_12_8, C7_1, C7_1};

enum Time_type : uint8_t
{
    hours = 0,
    minutes = 1,
    seconds = 2
};

enum Main_state : uint8_t
{
    state_display_time = 0,
    state_menu,
    state_dark_mode,
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
    state_main_menu_modify_item = 3,
    state_main_menu_save_and_back_to_menu = 4

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
    state_alarm_status_disable,
    state_alarm_status_buzzing
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

bool millis_blink_100_900ms()
{
    // true for shorter part of the second
    return (millis() - blink_offset) % 1000 > 900;
}

void set_blink_offset()
{
    // keeps blink return to the same value. e.g. menu blinking between value and text. while dial rotates, we want to only see the value.
    blink_offset = millis() % 1000;
}

uint16_t get_brightness_index_to_uSeconds_delay(uint8_t index)
{
#if (BRIGHTNESS_LEVELS == 4)
    uint8_t brightness_settings[] = {0, 1, 10, 80, 254}; // do not use 255, it creates an after glow once enabled. (TODO: why?!) zero is dark. but, maybe you want that... e.g. alarm active without display showing.
#elif (BRIGHTNESS_LEVELS == 3)
    // uint16_t brightness_settings[] = {100, 300, 100, 0};
    uint16_t brightness_settings[] = {100, 18000, 4000, 0}; // useconds all off time (between lit up leds)

#endif
    return brightness_settings[index];
}

void cycleBrightness(bool init)
{

    // #define CYCLING_GOES_BRIGHTER

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
        main_state = state_dark_mode;
        brightness_index = BRIGHTNESS_LEVELS;
    }
    ledDisplay.setBrightness(get_brightness_index_to_uSeconds_delay(brightness_index), false);
#ifdef ENABLE_SERIAL
    Serial.println(get_brightness_index_to_uSeconds_delay(brightness_index));

#endif
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

void divider_colon_to_display(bool active)
{
    visualsManager.setDecimalPointToDisplay(active, 1);
}

void refresh_indicator_dot()
{

    // alarm going off or snoozed has priority
    if (alarm_status_state == state_alarm_status_snoozing)
    {
        set_display_indicator_dot((millis() % 250) > 125);
    }
    else if (alarm_status_state == state_alarm_status_buzzing)
    {
        set_display_indicator_dot(true);
    }

    else if (main_state == state_dark_mode ){
        // do not set here. 
    }
    else if (main_state == state_display_time )
    {

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
// void set_display_indicator_dot(bool active, bool force_it=false)
{
    // only update display if "different"
    // if (active != display_dot_status_memory)
    // {
        visualsManager.setDecimalPointToDisplay(active, 0);
    // }
    display_dot_status_memory = active;
}

void seconds_to_display()
{
    int16_t timeAsNumber;
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
    timeAsNumber = 100 * (int16_t)minute_now + (int16_t)second_now;
    visualsManager.setNumberToDisplay(timeAsNumber, false);
    add_leading_zeros(timeAsNumber, true);
    divider_colon_to_display(second_now % 2);
}

void hour_minutes_to_display()
{
    int16_t timeAsNumber;
    timeAsNumber = 100 * ((int16_t)hour_now) + (int16_t)minute_now;
    visualsManager.setNumberToDisplay(timeAsNumber, false);
    add_leading_zeros(timeAsNumber, false);
    divider_colon_to_display(second_now % 2);
}

void set_time(Time_type t)
{
    if (button_down.isPressedEdge() || button_up.isPressedEdge() || button_down.getLongPressPeriodicalEdge() || button_up.getLongPressPeriodicalEdge())
    {
#ifdef PROTOTYPE_GRAVITY_RTC

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
#else

        if (t == hours)
        {
            nextStepRotate(&hour_now, button_up.isPressed(), 0, 23);
            rtcDS3231.setHour(hour_now);

            nextBlinkUpdateMillis -= TIME_HALF_BLINK_PERIOD_MILLIS;
            blinker = true;
            hour_minutes_to_display();
        }
        else if (t == minutes)
        {
            nextStepRotate(&minute_now, button_up.isPressed(), 0, 59);
            rtcDS3231.setMinute(minute_now);
            nextBlinkUpdateMillis -= TIME_HALF_BLINK_PERIOD_MILLIS;
            blinker = true;
            hour_minutes_to_display();
        }
        else if (t == seconds)
        {
            rtcDS3231.setSecond(0);
            second_now = rtcDS3231.getSecond();

            nextBlinkUpdateMillis -= TIME_HALF_BLINK_PERIOD_MILLIS;
            blinker = true;
            seconds_to_display();
        }
#endif
        // display updated time here. Even in the "off time" of the blinking process, it will still display the change for the remainder of the off-time. This is good.
    }

    if (millis() > nextBlinkUpdateMillis)
    {

        nextBlinkUpdateMillis = millis() + TIME_HALF_BLINK_PERIOD_MILLIS;

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

    if (millis() > nextDisplayTimeUpdateMillis)
    {
        nextDisplayTimeUpdateMillis = millis() + TIME_UPDATE_DELAY;

        hour_minutes_to_display();
    }

    if (button_alarm.isPressedEdge())
    {
        main_state = state_alarm_set;
    }

    if (alarm_status_state == state_alarm_status_triggered)
    {
        // key presses for non alarm functions only serve to quiet down the alarm when it's going off.
        return;
    }

    if (button_menu.isPressedEdge())
    {
        // main_state = static_cast<Main_state>(static_cast<int>(main_state) + 1);;
        main_state = state_menu;
    }

    if (button_kitchen_timer.isPressedEdge())
    {
        // kitchen_timer_refresh_display();
        kitchen_timer_state = state_enter;
        main_state = state_kitchen_timer;
    }

    if (button_brightness.isPressedEdge())
    {
        cycleBrightness(false);
    }
}

void eeprom_write_byte_if_changed(int address, uint8_t value)
{
    if (value != EEPROM.read(address))
    {
        EEPROM.write(address, value);
    }
}

void main_menu_state_refresh()
{
    switch (main_menu_state)
    {
    case (state_main_menu_init):
    {
        main_menu_state = state_main_menu_display_item;
        main_menu_item_index = 0; // by commenting this, preserve menu position. But, in reality, there was no need for this
        main_menu_display_update = true;
        divider_colon_to_display(false);
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
            time_set_index_helper = 0;
        }
        if (button_exit.isPressedEdge() || (millis() > watchdog_last_button_press_millis + MAIN_MENU_DISPLAY_ITEMS_AUTO_ESCAPE_MILLIS))
        {
            main_menu_state = state_main_menu_exit;
        }
        if (button_up.isPressedEdge() || button_down.isPressedEdge())
        {
            // Serial.println(main_menu_item_index);
            //  one cycle through menu and back to main menu.
            if (main_menu_item_index == MENU_MENU_ITEMS_COUNT - 1 && button_up.isPressed())
            {
                main_menu_state = state_main_menu_exit;
            }

            nextStepRotate(&main_menu_item_index, button_up.isPressed(), 0, MENU_MENU_ITEMS_COUNT - 1);

            main_menu_display_update = true;
            set_blink_offset();
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
                divider_colon_to_display(true);
            }
            break;
            case (MAIN_MENU_ITEM_SNOOZE_TIME):
            {
                visualsManager.setNumberToDisplay(alarm_snooze_duration_minutes, false);
            }
            break;
            case (MAIN_MENU_ITEM_ENABLE_HOURLY_BEEP):
            {
                visualsManager.setBoolToDisplay(hourly_beep_enabled);
            }
            break;
            case (MAIN_MENU_ITEM_TUNE):
            {

                visualsManager.setTextBufToDisplay(tune_name_buf);
            }
            break;
            case (MAIN_MENU_ITEM_ENABLE_SNOOZE_TIME_DECREASE):
            {
                visualsManager.setBoolToDisplay(enable_snooze_time_decrease);
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
            divider_colon_to_display(false);
            // visualsManager.setBoolToDisplay(true);
        }
    }
    break;

    case (state_main_menu_save_and_back_to_menu):
    {
        main_menu_state = state_main_menu_display_item;

        eeprom_write_byte_if_changed(EEPROM_ADDRESS_SNOOZE_TIME_MINUTES, alarm_snooze_duration_minutes);
        eeprom_write_byte_if_changed(EEPROM_ADDRESS_HOURLY_BEEP_ENABLED, hourly_beep_enabled);
        eeprom_write_byte_if_changed(EEPROM_ADDRESS_ALARM_ENABLE_SNOOZE_TIME_DECREASE, enable_snooze_time_decrease);
        eeprom_write_byte_if_changed(EEPROM_ADDRESS_ALARM_TUNE, alarm_tune_index);
    }
    break;
    case (state_main_menu_modify_item):
    {
        if (button_exit.isPressedEdge() || (millis() > watchdog_last_button_press_millis + MAIN_MENU_MODIFY_ITEMS_AUTO_ESCAPE_MILLIS))
        {
            main_menu_state = state_main_menu_save_and_back_to_menu;
        }

        switch (main_menu_item_index)
        {
        case (MAIN_MENU_ITEM_TIME_SET):
        {
            switch (time_set_index_helper)
            {
            case 0:
            {
                set_time(hours);
            }
            break;

            case 1:
            {
                set_time(minutes);
            }
            break;
            case 2:
            {
                set_time(seconds);
            }
            break;
            }
            if (button_enter.isPressedEdge())
            {
                time_set_index_helper++;
                if (time_set_index_helper == 3)
                {
                    main_menu_state = state_main_menu_save_and_back_to_menu;
                }
                // nextStepRotate(&time_set_index_helper, true, 0, 2);
            };
        }
        break;
        case (MAIN_MENU_ITEM_TUNE):
        {
            if (button_enter.isPressedEdge())
            {
                main_menu_state = state_main_menu_save_and_back_to_menu;
            }
            if (button_up.isPressedEdge() || button_down.isPressedEdge())
            {
                nextStepRotate(&alarm_tune_index, button_up.isPressed() || button_enter.isPressed(), 0, TUNES_COUNT - 1);

                for (uint8_t i = 0; i < 4; i++)
                {
                    tune_name_buf[i] = pgm_read_byte_near(menu_item_tune_names + 4 * alarm_tune_index + i);
                }
                buzzer.clearBuzzerNotesBuffer();
                play_tune(alarm_tune_index);
            }
            visualsManager.setTextBufToDisplay(tune_name_buf);
        }
        break;
        case (MAIN_MENU_ITEM_SNOOZE_TIME):
        {
            if (button_enter.isPressedEdge())
            {
                main_menu_state = state_main_menu_save_and_back_to_menu;
            }
            visualsManager.setNumberToDisplay(alarm_snooze_duration_minutes, false);
            if (button_up.isPressedEdge() || button_down.isPressedEdge())
            {
                nextStep(&alarm_snooze_duration_minutes, button_up.isPressed(), 0, 127, false);
            }
        }
        break;
        case (MAIN_MENU_ITEM_ENABLE_HOURLY_BEEP):
        {
            visualsManager.setBoolToDisplay(hourly_beep_enabled);
            if (button_enter.isPressedEdge())
            {
                main_menu_state = state_main_menu_save_and_back_to_menu;
            }
            if (button_up.isPressedEdge() || button_down.isPressedEdge())
            // if (button_up.isPressedEdge() || button_down.isPressedEdge() || button_enter.isPressedEdge())
            {
                hourly_beep_enabled = !hourly_beep_enabled;
            }
        }
        break;
        case (MAIN_MENU_ITEM_ENABLE_SNOOZE_TIME_DECREASE):
        {
            if (button_enter.isPressedEdge())
            {
                main_menu_state = state_main_menu_save_and_back_to_menu;
            }
            visualsManager.setBoolToDisplay(enable_snooze_time_decrease);
            if (button_up.isPressedEdge() || button_down.isPressedEdge())
            {
                enable_snooze_time_decrease = !enable_snooze_time_decrease;
            }
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

    if (millis() > watchdog_last_button_press_millis + DELAY_ALARM_AUTO_ESCAPE_MILLIS)
    {
        alarm_set_state = state_alarm_end;
    }

    if (button_exit.isPressedEdge() || ((button_alarm.getLongPressCount() == PERIODICAL_EDGES_DELAY) && button_alarm.getLongPressPeriodicalEdge()))
    {
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

    switch (alarm_set_state)
    {
    case state_alarm_init:
    {
        alarm_set_state = state_alarm_set_hours;
        // alarm_set_state = state_alarm_display;
        display_alarm();
    }
    break;
    case state_alarm_display:
    {
        if (button_down.isPressedEdge() || button_up.isPressedEdge() || ((button_alarm.getLongPressCount() == PERIODICAL_EDGES_DELAY) && button_alarm.getLongPressPeriodicalEdge()))
        {
            // Serial.println(PERIODICAL_EDGES_DELAY);

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
        if (button_alarm.isPressedEdge())
        {
            alarm_set_state = state_alarm_end;
        }
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
#ifdef PROTOTYPE_GRAVITY_RTC

            rtc.setAlarm(alarm_hour, alarm_minute);
#else

#endif
        }

        if (button_alarm.isPressedEdge())
        {
            display_alarm();
            // alarm_set_state = state_alarm_display;
            alarm_set_state = state_alarm_end;
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

        eeprom_write_byte_if_changed(EEPROM_ADDRESS_ALARM_HOUR, alarm_hour);
        eeprom_write_byte_if_changed(EEPROM_ADDRESS_ALARM_MINUTE, alarm_minute);
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
        total_snooze_time_minutes = 0;
        eeprom_write_byte_if_changed(EEPROM_ADDRESS_ALARM_SET_MEMORY, 0);
        eeprom_write_byte_if_changed(EEPROM_ADDRESS_ALARM_IS_SNOOZING, 0);
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
        total_snooze_time_minutes = 0;
        // Serial.println("Enable alarm");

        eeprom_write_byte_if_changed(EEPROM_ADDRESS_ALARM_SET_MEMORY, 255);
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
            case state_menu:
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
            case state_dark_mode:
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
        alarm_started_millis = millis();
        alarm_status_state = state_alarm_status_buzzing;
    }
    break;
    case (state_alarm_status_buzzing):
    {

        if (button_menu.getValueChanged() ||
            button_brightness.getValueChanged() ||
            button_alarm.getValueChanged() ||
            button_kitchen_timer.getValueChanged())
        {
            // set snooze time
            snooze_count++;
            long time_stamp_minutes = 60 * alarm_hour + alarm_minute;

            if (enable_snooze_time_decrease)
            {
                int8_t snooze_time_increment = (alarm_snooze_duration_minutes - (snooze_count - 1));
                if (snooze_time_increment < 1)
                {
                    // minimum one minute snoozing.
                    snooze_time_increment = 1;
                }
                total_snooze_time_minutes += snooze_time_increment;
            }
            else
            {
                total_snooze_time_minutes += alarm_snooze_duration_minutes;
            }
            time_stamp_minutes += (total_snooze_time_minutes);
            time_stamp_minutes %= 24 * 60;
            alarm_hour_snooze = time_stamp_minutes / 60;
            alarm_minute_snooze = time_stamp_minutes % 60;

            alarm_status_state = state_alarm_status_snoozing;
            eeprom_write_byte_if_changed(EEPROM_ADDRESS_ALARM_IS_SNOOZING, 255);

            buzzer.clearBuzzerNotesBuffer();
        }
        else
        { // make sure to add else if here, otherwise buffer does not stay empty ast snooze press..
            play_tune(alarm_tune_index);
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

void play_tune(uint8_t tune_index)
{
    if (buzzer.getBuzzerNotesBufferEmpty())
    // add notes when buffer empty and alarm ringing
    {
        switch (tune_index)
        {
        case (TUNE_DRYER_HAPPY):
        {
            for (uint8_t i = 0; i < LEN_TUNE_DRYER_HAPPY; i++)
            {
                byte note = pgm_read_byte_near(tune_dryer_happy + i);
                buzzer.addNoteToNotesBuffer(note);
            }
        }
        break;
        case (TUNE_ATTACK):
        {
            for (uint8_t i = 0; i < LEN_TUNE_ATTACK; i++)
            {
                byte note = pgm_read_byte_near(tune_attack + i);
                buzzer.addNoteToNotesBuffer(note);
            }
        }
        break;
        case (TUNE_RETREAT):
        {
            for (uint8_t i = 0; i < LEN_TUNE_RETREAT; i++)
            {
                byte note = pgm_read_byte_near(tune_retreat + i);
                buzzer.addNoteToNotesBuffer(note);
            }
        }
        break;
        case (TUNE_ALPHABET):
        {
            for (uint8_t i = 0; i < LEN_TUNE_ALPHABET; i++)
            {
                byte note = pgm_read_byte_near(tune_alphabet + i);
                buzzer.addNoteToNotesBuffer(note);
            }
        }
        break;
        case (TUNE_DRYER_UNHAPPY):
        {
            for (uint8_t i = 0; i < LEN_TUNE_DRYER_UNHAPPY; i++)
            {
                byte note = pgm_read_byte_near(tune_dryer_unhappy + i);
                buzzer.addNoteToNotesBuffer(note);
            }
        }
        break;
        case (TUNE_BUILDUP):
        {

            long time_since_start_millis = millis() - alarm_started_millis;
            uint16_t seconds_since_start = (uint16_t)(time_since_start_millis / 1000);

            uint16_t break_count;
            buzzer.addNoteToNotesBuffer(D7_1);
            if (seconds_since_start >= 20)
            {
                break_count = 1;
            }
            else
            {
                break_count = 1 + (400 - (uint16_t)((time_since_start_millis * time_since_start_millis) / 1000000)) / 4;
            }

            // buzzer.addNoteToNotesBuffer(random(10, 58));
            // for (uint8_t i = 0; i < 2; i++)
            for (uint8_t i = 0; i < break_count; i++)
            {
                buzzer.addNoteToNotesBuffer(REST_1_8);
            }
        }
        break;
        default:
        {
            buzzer.addNoteToNotesBuffer(C6_1);
            buzzer.addNoteToNotesBuffer(REST_2_8);
        }
        break;
        };
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
        if (button_kitchen_timer.isPressedEdge() || ((button_kitchen_timer.getLongPressCount() == PERIODICAL_EDGES_DELAY) && button_kitchen_timer.getLongPressPeriodicalEdge()))
        // if (button_kitchen_timer.isPressedEdge() )
        {
            button_kitchen_timer.setLongPressCount(666); // effectively disable more long press detections. This fixes the start/stop bug (because it still detects a long press from when it was started/stopped if activated with a normal edge.)
            kitchen_timer_state = state_running;
            kitchenTimer.start();
            buzzer.addNoteToNotesBuffer(G6_4);
            eeprom_write_byte_if_changed(EEPROM_ADDRESS_KITCHEN_TIMER_INIT_INDEX, kitchen_timer_set_time_index);
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

        if (button_alarm.isPressedEdge())
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
        else if (button_alarm.isPressedEdge())
        {
            main_state = state_display_time;
        }
        else if (button_kitchen_timer.isPressedEdge() || ((button_kitchen_timer.getLongPressCount() == PERIODICAL_EDGES_DELAY) && button_kitchen_timer.getLongPressPeriodicalEdge()))
        {
            button_kitchen_timer.setLongPressCount(666); // effectively disable more long press detections. This fixes the start/stop bug (because it still detects a long press from when it was started/stopped if activated with a normal edge.)
            kitchenTimer.reset();
            kitchen_timer_state = state_stopped_refresh_display;
            buzzer.addNoteToNotesBuffer(C6_4);
        }
        else if (button_up.isPressedEdge() || button_down.isPressedEdge())
        {
            kitchenTimer.setOffsetInitTimeMillis((1 - 2 * button_down.isPressed()) * 60000);
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
        uint8_t tune_happy_dryer[] = {A6_2, REST_6_8,
                                      Cs7_2, REST_6_8,
                                      E7_4, REST_3_8,
                                      Cs7_4, REST_3_8,
                                      E7_4, REST_3_8,
                                      A7_1, A7_1};
        for (uint8_t i = 0; i < 12; i++)
        {
            buzzer.addNoteToNotesBuffer(tune_happy_dryer[i]);
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
        main_state = state_display_time;
    }
    else if (button_kitchen_timer.isPressedEdge() || button_alarm.isPressedEdge() || button_menu.isPressedEdge())
    {
        // press button, time is displayed
        hour_minutes_to_display();
        divider_colon_to_display(true); // have a static time indication.
        
        if (alarm_status_state == state_alarm_status_is_enabled)
        {
            set_display_indicator_dot(true);
        }
        else
        {
            set_display_indicator_dot(false);
        }
        // display_time_state_refresh();

        //ledDisplay.setBrightness(get_brightness_index_to_uSeconds_delay(2), false);


        // nextDisplayTimeUpdateMillis = millis() + TIME_UPDATE_DELAY;

        // hour_minutes_to_display();
    

    // if (button_alarm.isPressedEdge())
    // {
    //     main_state = state_alarm_set;
    // }


    }
    else if (button_kitchen_timer.isUnpressedEdge() || button_alarm.isUnpressedEdge() || button_menu.isUnpressedEdge())
    {
        // release button, clock light off
        visualsManager.setBlankDisplay();
        ledDisplay.setBrightness(get_brightness_index_to_uSeconds_delay(0), false);
        set_display_indicator_dot(false);
    }
}

void refresh_main_state()
{

    switch (main_state)
    {
    case state_display_time:
    {
        display_time_state_refresh();
    }
    break;
    case state_menu:
    {
        main_menu_state_refresh();
    }
    break;
    case state_dark_mode:
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
        button_0.isPressed() ||
        button_1.isPressed() ||
        button_2.isPressed() ||
        button_3.isPressed())
    {
        watchdog_last_button_press_millis = millis();
    }
}

void checkHourlyBeep()
{
    if (minute_now == 0 && !hourly_beep_done_memory)
    {
        if (hourly_beep_enabled)
        {
            buzzer.addNoteToNotesBuffer(E8_8);
        }
    }

    hourly_beep_done_memory = (minute_now == 0);
}

void updateTimeNow()
{
    if (millis() > nextTimeUpdateMillis)
    {
        nextTimeUpdateMillis = millis() + TIME_UPDATE_DELAY;

// limit calls to peripheral by only loading time periodically
#ifdef PROTOTYPE_GRAVITY_RTC

        rtc.read();
        hour_now = rtc.hour;
        minute_now = rtc.minute;
        second_now = rtc.second;
#else
        bool is_h12;
        bool is_PM_time;
        hour_now = rtcDS3231.getHour(is_h12, is_PM_time);
        minute_now = rtcDS3231.getMinute();
        second_now = rtcDS3231.getSecond();

#endif
        // Serial.println(second_now);
    }
}

void measure_cycle_time()
{

    if (cycle_time_counter >= CYCLE_TIMES_WINDOW_SIZE + 1)
    {
        cycle_time_counter = 0;
        long sum = 0;
        for (uint8_t i = 0; i < CYCLE_TIMES_WINDOW_SIZE; i++)
        {
            long diff = (cycle_times_micros[i + 1] - cycle_times_micros[i]);
            sum += diff;
            // Serial.println(diff);
        }
        float average;
        average = 1.0 * (float)(sum) / CYCLE_TIMES_WINDOW_SIZE;
#ifdef ENABLE_SERIAL
        Serial.println(average);
#endif
        // Serial.println("********************");
    }
    else
    {
        // disregard processing cycle
        cycle_times_micros[cycle_time_counter] = micros();
        cycle_time_counter++;
    }
}

void main_application_loop()
{

    // input
    ledDisplay.refresh();
    button_0.refresh();
    ledDisplay.refresh();
    button_1.refresh();
    ledDisplay.refresh();
    button_2.refresh();
    ledDisplay.refresh();
    button_3.refresh();

    ledDisplay.refresh();

    // process
    updateTimeNow();
    ledDisplay.refresh();
    checkWatchDog();
    ledDisplay.refresh();
    checkHourlyBeep();
    ledDisplay.refresh();
    refresh_main_state();
    ledDisplay.refresh();
    refresh_indicator_dot();
    ledDisplay.refresh();
    alarm_status_refresh(); // needs to go after main state loop (to check for button press at time of alarm triggered not doing normal alarm function)
    ledDisplay.refresh();
    alarm_kitchen_timer_refresh();
    ledDisplay.refresh();

    // output
    buzzer.checkAndPlayNotesBuffer();
    ledDisplay.refresh();
    visualsManager.refresh();
    ledDisplay.refresh();
    // delay(DELAY_TO_REDUCE_LIGHT_FLICKER_MILLIS);
}

// ****DS3231 test****
// DS3231 rtcDS3231;

// bool century = false;
// bool h12Flag;
// bool pmFlag;

void setup()
{

#ifdef ENABLE_SERIAL
    Serial.begin(9600);
#endif

#ifdef PROTOTYPE_GRAVITY_RTC
    //  for (int i=0; i<5; i++){
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
    //}
#else

    // **** TEST DS3231 test
    // // Start the I2C interface
    // 	Wire.begin();
    //   for (int i=0; i<5; i++){
    //       delay(1000);
    //       Serial.print(rtcDS3231.getYear(), DEC);
    //       Serial.print("-");
    //       Serial.print(rtcDS3231.getMonth(century), DEC);
    //       Serial.print("-");
    //       Serial.print(rtcDS3231.getDate(), DEC);
    //       Serial.print(" ");
    //       Serial.print(rtcDS3231.getHour(h12Flag, pmFlag), DEC); //24-hr
    //       Serial.print(":");
    //       Serial.print(rtcDS3231.getMinute(), DEC);
    //       Serial.print(":");
    //       Serial.println(rtcDS3231.getSecond(), DEC);
    //   }
#endif

    blinker = false;
#ifdef PROTOTYPE_GRAVITY_RTC

    rtc.setup();

// Set the RTC time automatically: Calibrate RTC time by your computer time
// rtc.adjustRtc(F(__DATE__), F(__TIME__));

// Set the RTC time manually
// rtc.adjustRtc(2017,6,19,1,12,7,0);  //Set time: 2017/6/19, Monday, 12:07:00
// #define button_1 button_0
#else
    Wire.begin();
#endif

    button_0.init(PIN_BUTTON_0);
    button_1.init(PIN_BUTTON_1);
    button_2.init(PIN_BUTTON_2);
    button_3.init(PIN_BUTTON_3);

    ledDisplay.Begin(DISPLAY_IS_COMMON_ANODE, PIN_DISPLAY_DIGIT_0, PIN_DISPLAY_DIGIT_1, PIN_DISPLAY_DIGIT_2, PIN_DISPLAY_DIGIT_3, PIN_DISPLAY_DIGIT_BUTTON_LIGHTS, PIN_DISPLAY_SEGMENT_A, PIN_DISPLAY_SEGMENT_B, PIN_DISPLAY_SEGMENT_C, PIN_DISPLAY_SEGMENT_D, PIN_DISPLAY_SEGMENT_E, PIN_DISPLAY_SEGMENT_F, PIN_DISPLAY_SEGMENT_G, PIN_DISPLAY_SEGMENT_DP);
    visualsManager.setMultiplexerBuffer(ledDisplay.getDigits());

    buzzer.setPin(PIN_BUZZER);
    buzzer.setSpeedRatio(2);

    // check eeprom sanity. If new device, set values in eeprom
    if (EEPROM.read(EEPROM_ADDRESS_EEPROM_VALID) != EEPROM_VALID_VALUE)
    {
        EEPROM.write(EEPROM_ADDRESS_SNOOZE_TIME_MINUTES, FACTORY_DEFAULT_SNOOZE_TIME_MINUTES);
        EEPROM.write(EEPROM_ADDRESS_HOURLY_BEEP_ENABLED, FACTORY_DEFAULT_HOURLY_BEEP_ENABLED);
        EEPROM.write(EEPROM_ADDRESS_ALARM_SET_MEMORY, FACTORY_DEFAULT_ALARM_SET_MEMORY);
        EEPROM.write(EEPROM_ADDRESS_KITCHEN_TIMER_INIT_INDEX, FACTORY_DEFAULT_KITCHEN_TIMER);
        EEPROM.write(EEPROM_ADDRESS_ALARM_HOUR, FACTORY_DEFAULT_ALARM_HOUR);
        EEPROM.write(EEPROM_ADDRESS_ALARM_MINUTE, FACTORY_DEFAULT_ALARM_MINUTE);
        EEPROM.write(EEPROM_ADDRESS_EEPROM_VALID, EEPROM_VALID_VALUE);
        EEPROM.write(EEPROM_ADDRESS_ALARM_IS_SNOOZING, FACTORY_DEFAULT_ALARM_IS_SNOOZING);
        EEPROM.write(EEPROM_ADDRESS_ALARM_ENABLE_SNOOZE_TIME_DECREASE, FACTORY_DEFAULT_ALARM_ENABLE_SNOOZE_TIME_DECREASE);
        EEPROM.write(EEPROM_ADDRESS_ALARM_TUNE, FACTORY_DEFAULT_ALARM_TUNE);
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
    hourly_beep_enabled = EEPROM.read(EEPROM_ADDRESS_HOURLY_BEEP_ENABLED);
    enable_snooze_time_decrease = EEPROM.read(EEPROM_ADDRESS_ALARM_ENABLE_SNOOZE_TIME_DECREASE);
    uint8_t is_snoozing = EEPROM.read(EEPROM_ADDRESS_ALARM_IS_SNOOZING);
    uint8_t alarm_enabled = EEPROM.read(EEPROM_ADDRESS_ALARM_SET_MEMORY);
    alarm_tune_index = EEPROM.read(EEPROM_ADDRESS_ALARM_TUNE);
    for (uint8_t i = 0; i < 4; i++)
    {
        tune_name_buf[i] = pgm_read_byte_near(menu_item_tune_names + 4 * alarm_tune_index + i);
    }
    if (alarm_enabled > 64)
    {
        alarm_status_state = state_alarm_status_is_enabled;
    }
    else
    {
        alarm_status_state = state_alarm_status_is_not_enabled;
    }

    if (is_snoozing > 64)
    {
        // if power cut during snoozing, trigger alarm right away at power on.
        alarm_status_state = state_alarm_status_triggered;
    }

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
#ifdef PROTOTYPE_GRAVITY_RTC
// sets the alarm to one minute from upload
//   rtc.read();
//   alarm_hour = rtc.hour;
//   alarm_minute = rtc.minute + 1;
#else
    alarm_hour = rtcDS3231.getHour(false, false);
    alarm_minute = rtcDS3231.getMinute();
#endif
#endif

    snooze_count = 0;
    total_snooze_time_minutes = 0;

#ifdef PROTOTYPE_GRAVITY_RTC
// uint8_t test[32];
// rtc.readMemory(test);
//   for (uint8_t i=0;i<32;i++){
//     Serial.println(test[i]);
//   }
#endif
}

void loop()
{

    main_application_loop();

#ifdef ENABLE_SERIAL
#ifdef ENABLE_MEASURE_CYCLE_TIME
    measure_cycle_time();
#endif
#endif
}
