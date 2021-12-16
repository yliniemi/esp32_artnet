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

// #include "FastLED.h"
// FASTLED_USING_NAMESPACE
#include "I2SClocklessLedDriver.h"
I2SClocklessLedDriver driver;

//The following has to be adapted to your specifications
#define NUM_LEDS LED_WIDTH*LED_HEIGHT

//int pins[] = {4, 16, 17, 5, 18, 19, 21, 3, 1};
int pins[] = {4, 16, 17, 5, 18, 19, 21, 22, 23, 13, 12, 14, 27, 26, 33, 32, 25, 15, 3, 1, 2};
volatile int gotFrame = false;

uint8_t *ledsArtnet = NULL;

int maxCurrent = MAX_CURRENT;         // in milliwatts. can be changed later on with mqtt commands. be careful with this one. it might be best to disable this funvtionality altogether
int universeSize = UNIVERSE_SIZE;

ArtnetESP32 artnet;

TaskHandle_t task1, task2, task3;

char primarySsid[64];
char primaryPsk[64];
char hostname[64] = HOSTNAME;


void displayfunction()
{  
  gotFrame = true;
}


void maintenance(void* parameter)
{
  while(1)
  {
    reconnectToWifiIfNecessary();
    SerialOTAhandle();
    ArduinoOTA.handle();
   // displayFunction();
    static unsigned long previousTime = 0;
  
    if ((millis() - previousTime > 60000) || (millis() < previousTime))
    {
      previousTime = millis();
      Serial.println();
      Serial.printf("nb frames read: %d  nb of incomplete frames:%d lost:%.2f %%\n\r",artnet.frameslues,artnet.lostframes,(float)(artnet.lostframes*100)/artnet.frameslues);
      #ifdef USING_SERIALOTA
      SerialOTA.println();
      SerialOTA.printf("nb frames read: %d  nb of incomplete frames:%d lost:%.2f %%\n\r",artnet.frameslues,artnet.lostframes,(float)(artnet.lostframes*100)/artnet.frameslues);
      #endif
    }
    vTaskDelay(10000);    
  }
}


bool loadConfig()
{
  //allows serving of files from SPIFFS
  Serial.println("Mounting FS...");
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return false;
  }

  File configFile = SPIFFS.open(CONFIG_FILE_NAME, "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  if (configFile.size() > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate the memory pool on the stack.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<1024> jsonBuffer;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(jsonBuffer, configFile);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }

  // Copy values from the JsonObject to the Config
  
  if (jsonBuffer.containsKey("ssid"))
  {
    String stringSsid = jsonBuffer["ssid"];
    stringSsid.toCharArray(primarySsid, 64);
    Serial.println(String("primarySsid") + " = " + primarySsid);
  }
  
  if (jsonBuffer.containsKey("psk"))
  {
    String stringPsk = jsonBuffer["psk"];
    stringPsk.toCharArray(primaryPsk, 64);
    Serial.println(String("primaryPsk") + " = " + "********");
  }
  
  if (jsonBuffer.containsKey("hostname"))
  {
    String stringPsk = jsonBuffer["hostname"];
    stringPsk.toCharArray(hostname, 64);
    Serial.println(String("hostname") + " = " + hostname);
  }
  
  if (jsonBuffer.containsKey("maxCurrent"))
  {
    maxCurrent = jsonBuffer["maxCurrent"];
    Serial.println(String("maxCurrent") + " = " + maxCurrent);
  }
  
  if (jsonBuffer.containsKey("universeSize"))
  {
    universeSize = jsonBuffer["universeSize"];
    Serial.println(String("universeSize") + " = " + universeSize);
  }
  
  // We don't need the file anymore
  
  configFile.close();

  return true;
}


void flipOnAndOf(void* parameter)
{
  int beenOnFor = 0;
  int beenOffFor = 0;
  delay(2000);
  pinMode(ATX_ON, OUTPUT);
  digitalWrite(ATX_ON, 1);
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
    if (beenOffFor == 60)
    {
      digitalWrite(ATX_ON, 1);
      Serial.println("ATX_OFF");
    }
    if (beenOnFor == 20)
    {
      digitalWrite(ATX_ON, 0);
      Serial.println("ATX_ON");
    }
    if (beenOnFor > 10000000) beenOnFor = 10000000;                     // these are here to prevent overflow. useless really but who cares? not me. that was a rhetorical question but whatever.
    if (beenOffFor > 10000000) beenOffFor = 10000000;
  }
}

unsigned int limitCurrent(uint8_t *leds, unsigned int numLeds, unsigned int maxCurrent, unsigned int maxBrightness)
{
  unsigned int sum = 0;
  unsigned int brightness = 255;
  // std::accumulate(&leds[0], &leds[numLeds * 3], sum);
  for (int i = 0; i < numLeds * 3; i++) sum += leds[i];
  unsigned int mA = sum / 13;                   // 765 / 60 = 12.75    765 is 255 * 3 and that is 60 milliamps. from this we know we can get the milliamps by dividing the sum off all leds by 13
  // Serial.println(mA);
  if (mA > maxCurrent)
  {
    brightness = maxCurrent * 255 / mA;
  }
  if (brightness < maxBrightness)
  {
    Serial.println(String("Brightness set to: ") + brightness);
    return brightness;
  }
  else return maxBrightness;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");

  primarySsid[0] = 0;
  primaryPsk[0] = 0; 
  
  loadConfig();
  
  setupWifi(primarySsid, primaryPsk);
  
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  setupOTA(hostname);
  
  #ifdef USING_SERIALOTA
  setupSerialOTA(hostname);
  #endif
  
  Serial.println("0");
    
  randomSeed(esp_random());
  // set_max_power_in_volts_and_milliamps(5, maxCurrent);   // in my current setup the maximum current is 50A
  
  ledsArtnet = (uint8_t *)malloc(sizeof(uint8_t) * NUM_LEDS * 3);
  artnet.setFrameCallback(&displayfunction); //set the function that will be called back a frame has been received
  
  Serial.println("1");
  
  artnet.setLedsBuffer((uint8_t*)ledsArtnet); //set the buffer to put the frame once a frame has been received
  
  Serial.println("2");
  
  artnet.begin(NUM_LEDS, universeSize); //configure artnet

  /*
  artnet.begin(NUM_LEDS, universeSize); //configure artnet
  buffer_number=0;
  xTaskCreatePinnedToCore(
      cycleLedStrips, // Function to implement the task
      "cycleLedStrips", // Name of the task
      10000,  // Stack size in words
      NULL,  // Task input parameter
      0,  // Priority of the task
      &task2,  // Task handle.
      0); // not core 1
  */
  
  driver.initled((uint8_t*)ledsArtnet, pins, LED_WIDTH, LED_HEIGHT, ORDER_GRB);
  driver.setBrightness(255);
  
  Serial.println("3");
  
  xTaskCreatePinnedToCore(
      maintenance, // Function to implement the task
      "maintenance", // Name of the task
      10000,  // Stack size in words
      NULL,  // Task input parameter
      0,  // Priority of the task
      &task1,  // Task handle.
      0); // not core 1

  delay(2000);
  
  xTaskCreatePinnedToCore(
      flipOnAndOf, // Function to implement the task
      "flipOnAndOf", // Name of the task
      10000,  // Stack size in words
      NULL,  // Task input parameter
      0,  // Priority of the task
      &task2,  // Task handle.
      0); // Core where the task should run
  ;
  
  Serial.println("4");
  
  
}

void loop()
{
  artnet.readFrame(); //ask to read a full frame
  driver.setBrightness(limitCurrent(ledsArtnet, NUM_LEDS, MAX_CURRENT, 255));
  driver.showPixels((uint8_t*)ledsArtnet);
}
