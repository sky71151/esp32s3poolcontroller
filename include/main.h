#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include "Board.h"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "secrets.h"
#include "serial.h"
#include "firebaseT.h"
#include "version.h"
#include "ota.h"
#include "serial_format.h"

#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include "Update.h"
#include <ArduinoJson.h>
#include "Preferences.h"

#define DEBUG 1

extern FirebaseData fbdo;
extern FirebaseData Stream;
extern FirebaseData FirmwareStream;
extern FirebaseAuth auth;
extern FirebaseConfig config;

extern TaskHandle_t firebaseTaskHandle;

extern time_t timeNow;
extern time_t bootTime;

extern Board device;
extern SemaphoreHandle_t serialMutex;
extern bool firebaseInitialized;

extern bool streamRecieved;
extern Preferences preferences;
void mainTask(void *pvParameters);

enum ActionType {
  SEND_STRING,
  SEND_JSON
};

typedef struct {
  ActionType type;
  char path[128];
  char data[256]; // Used if type is SEND_STRING
  char json[512];  // Used if type is SEND_JSON
} FirebaseMsg;

extern FirebaseMsg firebaseMsg;
extern QueueHandle_t firebaseQueue;





#endif