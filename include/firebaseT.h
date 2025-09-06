#ifndef FIREBASE_H
#define FIREBASE_H

#include "main.h"

void firebaseTask(void *pvParameters);
void initFirebase();
void streamCallback(FirebaseStream data);
void streamTimeout(bool timeout);
void checkDeviceExists();
void firmwareCallback(FirebaseStream data);
void firmwareTimeout(bool timeout);
void setupStreamInputs();
void updateFirebase();


#endif
