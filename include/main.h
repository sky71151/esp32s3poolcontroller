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
#include "wifitask.h"
#include "firebase.h"

typedef struct
{
  const char *name;
  TaskHandle_t *handle;
  uint32_t stackWords;
} TaskStackInfo;

extern TaskHandle_t wifiHandle;
extern TaskHandle_t firebaseHandle;
extern TaskHandle_t statusHandle;
extern TaskHandle_t updateHandle;
extern TaskHandle_t mainHandle;
extern TaskHandle_t stackMonitorHandle;
extern TaskStackInfo taskStackInfos[];
extern FirebaseData fbdo;
extern FirebaseData fbdoStream;
extern FirebaseAuth auth;
extern FirebaseConfig config;
extern SemaphoreHandle_t serialMutex;
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

// Forward declarations
void safePrint(const String &msg);
void safePrintln(const String &msg);
//void gpioConfig();
//String getUniqueClientId();
//String readDipSwitches();
//void updateFirebaseInstant(String path, String data);
//void connectToWiFiTask(void *pvParameters);
//void initFirebaseTask(void *pvParameters);
void systemStatusTask(void *pvParameters);
//void updateTimeToFirebaseTask(void *pvParameters);
void mainTask(void *pvParameters);
void stackMonitorTask(void *pvParameters);
//void updateBootCount();
//void firmwareVersionCallback(FirebaseStream data);
//void streamTimeoutCallback(bool timeout);
//void streamCallback(FirebaseStream data);


#endif
