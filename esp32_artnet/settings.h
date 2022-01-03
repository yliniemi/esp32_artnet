#ifndef SETTINGS_H
#define SETTINGS_H

#define HOSTNAME "LEDwall"        // replace this with the name for this particular device. everyone deserves a unique name
#define LED_WIDTH 9
#define LED_HEIGHT 130
#define MAX_CURRENT 25000         // maxumum current in milliwatts. this is here so that your powersupply won't overheat or shutdown
#define USING_SERIALOTA           // uncomment this if you are not using SerialOTA
#define UNIVERSE_SIZE 130         // my setup is 170 leds per universe no matter if the last universe is not full.
#define USING_LED_BUFFER
#define FASTLED_ESP32_I2S
#define ATX_ON_PIN 25
#define ATX_ON_ENABLED true
#define MAX_BRIGHTNESS 255

#define CONFIG_FILE_NAME "/config.jsn"

// #define FASTLED_ESP32_I2S      //  uncomment this if you need to use more than 8 pins. the maximum number of led strips that I2S can push is 

#define OTA_ROUNDS 5

#endif
