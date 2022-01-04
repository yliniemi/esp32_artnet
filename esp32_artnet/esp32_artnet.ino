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
    Both.print(String("nb frames read: ") + artnet.frameslues + "  nb of incomplete frames: " + artnet.lostframes + "lost: " + (float)(artnet.lostframes*100)/artnet.frameslues + "\n\r");
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

  const int maxFileSize = 4096;
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

  // Allocate the memory pool on the stack.
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
  
  /*
  if (jsonBuffer.containsKey("maxCurrent"))
  {
    maxCurrent = jsonBuffer["maxCurrent"];
    Both.println(String("maxCurrent") + " = " + maxCurrent);
  }
  
  if (jsonBuffer.containsKey("universeSize"))
  {
    universeSize = jsonBuffer["universeSize"];
    Both.println(String("universeSize") + " = " + universeSize);
  }
  
  if (jsonBuffer.containsKey("OTArounds"))
  {
    OTArounds = jsonBuffer["OTArounds"];
    Both.println(String("OTArounds") + " = " + OTArounds);
  }
  
  if (jsonBuffer.containsKey("ledWidth"))
  {
    OTArounds = jsonBuffer["ledWidth"];
    Both.println(String("ledWidth") + " = " + OTArounds);
  }
  */

  readBuffer(jsonBuffer, "ledWidth", ledWidth);
  readBuffer(jsonBuffer, "ledHeight", ledHeight);
  readBuffer(jsonBuffer, "universeSize", universeSize);
  readBuffer(jsonBuffer, "maxCurrent", maxCurrent);
  readBuffer(jsonBuffer, "maxBrightness", maxBrightness);
  readBuffer(jsonBuffer, "atxOnEnabled", atxOnEnabled);
  readBuffer(jsonBuffer, "atxOnPin", atxOnPin);
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
    if (beenOffFor == 60)     // we wait a full minute for the artnet signal to return before 
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
  
  Both.println("4");
  
  
}

void loop()
{
  artnet.readFrame(); //ask to read a full frame
  driver.setBrightness(limitCurrent(ledsArtnet, numLeds, maxCurrent, maxBrightness));
  driver.showPixels((uint8_t*)ledsArtnet);
}
