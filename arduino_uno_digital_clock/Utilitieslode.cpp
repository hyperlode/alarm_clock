#include "Utilitieslode.h"
#include "Arduino.h"

void timeSecondsToClockString(char *textBuf, unsigned int seconds)
{
    // clockToString(textBuf, seconds/60, seconds % 60);
    if (seconds <= 3599)
    { // 3599  for one hour 5999 for 90 minutes
        intToDigitsString(textBuf, (seconds / 60) * 100 + seconds % 60, true);
    }
    else if (seconds <= 35999)
    { // 9h59 (and 59 seconds)

        intToDigitsString(textBuf, (seconds / 3600) * 1000 + seconds % 3600 / 60, true);
        textBuf[1] = 'H';
    }
    else
    {                                                                                        //>35999   ==> max uuHm  bvb. 99u9
        intToDigitsString(textBuf, (seconds / 3600) * 100 + seconds % 3600 / 60 / 10, true); // 39600-> 1100   40200 -> 1101 -> 11H1 (1 is ten minutes)
        textBuf[2] = 'H';
    }
}

unsigned int timeMillisToSeconds(long milliSeconds)
{ //, bool countDownTimer
    // //argument countDownTimer= true:,  59500 millis = 1:00   else:(counting up) 59500 millis = 0:59
    // unsigned int counterSeconds;
    // unsigned int counterMinutes ;

    // //added 20140421 lode : negative time will be converted to positive here. (no negative times exist, it just starts the other way around (so: countdown becomes count up...)
    // if (milliSeconds < 0) {
    // milliSeconds = - milliSeconds;
    // countDownTimer = !countDownTimer;
    // }
    // //end addition

    // if (countDownTimer){
    // //playersTimeLeftMillis when i.e. 59500 -> should still show 1:00 minute! because "we are in the 60st second" that moment
    // //		ceil(222.4);

    // counterMinutes = (unsigned int)(ceil(float(milliSeconds)/1000)) / 60;
    // counterSeconds = (unsigned int)(ceil(float(milliSeconds)/1000)) % 60;
    // }else{
    // counterMinutes = (unsigned int)((float(milliSeconds)/1000)) / 60;
    // counterSeconds = (unsigned int)((float(milliSeconds)/1000)) % 60;
    // }

    // return counterMinutes*60 + counterSeconds;

    if (milliSeconds < 0)
    {
        milliSeconds -= 999;
    }
    milliSeconds = (milliSeconds) / 1000;
    if (milliSeconds < 0)
    {
        milliSeconds *= -1;
    }

    return (unsigned int)milliSeconds;
}

void timeMillisToClockString(char *textBuf, long milliSeconds)
{ //, bool countDownTimer

    timeSecondsToClockString(textBuf, timeMillisToSeconds(milliSeconds)); //, countDownTimer
}

void intToDigitsString(char *textBuf, unsigned int number, bool trailingZeros)
{
    // for four digits

    // number of digits
    unsigned int counter = number;
    int numberOfDigits = 0;

    if (counter == 0)
    {
        numberOfDigits = 1;
    }

    while (counter > 0)
    {
        numberOfDigits += 1;
        counter = counter / 10;
    }

    // ASCII space = 32,  '0' =48
    int asciiOffsetForZero = 0;
    if (trailingZeros)
    {
        asciiOffsetForZero = 16;
    }
    // set trailing values to char array
    for (int i = 0; i < 4; i++)
    {
        textBuf[i] = 32 + asciiOffsetForZero;
    }

    // textBuf[0] = 'E'; --> char
    // textBuf[1] = 52;  --> ascii value
    counter = number;
    for (int i = 0; i < numberOfDigits; i++)
    {
        textBuf[4 - 1 - i] = 48 + counter % 10; // 48 = ascii value for char '0', 49 = ascii value for '1',...
        counter = counter / 10;
    };
}

uint8_t doBoolArrayToByte(bool bools[])
{
    // combines boolean values in 8 bits in a byte.   (sequence in byte: boolean 7 bit 7 -------- boolean0 bit 0)
    uint8_t combiner = 0;
    for (int i = 0; i < 8; i++)
    {
        combiner |= (bools[i] == true) << i; // comparision returns one or zero, and we shift that bit i places to the left.
    }
    return combiner;
}

void doByteToBoolArray(uint8_t combinationByte, bool **boolArray)
{
    // write to pointer array of bools

    for (byte i = 0; i < 8; i++)
    {
        *(boolArray[i]) = (1 << i) & combinationByte; // shift true i places to the left, make an AND comparision with combinationByte, will result in all zeros, except when combination byte position i is a one. and since everything that differs from zero is seen as true, we then have a true (else false)
    }
}

void setBit(uint8_t *bitContainer, bool value, byte index)
{
    // http://stackoverflow.com/questions/5612174/more-ram-efficient-boolean-array-arduino-environment
    //  http://stackoverflow.com/questions/47981/how-do-you-set-clear-and-toggle-a-single-bit-in-c-c
    //*bitContainer = *bitContainer | (1 << index);
    *bitContainer ^= (-value ^ *bitContainer) & (1 << index);

    // if (value){
    // *bitContainer |= 1 << index;

    // }else{
    // *bitContainer &= ~(1 << index);
    // }

    //*bitContainer = 245;
}

bool getBit(uint8_t *bitContainer, byte index)
{
    // http://stackoverflow.com/questions/5612174/more-ram-efficient-boolean-array-arduino-
    //  http://stackoverflow.com/questions/47981/how-do-you-set-clear-and-toggle-a-single-bit-in-c-c
    //  return *bitContainer & (1 << index);
    return (*bitContainer >> index) & 1;
}

bool stepChange(int8_t *counter, int8_t increment, int8_t minValue, int8_t maxValue, bool wrapAround)
{
    *counter += increment;
    checkBoundaries(counter, minValue, maxValue, wrapAround);
    return increment != 0;
}

void nextStep(int8_t *counter, bool countUpElseDown, int8_t minValue, int8_t maxValue, bool wrapAround)
{
    int8_t increment = -1 + (2 * countUpElseDown);
    stepChange(counter, increment, minValue, maxValue, wrapAround);
}

void nextStepRotate(int8_t *counter, bool countUpElseDown, int8_t minValue, int8_t maxValue)
{
    nextStep(counter, countUpElseDown, minValue, maxValue, true);
}

bool checkBoundaries(int8_t *counter, int8_t minValue, int8_t maxValue, bool rotate)
{
    if (*counter > maxValue)
    {
        if (rotate)
        {
            *counter = minValue;
        }
        else
        {
            *counter = maxValue;
        }
        return false;
    }
    else if (*counter < minValue)
    {
        if (rotate)
        {
            *counter = maxValue;
        }
        else
        {
            *counter = minValue;
        }
        return false;
    }
    return true;
}

// #ifdef __arm__
// // should use uinstd.h to define sbrk but Due causes a conflict
// extern "C" char* sbrk(int incr);
// #else  // __ARM__
// extern char *__brkval;
// #endif  // __arm__

// int freeMemory() {
// 	char top;
// #ifdef __arm__
// 	return &top - reinterpret_cast<char*>(sbrk(0));
// #elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
// 	return &top - __brkval;
// #else  // __arm__
// 	return __brkval ? &top - __brkval : &top - __malloc_heap_start;
// #endif  // __arm__
// }
