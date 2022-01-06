#define SETUPWIFI_CPP
#include "setupWifi.h"

#define TRY_RECONNECTING 5      // when losing wifi try WiFi.reconnect() this many time
#define TRY_DISCONNECTING 25    // if reconnecting doesn't work try first disconnecting and then doing WiFi.begin()

char* currentSsid;
char* currentPsk;

String WifiReconnectedAt = "";

TaskHandle_t task;

void reconnectToWifiIfNecessary(void* parameter)
{
  for (;;)
  {
    delay(10000);
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println();
      Serial.println("WiFi is disconnected");
      WifiReconnectedAt += WiFi.status();

      for (int i = 0; i < TRY_RECONNECTING && WiFi.status() != WL_CONNECTED; i++)
      {
        WiFi.reconnect();
        Serial.println();
        Serial.println("Trying to reconnect");
        delay(10000);
        WifiReconnectedAt += WiFi.status();
      }
      
      for (int i = 0; i < TRY_DISCONNECTING && WiFi.status() != WL_CONNECTED; i++)
      {
        WiFi.disconnect();
        delay(1000);
        WiFi.mode(WIFI_STA);
        WiFi.begin(currentSsid, currentPsk);
        Serial.println();
        Serial.print("Trying to connect to ");
        Serial.println(currentSsid);
        delay(9000);
        WifiReconnectedAt += WiFi.status();
      }
      
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println();
        Serial.println("WiFi is gone for good. Giving up. Nothing matters. I hope I can do better in my next life. Rebooting....");
      }
      int time = millis();
      WifiReconnectedAt += String(" WiFi reconnected at: ") + time / (1000 * 60 * 60) + ":" + time / (1000 * 60) % 60 + ":" + time / 1000 % 60 + "\r\n";
      void printReconnectHistory();
    }
  }
}

void printReconnectHistory()
{
  Serial.println();
  Serial.println(WifiReconnectedAt);
  #ifdef USING_SERIALOTA
  SerialOTA.println();
  SerialOTA.println(WifiReconnectedAt);
  #endif
}

void setupWifi(char* primarySsid, char* primaryPsk)
{
  
  if (primarySsid[0] != 0)
  {
    WiFi.disconnect();
    delay(500);
    currentSsid = primarySsid;
    currentPsk = primaryPsk;
    WiFi.mode(WIFI_STA);
    WiFi.begin(currentSsid, currentPsk);
    Serial.println();
    Serial.print("Trying to connect to ");
    Serial.println(currentSsid);
    delay(5000);
  }
  
  for (int i = 0; WiFi.waitForConnectResult() != WL_CONNECTED && wifiArray[i][0] != 0; i++)
  {
    WiFi.disconnect();
    delay(500);
    currentSsid = wifiArray[i][0];
    currentPsk = wifiArray[i][1];
    WiFi.mode(WIFI_STA);
    WiFi.begin(currentSsid, currentPsk);
    Serial.println();
    Serial.print("Trying to connect to ");
    Serial.println(currentSsid);
    delay(5000);
  }
  
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println();
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.print("Connection to ");
  Serial.print(currentSsid);
  Serial.println(" succeeded!");
  // WiFi.setAutoReconnect(true);  // this didn't work well enough. i had to do this another way
  WiFi.persistent(false);          // we don't want to save the credentials on the internal filessytem of the esp32
  // WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);   // this one was problematic too because you shouldn't do much right after trying to reconnect. perhaps fastled disabling interrupts does something nefarious

  xTaskCreatePinnedToCore(
      reconnectToWifiIfNecessary, // Function to implement the task
      "maintenance", // Name of the task
      10000,  // Stack size in words
      NULL,  // Task input parameter
      0,  // Priority of the task
      &task,  // Task handle.
      0); // not core 1
}

void setupWifi()
{
  setupWifi((char) 0, (char) 0);
}
