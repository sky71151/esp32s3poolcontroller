#include "main.h"
#include "board.h"
#include "wifitask.h"

void connectToWiFiTask(void *pvParameters)
{
  while (true)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      safePrint("(Re)connect WiFi: ");
      safePrintln(WIFI_SSID);
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      unsigned long startAttemptTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS)
      {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        safePrint(".");
      }
      if (WiFi.status() == WL_CONNECTED)
      {
        safePrintln("\nWiFi verbonden!");
        safePrint("IP adres: ");
        safePrintln(WiFi.localIP().toString());
      }
      else
      {
        safePrintln("\nWiFi verbinding mislukt!");
      }
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}