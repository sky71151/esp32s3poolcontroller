#include <Arduino.h>
#include <SPI.h>
#include "Board.h"

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
