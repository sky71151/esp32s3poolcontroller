#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "ota.h"

// Zet dit op je eigen GitHub release-asset URL
const char* ota_url = "https://github.com/sky71151/esp32s3poolcontroller/releases/download/latest/firmware.bin";

void performOTA() {
  WiFiClientSecure client;
  client.setInsecure(); // Voor testdoeleinden, accepteert elk certificaat (voor productie: gebruik root CA!)

  Serial.println("Start OTA update vanaf GitHub...");
  t_httpUpdate_return ret = httpUpdate.update(client, ota_url);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("OTA update mislukt: (%d) %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("Geen nieuwe update beschikbaar.");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("OTA update geslaagd! Herstart...");
      break;
  }
}