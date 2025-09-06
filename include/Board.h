#ifndef BOARD_H
#define BOARD_H

#include <Arduino.h>

// LED Configuration
#define LED_PIN 46

// Relay Configuration
const int RELAY_PINS[] = {15, 16, 17, 8, 14, 9, 3, 2};  // Fixed duplicate pin 14 -> 15
const int NUM_RELAYS = sizeof(RELAY_PINS) / sizeof(RELAY_PINS[0]);

// DIP Switch Configuration
const int DIP_SWITCH_PINS[] = {1, 48, 21, 18};
const int NUM_DIP_SWITCHES = sizeof(DIP_SWITCH_PINS) / sizeof(DIP_SWITCH_PINS[0]);

// Digital Input Configuration
const int DIGITAL_INPUT_PINS[] = {41, 42, 47, 40, 38, 39};
const int NUM_DIGITAL_INPUTS = sizeof(DIGITAL_INPUT_PINS) / sizeof(DIGITAL_INPUT_PINS[0]);

// Analog Input Configuration
const int ANALOG_INPUT_PINS[] = {4, 5, 6, 7};
#define NUM_ADC_INPUTS (sizeof(ANALOG_INPUT_PINS) / sizeof(ANALOG_INPUT_PINS[0]))
#define ANALOG_RESOLUTION_BITS 12     // ESP32-S3 has 12-bit ADC
#define ANALOG_MAX_VALUE 4095         // 2^12 - 1
#define ANALOG_REFERENCE_VOLTAGE 3.3  // ESP32-S3 reference voltage

// SPI Flash Configuration (W25Q128 - 16MB)
#define SPI_FLASH_CS_PIN    10
#define SPI_FLASH_MOSI_PIN  11
#define SPI_FLASH_CLK_PIN   12
#define SPI_FLASH_MISO_PIN  13
#define SPI_FLASH_SIZE_MB   16

// Serial Configuration
#define SERIAL_BAUD_RATE 115200

//prototypes
void gpioConfig();
String getUniqueClientId();
String readDipSwitches();

class Board {
public:
    int relays[8];
    int input[4];
    double adcValues[4];
    double adc[4];
    volatile bool inputChanged[6];
    volatile bool irsTriggered;
    String Id; // Unieke ID van het bord
    
    // Voeg hier meer eigenschappen/methodes toe

    Board(); // constructor
    void setRelay(int index, int value);
    bool readInput(int index);
    double getAdcValue(int index) const;
    void updateAdcValues();
    void updateInputValues();
    void setRelays(String relayValues);
    void Init();
    void handleInputInterrupt(int index);
    String getTime();
    // etc.
};

#endif // BOARD_H