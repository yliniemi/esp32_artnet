#include <myCredentials.h>        // oh yeah. there is myCredentials.zip on the root of this repository. include it as a library and then edit the file with your onw ips and stuff
#include "settings.h"

#include "setupWifi.h"
#include "OTA.h"

#ifdef USING_SERIALOTA
#include "SerialOTA.h"
#endif

#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// #define ARTNET_NO_TIMEOUT
#define ARTNET_ESP32_DEBUG_DISABLED
#include <ArtnetESP32.h>

// extern String WifiReconnectedAt;

// #include "FastLED.h"
// FASTLED_USING_NAMESPACE
#include "I2SClocklessLedDriver.h"
I2SClocklessLedDriver driver;

volatile int gotFrame = false;

uint8_t *ledsArtnet = NULL;

ArtnetESP32 artnet;

TaskHandle_t task1, task2, task3;

char primarySsid[64];
char primaryPsk[64];
char hostname[64] = HOSTNAME;


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
  //allows serving of files from SPIFFS
  
  Both.println("Mounting FS...");
  if (!SPIFFS.begin()) {
    Both.println("Failed to mount file system");
    return false;
  }

  File configFile = SPIFFS.open(CONFIG_FILE_NAME, FILE_WRITE);
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
  jsonBuffer["ssid"] = String(primarySsid);
  
  serializeJsonPretty(jsonBuffer, Serial);
  Serial.println();
  
  jsonBuffer["psk"] = String(primaryPsk);
  serializeJson(jsonBuffer, configFile);

  // We don't need the file anymore
  
  configFile.close();

  return true;
}

void changeSettings(void* parameter)
{
  String s;
  delay(10000);
  for (;;)
  {
    Serial.setTimeout(1000000000);                 // normally readStringUntil waits only a second
    Serial.println();
    Serial.println("If you want to change the settings type 'settings'");
    s = Serial.readStringUntil('\n');
    Serial.println(String("You typed: " + s));
    if (s.equals("settings"))
    {
      Serial.println("You are now in settings mode");
      Serial.println("When you're done making changes and want to save them on the flash rom type 'save'");
      Serial.println("If you made a mistake, you can cancel it by typing 'cancel'");
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
      Serial.println(String("ssid = ") + primarySsid);
      Serial.println(String("psk = ") + primaryPsk);

      for (;;)
      {
        Serial.println();
        Serial.println("Please type a variable name");
        s = Serial.readStringUntil('\n');
        Serial.println(String("You typed: " + s));
        
        if (s.equals("save"))
        {
          Serial.println("Saving your settings to the flash rom");
          saveConfig();
          break;
        }
        
        else if (s.equals("cancel"))
        {
          Serial.println("Cancelled saving the settings");
          Serial.println("Settings on ram are still changed. If you want to revert back to the old settings please reboot");
          break;
        }
        
        else if (s.equals("ssid"))
        {
          s = Serial.readStringUntil('\n');
          s.toCharArray(primarySsid, 64);
          Serial.println(String("ssid = ") + primarySsid);
        }
        
        else if (s.equals("psk"))
        {
          s = Serial.readStringUntil('\n');
          s.toCharArray(primaryPsk, 64);
          Serial.println(String("primaryPsk = ") + primaryPsk);
        }
        
        else if (s.equals("hostname"))
        {
          s = Serial.readStringUntil('\n');
          s.toCharArray(hostname, 64);
          Serial.println(String("hostname = ") + hostname);
        }
        
        else if (s.equals("ledWidth"))
        {
          s = Serial.readStringUntil('\n');
          ledWidth = s.toInt();
          Serial.println(String("ledWidth = ") + ledWidth);
        }
        
        else if (s.equals("ledHeight"))
        {
          s = Serial.readStringUntil('\n');
          ledHeight = s.toInt();
          Serial.println(String("ledHeight = ") + ledHeight);
        }
        
        else if (s.equals("universeSize"))
        {
          s = Serial.readStringUntil('\n');
          universeSize = s.toInt();
          Serial.println(String("universeSize = ") + universeSize);
        }
        
        else if (s.equals("maxCurrent"))
        {
          s = Serial.readStringUntil('\n');
          maxCurrent = s.toInt();
          Serial.println(String("maxCurrent = ") + maxCurrent);
        }
        
        else if (s.equals("maxBrightness"))
        {
          s = Serial.readStringUntil('\n');
          maxBrightness = s.toInt();
          Serial.println(String("maxBrightness = ") + maxBrightness);
        }
        
        else if (s.equals("atxOnEnabled"))
        {
          s = Serial.readStringUntil('\n');
          if (s.equals("true") || s.equals("1")) atxOnEnabled = true;
          else atxOnEnabled = false;
          Serial.println(String("atxOnEnabled = ") + atxOnEnabled);
        }
        
        else if (s.equals("atxOnPin"))
        {
          s = Serial.readStringUntil('\n');
          atxOnPin = s.toInt();
          Serial.println(String("atxOnPin = ") + atxOnPin);
        }
        
        else if (s.equals("turnOffDelay"))
        {
          s = Serial.readStringUntil('\n');
          turnOffDelay = s.toInt();
          Serial.println(String("turnOffDelay = ") + turnOffDelay);
        }
        
        else if (s.equals("OTArounds"))
        {
          s = Serial.readStringUntil('\n');
          OTArounds = s.toInt();
          Serial.println(String("OTArounds = ") + OTArounds);
        }
        
        else Serial.println("Unknown variable name");
        Serial.println();
        Serial.println();
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
    SerialOTAhandle();
    ArduinoOTA.handle();
  }
}


void debugInfo(void* parameter)
{
  for (;;)
  {
    delay(60000);
    Both.println();
    Both.print(String("nb frames read: ") + artnet.frameslues + " nb of incomplete frames: " + artnet.lostframes + " lost: " + (float)(artnet.lostframes*100)/artnet.frameslues + "\n\r");
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
  //allows serving of files from SPIFFS

  Both.println("Mounting FS...");
  if (!SPIFFS.begin()) {
    Both.println("Failed to mount file system");
    return false;
  }

  File configFile = SPIFFS.open(CONFIG_FILE_NAME, "r");
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

  // Copy values from the JsonObject to the Config
  
  if (jsonBuffer.containsKey("ssid"))
  {
    String stringSsid = jsonBuffer["ssid"];
    stringSsid.toCharArray(primarySsid, 64);
    primarySsid[63] = '\n';        // this is here so we don't accidentally pass a char array that never ends
    Both.println(String("primarySsid") + " = " + primarySsid);
  }
  
  if (jsonBuffer.containsKey("psk"))
  {
    String stringPsk = jsonBuffer["psk"];
    stringPsk.toCharArray(primaryPsk, 64);
    primaryPsk[63] = '\n';        // this is here so we don't accidentally pass a char array that never ends
    Both.println(String("primaryPsk") + " = " + "********");
  }
  
  if (jsonBuffer.containsKey("hostname"))
  {
    String stringPsk = jsonBuffer["hostname"];
    stringPsk.toCharArray(hostname, 64);
    hostname[63] = '\n';        // this is here so we don't accidentally pass a char array that never ends
    Both.println(String("hostname") + " = " + hostname);
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

  // We don't need the file anymore
  
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

  primarySsid[0] = 0;
  primaryPsk[0] = 0; 
  
  loadConfig();

  numLeds = ledWidth * ledHeight;           // this has to be done after loading config
  
  setupWifi(primarySsid, primaryPsk);
  
  Both.println("Ready");
  Both.print("IP address: ");
  Both.println(WiFi.localIP());
  
  setupOTA(hostname, OTA_PASSWORD, OTArounds);
  
  #ifdef USING_SERIALOTA
  setupSerialOTA(hostname);
  #endif
  
  #ifdef USING_SERIALOTA
  xTaskCreatePinnedToCore(
      handleOTAs, // Function to implement the task
      "handleOTAs", // Name of the task
      10000,  // Stack size in words
      NULL,  // Task input parameter
      0,  // Priority of the task
      &task3,  // Task handle.
      0); // Core where the task should run
  #endif

  
  xTaskCreatePinnedToCore(
        changeSettings, // Function to implement the task
        "changeSettings", // Name of the task
        10000,  // Stack size in words
        NULL,  // Task input parameter
        0,  // Priority of the task
        &task3,  // Task handle.
        0); // Core where the task should run
    
  //changeSettings();
  
  Both.println("0");
    
  randomSeed(esp_random());
  // set_max_power_in_volts_and_milliamps(5, maxCurrent);   // in my current setup the maximum current is 50A
  
  ledsArtnet = (uint8_t *)malloc(sizeof(uint8_t) * numLeds * 3);
  artnet.setFrameCallback(&displayfunction); //set the function that will be called back a frame has been received
  
  Both.println("1");
  
  artnet.setLedsBuffer((uint8_t*)ledsArtnet); //set the buffer to put the frame once a frame has been received
  
  Both.println("2");
  
  artnet.begin(numLeds, universeSize); //configure artnet

  driver.initled((uint8_t*)ledsArtnet, pins, ledWidth, ledHeight, ORDER_GRB);
  driver.setBrightness(maxBrightness);
  
  Both.println("3");
  
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
    
  Both.println(FILE_WRITE);
  Both.println(FILE_READ);
  
  
}

void loop()
{
  artnet.readFrame(); //ask to read a full frame
  driver.setBrightness(limitCurrent(ledsArtnet, numLeds, maxCurrent, maxBrightness));
  driver.showPixels((uint8_t*)ledsArtnet);
}
