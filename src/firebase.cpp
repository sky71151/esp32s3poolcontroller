#include "firebaseT.h"
#include "serial_format.h"

bool streamRecieved = false;

void firebaseTask(void *pvParameters)
{
    if (!initFirebase())
    {
    safePrintln(formatLog("ERROR", "Firebase initialisatie mislukt!"));
        vTaskDelete(NULL);
        firebaseInitialized = false;
    }else
    {
    safePrintln(formatLog("SUCCESS", "Firebase initialisatie gelukt!"));
        firebaseInitialized = true;
    }

    int counter = 0;
    FirebaseMsg receivedMsg;

    for (;;)
    {
        if (Firebase.ready())
        {
            if (counter % 100 == 0) // Elke seconde
            {
                //safePrintln("Firebase is ready.");
                int i = (6000 - counter) / 100;
                safePrintln(formatLog("INFO", "Tijd tot volgende update: " + String(i) + " seconden"));
            }

            if (streamRecieved)
            {
                streamRecieved = false;
                safePrintln(formatLog("INFO", "Stream data ontvangen, inputs updaten..."));
                sendStringToFirebaseQueue("/devices/" + device.Id + String(PATH_REGISTRATION_LASTSTREAM), device.getTime());
                //Firebase.RTDB.setString(&fbdo, "/devices/" + device.Id + "/Registration/laststreamreceived", device.getTime());
            }

            if(xQueueReceive(firebaseQueue, &receivedMsg, 0) == pdTRUE)
            {
                safePrintln(formatLog("INFO", "Bericht ontvangen van Firebase Queue."));
                if (receivedMsg.type == ActionType::SEND_STRING)
                {

                    if(Firebase.RTDB.setString(&fbdo, receivedMsg.path, receivedMsg.data))
                    {
                        safePrintln(formatLog("SUCCESS", "String succesvol verzonden van Queue naar Firebase."));
                    }
                    else
                    {
                        safePrintln(formatLog("ERROR", "Fout bij het verzenden van Queue naar Firebase."));
                    }
                }
                else if (receivedMsg.type == ActionType::SEND_JSON)
                {
                    safePrintln(formatLog("INFO", "Verwerk JSON bericht."));
                    // Verwerk het JSON bericht
                }
            }
        }
        else
        {
            safePrintln(formatLog("ERROR", "Firebase is not ready!"));
            safePrintln(formatLog("ERROR", fbdo.errorReason()));
            
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

bool initFirebase()
{
    safePrintln(formatLog("INFO", "Initialiseer Firebase config..."));
    config.database_url = DATABASE_URL;
    config.api_key = API_KEY;

    if (Firebase.signUp(&config, &auth, "", ""))
    {
    safePrintln(formatLog("SUCCESS", "Firebase aanmelding gelukt!"));
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
    }
    else
    {
    safePrintln(formatLog("ERROR", "Firebase aanmelding mislukt!"));
    safePrintln(formatLog("ERROR", fbdo.errorReason()));
        return false;
    }

    if (Firebase.ready())
    {
    safePrintln(formatLog("SUCCESS", "Firebase is klaar voor gebruik."));
        delay(500);
        checkDeviceExists();
        delay(500);
        setupStreamInputs();

    }
    else
    {
    safePrintln(formatLog("ERROR", "Firebase is niet klaar!"));
    safePrintln(formatLog("ERROR", fbdo.errorReason()));
        return false;
    }

    return true;
}

void streamCallback(FirebaseStream data)
{
    // Firebase.RTDB.setString(&fbdo, "/devices/" + device.Id + "/laststream", data.stringData());
    safePrintln(formatLog("STREAM", "Stream update:"));
    safePrintln(formatLog("STREAM", data.dataPath()));
    safePrintln(formatJson(data.stringData()));

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, data.stringData());
    if (error)
    {
    safePrintln(formatLog("ERROR", String("deserializeJson() failed: ") + error.f_str()));
        return;
    }
    else
    {
        if (doc.containsKey("DigitalInputs"))
        {
            String inputValues = doc["DigitalInputs"];
            device.updateInputValues();
            safePrintln(formatLog("DATA", "Nieuwe digitale inputs: " + inputValues));
        }
        if (doc.containsKey("Relays"))
        {
            String relayValues = doc["Relays"];
            device.setRelays(relayValues);
            safePrintln(formatLog("DATA", "Nieuwe relais waarden: " + relayValues));
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
            safePrintln(formatLog("DATA", "Nieuwe LED waarden: " + ledValues));
        }
    }
    streamRecieved = true;
}

void streamTimeout(bool timeout)
{
    if (timeout)
    {
    safePrintln(formatLog("WARN", "Stream timeout, reconnecting..."));
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
            safePrintln(formatLog("INFO", msg));
        }
        String pathTime = idPath;
        pathTime.concat(String(PATH_REGISTRATION_LASTBOOT));
        String lastSeenPath = idPath;
        lastSeenPath.concat(String(PATH_REGISTRATION_LASTSEEN));

        sendStringToFirebaseQueue(pathTime, timeStr);
        sendStringToFirebaseQueue(lastSeenPath, timeStr);

        /*if (Firebase.RTDB.setString(&fbdo, pathTime, timeStr))
        {
            safePrintln(formatLog("INFO", String("Boot Time update: ") + timeStr));
        }
        else
        {
            safePrintln(formatLog("ERROR", String("Fout bij uploaden boot tijd: ") + fbdo.errorReason()));
        }*/
        String pathFirmware = idPath;
        pathFirmware.concat("/DeviceInfo/firmware");
        String firmwareVersion = FIRMWARE_VERSION;
        firmwareVersion.concat(" ");
        firmwareVersion.concat(timeStr);
        sendStringToFirebaseQueue(pathFirmware, firmwareVersion);
        /*if (Firebase.RTDB.setString(&fbdo, pathFirmware.c_str(), firmwareVersion))
        {
            safePrintln(formatLog("INFO", String("Firmware version update: ") + firmwareVersion));
        }
        else
        {
            safePrintln(formatLog("ERROR", String("Fout bij uploaden firmware versie: ") + fbdo.errorReason()));
        }*/
    }
    else
    {

        String msg = "Pad bestaat niet: ";
        msg.concat(idPath);
    safePrintln(formatLog("WARN", msg));

        msg = "Device wordt geregistreerd: ";
        msg.concat(device.Id);
    safePrintln(formatLog("INFO", msg));

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
            safePrintln(formatLog("SUCCESS", "Device geregistreerd in Firebase Realtime Database."));
        }
        else
        {
            safePrintln(formatLog("ERROR", String("Fout bij registreren device: ") + fbdo.errorReason()));
        }
    }
}

void firmwareCallback(FirebaseStream data)
{
    safePrintln(formatLog("STREAM", "Firmware stream update:"));
    safePrintln(formatLog("STREAM", data.dataPath()));
    safePrintln(formatJson(data.stringData()));

    if (data.stringData() > FIRMWARE_VERSION)
    {
    safePrintln(formatLog("INFO", "Nieuwe firmware versie beschikbaar: " + data.stringData()));
        performOTA();
    }
    else
    {
    safePrintln(formatLog("INFO", "Firmware is up-to-date."));
    }
}

void firmwareTimeout(bool timeout)
{
    if (timeout)
    {
    safePrintln(formatLog("WARN", "Firmware stream timeout, reconnecting..."));
    }
}

void setupStreamInputs()
{
    safePrintln(formatLog("INFO", "Start stream op"));
    if (!Firebase.RTDB.beginStream(&Stream, "/devices/" + device.Id + "/GPIO"))
    {
    safePrintln(formatLog("ERROR", "Stream setup failed!"));
    safePrintln(formatLog("ERROR", fbdo.errorReason()));
    }
    else
    {
    safePrintln(formatLog("SUCCESS", "Stream setup gelukt!"));
    }
    Firebase.RTDB.setStreamCallback(&Stream, streamCallback, streamTimeout);

    safePrintln(formatLog("INFO", "start firmwarestream"));
    if (!Firebase.RTDB.beginStream(&FirmwareStream, "/firmware/latest_version/"))
    {
    safePrintln(formatLog("ERROR", "Firmware stream setup failed!"));
    safePrintln(formatLog("ERROR", fbdo.errorReason()));
    }
    else
    {
    safePrintln(formatLog("SUCCESS", "Firmware stream setup gelukt!"));
    }
    Firebase.RTDB.setStreamCallback(&FirmwareStream, firmwareCallback, firmwareTimeout);
}

void updateFirebase()
{
    if (!Firebase.ready())
    {
        safePrintln(formatLog("ERROR", "Firebase niet klaar voor update!"));
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
    safePrintln(formatLog("INFO", String("Tijd geüpload: ") + timeStr));
    safePrintln(formatLog("INFO", String("Vrije heap: ") + String(ESP.getFreeHeap()) + " bytes"));
    }
    else
    {
    safePrintln(formatLog("ERROR", String("Fout bij uploaden tijd: ") + fbdo.errorReason()));
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
    safePrintln(formatLog("INFO", String("Runtime geüpload: ") + runtimeStr));
    }
    else
    {
    safePrintln(formatLog("ERROR", String("Fout bij uploaden runtime: ") + fbdo.errorReason()));
    }
}
void sendStringToFirebaseQueue(const String& path, const String& value) {
    FirebaseMsg msg;

    msg.type = SEND_STRING;
    // De cruciale stap: Kopieer de data
    strncpy(msg.path, path.c_str(), sizeof(msg.path) - 1);
    msg.path[sizeof(msg.path) - 1] = '\0'; 

    strncpy(msg.data, value.c_str(), sizeof(msg.data) - 1);
    msg.data[sizeof(msg.data) - 1] = '\0';

    // De structuur wordt nu veilig naar de wachtrij gekopieerd
    if (xQueueSend(firebaseQueue, (void*)&msg, 0) != pdPASS) {
        Serial.println("Wachtrij vol!");
    }
}