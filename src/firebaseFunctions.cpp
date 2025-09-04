#include "firebaseFunctions.h"

// firebase paths
unsigned long lastStreamResetTime = 0;
const unsigned long streamResetInterval = 30 * 60 * 1000; // 30 minuten

void initFirebase()
{
    safePrint("Firebase Client v");
    safePrintln(FIREBASE_CLIENT_VERSION);
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    // Verhoog de SSL-buffergrootte om SSL-fouten te voorkomen
    fbdo.setBSSLBufferSize(32768, 4096);
    fbdoStream.setBSSLBufferSize(32768, 4096);
    fbdoInput.setBSSLBufferSize(32768, 4096);
    safePrint("Sign up new user... ");
    if (Firebase.signUp(&config, &auth, "", ""))
    {
        safePrintln("ok");
        signupOK = true;
    }
    else
        safePrintln(config.signer.signupError.message.c_str());

    // config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
    Firebase.begin(&config, &auth);
    String idPath = "devices/";
    idPath.concat(device.Id);
    String lastBootPath = idPath;
    lastBootPath.concat("/lastBoot");
    String firmwarePath = idPath;
    firmwarePath.concat("/DeviceInfo/firmware");

    if (Firebase.ready() && signupOK && !firebaseInitialized)
    {

        // check if device is already registered
        if (WiFi.status() == WL_CONNECTED && Firebase.ready() && !firebaseInitialized)
        {
            // Voorbeeld: controleer of device geregistreerd is met unieke ID
            if (Firebase.RTDB.pathExisted(&fbdo, idPath.c_str()))
            {
                {
                    String msg = "Pad bestaat: ";
                    msg.concat(idPath);
                    safePrintln(msg);
                }
                time_t now = time(nullptr);
                char timeStr[32];
                strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
                String pathTime = idPath;
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
                String pathFirmware = idPath;
                pathFirmware.concat("/DeviceInfo/firmware");
                String firmwareVersion = FIRMWARE_VERSION;
                firmwareVersion.concat(" ");
                firmwareVersion.concat(timeStr);
                if (Firebase.RTDB.setString(&fbdo, pathFirmware.c_str(), firmwareVersion))
                {
                    safePrint("Firmware version update: ");
                    safePrintln(firmwareVersion);
                    firebaseInitialized = true;
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                }
                else
                {
                    safePrint("Fout bij uploaden firmware versie: ");
                    safePrintln(fbdo.errorReason());
                }
            }
            else
            {

                String msg = "Pad bestaat niet: ";
                msg.concat(idPath);
                safePrintln(msg);

                msg = "Device wordt geregistreerd: ";
                msg.concat(device.Id);
                safePrintln(msg);

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

                if (Firebase.RTDB.setJSON(&fbdo, idPath.c_str(), &deviceJson))
                {
                    safePrintln("Device geregistreerd in Firebase Realtime Database.");
                    firebaseInitialized = true;
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                }
                else
                {
                    safePrint("Fout bij registreren device: ");
                    safePrintln(fbdo.errorReason());
                }
            }
        }
        else
        {
            safePrintln("Firebase is not ready");
        }
    }
    if (Firebase.ready() && signupOK && firebaseInitialized)
    {
        safePrintln("Firebase is ready and initialized");
        safePrintln("setting up Firebase RTDB stream");
        // connectFirmwareStream();
        // ConnectInputStream();
    }
}

void ConnectInputStream()
{
    if (fbdoInput.httpConnected())
    {
        Firebase.RTDB.endStream(&fbdoInput);
    }
    String StreamInputPath = "/devices/";
    StreamInputPath.concat(device.Id);
    StreamInputPath.concat("/GPIO");
    if (Firebase.RTDB.beginStream(&fbdoInput, StreamInputPath.c_str()))
    {
        Firebase.RTDB.setStreamCallback(&fbdoInput, streamCallbackinput, streamTimeoutCallbackinput);
        safePrintln("Input stream gestart!");
        inputStreamConnected = true;
    }
    else
    {
        safePrint("Stream start mislukt: ");
        safePrintln(fbdoInput.errorReason());
        inputStreamConnected = false;
        fbdoInput.clear(); // Reset de interne status
    }
}

void connectFirmwareStream()
{
    if (fbdoStream.httpConnected())
    {
        Firebase.RTDB.endStream(&fbdoStream);
    }

    if (Firebase.RTDB.beginStream(&fbdoStream, "/firmware/latest_version"))
    {
        Firebase.RTDB.setStreamCallback(&fbdoStream, streamCallback, streamTimeoutCallback);
        safePrintln("Stream gestart!");
        firmwareStreamConnected = true;
    }
    else
    {
        firmwareStreamConnected = false;
        safePrint("Stream start mislukt: ");
        safePrintln(fbdoStream.errorReason());
        fbdoStream.clear(); // Reset de interne status
    }
}

void manageFirebaseStreams()
{
    if (WiFi.status() == WL_CONNECTED && Firebase.ready() && firebaseInitialized)
    {
        unsigned long currentMillis = millis();

        // Check voor periodieke herstart van de streams
        if (currentMillis - lastStreamResetTime >= streamResetInterval)
        {
            safePrintln("[STREAM] Periodieke reset van alle streams...");
            Firebase.RTDB.endStream(&fbdoStream);
            Firebase.RTDB.endStream(&fbdoInput);
            firmwareStreamConnected = false;
            inputStreamConnected = false;
            lastStreamResetTime = currentMillis;
        }

        // Firmware stream
        if (!firmwareStreamConnected)
        {
            if (currentMillis - lastFirmwareConnectAttempt >= firmwareConnectInterval)
            {
                lastFirmwareConnectAttempt = currentMillis;
                safePrintln("[STREAM] Probeer firmware stream opnieuw te verbinden...");
                connectFirmwareStream();
            }
        }
        // Input stream
        if (!inputStreamConnected)
        {
            if (currentMillis - lastInputConnectAttempt >= inputConnectInterval)
            {
                lastInputConnectAttempt = currentMillis;
                safePrintln("[STREAM] Probeer input stream opnieuw te verbinden...");
                ConnectInputStream();
            }
        }
    }
}

void updateFirebaseTask(void *pvParameters)
{
    while (true)
    {
        if (WiFi.status() == WL_CONNECTED && Firebase.ready() && firebaseInitialized)
        {
            time_t now = time(nullptr);
            char timeStr[32];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
            String pathTime = "devices/";
            pathTime.concat(device.Id);
            pathTime.concat("/Registration/lastSeen");
            if (Firebase.RTDB.setString(&fbdo, pathTime, timeStr))
            {
                safePrint("Tijd geüpload: ");
                safePrintln(timeStr);
                // Log de hoeveelheid vrij geheugen
                safePrint("Vrije heap: ");
                safePrint(String(ESP.getFreeHeap()));
                safePrintln(" bytes");
            }
            else
            {
                safePrint("Fout bij uploaden tijd: ");
                safePrintln(fbdo.errorReason());
            }
            unsigned long runtimeMillis = millis();
            unsigned long totalMinutes = runtimeMillis / 60000;
            unsigned int hours = totalMinutes / 60;
            unsigned int minutes = totalMinutes % 60;
            char runtimeStr[16];
            snprintf(runtimeStr, sizeof(runtimeStr), "%02u:%02u", hours, minutes);
            String pathRuntime = "devices/";
            pathRuntime.concat(device.Id);
            pathRuntime.concat("/Registration/uptime");
            if (Firebase.RTDB.setString(&fbdo, pathRuntime, runtimeStr))
            {
                safePrint("Runtime geüpload: ");
                safePrintln(runtimeStr);
            }
            else
            {
                safePrint("Fout bij uploaden runtime: ");
                safePrintln(fbdo.errorReason());
            }
        }
        vTaskDelay(updateInterval / portTICK_PERIOD_MS);
    }
}

void streamCallbackinput(FirebaseStream data)
{
    if (fbdoInput.errorCode() != 0 && fbdoInput.errorCode() != 200)
    {
        String Message = "[STREAM] Error: ";
        Message.concat(String(fbdoInput.errorCode()));
        safePrintln(Message);
        inputStreamConnected = false;
    }
    else
    {
        streamReceived = true;
        digitalWrite(LED_PIN, HIGH);
        char InputData[32];
        strncpy(InputData, data.stringData().c_str(), sizeof(InputData) - 1);
        InputData[sizeof(InputData) - 1] = '\0';
        safePrint("[STREAM] Nieuwe waarde: ");
        safePrintln(InputData);
        safePrint("data afkomstig van path : ");
        safePrintln(data.dataPath());
        digitalWrite(LED_PIN, LOW);
        if (data.dataPath() == "/Led")
        {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        }
        // xQueueSendToBackFromISR(FirebaseInputQueue, &InputData, 0);
        // vTaskNotifyGiveFromISR(FirebaseInputTaskHandle, NULL);
    }
}

void streamTimeoutCallbackinput(bool timeout)
{
    if (timeout)
    {
        safePrintln("[STREAM] Timeout, probeer opnieuw...");
        Firebase.RTDB.endStream(&fbdoInput);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        inputStreamConnected = false;
    }
}

void streamCallback(FirebaseStream data)
{
    if (fbdoStream.errorCode() != 0 && fbdoStream.errorCode() != 200)
    {
        String Message = "[STREAM] Error: ";
        Message.concat(String(fbdoStream.errorCode()));
        safePrintln(Message);
        firmwareStreamConnected = false;
    }
    else
    {
        streamReceived = true;
        safePrint("[STREAM] Nieuwe waarde: ");
        safePrintln(data.stringData());
        // convert strindata to double
        if (atof(data.stringData().c_str()) > atof(FIRMWARE_VERSION))
        {
            safePrintln("[STREAM] Nieuwe firmware versie gedetecteerd, start OTA...!!");
            updateAvailable = true;
            digitalWrite(LED_PIN, HIGH);
        }
    }
}

void streamTimeoutCallback(bool timeout)
{
    if (timeout)
    {
        safePrintln("[STREAM] Timeout, probeer opnieuw...");
        Firebase.RTDB.endStream(&fbdoStream);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        firmwareStreamConnected = false;
    }
}
