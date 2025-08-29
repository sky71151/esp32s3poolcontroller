#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "Board.h"
#include "version.h"
#include "secrets.h"
#include "external_flash.h"
#include "ota.h"

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
TaskHandle_t stackMonitorHandle = nullptr;

TaskStackInfo taskStackInfos[] = {
    {"WiFiTask", &wifiHandle, WIFI_STACK},
    {"FirebaseTask", &firebaseHandle, FIREBASE_STACK},
    {"StatusTask", &statusHandle, STATUS_STACK},
    {"UpdateTask", &updateHandle, UPDATE_STACK},
    {"MainTask", &mainHandle, MAIN_STACK},
    {"StackMonitorTask", &stackMonitorHandle, STATUS_STACK},
};
const int numTasks = sizeof(taskStackInfos) / sizeof(taskStackInfos[0]);

FirebaseData fbdo;
FirebaseData fbdoStream;
FirebaseAuth auth;
FirebaseConfig config;

SemaphoreHandle_t serialMutex;

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 60000; // 1 minuut
String bootTimeStr = "";

bool bootTimeUploaded = false;
bool firebaseInitialized = false;
bool streamConnected = false;
String deviceId;
bool updateAvailable = false;

// Forward declarations
void safePrint(const String &msg);
void safePrintln(const String &msg);
//void gpioConfig();
//String getUniqueClientId();
//String readDipSwitches();
void updateFirebaseInstant(String path, String data);
void connectToWiFiTask(void *pvParameters);
void initFirebaseTask(void *pvParameters);
void systemStatusTask(void *pvParameters);
void updateTimeToFirebaseTask(void *pvParameters);
void mainTask(void *pvParameters);
void stackMonitorTask(void *pvParameters);
void updateBootCount();
void firmwareVersionCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
void streamCallback(FirebaseStream data);


#endif
