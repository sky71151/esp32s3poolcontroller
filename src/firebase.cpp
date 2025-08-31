#include "firebase.h"
#include "external_flash.h"
#include "ota.h"
#include "wifitask.h"
#include "board.h"

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
            //auth.user.email = USER_EMAIL;
            //auth.user.password = USER_PASSWORD;
            //Firebase.begin(&config, &auth); // auth leeg laat anonieme login toe

            Firebase.signUp(&config, &auth, "", "");

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
                safePrintln("Stream gestart!");
                streamConnected = true;
            }
            else
            {
                safePrint("Stream start mislukt: ");
                safePrintln(fbdoStream.errorReason());
            }
            String StreamInputPath = "/devices/";
            StreamInputPath.concat(deviceId);
            StreamInputPath.concat("/GPIO/Relays");
            if (Firebase.RTDB.beginStream(&fbdoInput, StreamInputPath.c_str()))
            {
                Firebase.RTDB.setStreamCallback(&fbdoInput, streamCallbackinput, streamTimeoutCallbackinput);
                safePrintln("Input stream gestart!");
            }
            else
            {
                safePrint("Stream start mislukt: ");
                safePrintln(fbdoInput.errorReason());
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
                String firmwareVersion = FIRMWARE_VERSION;
                firmwareVersion.concat(" ");
                firmwareVersion.concat(timeStr);
                if (Firebase.RTDB.setString(&fbdo, pathFirmware.c_str(), firmwareVersion))
                {
                    safePrint("Firmware version update: ");
                    safePrintln(firmwareVersion);
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

void updateTimeToFirebaseTask(void *pvParameters)
{
    while (true)
    {
        if (WiFi.status() == WL_CONNECTED && Firebase.ready() && firebaseInitialized)
        {
            time_t now = time(nullptr);
            char timeStr[32];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
            String pathTime = "devices/";
            pathTime.concat(deviceId);
            pathTime.concat("/Registration/lastSeen");
            if (Firebase.RTDB.setString(&fbdo, pathTime, timeStr))
            {
                safePrint("Tijd geüpload: ");
                safePrintln(timeStr);
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
            pathRuntime.concat(deviceId);
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

void streamCallback(FirebaseStream data)
{
    Serial.print("[STREAM] Nieuwe waarde: ");
    Serial.println(data.stringData());
    // convert strindata to double
    if (atof(data.stringData().c_str()) > atof(FIRMWARE_VERSION))
    {
        Serial.println("[STREAM] Nieuwe firmware versie gedetecteerd, start OTA...!!");
        // Firebase.RTDB.endStream(&fbdoStream);
        // vTaskSuspend(mainHandle);
        // vTaskSuspend(updateHandle);
        // vTaskSuspend(statusHandle);
        // vTaskSuspend(firebaseHandle);
        //  Suspend the stack monitor task if its handle is available
        //  If you want to suspend the stack monitor task, you need to store its handle.
        //  If you have a handle (e.g., TaskHandle_t stackMonitorHandle), use it here:
        //  vTaskSuspend(stackMonitorHandle);

        updateAvailable = true;
        // Turn on LED to indicate update available
        digitalWrite(LED_PIN, HIGH);

        // performOTA();
    }
}

void streamTimeoutCallback(bool timeout)
{
    if (timeout)
    {
        Serial.println("[STREAM] Timeout, probeer opnieuw...");
        Firebase.RTDB.endStream(&fbdoStream);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        streamConnected = false;
    }
}

void updateFirebaseInstant(String path, String data)
{
    String destination = "devices/";
    destination.concat(deviceId);
    destination.concat(path);
    if (Firebase.RTDB.setString(&fbdo, destination, data))
    {
        safePrint("volgende path is geupdated: ");
        safePrint(destination);
        safePrint(" -> ");
        safePrintln(data);
    }
}

void FirebaseInputTask(void *pvParameters)
{

    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        char cmd[32];
        while (xQueueReceive(FirebaseInputQueue, &cmd, 0) == pdTRUE)
        {
            int relaisStates[8] = {0};
            int idx = 0;
            char *token = strtok(cmd, ",");
            while (token != NULL && idx < 8)
            {
                relaisStates[idx++] = atoi(token);
                token = strtok(NULL, ",");
            }
            for (int i = 0; i < 8; i++)
            {
                digitalWrite(RELAY_PINS[i], relaisStates[i] ? HIGH : LOW);
            }
        }
    }
}

void streamCallbackinput(FirebaseStream data)
{
    char InputData[32];
    strncpy(InputData, data.stringData().c_str(), sizeof(InputData) - 1);
    InputData[sizeof(InputData) - 1] = '\0';
    safePrint("[STREAM] Nieuwe waarde: ");
    safePrintln(InputData);
    safePrint("data afkomstig van path : ");
    safePrintln(data.dataPath());
    xQueueSendToBackFromISR(FirebaseInputQueue, &InputData, 0);
    vTaskNotifyGiveFromISR(FirebaseInputTaskHandle, NULL);
}

void streamTimeoutCallbackinput(bool timeout)
{
    if (timeout)
    {
        Serial.println("[STREAM] Timeout, probeer opnieuw...");
        Firebase.RTDB.endStream(&fbdoInput);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        streamConnected = false;
    }
}