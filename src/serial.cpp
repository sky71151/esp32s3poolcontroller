#include "serial.h"


void safePrint(const String &msg)
{
  if (debugMode)
  {
    if (serialMutex)
    {
      if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE)
      {
        Serial.print(msg);
        xSemaphoreGive(serialMutex);
      }
    }
    else
    {
      Serial.print(msg);
    }
  }
}

void safePrintln(const String &msg)
{
  if (debugMode)
  {
    if (serialMutex)
    {
      if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE)
      {
        Serial.println(msg);
        xSemaphoreGive(serialMutex);
      }
    }
    else
    {
      Serial.println(msg);
    }
  }
}