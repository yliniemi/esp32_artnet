#ifndef SETTINGS_H
#define SETTINGS_H

#define CONFIG_FILE_NAME "/config.jsn"

// #define DEBUG                     // uncomment this to see what gets read from the config. including psk and ota password
#define USING_SERIALOTA           // uncomment this if you are not using SerialOTA
#define USING_LED_BUFFER
#define FASTLED_ESP32_I2S

int maxCurrent = 4000;
int ledWidth = 1;
int ledHeight = 300;
int numLeds = ledWidth * ledHeight;
int universeSize = 150;
int atxOnPin = 23;
bool atxOnEnabled = false;
int turnOffDelay = 60;
int maxBrightness = 255;
char OTApassword[64] = OTA_PASSWORD;
int OTArounds = 10;             // this is how many seconds we waste waiting for the OTA during boot. sometimes people make mistakes in their code - not me - and the program freezes. this way you can still update your code over the air even if you have some dodgy code in your loop
bool serialEnabled = true;
// The pins here are listed in preferential order. the ones at the end are quite iffy and should not be used unless you know what you are doing.
// int pins[64] = {13, 12, 14, 27, 26, 25, 33, 32, 15, 16, 17, 5, 18, 19, 21, 22, 23, 1, 3, 4, 2, 0};              // esp32 dev kit v1
int pins[64] = {12, 13, 15, 14, 2, 1, 3, 4, 0, 16};                                                             // esp32-cam without sd card
// int pins[64] = {4, 16, 17, 5, 18, 19, 21, 22, 23, 13, 12, 14, 27, 26, 33, 32, 25, 15, 3, 1, 2, 0};              // this is the pinout for Elina's version

char ssid[64] = "";
char psk[64] = "";
char hostname[64] = "LedMatrix";          // can be at most 32 characters

const int maxFileSize = 10240;

bool staticIpEnabled = false;
int ip[] = {192, 168, 69, 213};
int gateway[] = {192, 168, 69, 1};               // this is also the default dns server
int mask [] = {255, 255, 255, 0};
#endif
