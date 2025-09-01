#pragma once

#include "main.h"

class Device {
public:
    int relays[8];
    int input[4];
    double adcValues[4];
    double adc[4];
    // Voeg hier meer eigenschappen/methodes toe

    Device(); // constructor
    void setRelay(int index, int value);
    bool readInput(int index, int value);
    double getAdcValue(int index) const;
    void updateAdcValues();
    void updateInputValues();
    void setRelays(String relayValues);
    void Init();
    // etc.
};