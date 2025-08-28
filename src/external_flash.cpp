#include <Arduino.h>
#include <SPI.h>
#include "Board.h"
#include "external_flash.h"

#define LOG_FLASH_ADDR   0x10000      // Startadres loggebied
#define LOG_SECTOR_SIZE  4096         // 4k sector
#define LOG_ENTRY_SIZE   sizeof(LogEntry)
#define LOG_MAX_ENTRIES  (LOG_SECTOR_SIZE / LOG_ENTRY_SIZE)
static uint16_t logWriteIndex = 0;    // Houdt bij waar de volgende logregel komt



void initExternalFlash() {
    // SPI bus initialiseren met de juiste pinnen
    SPI.begin(SPI_FLASH_CLK_PIN, SPI_FLASH_MISO_PIN, SPI_FLASH_MOSI_PIN, SPI_FLASH_CS_PIN);
    pinMode(SPI_FLASH_CS_PIN, OUTPUT);
    digitalWrite(SPI_FLASH_CS_PIN, HIGH);

    // JEDEC ID commando sturen
    digitalWrite(SPI_FLASH_CS_PIN, LOW);
    SPI.transfer(0x9F); // JEDEC ID commando
    uint8_t manufacturer = SPI.transfer(0x00);
    uint8_t memoryType   = SPI.transfer(0x00);
    uint8_t capacity     = SPI.transfer(0x00);
    digitalWrite(SPI_FLASH_CS_PIN, HIGH);

    Serial.print("JEDEC ID: ");
    Serial.print(manufacturer, HEX); Serial.print(" ");
    Serial.print(memoryType, HEX); Serial.print(" ");
    Serial.println(capacity, HEX);
    logWriteIndex = 0; // Reset log index bij initialisatie
}

// Lees bytes uit de flash (max 256 per keer)
bool externalFlashRead(uint32_t addr, uint8_t* buffer, size_t len) {
    if (len == 0 || len > 256) return false;
    digitalWrite(SPI_FLASH_CS_PIN, LOW);
    SPI.transfer(0x03); // READ command
    SPI.transfer((addr >> 16) & 0xFF);
    SPI.transfer((addr >> 8) & 0xFF);
    SPI.transfer(addr & 0xFF);
    for (size_t i = 0; i < len; i++) {
        buffer[i] = SPI.transfer(0x00);
    }
    digitalWrite(SPI_FLASH_CS_PIN, HIGH);
    return true;
}

// Schrijf bytes naar de flash (max 256 per keer, sector moet gewist zijn)
bool externalFlashWrite(uint32_t addr, const uint8_t* buffer, size_t len) {
    if (len == 0 || len > 256) return false;
    // Write Enable
    digitalWrite(SPI_FLASH_CS_PIN, LOW);
    SPI.transfer(0x06);
    digitalWrite(SPI_FLASH_CS_PIN, HIGH);
    delayMicroseconds(1);
    // Page Program
    digitalWrite(SPI_FLASH_CS_PIN, LOW);
    SPI.transfer(0x02); // PAGE PROGRAM command
    SPI.transfer((addr >> 16) & 0xFF);
    SPI.transfer((addr >> 8) & 0xFF);
    SPI.transfer(addr & 0xFF);
    for (size_t i = 0; i < len; i++) {
        SPI.transfer(buffer[i]);
    }
    digitalWrite(SPI_FLASH_CS_PIN, HIGH);
    // Wachten tot klaar (poll WIP bit)
    unsigned long start = millis();
    while (1) {
        digitalWrite(SPI_FLASH_CS_PIN, LOW);
        SPI.transfer(0x05); // Read Status Register
        uint8_t status = SPI.transfer(0x00);
        digitalWrite(SPI_FLASH_CS_PIN, HIGH);
        if ((status & 0x01) == 0) break;
        if (millis() - start > 100) return false;
        delay(1);
    }
    return true;
}

bool externalFlashErase4k(uint32_t addr) {
    // Write Enable
    digitalWrite(SPI_FLASH_CS_PIN, LOW);
    SPI.transfer(0x06);
    digitalWrite(SPI_FLASH_CS_PIN, HIGH);
    delayMicroseconds(1);
    // 4KB Sector Erase
    digitalWrite(SPI_FLASH_CS_PIN, LOW);
    SPI.transfer(0x20); // 4KB Erase
    SPI.transfer((addr >> 16) & 0xFF);
    SPI.transfer((addr >> 8) & 0xFF);
    SPI.transfer(addr & 0xFF);
    digitalWrite(SPI_FLASH_CS_PIN, HIGH);
    // Wachten tot klaar
    unsigned long start = millis();
    while (1) {
        digitalWrite(SPI_FLASH_CS_PIN, LOW);
        SPI.transfer(0x05);
        uint8_t status = SPI.transfer(0x00);
        digitalWrite(SPI_FLASH_CS_PIN, HIGH);
        if ((status & 0x01) == 0) break;
        if (millis() - start > 1000) return false;
        delay(1);
    }
    return true;
}

// Logregel toevoegen (ringbuffer, overschrijft oudste als vol)
void logToFlash(const LogEntry* entry, size_t len) {
    if (len != LOG_ENTRY_SIZE) return; // Veiligheidscheck

    uint32_t addr = LOG_FLASH_ADDR + (logWriteIndex * LOG_ENTRY_SIZE);

    // Sector wissen als we aan het begin zijn (optioneel: alleen als sector vol is)
    if (logWriteIndex == 0) {
        externalFlashErase4k(LOG_FLASH_ADDR);
    }

    externalFlashWrite(addr, (const uint8_t*)entry, LOG_ENTRY_SIZE);

    logWriteIndex++;
    if (logWriteIndex >= LOG_MAX_ENTRIES) {
        logWriteIndex = 0; // Ringbuffer: terug naar begin
    }
}

// Uitlezen van logregels (voorbeeld)
void printLogFromFlash() {
    LogEntry entry;
    for (uint16_t i = 0; i < LOG_MAX_ENTRIES; i++) {
        uint32_t addr = LOG_FLASH_ADDR + (i * LOG_ENTRY_SIZE);
        externalFlashRead(addr, (uint8_t*)&entry, LOG_ENTRY_SIZE);
        // Check op geldige entry, bijvoorbeeld timestamp != 0xFFFFFFFF
        if (entry.timestamp != 0xFFFFFFFF && entry.timestamp != 0) {
            Serial.print("Log "); Serial.print(i); Serial.print(": ");
            Serial.print("Type: "); Serial.print(entry.type);
            Serial.print(", Value: "); Serial.print(entry.value);
            Serial.print(", Task: "); Serial.print(entry.taskName);
            Serial.print(", Heap: "); Serial.print(entry.freeHeap);
            Serial.print(", Stack: "); Serial.println(entry.stackWatermark);
        }
    }
}
