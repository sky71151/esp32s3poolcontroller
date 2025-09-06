#include "serial.h"

void serialInit()
{
  // Initialiseer de mutex in de setup-functie
  serialMutex = xSemaphoreCreateMutex();
  if (serialMutex == NULL)
  {
    // Indien de creatie faalt, doe iets om de fout te melden
    // zoals het laten knipperen van de LED
  }
}

#if DEBUG
void safePrint(const String &msg)
{
#if DEBUG
  if (serialMutex != NULL)
  {
    // Probeer de mutex te vergrendelen, wacht maximaal 1000 milliseconden
    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
      Serial.print(msg);
      // Geef de mutex vrij
      xSemaphoreGive(serialMutex);
    }
  }
#endif
}

void safePrintln(const String &msg)
{
#if DEBUG
  if (serialMutex != NULL)
  {
    // Probeer de mutex te vergrendelen, wacht maximaal 1000 milliseconden
    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
      Serial.println(msg);
      // Geef de mutex vrij
      xSemaphoreGive(serialMutex);
    }
  };
#endif
}
#endif