#ifndef FIREBASE_FUNCTIONS_H
#define FIREBASE_FUNCTIONS_H

#include "main.h"

extern unsigned long lastStreamResetTime;
extern const unsigned long streamResetInterval;

void initFirebase();

void ConnectInputStream();
void connectFirmwareStream();

void updateFirebaseTask(void *pvParameters);
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
void streamCallbackinput(FirebaseStream data);
void streamTimeoutCallbackinput(bool timeout);
void connectFirmwareStream();
void ConnectInputStream();
void manageFirebaseStreams();
void sendToFirebaseQueue(const String& path, const String& value, ActionType type)

#endif // FIREBASE_FUNCTIONS_H