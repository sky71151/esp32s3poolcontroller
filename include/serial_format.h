#ifndef SERIAL_FORMAT_H
#define SERIAL_FORMAT_H

#include "main.h"

String getTimestamp();
String formatLog(const String &level, const String &msg);
String formatJson(const String &json);

#endif
