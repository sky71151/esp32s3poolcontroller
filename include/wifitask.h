#ifndef WIFITASK_H
#define WIFITASK_H

#include "main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <WiFi.h>
#include "secrets.h"

void connectToWiFiTask(void *pvParameters);

#endif // WIFITASK_H
