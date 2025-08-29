#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#define WIFI_SSID           "Van_Baelen"       // Pas aan naar jouw WiFi netwerk
#define WIFI_PASSWORD       "Rob88333"   // Pas aan naar jouw WiFi wachtwoord
#define WIFI_TIMEOUT_MS     30000                // 30 seconden timeout voor verbinding

#define API_KEY "AIzaSyBoYWaBsPkQ2llH4sqxL1lG7ooHrmRe-GY"
#define DATABASE_URL "https://pool-671d1-default-rtdb.europe-west1.firebasedatabase.app/"

// Firebase authenticatie via e-mail/wachtwoord
#define USER_EMAIL "vbtechnieken@gmail.com"
#define USER_PASSWORD "qwerty"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void streamCallback(FirebaseStream data) {
  Serial.print("[STREAM] Nieuwe waarde: ");
  Serial.println(data.stringData());
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) Serial.println("[STREAM] Timeout, probeer opnieuw...");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Verbinden met WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi verbonden!");

  // Tijd synchroniseren
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = 0;
  while (now < 100000) {
    delay(100);
    now = time(nullptr);
  }
  Serial.println("Tijd gesynchroniseerd.");


  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.RTDB.beginStream(&fbdo, "/firmware/latest_version")) {
    Firebase.RTDB.setStreamCallback(&fbdo, streamCallback, streamTimeoutCallback);
    Serial.println("Stream gestart!");
  } else {
    Serial.print("Stream start mislukt: ");
    Serial.println(fbdo.errorReason());
  }
}

void loop() {
  // Niets nodig, alles gaat via callbacks
  delay(1000);
}
