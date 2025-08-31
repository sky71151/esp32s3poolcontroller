#ifndef FIREBASE_FUNCTIONS_H
#define FIREBASE_FUNCTIONS_H

#include "main.h"
void initFirebase();
void updateFirebaseTask(void *pvParameters);
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
void streamCallbackinput(FirebaseStream data);
void streamTimeoutCallbackinput(bool timeout);

#endif // FIREBASE_FUNCTIONS_H