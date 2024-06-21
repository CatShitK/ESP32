#include "Arduino.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"

#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18
#define I2S_DOUT      25
#define I2S_BCLK      26
#define I2S_LRC       27

Audio audio;

void setup() {
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    Serial.begin(115200);
    SD.begin(SD_CS);
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(15); // 0...21    控制音量

    audio.connecttoFS(SD, "/多余的解释_许嵩.mp3");
}

void loop()
{
    audio.loop();
}
