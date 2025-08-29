#pragma once

#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

void performOTA();
void downloadAndApplyFirmware();
bool startOTAUpdate(WiFiClient *client, int contentLength);