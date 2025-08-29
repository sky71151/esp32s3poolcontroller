#pragma once

#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

const char* ota_url = "https://github.com/<user>/<repo>/releases/download/<tag>/firmware.bin";