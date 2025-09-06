#include "wifiTask.h"

void connectToWiFiTask(void *pvParameters)
{
  while (true)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      #if DEBUG
      safePrint("(Re)connect WiFi: ");
      safePrintln(WIFI_SSID);
      #endif
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      unsigned long startAttemptTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS)
      {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        #if DEBUG
        safePrint(".");
        #endif
      }
      if (WiFi.status() == WL_CONNECTED)
      {
        #if DEBUG
        safePrintln("\nWiFi verbonden!");
        safePrint("IP adres: ");
        safePrintln(WiFi.localIP().toString());
        #endif
      }
      else
      {
        #if DEBUG
        safePrintln("\nWiFi verbinding mislukt!");
        #endif
      }
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}