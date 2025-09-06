#ifndef FIREBASE_H
#define FIREBASE_H

#include "main.h"

void firebaseTask(void *pvParameters);
bool initFirebase();
void streamCallback(FirebaseStream data);
void streamTimeout(bool timeout);
void checkDeviceExists();
void firmwareCallback(FirebaseStream data);
void firmwareTimeout(bool timeout);
void setupStreamInputs();
void updateFirebase();
void sendStringToFirebaseQueue(const String& path, const String& value);


#endif
