//Written by Lode Ameije 2013
#ifndef DisplayDigitsHandler_h
#define DisplayDigitsHandler_h

// #if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
// #else
// #include "WProgram.h"
// #endif

//#include "SevSeg5Digits.h"
//#include "Countdowntimer.h"
//#include "SuperTimer.h"
#include "Utilitieslode.h"

// #define ENABLE_SCROLL
#ifdef ENABLE_SCROLL

#define SCROLL_SPEED_DELAY_MILLIS 250
#endif

#define LED_DISP_BRIGHTNESS 8888

#define ONLY_TOP_SEGMENT_FAKE_ASCII 58
#define ONLY_MIDDLE_SEGMENT_FAKE_ASCII 59
#define ONLY_BOTTOM_SEGMENT_FAKE_ASCII 60
#define ONLY_TOP_AND_BOTTOM_SEGMENT_FAKE_ASCII 61
#define SPACE_FAKE_ASCII 62

#define ONLY_TOP_SEGMENT B00000001
#define ONLY_MIDDLE_SEGMENT B01000000
#define ONLY_BOTTOM_SEGMENT B00001000
#define ONLY_TOP_AND_BOTTOM_SEGMENT B00001001
#define NO_SEGMENTS B00000000

const byte selected_ascii_to_7seg_digit[] PROGMEM = {
    B00111111,                   //'0'
    B00000110,                   //'1'
    B01011011,                   //'2'
    B01001111,                   //'3'
    B01100110,                   //'4'
    B01101101,                   //'5'
    B01111101,                   //'6'
    B00000111,                   //'7'
    B01111111,                   //'8'
    B01101111,                   //'9'
    ONLY_TOP_SEGMENT,            //non ascii
    ONLY_MIDDLE_SEGMENT,         //non ascii
    ONLY_BOTTOM_SEGMENT,         //non ascii
    ONLY_TOP_AND_BOTTOM_SEGMENT, //non ascii
    NO_SEGMENTS,                 //non ascii
    B00000000,                   //non ascii
    B00000000,                   //non ascii
    B01110111,                   //'A'
    B01111100,                   //'B'
    B00111001,                   //'C'
    B01011110,                   //'D'
    B01111001,                   //'E'
    B01110001,                   //'F'
    B00111101,                   //'G'
    B01110100,                   //'H'
    B00000110,                   //'I'
    B00001110,                   //'J'
    B01110101,                   //'K'
    B00111000,                   //'L'
    B01010101,                   //'M'
    B01010100,                   //'N'
    B01011100,                   //'O'
    B01110011,                   //'P'
    B01100111,                   //'Q'
    B01010000,                   //'R'
    B01101101,                   //'S'
    B01111000,                   //'T'
    B00011100,                   //'U'
    B00011110,                   //'V'
    B00011101,                   //'W'
    B00110110,                   //'X'
    B01101110,                   //'Y'
    B00011011                    //'Z'

};

#define TEXT_SPACES 0
#define TEXT_PAUS 4
#define TEXT_RANDOM_BEEP 8
#define TEXT_BEEP 12
#define TEXT_EEPROM 16
#define TEXT_RESET 20
#define TEXT_DONE 24
#define TEXT_DOIT 28
#define TEXT_TILT 32
#define TEXT_RANDOM_SEGMENTS 36
#define TEXT_SAVE 40
#define TEXT_END 44
#define TEXT_LOAD 48
#define TEXT_AUTO 52
#define TEXT_SET 56
#define TEXT_HEAD 60
#define TEXT_TAIL 64
#define TEXT_YES 68
#define TEXT_NO 72
#define MAJOR 76
#define MINOR 80
#define PENTATONIC 84
#define BLUES 88
#define CHROMATIC 92
#define MANUAL 96
#define UP 100
#define DOWN 104
#define SAWTOOTH 108
#define RANDOM 112
#define CRAZY 116
#define TEXT_FISH 120
#define TEXT_QUANTITY 124
#define TEXT_BATTERY 128
#define TEXT_OFF 132

const byte standard_text[] PROGMEM = {
    SPACE_FAKE_ASCII, SPACE_FAKE_ASCII, SPACE_FAKE_ASCII, SPACE_FAKE_ASCII,
    'P', 'A', 'U', 'S',
    'R', 'N', 'D', 'B',
    'B', 'E', 'E', 'P',
    'E', 'E', 'P', 'R',
    'R', 'S', 'E', 'T',
    'D', 'O', 'N', 'E',
    'D', 'O', 'I', 'T',
    'T', 'I', 'L', 'T',
    '?', '?', '?', '?',
    'S', 'A', 'V', 'E',
    'E', 'N', 'D', SPACE_FAKE_ASCII,
    'L', 'O', 'A', 'D',
    'A', 'U', 'T', 'O',
    'S', 'E', 'T', SPACE_FAKE_ASCII,
    'H', 'E', 'A', 'D',                           //don't change position, indeces calculated, used in random
    'T', 'A', 'I', 'L',                           //don't change position, indeces calculated, used in random
    SPACE_FAKE_ASCII, 'N', 'O', SPACE_FAKE_ASCII, //don't change position, indeces calculated, used in random
    SPACE_FAKE_ASCII, 'Y', 'E', 'S',              //don't change position, indeces calculated, used in random
    'A', 'J', 'O', 'R',
    'I', 'N', 'O', 'R',
    'P', 'E', 'N', 'T',
    'B', 'L', 'U', 'E',
    'C', 'H', 'R', 'O',
    'M', 'A', 'N', 'U',
    SPACE_FAKE_ASCII, SPACE_FAKE_ASCII, 'U', 'P',
    SPACE_FAKE_ASCII, SPACE_FAKE_ASCII, 'D', 'O',
    'U', 'P', 'D', 'O',
    SPACE_FAKE_ASCII, 'R', 'N', 'D',
    'C', 'R', 'A', 'Y',
    'F', 'I', 'S', 'H',
    'Q', 'T', 'Y', SPACE_FAKE_ASCII,
    'B', 'A', 'T', 'T',
    SPACE_FAKE_ASCII, 'O', 'F', 'F'};

class DisplayManagement
{
public:
    DisplayManagement();
    // ~DisplayManagement();

    // void startUp(bool dispHasCommonAnode, byte D0, byte D1, byte D2, byte D3, byte D4, byte LedArrayDigit, byte S1, byte S2, byte S3, byte S4, byte S5, byte S6, byte S7, byte S8);
    // void startUp(bool dispHasCommonAnode, byte D0, byte D1, byte D2, byte D3, byte D4, byte S1, byte S2, byte S3, byte S4, byte S5, byte S6, byte S7, byte S8);

    // void setNumberToDisplayAsDecimalAsChars(int16_t number); //deprecated
    // void bufToScreenBits(char *textBuf, uint32_t *screenBits);
    // void SetSingleDigit(uint8_t value, int digit); //deprecated

    void setStandardTextToTextBuf(char *textBuf, uint8_t text_start_address);

    void setTextBufToDisplay(char *inText); // updateDisplayChars
    void setCharToDisplay(char character, uint8_t digit);
    char *getDisplayTextBufHandle();

    void setNumberToDisplayAsDecimal(int16_t number);               //updateDisplayNumber
    void setNumberToDisplay(int16_t number, boolean asHexadecimal); //updateDisplayNumber

    void setBinaryToDisplay(uint32_t value); //  updateDisplayAllBits

    void setDecimalPointsToDisplay(byte decimalPoints);
    void setDecimalPointToDisplay(boolean isOn, uint8_t digit); // updateDisplayDecimalPoint
    byte *getDecimalPointsHandle();

    void setLedArray(byte ledsAsBits); //  updateLights
    byte *getLedArrayHandle();
    void minutesToMinutesHoursString(char *textBuf, uint16_t minutes);
    // void progmemToDisplayBuffer(uint32_t* displayBuffer, const uint8_t* progmemAddress);
    void setBlankDisplay(); //eraseAll

    void refresh();

    void digitValueToChar(char *charBuf, int16_t value);
    void numberToBufAsDecimal(char *textBuf, int16_t number);
    void numberToBuf(char *textBuf, int16_t number, bool asHexadecimal);
    void blanksToBuf(char *textBuf);

    void convert_text4Bytes_to_32bits(char *text, uint32_t *binary);

    //void convert_4bytesArray_32bits(char* characters, uint32_t* displayAllSegments, boolean toArray);

    void charsToSevenSegment(char *text, byte *digits);
    void charToSevenSegment(char character, byte *digit);

    void setMultiplexerBuffer(byte *multiplexerDigits);

    //void displaySetTextAndDecimalPoints(char *inText, uint8_t *decimalPoints);  //deprecated

    // void setBrightness(byte value, bool exponential); // should not be done here!

    // void getActiveSegmentAddress(byte **carrier);

#ifdef ENABLE_SCROLL

    void writeStringToDisplay(char *shortText);
    void get5DigitsFromString(char *in, char *out, int startPos);
    void displayHandlerSequence(char *movie);
    void dispHandlerWithScroll(char *intext, bool comeScrollIn, bool scrollOnceElseInfinit);
    void doSequence();
    void doScroll();

    //check scroll status
    bool getIsScrolling();

    //set scrollstatus
    void setIsScrolling(bool enableScroll);
    void setScrollSpeed(long value);
#endif

private:
    //SevSeg5Digits sevseg;
    // byte brightness;

    //update
    char text[4];
    byte lights;
    byte decimalPoints;
    uint32_t displayBinary;

    byte *multiplexerData; // pointer to array containing the actual lights data.

    //byte *activeSegment;

#ifdef ENABLE_SCROLL
    int textStartPos; //can be negative
    bool comeScrollIn;
    bool scrollOnceElseInfinit;
    bool isScrolling;
    SuperTimer scrollNextMove;
    char *scrollTextAddress;
#endif
};
#endif
