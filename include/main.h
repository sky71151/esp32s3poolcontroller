#ifndef MAIN_H
#define MAIN_H

// Arduino libraries
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>

//eigen includes
#include "serial.h"
#include "wifiTask.h"
#include "board.h"
#include "secrets.h"
#include "firebaseFunctions.h"
#include "version.h"
#include "ota.h"
#include "Update.h"



#define WIFI_STACK 8192
#define FIREBASE_STACK 16384
#define STATUS_STACK 16384
#define UPDATE_STACK 16384
#define MAIN_STACK 16384
#define UPDATE_FIREBASE_STACK 16384
#define FIREBASE_STREAM_STACK 8000

#define DEBUG 1

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
extern TaskHandle_t updateFirebaseTaskHandle;
extern TaskHandle_t FirebaseStreamTaskHandle;

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
extern bool signupOK;
extern bool firmwareStreamConnected;
extern bool inputStreamConnected; 
extern bool streamReceived;

extern unsigned long lastFirmwareConnectAttempt;
extern const unsigned long firmwareConnectInterval;
extern unsigned long lastInputConnectAttempt;
extern const unsigned long inputConnectInterval;

extern bool debugMode;
extern time_t timeNow;

extern Board device;

#endif