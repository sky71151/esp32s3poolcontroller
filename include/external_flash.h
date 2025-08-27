#pragma once

void initExternalFlash();
bool externalFlashRead(uint32_t addr, uint8_t* buffer, size_t len);
bool externalFlashWrite(uint32_t addr, const uint8_t* buffer, size_t len);
