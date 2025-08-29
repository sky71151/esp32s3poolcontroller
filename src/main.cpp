#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "Board.h"
#include "version.h"
#include "secrets.h"
#include "external_flash.h"



#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ===================== VARIABELEN =====================
typedef struct
{
  const char *name;
  TaskHandle_t *handle;
  uint32_t stackWords;
} TaskStackInfo;

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

TaskStackInfo taskStackInfos[] = {
    {"WiFiTask", &wifiHandle, WIFI_STACK},
    {"FirebaseTask", &firebaseHandle, FIREBASE_STACK},
    {"StatusTask", &statusHandle, STATUS_STACK},
    {"UpdateTask", &updateHandle, UPDATE_STACK},
    {"MainTask", &mainHandle, MAIN_STACK},
};
const int numTasks = sizeof(taskStackInfos) / sizeof(taskStackInfos[0]);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

SemaphoreHandle_t serialMutex;

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 60000; // 1 minuut
String bootTimeStr = "";

bool bootTimeUploaded = false;
bool firebaseInitialized = false;
String deviceId;

// Forward declarations
void safePrint(const String &msg);
void safePrintln(const String &msg);
void gpioConfig();
String getUniqueClientId();
String readDipSwitches();
void updateFirebaseInstant(String path, String data);
void connectToWiFiTask(void *pvParameters);
void initFirebaseTask(void *pvParameters);
void systemStatusTask(void *pvParameters);
void updateTimeToFirebaseTask(void *pvParameters);
void mainTask(void *pvParameters);
void stackMonitorTask(void *pvParameters);
void updateBootCount();

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
  printLogFromFlash();
  deviceId = getUniqueClientId(); // Unieke FuseID van de esp32
  safePrintln(String("Apparaat gestart, unieke ID: ") + String(deviceId));
  xTaskCreatePinnedToCore(connectToWiFiTask, "WiFiTask", WIFI_STACK, NULL, 2, &wifiHandle, 0); // prioriteit 2
  // xTaskCreatePinnedToCore(systemStatusTask, "StatusTask", STATUS_STACK, NULL, 1, &statusHandle, 1);
  // WiFi-wachtrij met timeout (10s)
  unsigned long wifiWaitStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiWaitStart < 10000))
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  if (WiFi.status() != WL_CONNECTED) {
    safePrintln("[ERROR] WiFi niet verbonden na timeout in setup!");
  }
  // Belgische tijdzone: CET/CEST (winter/zomertijd automatisch)
  // TZ string: "CET-1CEST,M3.5.0,M10.5.0/3"

  configTime(3600, 3600, "pool.ntp.org");
  time_t now = 0;
  while (now < 100000)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    now = time(nullptr);
  }
  char bootStr[32];
  strftime(bootStr, sizeof(bootStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
  bootTimeStr = String(bootStr);
  safePrintln(String("Huidige tijd: ") + String(bootTimeStr));

  xTaskCreatePinnedToCore(initFirebaseTask, "FirebaseTask", FIREBASE_STACK, NULL, 2, &firebaseHandle, 1); // prioriteit 2
  xTaskCreatePinnedToCore(updateTimeToFirebaseTask, "UpdateTask", UPDATE_STACK, NULL, 1, &updateHandle, 1); // prioriteit 1
  xTaskCreatePinnedToCore(mainTask, "MainTask", MAIN_STACK, NULL, 1, &mainHandle, 1); // prioriteit 1
  if (debug)
  {
    xTaskCreatePinnedToCore(stackMonitorTask, "StackMonitorTask", STATUS_STACK, NULL, 0, NULL, 1); // prioriteit 0
  }
}

// BootCount flash logica
void updateBootCount()
{
  if (externalFlashRead(0, (uint8_t *)&bootCount, sizeof(bootCount)))
  {
    // Check op onbeschreven flash (0xFFFFFFFF)
    if (bootCount == 0xFFFFFFFF)
    {
      bootCount = 0;
    }
    bootCount++;
    if (externalFlashErase4k(0) && externalFlashWrite(0, (uint8_t *)&bootCount, sizeof(bootCount)))
    {
      Serial.print("BootCount opgeslagen: ");
      Serial.println(bootCount);
    }
    else
    {
      Serial.println("Fout bij wissen/schrijven van bootCount naar flash!");
    }
  }
  else
  {
    Serial.println("Fout bij lezen van bootCount uit flash!");
  }
}

void loop()
{
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// ===================== FREERTOS TAKEN (alfabetisch) =====================

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

void initFirebaseTask(void *pvParameters)
{
  String regPath = String("/devices/");
  regPath += deviceId;
  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED && !Firebase.ready())
    {
      // Firebase.reset(&config);
      config.api_key = API_KEY;
      config.database_url = DATABASE_URL;
      // Anonieme login: geen e-mail/wachtwoord invullen
      // auth.user.email en auth.user.password NIET instellen
      auth.user.email = USER_EMAIL;
      auth.user.password = USER_PASSWORD;
      Firebase.begin(&config, &auth); // auth leeg laat anonieme login toe
      Firebase.reconnectWiFi(true);
      safePrintln("Firebase opnieuw geïnitialiseerd (anoniem)");
      firebaseInitialized = false; // reset status bij herinitialisatie
    }
    if (WiFi.status() == WL_CONNECTED && Firebase.ready() && !firebaseInitialized)
    {
      // Voorbeeld: controleer of device geregistreerd is met unieke ID
      if (Firebase.RTDB.pathExisted(&fbdo, regPath.c_str()))
      {
        {
          String msg = "Pad bestaat: ";
          msg.concat(regPath);
          safePrintln(msg);
        }
        time_t now = time(nullptr);
        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        String pathTime = "devices/";
        pathTime.concat(deviceId);
        pathTime.concat("/Registration/lastBoot");
        if (Firebase.RTDB.setString(&fbdo, pathTime, timeStr))
        {
          safePrint("Boot Time update: ");
          safePrintln(timeStr);
        }
        else
        {
          safePrint("Fout bij uploaden boot tijd: ");
          safePrintln(fbdo.errorReason());
        }
        String pathFirmware = "devices/";
        pathFirmware.concat(deviceId);
        pathFirmware.concat("/DeviceInfo/firmware");
  if (Firebase.RTDB.setString(&fbdo, pathFirmware.c_str(), String(bootCount) + String(" ") + String(timeStr)))
        {
          safePrint("Firmware version update: ");
          safePrintln(String(bootCount));
        }
        else
        {
          safePrint("Fout bij uploaden firmware versie: ");
          safePrintln(fbdo.errorReason());
        }
        firebaseInitialized = true;
      }
      else
      {

        String msg = "Pad bestaat niet: ";
        msg.concat(regPath);
        safePrintln(msg);

        msg = "Device wordt geregistreerd: ";
        msg.concat(deviceId);
        safePrintln(msg);

        // Create device data JSON
        FirebaseJson deviceJson;

        // Device info section
        FirebaseJson DeviceInfo;
        DeviceInfo.set("clientId", deviceId);
        DeviceInfo.set("boardType", "ESP32-S3  R1N16");
        DeviceInfo.set("firmware", FIRMWARE_VERSION);
        deviceJson.set("DeviceInfo", DeviceInfo);

        // Registration section
        FirebaseJson Registration;
        Registration.set("firstRegistratione", bootTimeStr);
        Registration.set("lastBoot", bootTimeStr);
        Registration.set("lastSeen", bootTimeStr);
        Registration.set("uptime", "0");
        deviceJson.set("Registration", Registration);

        // GPIO section
        FirebaseJson Device;
        Device.set("Relays", "0,0,0,0,0,0,0,0");
        Device.set("DipSwitches", "0,0,0,0");
        Device.set("DigitalInputs", "0,0,0,0");
        Device.set("AnalogInputs", "0,0,0,0");
        deviceJson.set("GPIO", Device);

        if (Firebase.RTDB.setJSON(&fbdo, regPath.c_str(), &deviceJson))
        {
          safePrintln("Device geregistreerd in Firebase Realtime Database.");
          firebaseInitialized = true;
        }
        else
        {
          safePrint("Fout bij registreren device: ");
          safePrintln(fbdo.errorReason());
        }
      }

      // firebaseInitialized = true;
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

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
    if (Firebase.ready())
    {

      if (readDipSwitches() != PreviousDipSwitchState)
      {
        PreviousDipSwitchState = readDipSwitches();
        updateFirebaseInstant(DipSwitchPath, PreviousDipSwitchState);
      }
    }
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
    String firebaseStatus = Firebase.ready() ? "Verbonden" : "Niet verbonden";
    safePrint("[STATUS] Heap: ");
    safePrint(String(freeHeap));
    safePrint(" bytes | Stack: ");
    safePrint(String(stackHighWaterMark * sizeof(StackType_t)));
    safePrint(" bytes | WiFi: ");
    safePrint(wifiStatus);
    safePrint(" | Firebase: ");
    safePrintln(firebaseStatus);
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

void updateTimeToFirebaseTask(void *pvParameters)
{
  while (true)
  {
    if (WiFi.status() == WL_CONNECTED && Firebase.ready() && firebaseInitialized)
    {
      time_t now = time(nullptr);
      char timeStr[32];
      strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
      String pathTime = "devices/";
      pathTime.concat(deviceId);
      pathTime.concat("/Registration/lastSeen");
      if (Firebase.RTDB.setString(&fbdo, pathTime, timeStr))
      {
        safePrint("Tijd geüpload: ");
        safePrintln(timeStr);
      }
      else
      {
        safePrint("Fout bij uploaden tijd: ");
        safePrintln(fbdo.errorReason());
      }
      unsigned long runtimeMillis = millis();
      unsigned long totalMinutes = runtimeMillis / 60000;
      unsigned int hours = totalMinutes / 60;
      unsigned int minutes = totalMinutes % 60;
      char runtimeStr[16];
      snprintf(runtimeStr, sizeof(runtimeStr), "%02u:%02u", hours, minutes);
      String pathRuntime = "devices/";
      pathRuntime.concat(deviceId);
      pathRuntime.concat("/Registration/uptime");
      if (Firebase.RTDB.setString(&fbdo, pathRuntime, runtimeStr))
      {
        safePrint("Runtime geüpload: ");
        safePrintln(runtimeStr);
      }
      else
      {
        safePrint("Fout bij uploaden runtime: ");
        safePrintln(fbdo.errorReason());
      }
    }
    vTaskDelay(updateInterval / portTICK_PERIOD_MS);
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
    safePrintln((WiFi.status() == WL_CONNECTED) ? "Verbonden" : "Niet verbonden");
    safePrint("Firebase status: ");
    safePrintln(Firebase.ready() ? "Verbonden" : "Niet verbonden!");
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

void gpioConfig()
{
  pinMode(LED_PIN, OUTPUT);
  for (int i = 0; i < NUM_RELAYS; i++)
  {
    pinMode(RELAY_PINS[i], OUTPUT);
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
}

String getUniqueClientId()
{
  uint64_t mac = ESP.getEfuseMac();
  char macStr[13];
  snprintf(macStr, sizeof(macStr), "%012llX", mac);
  // return String("ESP32S3_") + String(macStr);
  return String(macStr);
}

String HuidigeTijd()
{
  time_t now = time(nullptr);
  char timeStr[32];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
  return String(timeStr);
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

void updateFirebaseInstant(String path, String data)
{
  String destination = "devices/";
  destination.concat(deviceId);
  destination.concat(path);
  if (Firebase.RTDB.setString(&fbdo, destination, data))
  {
    safePrint("volgende path is geupdated: ");
    safePrint(destination);
    safePrint(" -> ");
    safePrintln(data);
  }
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
