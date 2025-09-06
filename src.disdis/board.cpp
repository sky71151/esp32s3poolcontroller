#include "Board.h"
#include <Arduino.h>

void gpioConfig()
{
  pinMode(LED_PIN, OUTPUT);
  for (int i = 0; i < NUM_RELAYS; i++)
  {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }
  for (int i = 0; i < NUM_DIP_SWITCHES; i++)
  {
    pinMode(DIP_SWITCH_PINS[i], INPUT);
  }
  for (int i = 0; i < NUM_DIGITAL_INPUTS; i++)
  {
    pinMode(DIGITAL_INPUT_PINS[i], INPUT);
  }
  for (int i = 0; i < (int)(sizeof(ANALOG_INPUT_PINS) / sizeof(ANALOG_INPUT_PINS[0])); i++)
  {
    pinMode(ANALOG_INPUT_PINS[i], INPUT);
  }
  digitalWrite(LED_PIN, LOW);
}

String getUniqueClientId()
{
  uint64_t mac = ESP.getEfuseMac();
  char macStr[13];
  snprintf(macStr, sizeof(macStr), "%012llX", mac);
  // return String("ESP32S3_") + String(macStr);
  return String(macStr);
}

String readDipSwitches()
{
  String dipStates = "";
  for (int i = 0; i < NUM_DIP_SWITCHES; i++)
  {
    int state = digitalRead(DIP_SWITCH_PINS[i]);
    dipStates += String(state);
    if (i < NUM_DIP_SWITCHES - 1)
    {
      dipStates += ",";
    }
  }
  return dipStates;
}

Board device;

// ISR's voor alle 6 ingangen
void IRAM_ATTR inputISR0() { device.handleInputInterrupt(0); }
void IRAM_ATTR inputISR1() { device.handleInputInterrupt(1); }
void IRAM_ATTR inputISR2() { device.handleInputInterrupt(2); }
void IRAM_ATTR inputISR3() { device.handleInputInterrupt(3); }
void IRAM_ATTR inputISR4() { device.handleInputInterrupt(4); }
void IRAM_ATTR inputISR5() { device.handleInputInterrupt(5); }

Board::Board()
{
  for (int i = 0; i < 8; i++)
    relays[i] = 0;
  for (int i = 0; i < 6; i++)
  {
    input[i] = 0;
    inputChanged[i] = false;
  }
  for (int i = 0; i < 4; i++)
    adc[i] = 0.0;

  irsTriggered = false;
}

void Board::handleInputInterrupt(int index) {
    inputChanged[index] = true;
    irsTriggered = true;
}

void Board::setRelay(int index, int value)
{
  if (index >= 0 && index < 8)
  {
    digitalWrite(RELAY_PINS[index], value);
    relays[index] = value;
  }
}

void Board::setRelays(String relayValues)
{
  // Verwerk de binnenkomende string en stel de relais in
  char *token = strtok((char *)relayValues.c_str(), ",");
  int i = 0;
  while (token != nullptr && i < 8)
  {
    relays[i] = atoi(token);
    digitalWrite(RELAY_PINS[i], relays[i]);
    token = strtok(nullptr, ",");
    i++;
  }
}

bool Board::readInput(int index)
{
  if (index >= 0 && index < 6)
  {
    input[index] = digitalRead(DIGITAL_INPUT_PINS[index]);
    return input[index];
  }
  return -1;
}

double Board::getAdcValue(int index) const
{
  if (index >= 0 && index < 4)
  {
    double adc = analogRead(ANALOG_INPUT_PINS[index]);
    return adc;
  }

  return -1.0;
}

void Board::updateAdcValues()
{
  for (int i = 0; i < 4; i++)
  {
    adc[i] = analogRead(ANALOG_INPUT_PINS[i]);
  }
}

void Board::updateInputValues()
{
  for (int i = 0; i < 6; i++)
  {
    input[i] = digitalRead(DIGITAL_INPUT_PINS[i]);
  }
}

void Board::Init()
{
  pinMode(LED_PIN, OUTPUT);
  for (int i = 0; i < NUM_RELAYS; i++)
  {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
    relays[i] = 0;
  }
  for (int i = 0; i < NUM_DIP_SWITCHES; i++)
  {
    pinMode(DIP_SWITCH_PINS[i], INPUT);
  }
  for (int i = 0; i < NUM_DIGITAL_INPUTS; i++)
  {
    pinMode(DIGITAL_INPUT_PINS[i], INPUT);
  }
  for (int i = 0; i < (int)(sizeof(ANALOG_INPUT_PINS) / sizeof(ANALOG_INPUT_PINS[0])); i++)
  {
    pinMode(ANALOG_INPUT_PINS[i], INPUT);
  }
  digitalWrite(LED_PIN, LOW);
  Id = getUniqueClientId();

  attachInterrupt(DIGITAL_INPUT_PINS[0], inputISR0, CHANGE);
  attachInterrupt(DIGITAL_INPUT_PINS[1], inputISR1, CHANGE);
  attachInterrupt(DIGITAL_INPUT_PINS[2], inputISR2, CHANGE);
  attachInterrupt(DIGITAL_INPUT_PINS[3], inputISR3, CHANGE);
  attachInterrupt(DIGITAL_INPUT_PINS[4], inputISR4, CHANGE);
  attachInterrupt(DIGITAL_INPUT_PINS[5], inputISR5, CHANGE);
  
}