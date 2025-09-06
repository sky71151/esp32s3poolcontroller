#include "serial_format.h"

String getTimestamp() {
  time_t now = time(nullptr);
  struct tm *tm_info = localtime(&now);
  char buffer[16];
  sprintf(buffer, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
  return String("[") + buffer + "]";
}

String formatLog(const String &level, const String &msg) {
  return getTimestamp() + " [" + level + "] " + msg;
}

String formatJson(const String &json) {
  StaticJsonDocument<256> doc;
  String pretty;
  DeserializationError error = deserializeJson(doc, json);
  if (!error) {
    serializeJsonPretty(doc, pretty);
    return pretty;
  }
  return json;
}
