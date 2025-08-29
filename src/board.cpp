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
  for (int i = 0; i < NUM_ANALOG_INPUTS; i++)
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