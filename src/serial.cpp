#include "serial.h"

#if DEBUG
void safePrint(const String &msg)
{
  #if DEBUG
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
  #endif
}

void safePrintln(const String &msg)
{
  #if DEBUG
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
  #endif
}
#endif