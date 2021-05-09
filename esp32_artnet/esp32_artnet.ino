#include "settings.h"
#include <myCredentials.h>        // oh yeah. there is myCredentials.zip on the root of this repository. include it as a library and then edit the file with your onw ips and stuff

#include "setupWifi.h"
#include "OTA.h"

#ifdef USING_SERIALOTA
#include "SerialOTA.h"
#endif

#include <ArtnetESP32.h>

#include "FastLED.h"
FASTLED_USING_NAMESPACE

//The following has to be adapted to your specifications
#define NUM_LEDS LED_WIDTH*LED_HEIGHT
#define UNIVERSE_SIZE 170 //my setup is 170 leds per universe no matter if the last universe is not full.
CRGB leds[NUM_LEDS];

int maxCurrent = MAX_CURRENT;         // in milliwatts. can be changed later on with mqtt commands. be careful with this one. it might be best to disable this funvtionality altogether


ArtnetESP32 artnet;


void displayfunction()
{
  if (artnet.frameslues%100==0)
  {
    Serial.println(String("micros() = ") + micros());
    Serial.printf("nb frames read: %d  nb of incomplete frames:%d lost:%.2f %%\n\r",artnet.frameslues,artnet.lostframes,(float)(artnet.lostframes*100)/artnet.frameslues);
    //here the buffer is the led array hence a simple FastLED.show() is enough to display the array
  }
  
  // this is here so that we don't call Fastled.show() too fast. things froze if we did that
  // perhaps I should use microseconds here. I could shave off a couple of milliseconds
  // unsigned long expectedTime = LED_HEIGHT * 24 * 11 / (800 * 10) + 2;     // 1 ms for the reset pulse and (takes 50 us. better safe than sorry) 1 ms rounding 11/10 added 10 % extra just to be on the safe side
  static unsigned long expectedTime = LED_HEIGHT * 24 * 11 / 8 + 500;     // 500 us for the reset pulse and (takes 50 us. better safe than sorry) also added 10 % extra just to be on the safe side
  
  static unsigned long oldMicros = 0;
  unsigned long frameTime = micros() - oldMicros;
  if (frameTime > 6000000) delayMicroseconds(expectedTime);
  if (frameTime < expectedTime) delayMicroseconds(expectedTime - frameTime);
  oldMicros = micros();
   
  FastLED.show();
}


void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");
  
  setupWifi();
  
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  setupOTA();
  
  #ifdef USING_SERIALOTA
  setupSerialOTA();
  #endif

  FastLED.addLeds<NEOPIXEL, PIN_0>(leds, 0*LED_HEIGHT, LED_HEIGHT);
  
  #if LED_WIDTH > 1
  FastLED.addLeds<NEOPIXEL, PIN_1>(leds, 1*LED_HEIGHT, LED_HEIGHT);
  #endif
  #if LED_WIDTH > 2
  FastLED.addLeds<NEOPIXEL, PIN_2>(leds, 2*LED_HEIGHT, LED_HEIGHT);
  #endif
  #if LED_WIDTH > 3
  FastLED.addLeds<NEOPIXEL, PIN_3>(leds, 3*LED_HEIGHT, LED_HEIGHT);
  #endif
  #if LED_WIDTH > 4
  FastLED.addLeds<NEOPIXEL, PIN_4>(leds, 4*LED_HEIGHT, LED_HEIGHT);
  #endif
  #if LED_WIDTH > 5
  FastLED.addLeds<NEOPIXEL, PIN_5>(leds, 5*LED_HEIGHT, LED_HEIGHT);
  #endif
  #if LED_WIDTH > 6
  FastLED.addLeds<NEOPIXEL, PIN_6>(leds, 6*LED_HEIGHT, LED_HEIGHT);
  #endif
  #if LED_WIDTH > 7
  FastLED.addLeds<NEOPIXEL, PIN_7>(leds, 7*LED_HEIGHT, LED_HEIGHT);
  #endif
  
  randomSeed(esp_random());
  set_max_power_in_volts_and_milliamps(5, maxCurrent);   // in my current setup the maximum current is 50A
  
  artnet.setFrameCallback(&displayfunction); //set the function that will be called back a frame has been received
  artnet.setLedsBuffer((uint8_t*)leds); //set the buffer to put the frame once a frame has been received

  artnet.begin(NUM_LEDS,UNIVERSE_SIZE); //configure artnet


}

void loop()
{
  reconnectToWifiIfNecessary();
  SerialOTAhandle();
  ArduinoOTA.handle();

  
  artnet.readFrame(); //ask to read a full frame
}
