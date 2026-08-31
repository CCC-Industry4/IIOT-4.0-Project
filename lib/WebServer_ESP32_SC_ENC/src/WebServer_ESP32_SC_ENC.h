/****************************************************************************************************************************
  WebServer_ESP32_SC_ENC.h

  For Ethernet shields using ESP32_SC_ENC (ESP32_S2/S3/C3 + LwIP ENC28J60)

  WebServer_ESP32_SC_ENC28J60 is a library for the ESP32_S2/S3/C3 with LwIP Ethernet ENC28J60

  Based on and modified from ESP8266 https://github.com/esp8266/Arduino/releases
  Built by Khoi Hoang https://github.com/khoih-prog/WebServer_ESP32_SC_ENC
  Licensed under GPLv3 license

  Version: 1.2.1

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0   K Hoang      13/12/2022 Initial coding for ESP32_S3_ENC (ESP32_S3 + LwIP ENC28J60)
  1.1.0   K Hoang      19/12/2022 Add support to ESP32_S2_ENC (ESP32_S2 + LwIP ENC28J60)
  1.2.0   K Hoang      20/12/2022 Add support to ESP32_C3_ENC (ESP32_C3 + LwIP ENC28J60)
  1.2.1   K Hoang      11/01/2023 Increase default SPI clock to 20MHz from 8MHz
 *****************************************************************************************************************************/

#pragma once

#ifndef WEBSERVER_ESP32_SC_ENC_H
#define WEBSERVER_ESP32_SC_ENC_H

//////////////////////////////////////////////////////////////

#if ( ( defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2) ) && ( ARDUINO_ESP32_GIT_VER != 0x46d5afb1 ) )
  #define USING_CORE_ESP32_CORE_V200_PLUS      true

  #if (_ETHERNET_WEBSERVER_LOGLEVEL_ > 2)
    #warning Using code for ESP32 core v2.0.0+ in WebServer_ESP32_SC_ENC.h
  #endif

  #define WEBSERVER_ESP32_SC_ENC_VERSION       "WebServer_ESP32_SC_ENC v1.2.1 for core v2.0.0+"
#else
  #if (_ETHERNET_WEBSERVER_LOGLEVEL_ > 2)
    #warning Using code for ESP32 core v1.0.6- in WebServer_ESP32_SC_ENC.h
  #endif

  #define WEBSERVER_ESP32_SC_ENC_VERSION       "WebServer_ESP32_SC_ENC v1.2.1 for core v1.0.6-"
#endif

//////////////////////////////////////////////////////////////

#define WEBSERVER_ESP32_SC_ENC_VERSION_MAJOR    1
#define WEBSERVER_ESP32_SC_ENC_VERSION_MINOR    2
#define WEBSERVER_ESP32_SC_ENC_VERSION_PATCH    1

#define WEBSERVER_ESP32_SC_ENC_VERSION_INT      1002001

/////////////////////////////////////////////////////////////

#if ESP32
  #if (_ETHERNET_WEBSERVER_LOGLEVEL_ > 3)
    #warning Using ESP32 architecture for WebServer_ESP32_SC_ENC
  #endif

  #define BOARD_NAME      ARDUINO_BOARD
#else
  #error This code is designed to run on ESP32 platform! Please check your Tools->Board setting.
#endif

/////////////////////////////////////////////////////////////

#include <Arduino.h>

#include "WebServer_ESP32_SC_ENC_Debug.h"

//////////////////////////////////////////////////////////////

#include "enc28j60/esp32_sc_enc28j60.h"

#include "WebServer_ESP32_SC_ENC.hpp"
#include "WebServer_ESP32_SC_ENC_Impl.h"

/////////////////////////////////////////////////////////////

#endif    // WEBSERVER_ESP32_SC_ENC_H
