#include "serial_format.h"

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define RESET   "\033[0m"

String getTimestamp() {
  time_t now = time(nullptr);
  struct tm *tm_info = localtime(&now);
  char buffer[16];
  sprintf(buffer, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
  return String("[") + buffer + "]";
}

String formatLog(const String &level, const String &msg) {
    if (level == "ERROR") {
        return getTimestamp() + " [" + level + "] " + String(RED) + msg + RESET;
    }else if (level == "INFO") {
        return getTimestamp() + " [" + level + "] " + String(GREEN) + msg + RESET;
    } else if (level == "MAIN") {
        return getTimestamp() + " [" + level + "] " + String(BLUE) + msg + RESET;
    } else if (level == "SUCCESS") {
        return getTimestamp() + " [" + level + "] " + String(YELLOW) + msg + RESET;
    } else {
        return getTimestamp() + " [" + level + "] " + msg;
    }
    
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
