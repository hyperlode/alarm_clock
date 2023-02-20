// Written by Lode Ameije 2013
#ifndef Utilitieslode_h
#define Utilitieslode_h
// #if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
// #else
// #include "WProgram.h"
// #endif

void timeSecondsToClockString(char *textBuf, unsigned int seconds);

unsigned int timeMillisToSeconds(long milliSeconds);

void timeMillisToClockString(char *textBuf, long milliSeconds);
void intToDigitsString(char *textBuf, unsigned int number, bool trailingZeros);
uint8_t doBoolArrayToByte(bool bools[]);
void doByteToBoolArray(uint8_t combinationByte, bool **boolArray);

void setBit(uint8_t *bitContainer, bool value, byte index);

bool getBit(uint8_t *bitContainer, byte index);
int freeMemory();

bool stepChange(int8_t *counter, int8_t increment, int8_t minValue, int8_t maxValue, bool wrapAround);
void nextStep(int8_t *counter, bool countUpElseDown, int8_t minValue, int8_t maxValue, bool wrapAround);
void nextStepRotate(int8_t *counter, bool countUpElseDown, int8_t minValue, int8_t maxValue);
bool checkBoundaries(int8_t *counter, int8_t minValue, int8_t maxValue, bool rotate);

#endif
