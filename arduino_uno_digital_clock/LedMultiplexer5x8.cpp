/*
Written by Dean Reading, 2012
 Version 2.1; Includes a bug fix as pointed out by a community member, Ryan.
 Version 2.0; Now works for any digital pin arrangement, common anode and common cathode displays.
 
 Direct any questions or suggestions to deanreading@hotmail.com
 
 This library allows an arduino to easily display numbers in decimal format on
 a 4 digit 7-segment display without a separate 7-segment display controller.
 
 Hardware:
 4 digit 7 segment displays use 12 digital pins.
 
 There are:
 4 common pins; 1 for each digit.  These will be cathodes (negative pins) for
 common cathode displays, or anodes (positive pins) for common anode displays.
 I refer to these as digit pins.
 8 pins for the individual segments (seven segments plus the decimal point).
 I refer to these as segment pins.
 
 Connect the four digit pins with four limiting resistors in series to any digital or analog pins.
 Connect the eight cathodes to any digital or analog pins.
 
 I have a cheap one from China, and the pins of the display are in the following order:
 Top Row
 1,a,f,2,3,b
 Bottom Row
 e,d,dp,c,g,4
 Where the digit pins are 1-4 and the segment pins are a-g + dp
 
 Software:
 Call LedMultiplexer5x8.Begin in setup.  
 The first argument (boolean) tells whether the display is common cathode (0) or common
 anode (1).
 The next four arguments (bytes) tell the library which arduino pins are connected to
 the digit pins of the seven segment display.  Put them in order from left to right.
 The next eight arguments (bytes) tell the library which arduino pins are connected to
 the segment pins of the seven segment display.  Put them in order a to g then the dp.
 
 In summary, Begin(type, digit pins 1-4, segment pins a-g,dp)
 
 The calling program must run the PrintOutput() function repeatedly to get
 the number displayed.
 To set the number displayed, use the NewNum function.  Any number between
 -999 and 9999 can be displayed. Out of range numbers show up as '----'.
 To move the decimal place one digit to the left, use '1' as the second
 argument in NewNum. For example, if you wanted to display '3.141' you would
 call NewNum(3141,3);
 
 */

#include "LedMultiplexer5x8.h"


LedMultiplexer5x8::LedMultiplexer5x8()
{
    //Initial values
    // this->digitActive = 0;
    // this->extraLedArray = false;
}

bool LedMultiplexer5x8::getMode()
{
    return getBit(&this->boolContainer, MODEISCOMMONANODE);
}
//Begin
/*******************************************************************************************/
//Set pin modes and turns all displays off

// void LedMultiplexer5x8::Begin(boolean mode_isCommonAnode, byte D0, byte D1, byte D2, byte D3, byte D4, byte LedArrayDigit, byte S1, byte S2, byte S3, byte S4, byte S5, byte S6, byte S7, byte S8)
// {
// 	//for initialization with extra led array
// 	DigitPins[5] = LedArrayDigit;
// 	Begin(mode_isCommonAnode, D0, D1, D2, D3, D4, S1, S2, S3, S4, S5, S6, S7, S8);
// 	this->extraLedArray = true;
// }
// void LedMultiplexer5x8::Begin(boolean mode_isCommonAnode, byte D0, byte D1, byte D2, byte D3, byte D4, byte S1, byte S2, byte S3, byte S4, byte S5, byte S6, byte S7, byte S8)
void LedMultiplexer5x8::Begin(boolean mode_isCommonAnode, byte D0, byte D1, byte D2, byte D3, byte D4, byte S0, byte S1, byte S2, byte S3, byte S4, byte S5, byte S6, byte S7)
{

    //Assign input values to variables
    //Mode sets what the digit pins must be set at for it to be turned on.	0 for common cathode, 1 for common anode
    //this->mode_isCommonAnode=mode_isCommonAnode;
    setBit(&this->boolContainer, mode_isCommonAnode, MODEISCOMMONANODE);

    // if (mode_in==1){
    // DigitOn=true;
    // DigitOff=false;
    // SegOn=false;
    // SegOff=true;
    // }else{
    // DigitOn=false;
    // DigitOff=true;
    // SegOn=true;
    // SegOff=false;
    // }

    DigitPins[0] = D0;
    DigitPins[1] = D1;
    DigitPins[2] = D2;
    DigitPins[3] = D3;
    DigitPins[4] = D4;
    SegmentPins[0] = S0;
    SegmentPins[1] = S1;
    SegmentPins[2] = S2;
    SegmentPins[3] = S3;
    SegmentPins[4] = S4;
    SegmentPins[5] = S5;
    SegmentPins[6] = S6;
    SegmentPins[7] = S7;

    //Set Pin Modes as outputs
    for (byte digit = 0; digit < DIGITS_COUNT; digit++)
    {
        pinMode(DigitPins[digit], OUTPUT);
    }

    for (byte seg = 0; seg < 8; seg++)
    {
        pinMode(SegmentPins[seg], OUTPUT);
    }
    // https://www.electronicwings.com/users/sanketmallawat91/projects/215/frequency-changing-of-pwm-pins-of-arduino-uno#:~:text=The%20Arduino%20IDE%20has%20a,signal%20of%200%25%20duty%20cycle.
    // https://forum.arduino.cc/t/millis-and-timer-0/969036
   // TCCR0B = TCCR0B & B11111000 | B00000001; // PWM 31372.55 Hz pins 5 and 6   // fucks up millis and buzzer. 
    // TCCR1B = TCCR1B & B11111000 | B00000001; // PWM 62745.10 Hz pins 9 and 10  // no problem
    // TCCR2B = TCCR2B & B11111000 | B00000001; // PWM 31372.55 Hz pins 3 and 11  // causes weird behaviour and problems "suddenly"
    
    // no init needed, works with quick cycles anyways.
    // //Turn Everything Off
    // //Set all digit pins off.	Low for common anode, high for common cathode
    // for (byte digit=0;digit<6;digit++) {
    // 	digitalWrite(DigitPins[digit], DIGITOFF);
    // }

    // //Set all segment pins off.	High for common anode, low for common cathode
    // for (byte seg=0;seg<8;seg++) {
    // 	digitalWrite(SegmentPins[seg], SEGMENTOFF);
    // }
}



void LedMultiplexer5x8::outputPinsToBuffer(){
    ddr_bcd_buffer[0] = DDRB;
    ddr_bcd_buffer[1] = DDRC;
    ddr_bcd_buffer[2] = DDRD;
}

void LedMultiplexer5x8::bufferToOutputPins(){
    DDRB = ddr_bcd_buffer[0];
    DDRC = ddr_bcd_buffer[1];
    DDRD = ddr_bcd_buffer[2];
}

void LedMultiplexer5x8::setPinToOutputBuffer(int pin, bool outputElseInput){

  if (outputElseInput){
    // set bit to one
     if(pin<=7){
      ddr_bcd_buffer[2] |= (1<<pin);
    }else if (pin <=13){
      ddr_bcd_buffer[0] |= (1<<pin-8);
    }else if (pin == A0){
      ddr_bcd_buffer[1] |= (0b00000001);
    }else if (pin == A1){
      ddr_bcd_buffer[1] |= (0b00000010);
    }else if (pin == A2){
      ddr_bcd_buffer[1] |= (0b00000100);
    }else if (pin == A3){
      ddr_bcd_buffer[1] |= (0b00001000);
    }else if (pin == A4){
      ddr_bcd_buffer[1] |= (0b00010000);
    }else if (pin == A5){
      ddr_bcd_buffer[1] |= (0b00100000);
    }else{
      //ASSERT ERROR
      //#WARNING non resolved pin! 
    }
  }else{
    // set bit to zero
    if(pin<=7){
      ddr_bcd_buffer[2] &= ~(1<<pin);
    }else if (pin <=13){
      ddr_bcd_buffer[0] &= ~(1<<pin-8);
    }else if (pin == A0){
      ddr_bcd_buffer[1] &= ~(0b00000001);
    }else if (pin == A1){
      ddr_bcd_buffer[1] &= ~(0b00000010);
    }else if (pin == A2){
      ddr_bcd_buffer[1] &= ~(0b00000100);
    }else if (pin == A3){
      ddr_bcd_buffer[1] &= ~(0b00001000);
    }else if (pin == A4){
      ddr_bcd_buffer[1] &= ~(0b00010000);
    }else if (pin == A5){
      ddr_bcd_buffer[1] &= ~(0b00100000);
    }else{
      //ASSERT ERROR
      //#WARNING non resolved pin! 
    }
  }

}

void LedMultiplexer5x8::setPinFunction(int pin, bool outputElseInput){
  // pin numbers follow roughly the ports.
  // portD 0->7 (bit0=0,...)
  // portB 8->13 (bit0=8,...)
  // portC A0->A5 (bit0=A0,...)

  if (outputElseInput){
    // set bit to one
     if(pin<=7){
      DDRD |= (1<<pin);
    }else if (pin <=13){
      DDRB |= (1<<pin-8);
    }else if (pin == A0){
      DDRC |= (0b00000001);
    }else if (pin == A1){
      DDRC |= (0b00000010);
    }else if (pin == A2){
      DDRC |= (0b00000100);
    }else if (pin == A3){
      DDRC |= (0b00001000);
    }else if (pin == A4){
      DDRC |= (0b00010000);
    }else if (pin == A5){
      DDRC |= (0b00100000);
    }else{
      //ASSERT ERROR
      //#WARNING non resolved pin! 
    }
  }else{
    // set bit to zero
    if(pin<=7){
      DDRD &= ~(1<<pin);
    }else if (pin <=13){
      DDRB &= ~(1<<pin-8);
    }else if (pin == A0){
      DDRC &= ~(0b00000001);
    }else if (pin == A1){
      DDRC &= ~(0b00000010);
    }else if (pin == A2){
      DDRC &= ~(0b00000100);
    }else if (pin == A3){
      DDRC &= ~(0b00001000);
    }else if (pin == A4){
      DDRC &= ~(0b00010000);
    }else if (pin == A5){
      DDRC &= ~(0b00100000);
    }else{
      //ASSERT ERROR
      //#WARNING non resolved pin! 
    }
  }
  
  
}

//Refresh Display
/*******************************************************************************************/
//Cycles through each segment and turns on the correct digits for each.
//Leaves everything off
void LedMultiplexer5x8::refresh()
{
//       turn everything off at the start to reduce flickering. 
    //turn digits off. 
//    for (byte digit = 0; digit < DIGITS_COUNT; digit++)
//    {
//        digitalWrite(DigitPins[digit], DIGITOFF);
//        pinMode(DigitPins[digit], INPUT);
//    }
////    
////    //turn segments off
//    for (byte segment = 0; segment < 8; segment++)
//    {
//        digitalWrite(SegmentPins[segment], SEGMENTOFF);
//        pinMode(SegmentPins[segment], INPUT);
//    }

//    PORTD &= 0b00000011; // output value
//    PORTB &= 0b11000000;// output value

     // DDRD &= 0b00000011; // set relevant pins to input (kind of reset it all). 
     // DDRB &= 0b11000000; // set relevant pins to input (kind of reset it all). 

    
//       turn everything off at the start to reduce flickering. 

    outputPinsToBuffer();
    // digitValues[0] = 0x3f;
    // digitValues[1] = 0x3f;
    // digitValues[2] = 0x3f;
    // digitValues[3] = 0x3f;
    // delay(1);
    // digitValues[0] = 0x02;
    // digitValues[1] = 0x00;
    // digitValues[2] = 0x00;
    // digitValues[3] = 0x00;
    // delay(1);

     //delayMicroseconds( 10000);
    // delayMicroseconds( 1000);
    //turn digits off. 
    for (byte digit = 0; digit < DIGITS_COUNT; digit++)
    {
        //setPinToOutputBuffer(DigitPins[digit], false);
        // pinMode(DigitPins[digit], OUTPUT);
        digitalWrite(DigitPins[digit], DIGITOFF);
        //analogWrite(DigitPins[digit],  0);
    }
    // //turn segments on
    // for (byte segment = 0; segment < 8; segment++)
    // {
    //     setPinToOutputBuffer(SegmentPins[segment], true);
    // }
    //turn segments off
    for (byte segment = 0; segment < 8; segment++)
    {
        //setPinToOutputBuffer(SegmentPins[segment], false);
        digitalWrite(SegmentPins[segment], SEGMENTOFF);
    }

   
    // delayMicroseconds( (uint16_t)(255 - this->brightness) * 10);
    digitActive++;
 
    // if (digitActive > (3 + 1))
    if (digitActive > (3 + this->brightness))
    {
       //this is the brightness control, going into virtual segments, and doing nothing...
    //    if (digitActive > 7 + 200)
    // //    if (digitActive > 7 + this->brightness)
    //    {
    //        digitActive = 0;
    //    }
        digitActive = 0;
    }

    if (digitActive <= 3)
    {

            
            //Turn the relevant segment on
            //pinMode(SegmentPins[digitActive], OUTPUT);
            // setPinToOutputBuffer(SegmentPins[digitActive],true);
            digitalWrite(DigitPins[digitActive], DIGITON);

            //For each digit, turn relevant segments on
            for (byte segment = 0; segment < 8; segment++)
            //for (byte digit = DIGITS_COUNT-1; digit >= 0; digit--)
            {
                if (getBit(&digitValues[digitActive], segment))
                { 
    //                pinMode(DigitPins[digit], OUTPUT);
                    // setPinToOutputBuffer(DigitPins[digit],true);
                    // analogWrite(DigitPins[digit],  1);
                    //analogWrite(DigitPins[digit],  this->brightness);
                    // analogWrite(DigitPins[digit],  100);
                      digitalWrite(SegmentPins[segment], SEGMENTON);
                }
                //else{
                //setPinFunction(DigitPins[digit],false); // doing things one by one causes flickering... (sometimes segment is one while it shouldn't...)
                //digitalWrite(SegmentPins[segment], SEGMENTOFF);
                    //analogWrite(DigitPins[digit],  0);
                //}
            }
        // }
         
    }
   // delay(1);
   bufferToOutputPins();
}

void LedMultiplexer5x8::setBrightness(byte value, bool exponential)
{ 
  //brightness: 0 = max, the higher, the more delay.
  //smaller number is brighter
    uint16_t brightness_tmp;
    if (exponential)
    {
        brightness_tmp = value * value;
    }
    else
    {
        brightness_tmp = value;
    }
    if (brightness_tmp  > 255){
      brightness_tmp  = 255;
    }
    this->brightness = (uint8_t) brightness_tmp;
}

byte *LedMultiplexer5x8::getDigits()
{
    // get the handle to send the values of all the lights too.
    return digitValues;
}
