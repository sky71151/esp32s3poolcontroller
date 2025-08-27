#include <Adafruit_SPIFlash.h>
#include <SPI.h>
#include "Board.h"

// Gebruik de standaard SPI bus (SPI)
Adafruit_FlashTransport_SPI flashTransport(SPI_FLASH_CS_PIN, &SPI);
Adafruit_SPIFlash flash(&flashTransport);

void initExternalFlash() {
    // SPI bus initialiseren met de juiste pinnen
    SPI.begin(SPI_FLASH_CLK_PIN, SPI_FLASH_MISO_PIN, SPI_FLASH_MOSI_PIN, SPI_FLASH_CS_PIN);
    if (!flash.begin()) {
        Serial.println("Fout bij initialiseren van SPI Flash (W25Q128)!");
        while (1);
    } else {
        Serial.println("SPI Flash (W25Q128) succesvol geïnitialiseerd!");
    }
}
