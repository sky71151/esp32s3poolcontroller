
#include "main.h"

// Firebase project credentialsßß
FirebaseData fbdo;
FirebaseData fbdoStream;
FirebaseData fbdoInput;
FirebaseAuth auth;
FirebaseConfig config;

// Queue & Semaphores
SemaphoreHandle_t serialMutex;
QueueHandle_t FirebaseInputQueue;

// Task handles
TaskHandle_t firebaseTaskHandle;
TaskHandle_t wifiTaskHandle;
TaskHandle_t updateFirebaseTaskHandle;
TaskHandle_t mainTaskHandle;

// TaskStackInfo
TaskStackInfo taskStackInfos[] = {
    {"firebaseTask", &firebaseTaskHandle, FIREBASE_STACK},
    {"wifiTask", &wifiTaskHandle, WIFI_STACK},
    {"updateFirebaseTask", &updateFirebaseTaskHandle, UPDATE_FIREBASE_STACK},
    {"mainTask", &mainTaskHandle, MAIN_STACK}};

const int numTasks = sizeof(taskStackInfos) / sizeof(taskStackInfos[0]);

// variabelen
unsigned long dataMillis = 0;
int count = 0;

bool signupOK = false;
bool debugMode = true;
bool firebaseInitialized = false;
bool updateAvailable = false;
bool streamConnected = false;
bool bootTimeUploaded = false;
bool firmwareStreamConnected = false;
bool inputStreamConnected = false;
bool streamReceived = false;

const unsigned long updateInterval = 60000; // 1 minuut

// String device.Id;
String bootTimeStr = "";

time_t timeNow = 0;

// lokale variabelen
unsigned long lastFirmwareConnectAttempt = 0;
const unsigned long firmwareConnectInterval = 5000; // 5 seconden
unsigned long lastInputConnectAttempt = 0;
const unsigned long inputConnectInterval = 5000; // 5 seconden

// lokale functies
String HuidigeTijd();
void setupTime();
void mainTask(void *pvParameters);

void setup()
{
  Serial.begin(115200);
  delay(5000);
  Serial.println("Setup gestart!");
  // gpioConfig();
  device.Init();
  Serial.println("GPIO geconfigureerd.");
  String data = String("Apparaat gestart, unieke ID: ");
  data.concat(device.Id);
  safePrintln(data);

  xTaskCreatePinnedToCore(connectToWiFiTask, "connectToWiFiTask", WIFI_STACK, NULL, 1, &wifiTaskHandle, 1);
  // WiFi-wachtrij met timeout (10s)
  unsigned long wifiWaitStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiWaitStart < 10000))
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    safePrintln("[ERROR] WiFi niet verbonden na timeout in setup!");
  }

  setupTime();
  safePrint("Tijd gesynchroniseerd: ");
  safePrintln(HuidigeTijd());
  safePrintln("setup firebase");
  initFirebase();
  xTaskCreatePinnedToCore(updateFirebaseTask, "updateFirebaseTask", UPDATE_FIREBASE_STACK, NULL, 1, &updateFirebaseTaskHandle, 1);
  xTaskCreatePinnedToCore(mainTask, "mainTask", MAIN_STACK, NULL, 1, &mainTaskHandle, 1);
}

void loop()
{
  // Niets te doen, alles gebeurt in de FreeRTOS taak
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void mainTask(void *pvParameters)
{
  while (true)
  {
    //---------------------------------------------------------------------
    // Hoofdtaken uitvoeren
    //---------------------------------------------------------------------

    // check if interrupt is triggered
    //---------------------------------------------------------------------
    if (device.irsTriggered)
    {
      for (int i = 0; i < NUM_DIGITAL_INPUTS; i++)
      {
        if (device.inputChanged[i])
        {
          String message = String("Digitale ingang ");
          message.concat(String(i));
          message.concat(" veranderd: ");
          message.concat(String(device.readInput(i)));
          safePrintln(message);
          device.inputChanged[i] = false;
        }
      }

      device.irsTriggered = false;
    }

    // stream ontvangen
    //---------------------------------------------------------------------
    if (streamReceived)
    {
      safePrintln("[STREAM] Nieuwe stream data ontvangen.");
      String message = String("Laatst ontvangen stream: ");
      message.concat(HuidigeTijd());
      String path = "devices/";
      path.concat(device.Id);
      path.concat("/Registration/lastStreamData");
      Firebase.RTDB.setString(&fbdo, path, message);
      streamReceived = false;
    }

    // check if update is available
    //---------------------------------------------------------------------
    if (updateAvailable)
    {
      performOTA();
    }

    // check stream verbinding.
    //---------------------------------------------------------------------
    //manageFirebaseStreams();

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

String HuidigeTijd()
{
  time_t now = time(nullptr);
  char timeStr[32];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
  return String(timeStr);
}

void setupTime()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    configTime(3600, 3600, "pool.ntp.org");
    while (timeNow < 100000)
    {
      timeNow = time(nullptr);
    }
  }
}
