#include "BinaryInput.h"

BinaryInput::BinaryInput()
{
    setToggleValue(0);
}

void BinaryInput::setValue(bool value)
{
    this->value = value;
}

bool BinaryInput::getValue()
{
    return this->value;
}
bool BinaryInput::getValueChanged()

{

    return this->value != this->previousValue;
}

bool BinaryInput::getToggleValue()
{
    // toggle is used to mimic a latching button with a momentary button
    // it will automatically toggle at each edge up.
    return this->toggleValue;
}

void BinaryInput::setToggleValue(bool value)
{
    this->toggleValue = value;
}

void BinaryInput::refreshEdges()
{
    if (this->getValueChanged() && this->value)
    {
        this->toggleValue = !this->toggleValue;
    }
    this->previousValue = this->value;
}

bool BinaryInput::getEdgeDown()
{
    if (this->getValueChanged())
    {
        return !this->value;
    }
    else
    {
        return 0;
    }
}

bool BinaryInput::getEdgeUp()
{
    if (this->getValueChanged() && this->value)
    {

        // this->toggleValue = !this->toggleValue;
        return true;
    }
    else
    {
        return 0;
    }
}
