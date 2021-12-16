#ifndef SETUPWIFI_H
#define SETUPWIFI_H

#include <myCredentials.h>
#include "settings.h"

#include <WiFi.h>

#ifdef USING_SERIALOTA
#include "SerialOTA.h"
#endif

void reconnectToWifiIfNecessary();
void setupWifi(char* primarySsid, char* primaryPsk);
void setupWifi();

#endif
