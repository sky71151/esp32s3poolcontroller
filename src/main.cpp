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

TaskHandle_t firebaseTaskHandle = NULL;

SemaphoreHandle_t serialMutex = NULL;
QueueHandle_t firebaseQueue;
Preferences preferences;

FirebaseMsg firebaseMsg;

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

  safePrintln(formatLog("INFO", "create firebase queue"));

  firebaseQueue = xQueueCreate(5, sizeof(FirebaseMsg));
  if (firebaseQueue == NULL)
  {
    safePrintln(formatLog("ERROR", "Failed to create Firebase queue"));
  }else
  {
    safePrintln(formatLog("SUCCESS", "Firebase queue created"));
  }

  xTaskCreatePinnedToCore(firebaseTask, "FirebaseTask", 8192, NULL, 1, &firebaseTaskHandle, 1);
  xTaskCreatePinnedToCore(mainTask, "MainTask", 8192, NULL, 1, NULL, 1);
}

unsigned long lastTryTime = millis();

void mainTask(void *pvParameters)
{
  safePrintln(formatLog("MAIN", "mainTask gestart..."));
  for (;;)
  {
    // wait 30 seconden per try
    if (!firebaseInitialized && (millis() - lastTryTime > 30000))
    {
      lastTryTime = millis();
      // Firebase is not initialized, handle accordingly
      safePrintln(formatLog("ERROR", "Firebase is niet geïnitialiseerd!"));
      safePrintln(formatLog("INFO", "Herstart Firebase taak..."));
      // retry to start firebase task
      if (firebaseTaskHandle != NULL)
      {
        vTaskDelete(firebaseTaskHandle);
        firebaseTaskHandle = NULL;
      }
      else
      {
        safePrintln(formatLog("INFO", "Firebase taak is al gestopt, start een nieuwe..."));
        xTaskCreatePinnedToCore(firebaseTask, "FirebaseTask", 8192, NULL, 1, &firebaseTaskHandle, 1);
      }

      
    }
    safePrintln(formatLog("MAIN", "mainTask is running..."));
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wacht 1 seconde
  }

  
}

void loop()
{
}
