#include "MCP23S17.h"

#define MCP23S17_WRITE_CMD 0x40
#define MCP23S17_READ_CMD  0x41

#define IODIRA 0x00
#define IODIRB 0x01
#define GPIOA  0x12
#define GPIOB  0x13
#define OLATA  0x14
#define OLATB  0x15

MCP23S17::MCP23S17(uint8_t csPin) : _csPin(csPin) {}

void MCP23S17::begin() {
    pinMode(_csPin, OUTPUT);
    deselect();
    SPI.begin();
    // Zet alle pinnen als input bij start
    writeRegister(IODIRA, 0xFF);
    writeRegister(IODIRB, 0xFF);
}

void MCP23S17::pinMode(uint8_t pin, uint8_t mode) {
    uint8_t reg = (pin < 8) ? IODIRA : IODIRB;
    uint8_t bit = pin % 8;
    uint8_t iodir = readRegister(reg);
    if (mode == OUTPUT) {
        iodir &= ~(1 << bit);
    } else {
        iodir |= (1 << bit);
    }
    writeRegister(reg, iodir);
}

void MCP23S17::digitalWrite(uint8_t pin, uint8_t value) {
    uint8_t reg = (pin < 8) ? OLATA : OLATB;
    uint8_t bit = pin % 8;
    uint8_t olat = readRegister(reg);
    if (value) {
        olat |= (1 << bit);
    } else {
        olat &= ~(1 << bit);
    }
    writeRegister(reg, olat);
}

int MCP23S17::digitalRead(uint8_t pin) {
    uint8_t reg = (pin < 8) ? GPIOA : GPIOB;
    uint8_t bit = pin % 8;
    uint8_t gpio = readRegister(reg);
    return (gpio & (1 << bit)) ? HIGH : LOW;
}

void MCP23S17::writeRegister(uint8_t reg, uint8_t value) {
    select();
    SPI.transfer(MCP23S17_WRITE_CMD);
    SPI.transfer(reg);
    SPI.transfer(value);
    deselect();
}

uint8_t MCP23S17::readRegister(uint8_t reg) {
    select();
    SPI.transfer(MCP23S17_READ_CMD);
    SPI.transfer(reg);
    uint8_t value = SPI.transfer(0x00);
    deselect();
    return value;
}

void MCP23S17::select() {
    digitalWrite(_csPin, LOW);
}

void MCP23S17::deselect() {
    digitalWrite(_csPin, HIGH);
}
