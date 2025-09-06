#ifndef FIREBASE_H
#define FIREBASE_H

#include "main.h"

// Basis device pad (gebruik deviceId als variabele)
constexpr const char *PATH_DEVICES = "devices/";
constexpr const char *PATH_DEVICEINFO = "/DeviceInfo";
constexpr const char *PATH_DEVICEINFO_BOARDTYPE = "/DeviceInfo/boardType";
constexpr const char *PATH_DEVICEINFO_CLIENTID = "/DeviceInfo/clientId";
constexpr const char *PATH_DEVICEINFO_FIRMWARE = "/DeviceInfo/firmware";

constexpr const char *PATH_GPIO = "/GPIO";
constexpr const char *PATH_GPIO_ANALOGINPUTS = "/GPIO/AnalogInputs";
constexpr const char *PATH_GPIO_DIGITALINPUTS = "/GPIO/DigitalInputs";
constexpr const char *PATH_GPIO_DIPSWITCHES = "/GPIO/DipSwitches";
constexpr const char *PATH_GPIO_LED = "/GPIO/Led";
constexpr const char *PATH_GPIO_RELAYS = "/GPIO/Relays";

constexpr const char *PATH_REGISTRATION = "/Registration";
constexpr const char *PATH_REGISTRATION_FIRST = "/Registration/firstRegistratione";
constexpr const char *PATH_REGISTRATION_LASTBOOT = "/Registration/lastBoot";
constexpr const char *PATH_REGISTRATION_LASTSEEN = "/Registration/lastSeen";
constexpr const char *PATH_REGISTRATION_LASTSTREAM = "/Registration/laststreamreceived";
constexpr const char *PATH_REGISTRATION_UPTIME = "/Registration/uptime";

void firebaseTask(void *pvParameters);
bool initFirebase();
void streamCallback(FirebaseStream data);
void streamTimeout(bool timeout);
void checkDeviceExists();
void firmwareCallback(FirebaseStream data);
void firmwareTimeout(bool timeout);
void setupStreamInputs();
void updateFirebase();
void sendStringToFirebaseQueue(const String &path, const String &value);

#endif
