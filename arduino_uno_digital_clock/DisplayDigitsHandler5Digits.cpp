#include "DisplayDigitsHandler5Digits.h"

// changed from 4 to 5 digits 20140123 Lode

DisplayManagement::DisplayManagement()
{
    // brightness = (8888 - LED_DISP_BRIGHTNESS) / 1000;
    setBlankDisplay();

#ifdef ENABLE_SCROLL
    this->setScrollSpeed(SCROLL_SPEED_DELAY_MILLIS);
#endif
};

void DisplayManagement::blanksToBuf(char *textBuf)
{
    // textBuf[0]=SPACE_FAKE_ASCII;
    // textBuf[1]=SPACE_FAKE_ASCII;
    // textBuf[2]=SPACE_FAKE_ASCII;
    // textBuf[3]=SPACE_FAKE_ASCII;
    setStandardTextToTextBuf(textBuf, TEXT_SPACES);
}

void DisplayManagement::setStandardTextToTextBuf(char *textBuf, uint8_t text_start_address)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        textBuf[i] = pgm_read_byte_near(standard_text + text_start_address + i);
    }
}

void DisplayManagement::setTextBufToDisplay(char *inText) // updateDisplayChars
{
    for (uint8_t i = 0; i < 4; i++)
    {
        setCharToDisplay(inText[i], i);
    }
}

void DisplayManagement::setCharToDisplay(char character, uint8_t digit)
{
    this->text[digit] = character;
}

char *DisplayManagement::getDisplayTextBufHandle()
{
    return this->text;
}

void DisplayManagement::setNumberToDisplayAsDecimal(int16_t number)
{ // updateDisplayNumber
    setNumberToDisplay(number, false);
}

void DisplayManagement::setNumberToDisplay(int16_t number, boolean asHexadecimal) // updateDisplayNumber
{
    numberToBuf(this->text, number, asHexadecimal);
    this->setTextBufToDisplay(text);
}

void DisplayManagement::setBoolToDisplay(bool value)
{
    if (value)
    {
        setStandardTextToTextBuf(this->text, TEXT_YES);
    }
    else
    {
        setStandardTextToTextBuf(this->text, TEXT_NO);
    }
}

void DisplayManagement::setBinaryToDisplay(uint32_t value) // updateDisplayAllBits
{
    // value has 32 bits, that's 4x 8 bits. so for four digits by 8 segements.  digit1: DP G F ..... A  , DIGIT 2 DP G ...., ...
    // sevseg.setBinaryToDisplay(value);
    this->displayBinary = value;
}

// void DisplayManagement::progmemToDisplayBuffer(uint32_t* displayBuffer, const uint8_t* progmemAddress)
// {
// 	// will take four bytes starting from the progmemAddress (which contains an address)
// 	// progmemPosition =
// 	//*displayBuffer = 0;
// 	for (uint8_t i = 0; i < 4; i++)
// 	{
// 		*displayBuffer |= (uint32_t)pgm_read_byte_near(progmemAddress + (i)) << (8 * i); //* 4 --> 4 bytes per dword
// 	}
// }

void DisplayManagement::setDecimalPointsToDisplay(byte decimalPoints)
{
    // 0x00001111 four decimal points.
    this->decimalPoints = decimalPoints;
}

void DisplayManagement::setDecimalPointToDisplay(boolean isOn, uint8_t digit) // updateDisplayDecimalPoint
{
    // sevseg.SetDecPointSingle(isOn, digit);
    if (isOn)
    {
        this->decimalPoints |= 1 << digit;
    }
    else
    {
        this->decimalPoints &= ~(1 << digit);
    }

#ifdef ARDUINO_SIMULATION
    mtc->setPoint(isOn, digit);
#endif
}

byte *DisplayManagement::getDecimalPointsHandle()
{
    return &decimalPoints;
}

void DisplayManagement::setLedArray(byte ledsAsBits) //  updateLights
{
    // sevseg.setLedArray(ledsAsBits);
    this->lights = ledsAsBits;
};

byte *DisplayManagement::getLedArrayHandle()
{
    return &this->lights;
}

void DisplayManagement::convert_text4Bytes_to_32bits(char *text, uint32_t *binary)
{
    *binary = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        byte digitBinary;
        charToSevenSegment(text[i], &digitBinary);
        *binary |= (uint32_t)digitBinary << (8 * i);
    }
}

// void DisplayManagement::convert_4bytesArray_32bits(char* characters, uint32_t* displayAllSegments, boolean toArray){

// 	if (toArray){
// 		for(uint8_t i=0;i<4;i++){
// 			characters[i] = (uint8_t)((*displayAllSegments >> (i * 8)) & 0xFF) ;
// 		}
// 	}else{
// 		*displayAllSegments = 0;

// 		for(uint8_t i=0;i<4;i++){
// 			*displayAllSegments |= (uint32_t)(characters[i]) << (8*i);
// 		}
// 	}
// }

void DisplayManagement::setBlankDisplay()
{
    // this->setBinaryToDisplay(0x00000000); //reset display, includes the decimal points.
    this->displayBinary = 0;
    setStandardTextToTextBuf(this->text, TEXT_SPACES);
    this->decimalPoints = 0;
    this->lights = 0;
}

// void DisplayManagement::bufToScreenBits(char *textBuf, uint32_t *screenBits)
// {
// 	*screenBits = 0;
// 	for (uint8_t i = 0; i < 4; i++)
// 	{
// 		*screenBits |= (uint32_t)(textBuf[i]) << (8 * i);
// 	}
// }

void DisplayManagement::minutesToMinutesHoursString(char *textBuf, uint16_t minutes)
{

    textBuf[0] = 48 + minutes / 600;
    minutes %= 600;
    textBuf[1] = 48 + minutes / 60;
    minutes %= 60;
    textBuf[2] = 48 + minutes / 10;
    textBuf[3] = 48 + minutes % 10;
}

void DisplayManagement::numberToBufAsDecimal(char *textBuf, int16_t number)
{
    numberToBuf(textBuf, number, false);
}

void DisplayManagement::digitValueToChar(char *charBuf, int16_t value)
{
    // convert a value to its char representation. 48 = 0
    // for ONE char.
    if (value > 9)
    {
        // in ascii table, there is a gap of 7 positions between the numbers and letters.
        value += 7;
    }

    *charBuf = 48 + value; // ascii 48 = 0
}

void DisplayManagement::numberToBuf(char *textBuf, int16_t number, bool asHexadecimal)
{
    // negative numbers made absolute!
    // textbuf four positions.

    int16_t c;
    c = abs(number);

    blanksToBuf(textBuf);

    for (uint8_t i = 0; i < 4; i++)
    {
        byte digitValue;
        digitValue = c % (10 + asHexadecimal * 6);
        // if (digitValue > 9)
        // {
        // // in ascii table, there is a gap of 7 positions between the numbers and letters.
        // digitValue += 7;
        // }

        // textBuf[3 - i] = 48 + digitValue; //ascii 48 = 0
        digitValueToChar(&textBuf[3 - i], digitValue);

        c /= 10 + asHexadecimal * 6;

        if (c == 0)
        {
            break;
        }
    }
}

// void DisplayManagement::SetSingleDigit(uint8_t value, int digit){
//   //arbitrary segments lights per digit. according to bits set in value (bit 0=A , bit6 is seg G, bit 7 = decimal point).
//   sevseg.SetSingleDigit(value, digit);
// }

// void DisplayManagement::displaySetTextAndDecimalPoints(char *inText, uint8_t *decimalPoints)
// {
// 	setTextBufToDisplay(inText);
// 	// always think of 5 digits.
// 	for (uint8_t i = 0; i < 5; i++)
// 	{
// 		setDecimalPointToDisplay(*decimalPoints & (0x01 << i), i);
// 	}
// }

// Create Array
/*******************************************************************************************/
// From the numbers found, says which LEDs need to be turned on

void DisplayManagement::charsToSevenSegment(char *text, byte *digits)
{
    // expect 4 chars.
    for (byte index = 0; index < 4; index++)
    {
        charToSevenSegment(text[index], &digits[index]);
    }
}

void DisplayManagement::charToSevenSegment(char character, byte *digit)
{

    // if (character == ' ')
    // {
    // 	character = SPACE_FAKE_ASCII;
    // }

    if (character == '?')
    {
        // toggles one segment of the digit
        byte i = random(0, 7);
        setBit(digit, !getBit(digit, i), i); // random on or off...
    }
    else if (character < 91 && character > 47)
    {
        *digit = pgm_read_byte_near(selected_ascii_to_7seg_digit + character - 48);
    }
    else
    {
        // illegal state
    }
}

// void DisplayManagement::getActiveSegmentAddress(byte **carrier)
// {
// 	*carrier = this->activeSegment;
// }

void DisplayManagement::setMultiplexerBuffer(byte *multiplexerDigits)
{
    this->multiplexerData = multiplexerDigits;
}

void DisplayManagement::refresh()
{

    // set it all up.

    for (uint8_t i = 0; i < 5; i++)
    {

        // start with a clean sheet.
        multiplexerData[i] = 0;

        if (i < 4)
        {
            // led display digits
            charToSevenSegment(text[i], &multiplexerData[i]);
            multiplexerData[i] |= (uint8_t)((displayBinary >> (i * 8)) & 0xFF);
            multiplexerData[i] |= getBit(&this->decimalPoints, i) << 7;
        }
        else
        {
            // button lights
            multiplexerData[i] = lights;
        }
    }
}

#ifdef ENABLE_SCROLL

void DisplayManagement::writeStringToDisplay(char *shortText)
{
    sevseg.NewText(shortText);
}
void DisplayManagement::get5DigitsFromString(char *in, char *out, int startPos)
{ //, int spacesBetweenRepeat
    // startPos can be a negative number (for scrolling in at the start), all negative numbers are "spaces". i.e. -2  => [' ', ' ', pos0char, pos1char]
    bool endOfString = false;
    // int spacesAtEnd = 0;

    // check for end of string befor startpos in  incoming text. If so, then all spaces...
    for (int i = 0; i < startPos - 1; i++)
    {
        if (i > 0 && in[i] == '\0')
        {
            endOfString = true;
        }
    }

    for (int i = 0; i < 5; i++)
    {
        if (startPos + i < 0)
        {
            out[i] = ' ';
        }
        else if (in[startPos + i] != '\0')
        {
            out[i] = in[startPos + i];
        }
        else
        {
            endOfString = true;
        }
        if (endOfString)
        {
            out[i] = ' ';
        }
    }
}

void DisplayManagement::displayHandlerSequence(char *movie)
{
    // take each time the 4 next chars and displays them at once (time in between = scrollNextMove timer)
    this->scrollTextAddress = movie;
    scrollNextMove.start();
    this->textStartPos = 0;
    get5DigitsFromString(this->scrollTextAddress, text, this->textStartPos);
    writeStringToDisplay(text);
    this->isScrolling = true;
}

void DisplayManagement::doSequence()
{
    if (this->isScrolling && scrollNextMove.getTimeIsNegative())
    {

        this->textStartPos += 5;

        // check for end of string in "next sequence"
        bool endOfString = false;
        for (int i = this->textStartPos; i < this->textStartPos + 5; i++)
        {
            if (this->scrollTextAddress[i] == '\0')
            {
                endOfString = true;
            }
        }

        if (endOfString)
        {
            this->textStartPos = 0;
        }

        // update display
        get5DigitsFromString(this->scrollTextAddress, text, this->textStartPos);
        writeStringToDisplay(text);
        // scrollNextMove.reset(); //start includes reset.
        scrollNextMove.start();
    }
}

void DisplayManagement::dispHandlerWithScroll(char *intext, bool comeScrollIn, bool scrollOnceElseInfinit)
{

    this->scrollTextAddress = intext;
    this->scrollOnceElseInfinit = scrollOnceElseInfinit;
    this->comeScrollIn = comeScrollIn;

    if (this->comeScrollIn)
    {
        this->textStartPos = -5;
    }
    else
    {
        this->textStartPos = 0;
    }

    // scrollNextMove.reset();
    scrollNextMove.start();

    get5DigitsFromString(intext, text, this->textStartPos);
    writeStringToDisplay(text);
    this->isScrolling = true;
}

void DisplayManagement::doScroll()
{ // char * intext
    // activate next move
    if (this->isScrolling && !scrollNextMove.getTimeIsNegative())
    {
        this->textStartPos++;
        // check for end of scrollstring
        if (this->textStartPos > 0 && this->scrollTextAddress[textStartPos - 1] == '\0')
        {
            if (this->scrollOnceElseInfinit)
            {
                this->isScrolling = false;
            }

            if (this->comeScrollIn)
            {
                this->textStartPos = -5;
            }
            else
            {
                this->textStartPos = 0;
            }
        }

        // dispUpdate
        get5DigitsFromString(this->scrollTextAddress, text, this->textStartPos);
        writeStringToDisplay(text);

        // scrollNextMove.reset();
        scrollNextMove.start();
    }
}

// check scroll status
bool DisplayManagement::getIsScrolling()
{
    return this->isScrolling;
}

// set scrollstatus
void DisplayManagement::setIsScrolling(bool enableScroll)
{
    this->isScrolling = enableScroll;
}

void DisplayManagement::setScrollSpeed(long value)
{
    scrollNextMove.setInitTimeMillis(value * -1);
    // scrollNextMove.start();
}

#endif
