//Written by Lode Ameije 2013
#ifndef SuperTimer_h
#define SuperTimer_h

// #define ENABLE_CALLIBRATION

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "Utilitieslode.h"

class SuperTimer
{
public:
    SuperTimer();
    void incrementInitTimeMillis(int16_t deltaMillis);
    void setInitTimeMillis(long milliSeconds);
    void setInitTimeSecs(long seconds);
    long getInitTimeMillis();
    int getInitTimeSecs();
    void setInitCountDownTimeSecs(uint16_t countDownSeconds);
    void setInitCountDownTimeMillis(long countDownMillis);

    void start(long initTimeMillis);
    void startCountDownWithSeconds(unsigned int countDownSeconds);
    void start();
    void startPaused(bool paused);
    void startComplete(bool startInPauseMode, unsigned long startedMillis);
    bool getIsStarted();
    void setIsStarted(bool isStarted);

    void continu();
    void pause();
    void paused(bool pause);
    bool getIsPaused();

    void reset();

    void setOffsetInitTimeMillis(long offsetMillis);

    long getTimeMillis();
    bool getCountDownTimerElapsedAndRestart();
    bool getTimeIsNegative();
    long getTimeSeconds();
    unsigned int getTimeSecondsAbsolute();
    unsigned int getTimeSecondsCountDownTimer();
    long getTimeMillisCountDownTimer();
    void getTimeString(char *textBuf);
    bool getSecondsBlinker();

#ifdef ENABLE_CALLIBRATION
    void setCallibrationMillis(float *callibrationRatio);
    float getCallibrationMillis();
    unsigned long getMillisCallibrated();

#endif
    bool getInFirstGivenHundredsPartOfSecond(int hundreds);
    bool getEdgeSinceLastCallFirstGivenHundredsPartOfSecond(int hundreds, bool positiveEdge, bool negativeEdge);

private:                           //not accessible in subclasses (work with getters and setter to access then)
    void setIsPaused(bool paused); //only for internal use, to set the variable...

#ifdef ENABLE_CALLIBRATION
    float *callibrationConstant;
#endif

    long startedMillis;
    long pauseStartedMillis;
    long initTimeMillis;

    uint8_t boolContainer;

#define BITLOCATION_ISSTARTED 0
#define BITLOCATION_ISPAUSED 1
#define BITLOCATION_INFIRSTPARTOFSECOND_EGDE 2
};
#endif
