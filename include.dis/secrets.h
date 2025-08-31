#ifndef SECRETS_H
#define SECRETS_H

#include <Arduino.h>

// Firebase configuratie
// Verkrijg deze waarden van je Firebase project console
#define API_KEY "AIzaSyBoYWaBsPkQ2llH4sqxL1lG7ooHrmRe-GY"
#define DATABASE_URL "https://pool-671d1-default-rtdb.europe-west1.firebasedatabase.app/"
#define USER_EMAIL "vbtechnieken@gmail.com"
#define USER_PASSWORD "qwerty"

// Firebase Project ID (afgeleid van DATABASE_URL)
#define FIREBASE_PROJECT_ID "pool-671d1"

// WiFi Configuration
#define WIFI_SSID           "Van_Baelen"       // Pas aan naar jouw WiFi netwerk
#define WIFI_PASSWORD       "Rob88333"   // Pas aan naar jouw WiFi wachtwoord
#define WIFI_TIMEOUT_MS     30000                // 30 seconden timeout voor verbinding

#endif // SECRETS_H