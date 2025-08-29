#ifndef FIREBASE_H
#define FIREBASE_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>

extern FirebaseData fbdo;
extern FirebaseData fbdoStream;
extern FirebaseAuth auth;
extern FirebaseConfig config;
extern bool firebaseInitialized;
extern bool streamConnected;
extern String deviceId;

void initFirebase();
void handleFirebaseTasks();
void updateFirebaseInstant(String path, String data);
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);

#endif // FIREBASE_H
