#include "firebaseFunctions.h"

// firebase paths
unsigned long lastStreamResetTime = 0;
const unsigned long streamResetInterval = 30 * 60 * 1000; // 30 minuten

void initFirebase()
{
    #if DEBUG
    safePrint("Firebase Client v");
    safePrintln(FIREBASE_CLIENT_VERSION);
    #endif
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    // Verhoog de SSL-buffergrootte om SSL-fouten te voorkomen
    fbdo.setBSSLBufferSize(32768, 4096);
    fbdoStream.setBSSLBufferSize(32768, 4096);
    fbdoInput.setBSSLBufferSize(32768, 4096);
    #if DEBUG
    safePrint("Sign up new user... ");
    #endif
    if (Firebase.signUp(&config, &auth, "", ""))
    {
        #if DEBUG
        safePrintln("ok");
        #endif
        signupOK = true;
    }
    else{
        #if DEBUG
        safePrintln("Fout bij aanmaken gebruiker: ");
        safePrintln(config.signer.signupError.message.c_str());
        #endif
    }
        

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
                    #if DEBUG
                    safePrintln(msg);
                    #endif
                }
                time_t now = time(nullptr);
                char timeStr[32];
                strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
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
                    firebaseInitialized = true;
                    vTaskDelay(500 / portTICK_PERIOD_MS);
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
                    #if DEBUG
                    safePrintln("Device geregistreerd in Firebase Realtime Database.");
                    #endif
                    firebaseInitialized = true;
                    vTaskDelay(500 / portTICK_PERIOD_MS);
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
        else
        {
            #if DEBUG
            safePrintln("Firebase is not ready");
            #endif
        }
    }
    if (Firebase.ready() && signupOK && firebaseInitialized)
    {
        #if DEBUG
        safePrintln("Firebase is ready and initialized");
        safePrintln("setting up Firebase RTDB stream");
        #endif
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
        #if DEBUG
        safePrintln("Input stream gestart!");
        #endif
        inputStreamConnected = true;
    }
    else
    {
        #if DEBUG
        safePrint("Stream start mislukt: ");
        safePrintln(fbdoInput.errorReason());
        #endif
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
        #if DEBUG
        safePrintln("Stream gestart!");
        #endif
        firmwareStreamConnected = true;
    }
    else
    {
        firmwareStreamConnected = false;
        #if DEBUG
        safePrint("Stream start mislukt: ");
        safePrintln(fbdoStream.errorReason());
        #endif
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
            #if DEBUG
            safePrintln("[STREAM] Periodieke reset van alle streams...");
            #endif
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
                #if DEBUG
                safePrintln("[STREAM] Probeer firmware stream opnieuw te verbinden...");
                #endif
                connectFirmwareStream();
            }
        }
        // Input stream
        if (!inputStreamConnected)
        {
            if (currentMillis - lastInputConnectAttempt >= inputConnectInterval)
            {
                lastInputConnectAttempt = currentMillis;
                #if DEBUG
                safePrintln("[STREAM] Probeer input stream opnieuw te verbinden...");
                #endif
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
        vTaskDelay(updateInterval / portTICK_PERIOD_MS);
    }
}

void streamCallbackinput(FirebaseStream data)
{
    if (fbdoInput.errorCode() != 0 && fbdoInput.errorCode() != 200)
    {
        String Message = "[STREAM] Error: ";
        Message.concat(String(fbdoInput.errorCode()));
        #if DEBUG
        safePrintln(Message);
        #endif
        inputStreamConnected = false;
    }
    else
    {
        streamReceived = true;

        char InputData[32];
        strncpy(InputData, data.stringData().c_str(), sizeof(InputData) - 1);
        InputData[sizeof(InputData) - 1] = '\0';
        #if DEBUG
        safePrint("[STREAM] Nieuwe waarde: ");
        safePrintln(InputData);
        safePrint("data afkomstig van path : ");
        safePrintln(data.dataPath());
        #endif
        if (data.dataPath() == "/Led")
        {
            if (data.stringData() == "1")
            {
                digitalWrite(LED_PIN, HIGH);
            }
            else if (data.stringData() == "0")
            {
                digitalWrite(LED_PIN, LOW);
            }
        }
        // xQueueSendToBackFromISR(FirebaseInputQueue, &InputData, 0);
        // vTaskNotifyGiveFromISR(FirebaseInputTaskHandle, NULL);
    }
}

void streamTimeoutCallbackinput(bool timeout)
{
    if (timeout)
    {
        #if DEBUG
        safePrintln("[STREAM] Timeout, probeer opnieuw...");
        #endif
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
        #if DEBUG
        safePrintln(Message);
        #endif
        firmwareStreamConnected = false;
    }
    else
    {
        streamReceived = true;
        #if DEBUG
        safePrint("[STREAM] Nieuwe waarde: ");
        safePrintln(data.stringData());
        #endif
        // convert strindata to double
        if (atof(data.stringData().c_str()) > atof(FIRMWARE_VERSION))
        {
            #if DEBUG
            safePrintln("[STREAM] Nieuwe firmware versie gedetecteerd, start OTA...!!");
            #endif
            updateAvailable = true;
            digitalWrite(LED_PIN, HIGH);
        }
    }
}

void streamTimeoutCallback(bool timeout)
{
    if (timeout)
    {
        #if DEBUG
        safePrintln("[STREAM] Timeout, probeer opnieuw...");
        #endif
        Firebase.RTDB.endStream(&fbdoStream);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        firmwareStreamConnected = false;
    }
}
