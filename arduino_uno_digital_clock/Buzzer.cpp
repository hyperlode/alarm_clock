#include "Buzzer.h"
#include "Arduino.h"

// buffer has two indeces: for adding new notes (programSlotcounter) and for playing bufferPlayIndex.

Buzzer::Buzzer()
{
    //constructor
    clearBuzzerNotesBuffer();
    this->soundFinishedTimeMillis = 0;
    this->speedScale = 1;
    this->transpose = 0;
}

void Buzzer::clearBuzzerNotesBuffer()
{
    bufferPlayIndex = 1;
    bufferProgramIndex = 0;
    this->soundFinishedTimeMillis = millis(); //ready to right away detect new sounds from buffer
}

void Buzzer::changeTranspose(int8_t delta)
{
    this->setTranspose(this->transpose + delta);
}

void Buzzer::setTranspose(int8_t offset)
{
    if (offset < -12)
    {
        this->transpose = -12;
    }
    else if (offset > 12)
    {
        this->transpose = 12;
    }
    else
    {
        this->transpose = offset;
    }
}

void Buzzer::setPin(uint8_t pin)
{
    this->pin = pin;
    pinMode(this->pin, OUTPUT);
}

uint8_t Buzzer::getPin()
{
    return this->pin;
}

void Buzzer::addNoteToNotesBuffer(uint8_t note)
{
    //load one note to next slot in notesBuffer

    //sound:
    //0 = empty slot
    //+ 1-62 = note (1 = 110Hz A , and going up 1/12 of an octave each number
    //+ 63, 126, 189, 252 = duration = 1/8, 1/4, 1/2, 1
    //program in next available slot AFTER bufferPlayIndex

    //A0sharp 1/2 = 191
    //1/8 stop = 63

    //this->bufferProgramIndex = getNextBuzzerNotesBufferSlot(true);
    this->bufferProgramIndex = getNextProgramIndex();

    this->buzzerNotesBuffer[this->bufferProgramIndex] = note;

    //http://members.efn.org/~qehn/global/building/cents.htm
    //F = {[(2)^1/12]^n} * 220 Hz //220Hz for A 440 , 880 .... for other octaves
}

uint8_t Buzzer::addRandomSoundToNotesBuffer(uint8_t lowest, uint8_t highest)
{
    uint8_t r = random(lowest, highest);
    addNoteToNotesBuffer(r);
    return r;
}

void Buzzer::checkAndPlayNotesBuffer()
{
    //play all the sounds in the notesBuffer (buffer)
    //0 stands for free and programmable
    //one bufferPlayIndex
    //delay(100);
    if (millis() > this->soundFinishedTimeMillis)
    {
        if (this->getNextProgramIndex() != this->bufferPlayIndex)
        {
            uint8_t current_note = this->buzzerNotesBuffer[this->bufferPlayIndex];

            //F = {[(2)^1/12]^n} * 220 Hz //220Hz for A 440 , 880 .... for other octaves
            float freq = 1;

            {
                // convert note to freq
                for (uint8_t i = 0; i < (current_note % NOTES_COUNT) + this->transpose; i++)
                {
                    //same result as: (but 2K less memory!!)
                    //freq = pow(1.059463,(buzzerNotesBuffer[this->bufferPlayIndex])-1 % 63);
                    freq *= 1.059463;
                }
            }

            uint8_t eight_notes_length_multiplier;

            if (current_note <= LAST_NOTE)
            {
                freq *= BUZZER_NOTES_BUFFER_BASE_FREQUENCY; //frequency
                eight_notes_length_multiplier = (1 << (current_note / NOTES_COUNT));
                tone(this->pin, (unsigned int)freq); //duration, number is exponent of 2.
                noteFinishedEdge = true;
            }
            else
            {
                eight_notes_length_multiplier = current_note - LAST_NOTE; // rests are situated behind the last notes in the .h file
                this->buzzerSilent();
            }

            unsigned long toneLength = (unsigned long)(this->speedScale * BUZZER_NOTES_BUFFER_EIGHTNOTE_DURATION_MILLIS * eight_notes_length_multiplier);

            this->soundFinishedTimeMillis = millis() + toneLength;

            //move active slot
            this->bufferPlayIndex = this->getNextPlayIndex();
        }
        else
        {
            if (noteFinishedEdge)
            {
                noteFinishedEdge = false;
                this->buzzerSilent();
            }
        }
    }
}

void Buzzer::changeSpeedRatio(int8_t delta)
{
    // provide -1, 0 or 1 for best results
    this->speedScale *= 1 - 0.01 * (float)delta;
}

void Buzzer::setSpeedRatio(float speedMultiplier)
{
    // argument is a long: 1 is normal speed, 0.5 is double, 2 is half, ....
    this->speedScale = speedMultiplier;
}

uint8_t Buzzer::getNextProgramIndex()
{
    //returns value of next slot
    uint8_t index = this->bufferProgramIndex;
    index++;
    if (index >= BUZZER_NOTES_BUFFER_LENGTH)
    {
        index = 0;
    }
    return index;
}

uint8_t Buzzer::getNextPlayIndex()
{
    //returns value of next slot
    uint8_t index = this->bufferPlayIndex;
    index++;
    if (index >= BUZZER_NOTES_BUFFER_LENGTH)
    {
        index = 0;
    }
    return index;
}

void Buzzer::buzzerSilent()
{
    noTone(this->pin);
}

void Buzzer::buzzerSilentClearBuffer()
{
    //erase contents of buzzerNotesBuffer and switch off.
    this->buzzerSilent();
    this->clearBuzzerNotesBuffer();
}

void Buzzer::playTone(unsigned int freq, unsigned long duration_millis)
{
    // set 0 to duration_millis for indefinite length (will sound until notone or another tone command is given. )

    //this->buzzerSilentClearBuffer();
    if (duration_millis == 0)
    {
        tone(this->pin, freq);
    }
    else
    {
        tone(this->pin, freq, duration_millis);
    }
}

uint8_t Buzzer::getBuzzerNotesBufferEmpty()
{
    return getNextProgramIndex() == this->bufferPlayIndex;
}

void Buzzer::lastPlayedNoteToDisplay(char *textBuf, uint8_t *decimalPoints)
{
    noteToDisplay(textBuf, decimalPoints, this->buzzerNotesBuffer[this->bufferPlayIndex]);
}

void Buzzer::noteToDisplay(char *textBuf, uint8_t *decimalPoints, uint8_t note)
{
    // assume 4 chars textBuf (empty at arrival (4 spaces))
    // assumme value of decimalPoints at start is 0x00

    uint8_t noteVal = note % NOTES_COUNT;

    if (note > LAST_NOTE)
    {
        // rest
        textBuf[0] = 59; // ONLY_MIDDLE_SEGMENT_FAKE_ASCII '-';;
        textBuf[1] = 59; // ONLY_MIDDLE_SEGMENT_FAKE_ASCII '-';;
    }
    else
    {
        textBuf[1] = (noteVal + 45) / 12 + 48;
        bool sharps[12] = {false, true, false, false, true, false, true, false, false, true, false, true};
        char notes_chars[12] = {'A', 'A', 'B', 'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G'};
        noteVal %= 12;

        // sharp indicated by a decimal point
        *decimalPoints = 0;
        *decimalPoints = sharps[noteVal] << 0;

        textBuf[0] = notes_chars[noteVal];
    }

    textBuf[2] = 62; // SPACE_FAKE_ASCII = 62 optimized: assume empty at start

    uint8_t length = this->getLength(note);
    if (length > 9)
    {
        // in ascii table, there is a gap of 7 positions between the numbers and letters. (for the rests, we need hex values.)
        length += 7;
    }
    textBuf[3] = length + 48;
}

uint8_t Buzzer::getLength(uint8_t note)
{
    if (note > LAST_NOTE)
    {
        return note - LAST_NOTE; // lenght in multiple of 1/8.
    }
    else
    {
        return 0x01 << (3 - (note / NOTES_COUNT)); // 2^(3 -x) --> note length is 8,4,2,1
    }
}

void Buzzer::changeNoteToNextLength(int16_t *note)
{
    *note += NOTES_COUNT;
    if (*note > LAST_NOTE)
    {
        *note -= 4 * NOTES_COUNT;
    }
}