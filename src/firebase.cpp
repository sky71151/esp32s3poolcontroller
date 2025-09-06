#include "firebaseT.h"

bool streamRecieved = false;

void firebaseTask(void *pvParameters)
{
    initFirebase();
    int counter = 0;

    for (;;)
    {
        if (Firebase.ready())
        {
            if (counter % 100 == 0) // Elke seconde
            {
                //safePrintln("Firebase is ready.");
                int i = (6000 - counter) / 100;
                safePrintln("Tijd tot volgende update: " + String(i) + " seconden");
            }

            if (streamRecieved)
            {
                streamRecieved = false;
                safePrintln("Stream data ontvangen, inputs updaten...");
                Firebase.RTDB.setString(&fbdo, "/devices/" + device.Id + "/Registration/laststreamreceived", device.getTime());
            }
        }
        else
        {
            safePrintln("Firebase is not ready!");
            safePrintln(fbdo.errorReason());
        }

        if (counter >= 6000) // Elke 60 iteraties (ongeveer elk uur)
        {
            counter = 0;
            updateFirebase();
        }

        counter++;

        vTaskDelay(pdMS_TO_TICKS(10)); // Wacht 10 milliseconden
    }
}

void initFirebase()
{
    safePrintln("Initialiseer Firebase config...");
    config.database_url = DATABASE_URL;
    config.api_key = API_KEY;

    if (Firebase.signUp(&config, &auth, "", ""))
    {
        safePrintln("Firebase aanmelding gelukt!");
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
    }
    else
    {
        safePrintln("Firebase aanmelding mislukt!");
        safePrintln(fbdo.errorReason());
    }

    if (Firebase.ready())
    {
        safePrintln("Firebase is klaar voor gebruik.");
        delay(500);
        checkDeviceExists();
        delay(500);
        setupStreamInputs();
    }
    else
    {
        safePrintln("Firebase is niet klaar!");
        safePrintln(fbdo.errorReason());
    }
}

void streamCallback(FirebaseStream data)
{
    // Firebase.RTDB.setString(&fbdo, "/devices/" + device.Id + "/laststream", data.stringData());
    safePrintln("Stream update:");
    safePrintln(data.dataPath());
    safePrintln(data.stringData());

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, data.stringData());
    if (error)
    {
        safePrint(F("deserializeJson() failed: "));
        safePrintln(error.f_str());
        return;
    }
    else
    {
        if (doc.containsKey("DigitalInputs"))
        {
            String inputValues = doc["DigitalInputs"];
            device.updateInputValues();
            safePrintln("Nieuwe digitale inputs: " + inputValues);
        }
        if (doc.containsKey("Relays"))
        {
            String relayValues = doc["Relays"];
            device.setRelays(relayValues);
            safePrintln("Nieuwe relais waarden: " + relayValues);
        }
        if (doc.containsKey("Led"))
        {
            String ledValues = doc["Led"];

            if (ledValues == "0")
            {
                digitalWrite(LED_PIN, LOW); // LED aan
            }
            else
            {
                digitalWrite(LED_PIN, HIGH); // LED uit
            }
            safePrintln("Nieuwe LED waarden: " + ledValues);
        }
    }
    streamRecieved = true;
}

void streamTimeout(bool timeout)
{
    if (timeout)
    {
        safePrintln("Stream timeout, reconnecting...");
    }
}

void checkDeviceExists()
{
    timeNow = time(nullptr);
    bootTime = timeNow;
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&timeNow));
    String idPath = "devices/";
    idPath.concat(device.Id);
    String lastBootPath = idPath;
    lastBootPath.concat("/lastBoot");
    String firmwarePath = idPath;
    firmwarePath.concat("/DeviceInfo/firmware");
    // Voorbeeld: controleer of device geregistreerd is met unieke ID
    if (Firebase.RTDB.pathExisted(&fbdo, idPath.c_str()))
    {
        {
            String msg = "Pad bestaat: ";
            msg.concat(idPath);
#if DEBUG
            safePrintln(msg);
#endif
        }
        String pathTime = idPath;
        pathTime.concat("/Registration/lastBoot");
        if (Firebase.RTDB.setString(&fbdo, pathTime, timeStr))
        {
#if DEBUG
            safePrint("Boot Time update: ");
            safePrintln(timeStr);
#endif
        }
        else
        {
#if DEBUG
            safePrint("Fout bij uploaden boot tijd: ");
            safePrintln(fbdo.errorReason());
#endif
        }
        String pathFirmware = idPath;
        pathFirmware.concat("/DeviceInfo/firmware");
        String firmwareVersion = FIRMWARE_VERSION;
        firmwareVersion.concat(" ");
        firmwareVersion.concat(timeStr);
        if (Firebase.RTDB.setString(&fbdo, pathFirmware.c_str(), firmwareVersion))
        {
#if DEBUG
            safePrint("Firmware version update: ");
            safePrintln(firmwareVersion);
#endif
        }
        else
        {
#if DEBUG
            safePrint("Fout bij uploaden firmware versie: ");
            safePrintln(fbdo.errorReason());
#endif
        }
    }
    else
    {

        String msg = "Pad bestaat niet: ";
        msg.concat(idPath);
#if DEBUG
        safePrintln(msg);
#endif

        msg = "Device wordt geregistreerd: ";
        msg.concat(device.Id);
#if DEBUG
        safePrintln(msg);
#endif

        // Create device data JSON
        FirebaseJson deviceJson;

        // Device info section
        FirebaseJson DeviceInfo;
        DeviceInfo.set("clientId", device.Id);
        DeviceInfo.set("boardType", "ESP32-S3  R1N16");
        DeviceInfo.set("firmware", FIRMWARE_VERSION);
        deviceJson.set("DeviceInfo", DeviceInfo);

        // Registration section
        FirebaseJson Registration;
        Registration.set("firstRegistratione", bootTime);
        Registration.set("lastBoot", bootTime);
        Registration.set("lastSeen", bootTime);
        Registration.set("uptime", "0");
        deviceJson.set("Registration", Registration);

        // GPIO section
        FirebaseJson Device;
        Device.set("Relays", "0,0,0,0,0,0,0,0");
        Device.set("DipSwitches", "0,0,0,0");
        Device.set("DigitalInputs", "0,0,0,0");
        Device.set("AnalogInputs", "0,0,0,0");
        deviceJson.set("GPIO", Device);

        if (Firebase.RTDB.setJSON(&fbdo, idPath.c_str(), &deviceJson))
        {
#if DEBUG
            safePrintln("Device geregistreerd in Firebase Realtime Database.");
#endif
        }
        else
        {
#if DEBUG
            safePrint("Fout bij registreren device: ");
            safePrintln(fbdo.errorReason());
#endif
        }
    }
}

void firmwareCallback(FirebaseStream data)
{
    safePrintln("Firmware stream update:");
    safePrintln(data.dataPath());
    safePrintln(data.stringData());

    if (data.stringData() > FIRMWARE_VERSION)
    {
        safePrintln("Nieuwe firmware versie beschikbaar: " + data.stringData());
        performOTA();
    }
    else
    {
        safePrintln("Firmware is up-to-date.");
    }
}

void firmwareTimeout(bool timeout)
{
    if (timeout)
    {
        safePrintln("Firmware stream timeout, reconnecting...");
    }
}

void setupStreamInputs()
{
    Serial.println("Start stream op");
    if (!Firebase.RTDB.beginStream(&Stream, "/devices/" + device.Id + "/GPIO"))
    {
        Serial.println("Stream setup failed!");
        Serial.println(fbdo.errorReason());
    }
    else
    {
        Serial.println("Stream setup gelukt!");
    }
    Firebase.RTDB.setStreamCallback(&Stream, streamCallback, streamTimeout);

    Serial.println("start firmwarestream");
    if (!Firebase.RTDB.beginStream(&FirmwareStream, "/firmware/latest_version/"))
    {
        Serial.println("Firmware stream setup failed!");
        Serial.println(fbdo.errorReason());
    }
    else
    {
        Serial.println("Firmware stream setup gelukt!");
    }
    Firebase.RTDB.setStreamCallback(&FirmwareStream, firmwareCallback, firmwareTimeout);
}

void updateFirebase()
{
    if (!Firebase.ready())
    {
        safePrintln("Firebase niet klaar voor update!");
        return;
    }
    timeNow = time(nullptr);
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&timeNow));
    String pathTime = "devices/";
    pathTime.concat(device.Id);
    pathTime.concat("/Registration/lastSeen");
    if (Firebase.RTDB.setString(&fbdo, pathTime, timeStr))
    {
#if DEBUG
        safePrint("Tijd geüpload: ");
        safePrintln(timeStr);
        // Log de hoeveelheid vrij geheugen
        safePrint("Vrije heap: ");
        safePrint(String(ESP.getFreeHeap()));

        safePrintln(" bytes");
#endif
    }
    else
    {
#if DEBUG
        safePrint("Fout bij uploaden tijd: ");
        safePrintln(fbdo.errorReason());
#endif
    }
    // unsigned long runtimeMillis = millis();
    // unsigned long totalMinutes = runtimeMillis / 60000;
    // unsigned int hours = totalMinutes / 60;
    // unsigned int minutes = totalMinutes % 60;

    // snprintf(runtimeStr, sizeof(runtimeStr), "%02u:%02u", hours, minutes);
    char runtimeStr[16];
    time_t uptime = timeNow - bootTime;
    unsigned int hours = uptime / 3600;
    unsigned int minutes = (uptime % 3600) / 60;
    snprintf(runtimeStr, sizeof(runtimeStr), "%02u:%02u", hours, minutes);
    String pathRuntime = "devices/";
    pathRuntime.concat(device.Id);
    pathRuntime.concat("/Registration/uptime");
    if (Firebase.RTDB.setString(&fbdo, pathRuntime, runtimeStr))
    {
#if DEBUG
        safePrint("Runtime geüpload: ");
        safePrintln(runtimeStr);
#endif
    }
    else
    {
#if DEBUG
        safePrint("Fout bij uploaden runtime: ");
        safePrintln(fbdo.errorReason());
#endif
    }
}