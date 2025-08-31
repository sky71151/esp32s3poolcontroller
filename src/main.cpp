#include "main.h"
#include "firebase.h"
#include "external_flash.h"
#include "ota.h"
#include "wifitask.h"
#include "board.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ===================== VARIABELEN =====================

// Persistent boot counter
uint32_t bootCount = 0;
bool flashReady = false;

#define WIFI_STACK 8192
#define FIREBASE_STACK 16384
#define STATUS_STACK 16384
#define UPDATE_STACK 16384
#define MAIN_STACK 16384

#define debug true

TaskHandle_t wifiHandle = nullptr;
TaskHandle_t firebaseHandle = nullptr;
TaskHandle_t statusHandle = nullptr;
TaskHandle_t updateHandle = nullptr;
TaskHandle_t mainHandle = nullptr;
TaskHandle_t stackMonitorHandle = nullptr;
TaskHandle_t FirebaseInputTaskHandle = nullptr;

TaskStackInfo taskStackInfos[] = {
    {"WiFiTask", &wifiHandle, WIFI_STACK},
    {"FirebaseTask", &firebaseHandle, FIREBASE_STACK},
    {"StatusTask", &statusHandle, STATUS_STACK},
    {"UpdateTask", &updateHandle, UPDATE_STACK},
    {"MainTask", &mainHandle, MAIN_STACK},
    {"StackMonitorTask", &stackMonitorHandle, STATUS_STACK},
    {"FirebaseInputTask", &FirebaseInputTaskHandle, STATUS_STACK},
};
const int numTasks = sizeof(taskStackInfos) / sizeof(taskStackInfos[0]);

FirebaseData fbdo;
FirebaseData fbdoStream;
FirebaseData fbdoInput;
FirebaseAuth auth;
FirebaseConfig config;

SemaphoreHandle_t serialMutex;
QueueHandle_t FirebaseInputQueue = nullptr;

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 60000; // 1 minuut
String bootTimeStr = "";

bool bootTimeUploaded = false;
bool firebaseInitialized = false;
bool streamConnected = false;
String deviceId;
bool updateAvailable = false;

//void firebaseTask(void *pvParameters);

// ===================== SETUP & LOOP =====================

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  delay(5000);
  serialMutex = xSemaphoreCreateMutex();

  gpioConfig();
  // Probeer flash te initialiseren en update bootCount
  initExternalFlash();
  updateBootCount();
  // printLogFromFlash();
  deviceId = getUniqueClientId(); // Unieke FuseID van de esp32
  String data = String("Firmware versie: ");
  data.concat(FIRMWARE_VERSION);
  safePrintln(data);
  data = String("Apparaat gestart, unieke ID: ");
  data.concat(deviceId);
  safePrintln(data);
  FirebaseInputQueue = xQueueCreate(10, 32);
  xTaskCreatePinnedToCore(connectToWiFiTask, "WiFiTask", WIFI_STACK, NULL, 2, &wifiHandle, 0); // prioriteit 2

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
  // Belgische tijdzone: CET/CEST (winter/zomertijd automatisch)
  // TZ string: "CET-1CEST,M3.5.0,M10.5.0/3"

  configTime(3600, 3600, "pool.ntp.org");
  // configTime(0, 0, "pool.ntp.org");
  time_t now = 0;
  while (now < 100000)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    now = time(nullptr);
  }

  safePrintln("Tijd gesynchroniseerd.");
  char bootStr[32];
  strftime(bootStr, sizeof(bootStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
  bootTimeStr = String(bootStr);
  String msg = String("Huidige tijd: ");
  msg.concat(bootTimeStr);
  safePrintln(msg);

    // Start FreeRTOS taak voor Firebase initialisatie en uploads
  xTaskCreatePinnedToCore(
    firebaseTask,    // Functie
    "FirebaseTask", // Naam
    8192,            // Stack grootte (iets groter ivm init)
    NULL,            // Parameters
    1,               // Prioriteit
    NULL,            // Handle
    1                // Core (1 = app core op ESP32)
  );

  //xTaskCreatePinnedToCore(initFirebaseTask, "FirebaseTask", FIREBASE_STACK, NULL, 2, &firebaseHandle, 1);   // prioriteit 2
  //xTaskCreatePinnedToCore(updateTimeToFirebaseTask, "UpdateTask", UPDATE_STACK, NULL, 1, &updateHandle, 1); // prioriteit 1
  xTaskCreatePinnedToCore(mainTask, "MainTask", MAIN_STACK, NULL, 1, &mainHandle, 1);
  //xTaskCreatePinnedToCore(FirebaseInputTask, "FirebaseInputTask", 4096, NULL, configMAX_PRIORITIES - 1, &FirebaseInputTaskHandle, 1);               // prioriteit 1
  if (debug)
  {
    xTaskCreatePinnedToCore(stackMonitorTask, "StackMonitorTask", STATUS_STACK, NULL, 0, &stackMonitorHandle, 1); // prioriteit 0
  }
}

void loop()
{
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// ===================== FREERTOS TAKEN (alfabetisch) =====================


void mainTask(void *pvParameters)
{
  const String DipSwitchPath = String("/GPIO/DipSwitches");
  const String DigitalInputPath = String("/GPIO/DigitalInputs");
  const String AnalogInputPath = String("/GPIO/AnalogInputs");
  const String RelayPath = String("/GPIO/Relays");
  String PreviousDipSwitchState = "";
  String PreviousDigitalInputState = "";
  String PreviousAnalogInputState = "";
  String PreviousRelayState = "";
  while (true)
  {
    if (updateAvailable)
    {
      TaskHandle_t handle;
      for (int i = 0; i < numTasks; i++)
      {
        handle = *(taskStackInfos[i].handle);
        String data = String("[MAIN] Controleren taak: ");
        data.concat(i);
        safePrintln(data);
        if (handle == nullptr || handle == mainHandle)
        {
          String taskName = String("[MAIN] Taak ");
          taskName.concat(i);
          taskName.concat(" is niet gestart.");
          safePrintln(taskName);
          safePrintln("[MAIN] Of overslaan van opschorting van MainTask om deadlock te voorkomen.");
          safePrintln("[MAIN] MainTask is actief en blijft actief!");
          continue;
        }
        // check if task is running
        eTaskState state = eTaskGetState(handle);

        String taskName = String("[MAIN] Task ");
        taskName.concat(i);
        taskName.concat(" state: ");
        taskName.concat(state);
        safePrintln(taskName);
        if (state == eRunning)
        {
          String data = String("[MAIN] Task ");
          data.concat(i);
          data.concat(" is running (eRunning). Probeer te suspenden...");
          safePrintln(data);
          vTaskSuspend(handle);
          data = String("[MAIN] Task ");
          data.concat(i);
          data.concat(" is gesuspendeerd!");
          safePrintln(data);
        }
        else
        {
          data = String("[MAIN] Task ");
          data.concat(i);
          data.concat(" is NIET running (state=");
          data.concat(state);
          data.concat(")");
          safePrintln(data);
        }
      }

      performOTA();
    }
     /* 
    if (Firebase.ready())
    {

      if (readDipSwitches() != PreviousDipSwitchState)
      {
        PreviousDipSwitchState = readDipSwitches();
        updateFirebaseInstant(DipSwitchPath, PreviousDipSwitchState);
      }
    }*/
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void systemStatusTask(void *pvParameters)
{
  while (true)
  {
    uint32_t freeHeap = ESP.getFreeHeap();
    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    String wifiStatus = (WiFi.status() == WL_CONNECTED) ? "Verbonden" : "Niet verbonden";
    //String firebaseStatus = Firebase.ready() ? "Verbonden" : "Niet verbonden";
    safePrint("[STATUS] Heap: ");
    safePrint(String(freeHeap));
    safePrint(" bytes | Stack: ");
    safePrint(String(stackHighWaterMark * sizeof(StackType_t)));
    safePrint(" bytes | WiFi: ");
    safePrint(wifiStatus);
    safePrint(" | Firebase: ");
    //safePrintln(firebaseStatus);
    safePrintln("[TASKS] Naam | Stack gebruikt (bytes) | Stack totaal (bytes) | % gebruikt");
    for (int i = 0; i < numTasks; i++)
    {
      TaskStackInfo &info = taskStackInfos[i];
      if (*(info.handle))
      {
        UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(*(info.handle));
        uint32_t stackTotalBytes = info.stackWords * sizeof(StackType_t);
        uint32_t stackFreeBytes = highWaterMark * sizeof(StackType_t);
        uint32_t stackUsedBytes = stackTotalBytes - stackFreeBytes;
        int percentUsed = (stackUsedBytes * 100) / stackTotalBytes;
        safePrint("  ");
        safePrint(info.name);
        safePrint(" | ");
        safePrint(String(stackUsedBytes));
        safePrint("/");
        safePrint(String(stackTotalBytes));
        safePrint(" = ");
        safePrint(String(percentUsed));
        safePrint("%");
        safePrintln(" used");
      }
    }
    vTaskDelay(30000 / portTICK_PERIOD_MS);
  }
}

// Zeer uitgebreide stack/heap monitoring task
void stackMonitorTask(void *pvParameters)
{
  while (true)
  {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    safePrintln("\n[STACK MONITOR] =====================");
    safePrint("Vrije heap: ");
    safePrint(String(freeHeap));
    safePrint(" bytes | Min. ooit: ");
    safePrintln(String(minFreeHeap));

    // Print status van alle bekende taken
    safePrintln("Naam | Stack gebruikt (bytes) | Stack totaal (bytes) | % gebruikt | HighWaterMark | Handle");
    for (int i = 0; i < numTasks; i++)
    {
      TaskStackInfo &info = taskStackInfos[i];
      if (*(info.handle))
      {
        UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(*(info.handle));
        uint32_t stackTotalBytes = info.stackWords * sizeof(StackType_t);
        uint32_t stackFreeBytes = highWaterMark * sizeof(StackType_t);
        uint32_t stackUsedBytes = stackTotalBytes - stackFreeBytes;
        int percentUsed = (stackUsedBytes * 100) / stackTotalBytes;
        safePrint(info.name);
        safePrint(" | ");
        safePrint(String(stackUsedBytes));
        safePrint("/");
        safePrint(String(stackTotalBytes));
        safePrint(" = ");
        safePrint(String(percentUsed));
        safePrint("% | ");
        safePrint(String(stackFreeBytes));
        safePrint(" | 0x");
        safePrint(String((uint32_t)(*(info.handle)), HEX));
        safePrintln("");
      }
      else
      {
        safePrint(info.name);
        safePrintln(" | (geen handle!)");
      }
    }

    // Print status van huidige taak (optioneel)
    UBaseType_t currHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    safePrint("Huidige taak stack high water mark: ");
    safePrintln(String(currHighWaterMark * sizeof(StackType_t)));

    // Print WiFi en Firebase status
    safePrint("WiFi status: ");
    safePrintln((WiFi.status() == WL_CONNECTED) ? "Verbonden" : "Niet verbonden!");
    //safePrint("Firebase status: ");
    //safePrintln(Firebase.ready() ? "Verbonden" : "Niet verbonden!");
    vTaskDelay(15000 / portTICK_PERIOD_MS); // elke 15s
  }
}

// ===================== OVERIGE FUNCTIES =====================

void safePrint(const String &msg)
{
  if (debug)
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
  if (debug)
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

String HuidigeTijd()
{
  time_t now = time(nullptr);
  char timeStr[32];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
  return String(timeStr);
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  LogEntry entry;
  entry.timestamp = millis();
  entry.type = 1; // stack overflow
  entry.value = 0xFE;
  strncpy(entry.taskName, pcTaskName, sizeof(entry.taskName));
  entry.freeHeap = ESP.getFreeHeap();
  entry.stackWatermark = uxTaskGetStackHighWaterMark(xTask);
  logToFlash(&entry, sizeof(entry));
}


