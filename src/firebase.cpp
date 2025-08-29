#include "firebase.h"

void initFirebaseTask(void *pvParameters)
{
  String regPath = String("/devices/");
  regPath += deviceId;

  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED && !Firebase.ready())
    {
      // Firebase.reset(&config);
      config.api_key = API_KEY;
      config.database_url = DATABASE_URL;
      // Anonieme login: geen e-mail/wachtwoord invullen
      // auth.user.email en auth.user.password NIET instellen
      auth.user.email = USER_EMAIL;
      auth.user.password = USER_PASSWORD;
      Firebase.begin(&config, &auth); // auth leeg laat anonieme login toe
      Firebase.reconnectWiFi(true);

      safePrintln("Firebase opnieuw geïnitialiseerd (anoniem)");
      firebaseInitialized = false; // reset status bij herinitialisatie
      streamConnected = false;
    }

    if (WiFi.status() == WL_CONNECTED && Firebase.ready() && firebaseInitialized && !streamConnected)
    {
      if (Firebase.RTDB.beginStream(&fbdoStream, "/firmware/latest_version"))
      {
        Firebase.RTDB.setStreamCallback(&fbdoStream, streamCallback, streamTimeoutCallback);
        Serial.println("Stream gestart!");
        streamConnected = true;
      }
      else
      {
        Serial.print("Stream start mislukt: ");
        Serial.println(fbdoStream.errorReason());
      }
    }

    if (WiFi.status() == WL_CONNECTED && Firebase.ready() && !firebaseInitialized)
    {
      // Voorbeeld: controleer of device geregistreerd is met unieke ID
      if (Firebase.RTDB.pathExisted(&fbdo, regPath.c_str()))
      {
        {
          String msg = "Pad bestaat: ";
          msg.concat(regPath);
          safePrintln(msg);
        }
        time_t now = time(nullptr);
        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        String pathTime = "devices/";
        pathTime.concat(deviceId);
        pathTime.concat("/Registration/lastBoot");
        if (Firebase.RTDB.setString(&fbdo, pathTime, timeStr))
        {
          safePrint("Boot Time update: ");
          safePrintln(timeStr);
        }
        else
        {
          safePrint("Fout bij uploaden boot tijd: ");
          safePrintln(fbdo.errorReason());
        }
        String pathFirmware = "devices/";
        pathFirmware.concat(deviceId);
        pathFirmware.concat("/DeviceInfo/firmware");
        if (Firebase.RTDB.setString(&fbdo, pathFirmware.c_str(), String(FIRMWARE_VERSION) + String(" ") + String(timeStr)))
        {
          safePrint("Firmware version update: ");
          safePrintln(String(FIRMWARE_VERSION));
        }
        else
        {
          safePrint("Fout bij uploaden firmware versie: ");
          safePrintln(fbdo.errorReason());
        }
        firebaseInitialized = true;
      }
      else
      {

        String msg = "Pad bestaat niet: ";
        msg.concat(regPath);
        safePrintln(msg);

        msg = "Device wordt geregistreerd: ";
        msg.concat(deviceId);
        safePrintln(msg);

        // Create device data JSON
        FirebaseJson deviceJson;

        // Device info section
        FirebaseJson DeviceInfo;
        DeviceInfo.set("clientId", deviceId);
        DeviceInfo.set("boardType", "ESP32-S3  R1N16");
        DeviceInfo.set("firmware", FIRMWARE_VERSION);
        deviceJson.set("DeviceInfo", DeviceInfo);

        // Registration section
        FirebaseJson Registration;
        Registration.set("firstRegistratione", bootTimeStr);
        Registration.set("lastBoot", bootTimeStr);
        Registration.set("lastSeen", bootTimeStr);
        Registration.set("uptime", "0");
        deviceJson.set("Registration", Registration);

        // GPIO section
        FirebaseJson Device;
        Device.set("Relays", "0,0,0,0,0,0,0,0");
        Device.set("DipSwitches", "0,0,0,0");
        Device.set("DigitalInputs", "0,0,0,0");
        Device.set("AnalogInputs", "0,0,0,0");
        deviceJson.set("GPIO", Device);

        if (Firebase.RTDB.setJSON(&fbdo, regPath.c_str(), &deviceJson))
        {
          safePrintln("Device geregistreerd in Firebase Realtime Database.");
          firebaseInitialized = true;
        }
        else
        {
          safePrint("Fout bij registreren device: ");
          safePrintln(fbdo.errorReason());
        }
      }

      // firebaseInitialized = true;
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}