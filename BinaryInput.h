
#ifndef BinaryInput_h
#define BinaryInput_h

class BinaryInput
{

public:
    BinaryInput();
    void setValue(bool value);
    bool getValue();
    bool getEdgeDown();
    bool getEdgeUp();
    bool getValueChanged();
    void refreshEdges();
    bool getToggleValue();
    void setToggleValue(bool value);

private:
    bool previousValue;
    bool value;
    bool toggleValue;
};

#endif
