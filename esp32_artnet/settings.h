#ifndef SETTINGS_H
#define SETTINGS_H

#define HOSTNAME "LEDwall"        // replace this with the name for this particular device. everyone deserves a unique name

#define CONFIG_FILE_NAME "/config.jsn"

#define USING_SERIALOTA           // uncomment this if you are not using SerialOTA
#define USING_LED_BUFFER
#define FASTLED_ESP32_I2S

int maxCurrent = 4000;
int ledWidth = 1;
int ledHeight = 130;
int numLeds = ledWidth * ledHeight;
int universeSize = 130;
int atxOnPin = 25;
bool atxOnEnabled = true;
int turnOffDelay = 60;
int maxBrightness = 255;
int OTArounds = 30;             // this is how many seconds we waste waiting for the OTA during boot. sometimes people make mistakes in their code - not me - and the program freezes. this way you can still update your code over the air even if you have some dodgy code in your loop
int pins[64] = {4, 16, 17, 5, 18, 19, 21, 22, 23, 13, 12, 14, 27, 26, 33, 32, 25, 15, 3, 1, 2};              // this is the pinout for Elina's version
// maybe include a possible static ip hare

const int maxFileSize = 4096;

#endif
