#include "main.h"
// Professionele serial logging


FirebaseData fbdo;
FirebaseData Stream;
FirebaseData FirmwareStream;
FirebaseAuth auth;
FirebaseConfig config;

time_t timeNow = 0;
time_t bootTime = 0;

bool firebaseInitialized = false;

SemaphoreHandle_t serialMutex = NULL;
QueueHandle_t firebaseQueue;
Preferences preferences;

void setup()
{
  serialInit();
  Serial.begin(115200);
  delay(5000);
  device.Init();
  safePrintln(formatLog("INFO", "Start WiFi verbinding..."));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int wifiTries = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTries < 40)
  {
    delay(500);
    safePrint("."); // Progress indicator, geen loglevel nodig
    wifiTries++;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    safePrintln("");
    safePrintln(formatLog("SUCCESS", "WiFi connected!"));
    configTime(3600, 3600, "pool.ntp.org");
    while (timeNow < 100000)
    {
      timeNow = time(nullptr);
    }

    safePrintln(formatLog("INFO", "Huidige tijd: " + device.getTime()));
  }
  else
  {
    safePrintln("");
    safePrintln(formatLog("ERROR", "WiFi verbinding mislukt!"));
  }
  
  xTaskCreatePinnedToCore(firebaseTask, "FirebaseTask", 8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(mainTask, "MainTask", 8192, NULL, 1, NULL, 1);
}

unsigned long lastTryTime = millis();

void mainTask(void *pvParameters)
{
  for (;;)
  {
    //wait 30 seconden per try
    if (!firebaseInitialized && (millis() - lastTryTime > 30000))
    {
      // Firebase is not initialized, handle accordingly
  safePrintln(formatLog("ERROR", "Firebase is niet geïnitialiseerd!"));
  safePrintln(formatLog("INFO", "Herstart Firebase taak..."));
      //retry to start firebase task
      xTaskCreatePinnedToCore(firebaseTask, "FirebaseTask", 8192, NULL, 1, NULL, 1);
      lastTryTime = millis();
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wacht 1 seconde
  }
}

void loop()
{
}
