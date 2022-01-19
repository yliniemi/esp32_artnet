/*
ArtNet LED Matrix
Copyright (C) 2021 Antti Yliniemi
This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License version 3 as published by the Free Software Foundation.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details. 
https://www.gnu.org/licenses/agpl-3.0.txt
*/

// My GitHUb https://github.com/yliniemi/

/*
I use these two libraries by Yvez Basin. He was also kind enough to help me with this source code.
https://github.com/hpwit/artnetESP32
https://github.com/hpwit/I2SClockBasedLedDriver
*/


#include <myCredentials.h>        // oh yeah. there is myCredentials.zip on the root of this repository. include it as a library and then edit the file with your onw ips and stuff
#include "settings.h"

#include "setupWifi.h"
#include "OTA.h"

#ifdef USING_SERIALOTA
#include "SerialOTA.h"
#endif

// #define USING_SPIFFS          // this is commented because we live int the future. uncomment if you need spiffs for compatibility reasons
#ifdef USING_SPIFFS
#include <SPIFFS.h>
#endif
#ifndef USING_SPIFFS
#include <LITTLEFS.h>
#endif

#include <ArduinoJson.h>

#define ARTNET_NO_TIMEOUT_REPORTING
#include <ArtnetESP32.h>

#include "I2SClocklessLedDriver.h"
I2SClocklessLedDriver driver;

volatile int gotFrame = false;

uint8_t *ledsArtnet = NULL;

ArtnetESP32 artnet;

TaskHandle_t task1, task2, task3;


class PrintSerialAndOTA
{
  public:
  
  template <typename T> void print(T argument)
  {
    Serial.print(argument);
    #ifdef USING_SERIALOTA
    SerialOTA.print(argument);
    #endif
  }

  template <typename T> void println(T argument)
  {
    Serial.println(argument);
    #ifdef USING_SERIALOTA
    SerialOTA.println(argument);
    #endif
  }
  
  void println()
  {
    Serial.println();
    #ifdef USING_SERIALOTA
    SerialOTA.println();
    #endif
  }
};

PrintSerialAndOTA Both;


bool saveConfig()
{
  Both.println("Mounting filesystem for writing");
  #ifdef USING_SPIFFS
  if (!SPIFFS.begin())
  #endif
  #ifndef USING_SPIFFS
  if (!LITTLEFS.begin())
  #endif
  {
    Both.println("Failed to mount file system");
    return false;
  }

  #ifdef USING_SPIFFS
  File configFile = SPIFFS.open(CONFIG_FILE_NAME, FILE_WRITE);
  #endif
  #ifndef USING_SPIFFS
  File configFile = LITTLEFS.open(CONFIG_FILE_NAME, FILE_WRITE);
  #endif
  if (!configFile) {
    Both.println("Failed to open config file");
    return false;
  }

  if (configFile.size() > maxFileSize) {
    Both.println("Config file size is too large");
    return false;
  }

  DynamicJsonDocument jsonBuffer(maxFileSize);
  
  jsonBuffer["ledWidth"] = ledWidth;
  jsonBuffer["ledHeight"] = ledHeight;
  jsonBuffer["universeSize"] = universeSize;
  jsonBuffer["maxCurrent"] = maxCurrent;
  jsonBuffer["maxBrightness"] = maxBrightness;
  jsonBuffer["atxOnEnabled"] = atxOnEnabled;
  jsonBuffer["atxOnPin"] = atxOnPin;
  jsonBuffer["turnOffDelay"] = turnOffDelay;
  jsonBuffer["OTArounds"] = OTArounds;
  jsonBuffer["hostname"] = String(hostname);
  jsonBuffer["staticIpEnabled"] = staticIpEnabled;
  jsonBuffer["serialEnabled"] = serialEnabled;
  jsonBuffer["ssid"] = String(ssid);
  
  serializeJsonPretty(jsonBuffer, Serial);
  Serial.println();
  
  JsonArray jsonPins = jsonBuffer.createNestedArray("pins");
  copyArray(pins, jsonPins);
  
  JsonArray jsonIp = jsonBuffer.createNestedArray("ip");
  copyArray(ip, jsonIp);
  
  JsonArray jsonGateway = jsonBuffer.createNestedArray("gateway");
  copyArray(gateway, jsonGateway);
  
  JsonArray jsonMask = jsonBuffer.createNestedArray("mask");
  copyArray(mask, jsonMask);
  
  serializeJson(jsonBuffer, Serial);
  Serial.println();
  
  jsonBuffer["psk"] = String(psk);
  jsonBuffer["OTApassword"] = String(OTApassword);
  
  serializeJson(jsonBuffer, configFile);
  
  configFile.close();
  
  return true;
}

void changeSettings()
{
  String s;
  {
    Serial.setTimeout(5000);                 // normally readStringUntil waits only a second
    Serial.println();
    Serial.println("If you want load your current settings and edit them type 'edit'");
    Serial.println("If there is a problem and you don't want to load the config file type 'set'");
    Serial.println("You have 5 seconds to comply");
    s = Serial.readStringUntil('\n');
    s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
    Serial.println(String("You typed: " + s));
    Serial.setTimeout(1000000000);
    if (s.equals("set") || s.equals("edit"))
    {
      if (s.equals("edit"))
      {
        loadConfig();
      }
      Serial.println("You are now in settings mode");
      Serial.println("When you're done making changes and want to save them on the flash rom type 'save'");
      Serial.println("If you made a mistake, you can cancel it by typing 'cancel'");
      Serial.println("If you want to format the filesystem type 'format'");
      Serial.println("To reboot type, you guessed it, 'reboot'");
      Serial.println();
      Serial.println("Here is a list of all the options and their current values");
      Serial.println(String("ledWidth = ") + ledWidth);
      Serial.println(String("ledHeight = ") + ledHeight);
      Serial.println(String("universeSize = ") + universeSize);
      Serial.println(String("maxCurrent = ") + maxCurrent);
      Serial.println(String("maxBrightness = ") + maxBrightness);
      Serial.println(String("atxOnEnabled = ") + atxOnEnabled);
      Serial.println(String("atxOnPin = ") + atxOnPin);
      Serial.println(String("turnOffDelay = ") + turnOffDelay);
      Serial.println(String("OTArounds = ") + OTArounds);
      Serial.println(String("hostname = ") + hostname);
      Serial.println(String("ssid = ") + ssid);
      Serial.println(String("psk = ") + "********");
      Serial.println(String("OTApassword = ") + "********");
      Serial.println(String("serialEnabled = ") + serialEnabled);
      
      Serial.print(String("pins = {"));
      for (int index = 0; index < 64; index++)
      {
        Serial.print(String(pins[index]) + ", ");
      }
      Serial.println("}");
      
      Serial.println(String("staticIpEnabled = ") + staticIpEnabled);
      
      Serial.print(String("ip = {"));
      for (int index = 0; index < 4; index++)
      {
        Serial.print(String(ip[index]) + ", ");
      }
      Serial.println("}");
      
      Serial.print(String("gateway = {"));
      for (int index = 0; index < 4; index++)
      {
        Serial.print(String(gateway[index]) + ", ");
      }
      Serial.println("}");
      
      Serial.print(String("mask = {"));
      for (int index = 0; index < 4; index++)
      {
        Serial.print(String(mask[index]) + ", ");
      }
      Serial.println("}");
      
      for (;;)
      {
        Serial.println();
        Serial.println("Please type a variable name or a command");
        s = Serial.readStringUntil('\n');
        s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
        Serial.println(String("You typed: " + s));
        
        if (s.equals("save"))
        {
          Serial.println("Saving your settings to the flash rom");
          saveConfig();
          Serial.println("Saving complete. Rebooting in one second");
          delay(1000);
          ESP.restart();
        }
        
        else if (s.equals("cancel"))
        {
          Serial.println("Cancelled saving the settings");
          Serial.println("Continuing with normal boot");
          break;
        }
        
        else if (s.equals("format"))
        {
          Serial.println("Formatting the filesystem");
          #ifdef USING_SPIFFS
          SPIFFS.format();
          #endif
          #ifndef USING_SPIFFS
          LITTLEFS.format();
          #endif
        }
        
        else if (s.equals("reboot"))
        {
          Serial.println("Rebooting in 5 seconds");
          delay(5000);
          ESP.restart();
        }
        
        else if (s.equals("ssid"))
        {
          s = Serial.readStringUntil('\n');
          if (s.charAt(s.length() - 1) == '\r') s.remove(s.length() - 1);
          s.toCharArray(ssid, 64);
          Serial.println(String("ssid = ") + ssid);
        }
        
        else if (s.equals("psk"))
        {
          s = Serial.readStringUntil('\n');
          if (s.charAt(s.length() - 1) == '\r') s.remove(s.length() - 1);
          s.toCharArray(psk, 64);
          Serial.println(String("psk = ") + psk);
        }
        
        else if (s.equals("hostname"))
        {
          s = Serial.readStringUntil('\n');
          if (s.charAt(s.length() - 1) == '\r') s.remove(s.length() - 1);
          s.toCharArray(hostname, 64);
          Serial.println(String("hostname = ") + hostname);
        }
        
        else if (s.equals("OTApassword"))
        {
          s = Serial.readStringUntil('\n');
          if (s.charAt(s.length() - 1) == '\r') s.remove(s.length() - 1);
          s.toCharArray(OTApassword, 64);
          Serial.println(String("OTApassword = ") + OTApassword);
        }
        
        else if (s.equals("ledWidth"))
        {
          s = Serial.readStringUntil('\n');
          s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
          ledWidth = s.toInt();
          Serial.println(String("ledWidth = ") + ledWidth);
        }
        
        else if (s.equals("ledHeight"))
        {
          s = Serial.readStringUntil('\n');
          s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
          ledHeight = s.toInt();
          Serial.println(String("ledHeight = ") + ledHeight);
        }
        
        else if (s.equals("universeSize"))
        {
          s = Serial.readStringUntil('\n');
          s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
          universeSize = s.toInt();
          Serial.println(String("universeSize = ") + universeSize);
        }
        
        else if (s.equals("maxCurrent"))
        {
          s = Serial.readStringUntil('\n');
          s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
          maxCurrent = s.toInt();
          Serial.println(String("maxCurrent = ") + maxCurrent);
        }
        
        else if (s.equals("maxBrightness"))
        {
          s = Serial.readStringUntil('\n');
          s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
          maxBrightness = s.toInt();
          Serial.println(String("maxBrightness = ") + maxBrightness);
        }
        
        else if (s.equals("atxOnEnabled"))
        {
          s = Serial.readStringUntil('\n');
          s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
          if (s.equals("true") || s.equals("1")) atxOnEnabled = true;
          else atxOnEnabled = false;
          Serial.println(String("atxOnEnabled = ") + atxOnEnabled);
        }
        
        else if (s.equals("atxOnPin"))
        {
          s = Serial.readStringUntil('\n');
          s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
          atxOnPin = s.toInt();
          Serial.println(String("atxOnPin = ") + atxOnPin);
        }
        
        else if (s.equals("turnOffDelay"))
        {
          s = Serial.readStringUntil('\n');
          s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
          turnOffDelay = s.toInt();
          Serial.println(String("turnOffDelay = ") + turnOffDelay);
        }
        
        else if (s.equals("OTArounds"))
        {
          s = Serial.readStringUntil('\n');
          s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
          OTArounds = s.toInt();
          Serial.println(String("OTArounds = ") + OTArounds);
        }
        
        else if (s.equals("pins"))
        {
          Serial.println("You can set the pins one by one");
          Serial.println("You start with the first pin and can define maximum 64 pins");
          Serial.println("That's like twice as much as esp32 has but the extra ones don't get called anyways");
          for (int index = 0; index < 64; index++)
          {
            Serial.println();
            Serial.println(String("pin ") + index);
            s = Serial.readStringUntil('\n');
            s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
            if (s.equals("done"))
            {
              break;
            }
            pins[index] = s.toInt();
            Serial.println(String("pins[") + index + "]" + " = " + pins[index]);
          }
          Serial.println("Done making changes to the pins");
        }
        
        else if (s.equals("staticIpEnabled"))
        {
          s = Serial.readStringUntil('\n');
          s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
          if (s.equals("true") || s.equals("1")) staticIpEnabled = true;
          else staticIpEnabled = false;
          Serial.println(String("atxOnEnabled = ") + staticIpEnabled);
        }
        
        else if (s.equals("ip"))
        {
          Serial.println("Please type the device ip address one number at a time");
          for (int index = 0; index < 4; index++)
          {
            for (int i = 0; i < index; i++)
            {
              Serial.print(String(ip[i]) + ".");
            }
            Serial.println();
            s = Serial.readStringUntil('\n');
            s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
            ip[index] = s.toInt();
          }
        }
        
        else if (s.equals("gateway"))
        {
          Serial.println("Please type the gateway ip address one number at a time");
          for (int index = 0; index < 4; index++)
          {
            for (int i = 0; i < index; i++)
            {
              Serial.print(String(gateway[i]) + ".");
            }
            Serial.println();
            s = Serial.readStringUntil('\n');
            s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
            gateway[index] = s.toInt();
          }
        }
        
        else if (s.equals("mask"))
        {
          Serial.println("Please type the mask number at a time");
          for (int index = 0; index < 4; index++)
          {
            for (int i = 0; i < index; i++)
            {
              Serial.print(String(mask[i]) + ".");
            }
            Serial.println();
            s = Serial.readStringUntil('\n');
            s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
            mask[index] = s.toInt();
          }
        }
        
        else if (s.equals("serialEnabled"))
        {
          s = Serial.readStringUntil('\n');
          s.trim();             // some terminals add an extra return character /r. we remove that. we get rid of other whitespaces with this command
          if (s.equals("true") || s.equals("1")) staticIpEnabled = true;
          else staticIpEnabled = false;
          Serial.println(String("serialEnabled = ") + serialEnabled);
        }
        
        else Serial.println("Unknown variable name");
      }
    }
    else Serial.println("Ah ah aah, you didn't say the magic word");
  }
}


void displayfunction()
{  
  gotFrame = true;
}


void handleOTAs(void* parameter)
{
  for (;;)
  {
    delay(100);
    #ifdef USING_SERIALOTA
    SerialOTAhandle();
    #endif
    ArduinoOTA.handle();
  }
}


void debugInfo(void* parameter)
{
  for (;;)
  {
    delay(60000);
    Both.println();
    Both.print(String(artnet.frameslues) + " full frames, " + artnet.lostframes + " incomplete frames, " + (float)(artnet.lostframes*100)/(artnet.frameslues + artnet.lostframes) + " % lost\n\r");
    printReconnectHistory();
  }
}


template <typename T1, typename T2>
void readBuffer(DynamicJsonDocument jsonBuffer, T1& name, T2& variable)
{
  if (jsonBuffer.containsKey(name))
  {
    variable = jsonBuffer[name];
    Both.println(String(name) + " = " + variable);
  }
  else
  {
    Both.println(String(name) + " not found");
  }
}
  

bool loadConfig()
{
  Both.println("Mounting filesystem for reading");
  #ifdef USING_SPIFFS
  if (!SPIFFS.begin())
  #endif
  #ifndef USING_SPIFFS
  if (!LITTLEFS.begin())
  #endif
  {
    Both.println("Failed to mount file system");
    return false;
  }

  #ifdef USING_SPIFFS
  File configFile = SPIFFS.open(CONFIG_FILE_NAME, FILE_READ);
  #endif
  #ifndef USING_SPIFFS
  File configFile = LITTLEFS.open(CONFIG_FILE_NAME, FILE_READ);
  #endif
  if (!configFile) {
    Both.println("Failed to open config file");
    return false;
  }

  if (configFile.size() > maxFileSize) {
    Both.println("Config file size is too large");
    return false;
  }

  // Allocate the memory pool on the heap.
  // Use arduinojson.org/assistant to compute the capacity.
  DynamicJsonDocument jsonBuffer(maxFileSize);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(jsonBuffer, configFile);

  // Test if parsing succeeds.
  if (error) {
    Both.print(F("deserializeJson() failed: "));
    Both.println(error.f_str());
    return false;
  }

  if (jsonBuffer.containsKey("ssid"))
  {
    String s = jsonBuffer["ssid"];
    s.toCharArray(ssid, 64);
    ssid[63] = '\n';        // this is here so we don't accidentally pass a char array that never ends
    Both.println(String("ssid = ") + ssid);
  }
  
  if (jsonBuffer.containsKey("psk"))
  {
    String s = jsonBuffer["psk"];
    s.toCharArray(psk, 64);
    psk[63] = '\n';        // this is here so we don't accidentally pass a char array that never ends
    Both.println(String("psk = ") + "********");
  }
  
  if (jsonBuffer.containsKey("hostname"))
  {
    String s = jsonBuffer["hostname"];
    s.toCharArray(hostname, 64);
    hostname[63] = '\n';        // this is here so we don't accidentally pass a char array that never ends
    Both.println(String("hostname = ") + hostname);
  }
  
  if (jsonBuffer.containsKey("OTApassword"))
  {
    String s = jsonBuffer["OTApassword"];
    s.toCharArray(OTApassword, 64);
    OTApassword[63] = '\n';        // this is here so we don't accidentally pass a char array that never ends
    Both.println(String("OTApassword = ") + "********");
  }
  
  readBuffer(jsonBuffer, "ledWidth", ledWidth);
  readBuffer(jsonBuffer, "ledHeight", ledHeight);
  readBuffer(jsonBuffer, "universeSize", universeSize);
  readBuffer(jsonBuffer, "maxCurrent", maxCurrent);
  readBuffer(jsonBuffer, "maxBrightness", maxBrightness);
  readBuffer(jsonBuffer, "atxOnEnabled", atxOnEnabled);
  readBuffer(jsonBuffer, "atxOnPin", atxOnPin);
  readBuffer(jsonBuffer, "turnOffDelay", turnOffDelay);
  readBuffer(jsonBuffer, "OTArounds", OTArounds);
  readBuffer(jsonBuffer, "staticIpEnabled", staticIpEnabled);
  readBuffer(jsonBuffer, "serialEnabled", serialEnabled);

  JsonArray jsonPins = jsonBuffer["pins"];
  copyArray(jsonPins, pins);
  
  JsonArray jsonIp = jsonBuffer["ip"];
  copyArray(jsonIp, ip);
  
  JsonArray jsonGateway = jsonBuffer["gateway"];
  copyArray(jsonGateway, gateway);
  
  JsonArray jsonMask = jsonBuffer["mask"];
  copyArray(jsonMask, mask);
    
  Serial.print("pins = {");
  for (int index = 0; index < 64; index++)
  {
    Serial.print(String(pins[index]) + ", ");
  }
  Serial.println("}");

  Serial.print("ip = {");
  for (int index = 0; index < 4; index++)
  {
    Serial.print(String(ip[index]) + ", ");
  }
  Serial.println("}");

  Serial.print("gateway = {");
  for (int index = 0; index < 4; index++)
  {
    Serial.print(String(gateway[index]) + ", ");
  }
  Serial.println("}");

  Serial.print("mask = {");
  for (int index = 0; index < 4; index++)
  {
    Serial.print(String(mask[index]) + ", ");
  }
  Serial.println("}");

  #ifdef DEBUG
  serializeJson(jsonBuffer, Serial);
  #endif
  
  configFile.close();

  return true;
}


void flipOnAndOf(void* parameter)
{
  int beenOnFor = 0;
  int beenOffFor = 0;
  delay(2000);
  pinMode(atxOnPin, OUTPUT);
  digitalWrite(atxOnPin, 1);
  delay(2000);
  for (;;)
  {
    delay(1000);
    if (gotFrame == true)
    {
      beenOnFor++;
      beenOffFor = 0;
    }
    else
    {
      beenOffFor++;
      beenOnFor = 0;
    }
    gotFrame = false;
    if (beenOffFor == turnOffDelay)     // we wait a full minute for the artnet signal to return before 
    {
      digitalWrite(atxOnPin, 1);
      Both.println("ATX_OFF");
    }
    if (beenOnFor == 1)
    {
      digitalWrite(atxOnPin, 0);
      Both.println("ATX_ON");
    }
    // if (beenOnFor > 10000000) beenOnFor = 10000000;                     // these are here to prevent overflow. useless really but who cares? not me. that was a rhetorical question but whatever.
    // if (beenOffFor > 10000000) beenOffFor = 10000000;
  }
}

unsigned int limitCurrent(uint8_t *leds, unsigned int numLeds, unsigned int maxCurrent, unsigned int maxBrightness)
{
  unsigned int sum = 0;
  unsigned int brightness = 255;
  // std::accumulate(&leds[0], &leds[numLeds * 3], sum);
  for (int i = 0; i < numLeds * 3; i++) sum += leds[i];
  unsigned int mA = sum / 13;                   // 765 / 60 = 12.75    765 is 255 * 3 and that is 60 milliamps. from this we know we can get the milliamps by dividing the sum off all leds by 13
  // Both.println(mA);
  if (mA > maxCurrent)
  {
    brightness = maxCurrent * 255 / mA;
  }
  if (brightness < maxBrightness)
  {
    // Both.println(String("Brightness set to: ") + brightness);
    return brightness;
  }
  else return maxBrightness;
}

void setup()
{
  Serial.begin(115200);
  
  Both.println("Booting");
  
  changeSettings();
  
  loadConfig();
  
  numLeds = ledWidth * ledHeight;           // this has to be done after loading config
  
  if (staticIpEnabled)
  {
    WiFi.config(IPAddress(ip[0], ip[1], ip[2], ip[3]), IPAddress(gateway[0], gateway[1], gateway[2], gateway[3]), IPAddress(mask[0], mask[1], mask[2], mask[3]), IPAddress(gateway[0], gateway[1], gateway[2], gateway[3]));
  }
  WiFi.setHostname(hostname);
  setupWifi(ssid, psk);
  
  Both.println("Ready");
  Both.print("IP address: ");
  Both.println(WiFi.localIP());
  Both.print("MAC Address: ");
  Both.println(WiFi.macAddress());
  
  setupOTA(hostname, OTApassword, OTArounds);
  
  #ifdef USING_SERIALOTA
  setupSerialOTA(hostname);
  #endif
  
  xTaskCreatePinnedToCore(
      handleOTAs, // Function to implement the task
      "handleOTAs", // Name of the task
      10000,  // Stack size in words
      NULL,  // Task input parameter
      0,  // Priority of the task
      &task3,  // Task handle.
      0); // Core where the task should run
    
  randomSeed(esp_random());
  // set_max_power_in_volts_and_milliamps(5, maxCurrent);   // a relic from the time of FastLED
  
  ledsArtnet = (uint8_t *)malloc(sizeof(uint8_t) * numLeds * 3);
  artnet.setFrameCallback(&displayfunction); //set the function that will be called back a frame has been received
  
  artnet.setLedsBuffer((uint8_t*)ledsArtnet); //set the buffer to put the frame once a frame has been received
    
  artnet.begin(numLeds, universeSize); //configure artnet
  
  if (!serialEnabled)
  {
    Serial.println("Disabling serial so I can use those two pins for something else");
    Serial.end();
  }
  
  driver.initled((uint8_t*)ledsArtnet, pins, ledWidth, ledHeight, ORDER_GRB);
  driver.setBrightness(maxBrightness);
  
  delay(2000);
  
  if (atxOnEnabled)
  {
      xTaskCreatePinnedToCore(
          flipOnAndOf, // Function to implement the task
          "flipOnAndOf", // Name of the task
          10000,  // Stack size in words
          NULL,  // Task input parameter
          0,  // Priority of the task
          &task1,  // Task handle.
          0); // Core where the task should run
  }
  
  xTaskCreatePinnedToCore(
        debugInfo, // Function to implement the task
        "debugInfo", // Name of the task
        10000,  // Stack size in words
        NULL,  // Task input parameter
        0,  // Priority of the task
        &task2,  // Task handle.
        0); // Core where the task should run
  
}

void loop()
{
  artnet.readFrame(); //ask to read a full frame
  driver.setBrightness(limitCurrent(ledsArtnet, numLeds, maxCurrent, maxBrightness));
  driver.showPixels((uint8_t*)ledsArtnet);
}
