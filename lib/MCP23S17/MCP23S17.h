#ifndef MCP23S17_H
#define MCP23S17_H

#include <Arduino.h>
#include <SPI.h>

class MCP23S17 {
public:
    MCP23S17(uint8_t csPin);
    void begin();
    void pinMode(uint8_t pin, uint8_t mode);
    void digitalWrite(uint8_t pin, uint8_t value);
    int digitalRead(uint8_t pin);

private:
    uint8_t _csPin;
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    void select();
    void deselect();
};

#endif
