#ifndef OTA_H
#define OTA_H

#include "main.h"


void performOTA();
void downloadAndApplyFirmware();
bool startOTAUpdate(WiFiClient *client, int contentLength);
#endif