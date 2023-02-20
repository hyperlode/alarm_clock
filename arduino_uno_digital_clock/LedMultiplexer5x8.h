//Written by Dean Reading, 2012.  deanreading@hotmail.com
//See .cpp file for info

#ifndef LedMultiplexer5x8_h
#define LedMultiplexer5x8_h

//#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#include "Utilitieslode.h"
// #include "uservariables.h"
//#else
//#include "WProgram.h"
//#endif


#define DIGITS_COUNT 4

class LedMultiplexer5x8
{

public:
    LedMultiplexer5x8();

    //Public Functions
    //  void PrintOutput();
    //  void PrintOutput1And4();
    void refresh();
    //void NewText(char *text);
    // void Begin(boolean mode_in, byte C1, byte C2, byte C3, byte C4, byte C5, byte UC1, byte UC2, byte UC3, byte UC4, byte UC5, byte UC6, byte UC7, byte UC8);
    void Begin(boolean mode_isCommonAnode, byte D0, byte D1, byte D2, byte D3, byte D4, byte S0, byte S1, byte S2, byte S3, byte S4, byte S5, byte S6, byte S7);

    //void Begin(boolean mode_in, byte C1, byte C2, byte C3, byte C4, byte C5, byte LedArrayDigit, byte UC1, byte UC2, byte UC3, byte UC4, byte UC5, byte UC6, byte UC7, byte UC8);

    uint8_t *getDigits();
    //void SetSingleDigit(int8_t value, int digit);
    //void setBinaryToDisplay(uint32_t value);
    //void SetDecPointSingle(boolean decDig, int digit);
    //void setLedArray(byte ledsAsBits);

    void setBrightness(uint16_t value, bool exponential);
    bool getMode();

    //Public Variables
    byte digitActive;

private:
    //Private Functions
    //void CreateArray();

    //Private Variables
    // boolean mode,DigitOn,DigitOff,SegOn,SegOff;
    uint16_t brightness;

    uint32_t test;
    byte DigitPins[5];
    byte SegmentPins[8];
    byte digitValues[5];

    enum class StateMultiplexer { state_digit_on, state_all_off, state_setup_digit };
    StateMultiplexer stateMultiplexer = StateMultiplexer::state_all_off;
    // uint16_t ticker = 0;
    long timestampMicros;



    
    byte activeSegmentsCount;
    byte digitLightUpCycles;
    byte allOffCycles;
    uint8_t ddr_bcd_buffer[3];

    // boolean lights[5][8];
    //uint8_t lights[5];

    //uint8_t ledArrayValues;
    //bool extraLedArray;
    //byte chars[5];
    // char *text;
    //boolean decPoints[5];
    void setPinFunction(int pin, bool outputElseInput);

    void outputPinsToBuffer();
    void bufferToOutputPins();
    void setPinToOutputBuffer(int pin, bool outputElseInput);

    uint8_t boolContainer;

    // #define TEMPDECPOINTMEMORY 0
    #define MODEISCOMMONANODE 1 //should be defined in uservariables

    #define DIGITON getMode()
    #define DIGITOFF !getMode()
    #define SEGMENTON !getMode()
    #define SEGMENTOFF getMode()

    //  byte digitActive;
};

#endif
