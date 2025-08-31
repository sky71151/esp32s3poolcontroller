#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "version.h"
#include "secrets.h"
#include <Firebase_ESP_Client.h>

typedef struct {
  const char *name;
  TaskHandle_t *handle;
  uint32_t stackWords;
} TaskStackInfo;

// Globale variabelen (alleen extern!)
extern TaskHandle_t wifiHandle;
extern TaskHandle_t firebaseHandle;
extern TaskHandle_t statusHandle;
extern TaskHandle_t updateHandle;
extern TaskHandle_t mainHandle;
extern TaskHandle_t stackMonitorHandle;
extern TaskHandle_t FirebaseInputTaskHandle;
extern TaskStackInfo taskStackInfos[];
extern const int numTasks;
extern FirebaseData fbdo;
extern FirebaseData fbdoStream;
extern FirebaseData fbdoInput;
extern FirebaseAuth auth;
extern FirebaseConfig config;
extern SemaphoreHandle_t serialMutex;
extern QueueHandle_t FirebaseInputQueue;
extern unsigned long lastUpdate;
extern const unsigned long updateInterval;
extern String bootTimeStr;
extern bool bootTimeUploaded;
extern bool firebaseInitialized;
extern bool streamConnected;
extern String deviceId;
extern bool updateAvailable;
extern uint32_t bootCount;
extern bool flashReady;

// Alleen forward declarations die je in andere modules gebruikt
void safePrint(const String &msg);
void safePrintln(const String &msg);
void systemStatusTask(void *pvParameters);
void mainTask(void *pvParameters);
void stackMonitorTask(void *pvParameters);

#endif
