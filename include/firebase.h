#ifndef FIREBASE_H
#define FIREBASE_H

#include <Firebase_ESP_Client.h>
#include "main.h"
#include "version.h"
#include "secrets.h"

void initFirebaseTask(void *pvParameters);
void updateTimeToFirebaseTask(void *pvParameters);
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
void updateFirebaseInstant(String path, String data);
void FirebaseInputTask(void *pvParameters);
void streamCallbackinput(FirebaseStream data);
void streamTimeoutCallbackinput(bool timeout);

#endif
