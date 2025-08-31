#pragma once

struct LogEntry {
  uint32_t timestamp;
  uint8_t  type;           // 0 = reset, 1 = stack overflow, etc.
  uint8_t  value;          // reset-reason of evt. extra info
  char     taskName[16];
  uint32_t freeHeap;       // extra: vrije heap bij crash
  uint32_t stackWatermark; // extra: minimale vrije stack van de task
};

void initExternalFlash();
bool externalFlashRead(uint32_t addr, uint8_t* buffer, size_t len);
bool externalFlashWrite(uint32_t addr, const uint8_t* buffer, size_t len);
bool externalFlashErase4k(uint32_t addr);
void logToFlash(const LogEntry* entry, size_t len);
void printLogFromFlash();
void updateBootCount();
