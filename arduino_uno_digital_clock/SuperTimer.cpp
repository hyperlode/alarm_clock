#include "SuperTimer.h"

SuperTimer::SuperTimer()
{
    setIsStarted(false);
    setIsPaused(false);
    this->initTimeMillis = 0;

#ifdef ENABLE_CALLIBRATION
    *this->callibrationConstant = 1.0; //default is 1 // DISABLED
#endif
}

//INIT TIME

void SuperTimer::incrementInitTimeMillis(int16_t deltaMillis)
{
    this->initTimeMillis += (long)deltaMillis;

    // this mode is used in countdown timer.
    if (getInitTimeMillis() > -5)
    {
        this->initTimeMillis = -5;
    }
}

void SuperTimer::setInitTimeMillis(long milliSeconds)
{
    this->initTimeMillis = milliSeconds;
}

void SuperTimer::setOffsetInitTimeMillis(long offsetMillis)
{
    //make distinction between change during runtime and not, if during runtime: the original set time will not be affected.
    //if offset is negative, time goes down. if positive, time goes up.
    if (getIsStarted())
    {
        // if (getIsPaused())
        // {
        // 	this->startedMillis = this->startedMillis - offsetMillis;
        // }
        // else
        // {
        // 	this->startedMillis = this->startedMillis - offsetMillis; //-// we have after a pause a new starting point, but, from that new starting point, we subtract the already exceeded time.
        // }
        this->startedMillis = this->startedMillis - offsetMillis;
    }
    else
    {
        this->initTimeMillis += offsetMillis;
    }
}

void SuperTimer::setInitCountDownTimeSecs(uint16_t countDownSeconds)
{
    this->setInitCountDownTimeMillis(((long)countDownSeconds) * 1000); // not -1000 because we call another COUNTDOWN
}

void SuperTimer::setInitCountDownTimeMillis(long countDownMillis)
{
    setInitTimeMillis((countDownMillis) * -1);
}

long SuperTimer::getInitTimeMillis()
{
    return this->initTimeMillis;
}

int SuperTimer::getInitTimeSecs()
{
    return timeMillisToSeconds(getInitTimeMillis());
}

//START BEHAVIOUR

void SuperTimer::start(long initTimeMillis)
{
    //positive initTimeMillis is chrono with offset..., negative is countdowntimer
    this->initTimeMillis = initTimeMillis;
    start();
}

void SuperTimer::startCountDownWithSeconds(unsigned int countDownSeconds)
{
    start(-1000 * (long)countDownSeconds);
}

void SuperTimer::start()
{
    startPaused(false);
}

void SuperTimer::startPaused(bool paused)
{
#ifdef ENABLE_CALLIBRATION
    startComplete(paused, getMillisCallibrated());
#else
    startComplete(paused, millis());

#endif
}

void SuperTimer::startComplete(bool startInPauseMode, unsigned long startedMillisPassing)
{
    //boolean argument: if true, start in paused mode, is better than first start and then pause (no lag at all!)
    //unsigned long: provided time for correcting the start value (if timer has to connect seamlessly to another event, include the millis() of the event (i.e. the ending of another timer)

    this->startedMillis = (long)startedMillisPassing - this->initTimeMillis; //if initTimeMillis is positive: chronooffset, if negative, countdowntimer (time will be negative until zero reached)
    setIsStarted(true);

    //make sure if we start in pause mode, that the exact time of start is the start of the pause.
    if (startInPauseMode)
    {
        setIsPaused(true);
#ifdef ENABLE_CALLIBRATION
        this->pauseStartedMillis = (long)getMillisCallibrated();
#else
        this->pauseStartedMillis = millis();
#endif
    }
    else
    {
        setIsPaused(false);
    }
}

bool SuperTimer::getIsStarted()
{
    //return this->isStarted;
    return getBit(&this->boolContainer, BITLOCATION_ISSTARTED);
}

void SuperTimer::setIsStarted(bool isStarted)
{
    //this->isStarted = isStarted;
    setBit(&this->boolContainer, isStarted, BITLOCATION_ISSTARTED);
}

//PAUSE BEHAVIOUR

void SuperTimer::continu()
{
    paused(false);
}

void SuperTimer::pause()
{
    paused(true);
}

void SuperTimer::paused(bool pause)
{
    if (pause)
    {
        if (!getIsPaused())
        { //only do this when not yet paused!
#ifdef ENABLE_CALLIBRATION
            this->pauseStartedMillis = (long)getMillisCallibrated();
#else
            this->pauseStartedMillis = millis();
#endif
        }
    }
    else
    {
        if (getIsPaused())
        {
#ifdef ENABLE_CALLIBRATION
            this->startedMillis = (long)getMillisCallibrated() - (this->pauseStartedMillis - this->startedMillis); // we have after a pause a new starting point, but, from that new starting point, we subtract the already exceeded time.
#else
            this->startedMillis = millis() - (this->pauseStartedMillis - this->startedMillis); // we have after a pause a new starting point, but, from that new starting point, we subtract the already exceeded time.

#endif
        }
    }

    setIsPaused(pause);
}

bool SuperTimer::getIsPaused()
{
    return getBit(&this->boolContainer, BITLOCATION_ISPAUSED);
}

void SuperTimer::setIsPaused(bool paused)
{
    setBit(&this->boolContainer, paused, BITLOCATION_ISPAUSED);
}

//RESET

void SuperTimer::reset()
{
    // TODO not happy with this. It does not even set the time back to its init time. CHange it!
    // UPDATE: doesn't seem to bad after all.
    setIsPaused(false);
    setIsStarted(false);
}

//MODIFY

//INQUIRY

long SuperTimer::getTimeMillis()
{
    //get the timertimer  (pauses will change startedMillis, so this time is without the pauses.)
    //if negative, countdowntimer
    //if positive, chrono

    if (getIsStarted())
    {
        if (getIsPaused())
        {
            return this->pauseStartedMillis - this->startedMillis;
        }
        else
        {
#ifdef ENABLE_CALLIBRATION
            return (long)getMillisCallibrated() - this->startedMillis;
#else
            return millis() - this->startedMillis;
#endif
        }
    }
    else
    {
        return getInitTimeMillis();
    }
}

bool SuperTimer::getCountDownTimerElapsedAndRestart()
{
    if (!this->getTimeIsNegative())
    {
        this->start();
        return true;
    }
    else
    {
        return false;
    }
}
bool SuperTimer::getTimeIsNegative()
{
    // as long as time is negative, timer is counting down. Else, counting up.
    return getTimeMillis() <= 0;
}

long SuperTimer::getTimeSeconds()
{

    return (long)getTimeSecondsAbsolute() * (1 - 2 * getTimeIsNegative()); //-1 if negative
}

unsigned int SuperTimer::getTimeSecondsAbsolute()
{
    return timeMillisToSeconds(getTimeMillis());
}

unsigned int SuperTimer::getTimeSecondsCountDownTimer()
{
    //no negative time, if timer elapsed, then zero.
    //will always return a positive number!
    if (getTimeIsNegative())
    {
        return timeMillisToSeconds(getTimeMillis());
    }
    else
    {
        return 0;
    };
}

long SuperTimer::getTimeMillisCountDownTimer()
{
    //no negative time, if timer elapsed, then zero.
    //will always return a positive number!

    if (getTimeIsNegative())
    {
        return -getTimeMillis();
    }
    else
    {
        return 0;
    }
}

void SuperTimer::getTimeString(char *textBuf)
{
    //will always return a string with a positive time value ...
    timeMillisToClockString(textBuf, getTimeMillis());
}

bool SuperTimer::getSecondsBlinker()
{
    //ON when paused or at on time during blinking
    //OFF at off time during blinking

    if (getIsStarted())
    {
        return getInFirstGivenHundredsPartOfSecond(500);
    }
    else
    {
        return true;
    }
}

//ADMINISTRATION
#ifdef ENABLE_CALLIBRATION

//callibration. should be done at a higher level. every clock should have the same speed.... , so the millis() function should be modified,
void SuperTimer::setCallibrationMillis(float *callibrationRatio)
{
    // if higher than 1: clock goes faster, if lower than 1 clock goes slower
    this->callibrationConstant = callibrationRatio;
}

float SuperTimer::getCallibrationMillis()
{
    return *this->callibrationConstant;
}

unsigned long SuperTimer::getMillisCallibrated()
{

    return millis(); // bridge callibration  todo

    //   return (unsigned long)millis(); // bridge callibration  todo
    // 	return (unsigned long)(millis() *
    // #if defined DISABLE_CALIBRATION && defined ARDUINO_SIMULATION
    // 		1
    // #else
    // 		*this->callibrationConstant
    // #endif

    // 	);
}
#endif

//original countdowntimer

bool SuperTimer::getInFirstGivenHundredsPartOfSecond(int hundreds)
{
    //calc like this: if timeleft is 9600 millis ==> first 400 millis of seconds is passed...
    // bool inFirstPartOfSecond = getTimeLeftMillis()%1000 <= 1000 - hundreds

    if (getTimeMillis() >= 0)
    {
        //chrono
        return abs(getTimeMillis() % 1000) <= (hundreds);
    }
    else
    {
        //countdown
        return abs(getTimeMillis()) % 1000 >= (1000 - hundreds);
    }
}

bool SuperTimer::getEdgeSinceLastCallFirstGivenHundredsPartOfSecond(int hundreds, bool positiveEdge, bool negativeEdge)
{
    //check edge of first given hundreds since this function was last called
    // if onlyPositiveEdge = false : edge on start and end (positive and negative edge) of first given part of hundreds
    // if onlyPositiveEdge = true :  edge on start  (positive edge) of first given part of hundreds
    bool inFirstGivenHundredsPartOfSecond = getInFirstGivenHundredsPartOfSecond(hundreds);

    // // if ( inFirstGivenHundredsPartOfSecond != getBit(&this->boolContainer,BITLOCATION_INFIRSTPARTOFSECOND_EGDE) && (!positiveEdge || inFirstGivenHundredsPartOfSecond  )){
    // if ( inFirstGivenHundredsPartOfSecond != getBit(&this->boolContainer,BITLOCATION_INFIRSTPARTOFSECOND_EGDE) &&
    // 	 (
    // 		 (positiveEdge && negativeEdge) ||
    // 		 (positiveEdge && inFirstGivenHundredsPartOfSecond) ||
    // 		 (negativeEdge && !inFirstGivenHundredsPartOfSecond  ))
    // 	 ){
    if ((positiveEdge && inFirstGivenHundredsPartOfSecond && !getBit(&this->boolContainer, BITLOCATION_INFIRSTPARTOFSECOND_EGDE)) ||
        (negativeEdge && !inFirstGivenHundredsPartOfSecond && getBit(&this->boolContainer, BITLOCATION_INFIRSTPARTOFSECOND_EGDE)))
    {

        //previousCheckInFirstPartOfSecond = inFirstGivenHundredsPartOfSecond;
        setBit(&this->boolContainer, inFirstGivenHundredsPartOfSecond, BITLOCATION_INFIRSTPARTOFSECOND_EGDE);
        return true;
    }
    else
    {
        // previousCheckInFirstPartOfSecond = inFirstGivenHundredsPartOfSecond;
        setBit(&this->boolContainer, inFirstGivenHundredsPartOfSecond, BITLOCATION_INFIRSTPARTOFSECOND_EGDE);
        return false;
    }
}
