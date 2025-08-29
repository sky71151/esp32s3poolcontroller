#ifndef FIREBASE_H
#define FIREBASE_H

#include <Firebase_ESP_Client.h>


void initFirebaseTask(void *pvParameters);
void updateTimeToFirebaseTask(void *pvParameters);
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
void updateFirebaseInstant(String path, String data);

#endif
