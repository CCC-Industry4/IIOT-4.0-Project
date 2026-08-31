/****************************************************************************************************************************
  WebServer_ESP32_SC_W5500_Impl.h

  For Ethernet shields using ESP32_SC_W5500 (ESP32_S2/S3/C3 + LwIP W5500)

  WebServer_ESP32_SC_W5500 is a library for the ESP32_S2/S3/C3 with LwIP Ethernet W5500

  Based on and modified from ESP8266 https://github.com/esp8266/Arduino/releases
  Built by Khoi Hoang https://github.com/khoih-prog/WebServer_ESP32_SC_W5500
  Licensed under GPLv3 license

  Version: 1.2.1

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0   K Hoang      13/12/2022 Initial coding for ESP32_S3_W5500 (ESP32_S3 + W5500)
  1.0.1   K Hoang      14/12/2022 Using SPI_DMA_CH_AUTO instead of manually selected
  1.1.0   K Hoang      19/12/2022 Add support to ESP32_S2_W5500 (ESP32_S2 + W5500)
  1.2.0   K Hoang      20/12/2022 Add support to ESP32_C3_W5500 (ESP32_C3 + W5500)
  1.2.1   K Hoang      22/12/2022 Remove unused variable to avoid compiler warning and error
 *****************************************************************************************************************************/

#pragma once

#ifndef WEBSERVER_ESP32_SC_W5500_IMPL_H
#define WEBSERVER_ESP32_SC_W5500_IMPL_H

//////////////////////////////////////////////////////////////

bool ESP32_W5500_eth_connected = false;

void ESP32_W5500_onEvent()
{
  WiFi.onEvent(ESP32_W5500_event);
}

//////////////////////////////////////////////////////////////

void ESP32_W5500_waitForConnect()
{
  while (!ESP32_W5500_eth_connected)
    delay(100);
}

//////////////////////////////////////////////////////////////

bool ESP32_W5500_isConnected()
{
  return ESP32_W5500_eth_connected;
}

//////////////////////////////////////////////////////////////

void ESP32_W5500_event(WiFiEvent_t event)
{
  switch (event)
  {
      //#if USING_CORE_ESP32_CORE_V200_PLUS
#if ( ( defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2) ) && ( ARDUINO_ESP32_GIT_VER != 0x46d5afb1 ) )
    // For breaking core v2.0.0
    // Why so strange to define a breaking enum arduino_event_id_t in WiFiGeneric.h
    // compared to the old system_event_id_t, now in tools/sdk/esp32/include/esp_event/include/esp_event_legacy.h
    // You can preserve the old enum order and just adding new items to do no harm
    case ARDUINO_EVENT_ETH_START:
      ET_LOG(F("\nETH Started"));
      //set eth hostname here
      ETH.setHostname("ESP32_SC_W5500");
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      ET_LOG(F("ETH Connected"));
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      if (!ESP32_W5500_eth_connected)
      {
        ET_LOG3(F("ETH MAC: "), ETH.macAddress(), F(", IPv4: "), ETH.localIP());

        if (ETH.fullDuplex())
        {
          ET_LOG0(F("FULL_DUPLEX, "));
        }
        else
        {
          ET_LOG0(F("HALF_DUPLEX, "));
        }

        ET_LOG1(ETH.linkSpeed(), F("Mbps"));

        ESP32_W5500_eth_connected = true;
      }

      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      ET_LOG("ETH Disconnected");
      ESP32_W5500_eth_connected = false;
      break;

    case ARDUINO_EVENT_ETH_STOP:
      ET_LOG("\ETH Stopped");
      ESP32_W5500_eth_connected = false;
      break;

#else

    // For old core v1.0.6-
    // Core v2.0.0 defines a stupid enum arduino_event_id_t, breaking any code for ESP32_W5500 written for previous core
    // Why so strange to define a breaking enum arduino_event_id_t in WiFiGeneric.h
    // compared to the old system_event_id_t, now in tools/sdk/esp32/include/esp_event/include/esp_event_legacy.h
    // You can preserve the old enum order and just adding new items to do no harm
    case SYSTEM_EVENT_ETH_START:
      ET_LOG(F("\nETH Started"));
      //set eth hostname here
      ETH.setHostname("ESP32_W5500");
      break;

    case SYSTEM_EVENT_ETH_CONNECTED:
      ET_LOG(F("ETH Connected"));
      break;

    case SYSTEM_EVENT_ETH_GOT_IP:
      if (!ESP32_W5500_eth_connected)
      {
        ET_LOG3(F("ETH MAC: "), ETH.macAddress(), F(", IPv4: "), ETH.localIP());

        if (ETH.fullDuplex())
        {
          ET_LOG0(F("FULL_DUPLEX, "));
        }
        else
        {
          ET_LOG0(F("HALF_DUPLEX, "));
        }

        ET_LOG1(ETH.linkSpeed(), F("Mbps"));

        ESP32_W5500_eth_connected = true;
      }

      break;

    case SYSTEM_EVENT_ETH_DISCONNECTED:
      ET_LOG("ETH Disconnected");
      ESP32_W5500_eth_connected = false;
      break;

    case SYSTEM_EVENT_ETH_STOP:
      ET_LOG("\nETH Stopped");
      ESP32_W5500_eth_connected = false;
      break;
#endif

    default:
      break;
  }
}

//////////////////////////////////////////////////////////////

#endif    // WEBSERVER_ESP32_SC_W5500_IMPL_H
