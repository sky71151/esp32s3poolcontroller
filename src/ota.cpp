#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include "ota.h"
#include "Update.h"
#include <HTTPClient.h>

// Zet dit op je eigen GitHub release-asset URL
const char *firmwareUrl = "https://github.com/sky71151/esp32s3poolcontroller/raw/refs/heads/main/.pio/build/esp32-s3-devkitc-1/firmware.bin";

void performOTA()
{
  downloadAndApplyFirmware(); 
}

void downloadAndApplyFirmware()
{
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(firmwareUrl);

  int httpCode = http.GET();
  Serial.printf("HTTP GET code: %d\n", httpCode);

  if (httpCode == HTTP_CODE_OK)
  {
    int contentLength = http.getSize();
    Serial.printf("Firmware size: %d bytes\n", contentLength);

    if (contentLength > 0)
    {
      WiFiClient *stream = http.getStreamPtr();
      if (startOTAUpdate(stream, contentLength))
      {
        Serial.println("OTA update successful, restarting...");
        delay(2000);
        ESP.restart();
      }
      else
      {
        Serial.println("OTA update failed");
      }
    }
    else
    {
      Serial.println("Invalid firmware size");
    }
  }
  else
  {
    Serial.printf("Failed to fetch firmware. HTTP code: %d\n", httpCode);
  }
  http.end();
}

bool startOTAUpdate(WiFiClient *client, int contentLength)
{
  Serial.println("Initializing update...");
  if (!Update.begin(contentLength))
  {
    Serial.printf("Update begin failed: %s\n", Update.errorString());
    return false;
  }

  Serial.println("Writing firmware...");
  size_t written = 0;
  int progress = 0;
  int lastProgress = 0;

  // Timeout variables
  const unsigned long timeoutDuration = 120 * 1000; // 10 seconds timeout
  unsigned long lastDataTime = millis();

  while (written < contentLength)
  {
    if (client->available())
    {
      uint8_t buffer[128];
      size_t len = client->read(buffer, sizeof(buffer));
      if (len > 0)
      {
        Update.write(buffer, len);
        written += len;

        // Calculate and print progress
        progress = (written * 100) / contentLength;
        if (progress != lastProgress)
        {
          Serial.printf("Writing Progress: %d%%\n", progress);
          lastProgress = progress;
        }
      }
    }
    // Check for timeout
    if (millis() - lastDataTime > timeoutDuration)
    {
      Serial.println("Timeout: No data received for too long. Aborting update...");
      Update.abort();
      return false;
    }

    yield();
  }
  Serial.println("\nWriting complete");

  if (written != contentLength)
  {
    Serial.printf("Error: Write incomplete. Expected %d but got %d bytes\n", contentLength, written);
    Update.abort();
    return false;
  }

  if (!Update.end())
  {
    Serial.printf("Error: Update end failed: %s\n", Update.errorString());
    return false;
  }

  Serial.println("Update successfully completed");
  return true;
}