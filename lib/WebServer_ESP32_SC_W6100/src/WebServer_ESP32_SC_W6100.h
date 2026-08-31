/****************************************************************************************************************************
  WebServer_ESP32_SC_W6100.h

  For Ethernet shields using ESP32_SC_W6100 (ESP32_S2/S3/C3 + LwIP W6100)

  WebServer_ESP32_SC_W6100 is a library for the ESP32_S2/S3/C3 with LwIP Ethernet W6100

  Based on and modified from ESP8266 https://github.com/esp8266/Arduino/releases
  Built by Khoi Hoang https://github.com/khoih-prog/WebServer_ESP32_SC_W6100
  Licensed under GPLv3 license

  Version: 1.2.1

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.2.1   K Hoang      08/01/2023 Initial coding for ESP32_SC_W6100 (ESP32_SC + W6100)
 *****************************************************************************************************************************/

#pragma once

#ifndef WEBSERVER_ESP32_SC_W6100_H
#define WEBSERVER_ESP32_SC_W6100_H

//////////////////////////////////////////////////////////////

#if ( ( defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2) ) && ( ARDUINO_ESP32_GIT_VER != 0x46d5afb1 ) )
  #define USING_CORE_ESP32_CORE_V200_PLUS      true

  #if (_ETHERNET_WEBSERVER_LOGLEVEL_ > 3)
    #warning Using code for ESP32 core v2.0.0+ in WebServer_ESP32_SC_W6100.h
  #endif

  #define WEBSERVER_ESP32_SC_W6100_VERSION       "WebServer_ESP32_SC_W6100 v1.2.1 for core v2.0.0+"
#else
  #if (_ETHERNET_WEBSERVER_LOGLEVEL_ > 3)
    #warning Using code for ESP32 core v1.0.6- in WebServer_ESP32_SC_W6100.h
  #endif

  #define WEBSERVER_ESP32_SC_W6100_VERSION       "WebServer_ESP32_SC_W6100 v1.2.1 for core v1.0.6-"
#endif

//////////////////////////////////////////////////////////////

#define WEBSERVER_ESP32_SC_W6100_VERSION_MAJOR    1
#define WEBSERVER_ESP32_SC_W6100_VERSION_MINOR    2
#define WEBSERVER_ESP32_SC_W6100_VERSION_PATCH    1

#define WEBSERVER_ESP32_SC_W6100_VERSION_INT      1002001

/////////////////////////////////////////////////////////////

#include <Arduino.h>

#include "WebServer_ESP32_SC_W6100_Debug.h"

//////////////////////////////////////////////////////////////

#include "w6100/esp32_sc_w6100.h"

#include "WebServer_ESP32_SC_W6100.hpp"
#include "WebServer_ESP32_SC_W6100_Impl.h"

//////////////////////////////////////////////////////////////

#endif    // WEBSERVER_ESP32_SC_W6100_H
