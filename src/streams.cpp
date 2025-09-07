#include "streams.h"

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