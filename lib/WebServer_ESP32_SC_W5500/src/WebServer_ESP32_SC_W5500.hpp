/****************************************************************************************************************************
  WebServer_ESP32_SC_W5500.h
  
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

#ifndef WEBSERVER_ESP32_SC_W5500_HPP
#define WEBSERVER_ESP32_SC_W5500_HPP

/////////////////////////////////////////////////////////////

#if ( ARDUINO_ESP32S2_DEV || ARDUINO_FEATHERS2 || ARDUINO_ESP32S2_THING_PLUS || ARDUINO_MICROS2 || \
      ARDUINO_METRO_ESP32S2 || ARDUINO_MAGTAG29_ESP32S2 || ARDUINO_FUNHOUSE_ESP32S2 || \
      ARDUINO_ADAFRUIT_FEATHER_ESP32S2_NOPSRAM )
  #if (_ETHERNET_WEBSERVER_LOGLEVEL_ > 3)
		#warning Using ESP32_S2. To follow library instructions to install esp32-s2 core and WebServer Patch
		#warning You have to select HUGE APP or 1.9-2.0 MB APP to be able to run Config Portal. Must use PSRAM
  #endif
  
  #define USING_ESP32_S2        true
  
  #ifndef SHIELD_TYPE
  	#define SHIELD_TYPE         "ESP32_S2_W5500"
	#endif
   
#elif ( ARDUINO_ESP32C3_DEV )
	#if (_ETHERNET_WEBSERVER_LOGLEVEL_ > 3)
		#if ( defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2) )
		  #warning Using ESP32_C3 using core v2.0.0+. Either LittleFS, SPIFFS or EEPROM OK.
		#else
		  #warning Using ESP32_C3 using core v1.0.6-. To follow library instructions to install esp32-c3 core. Only SPIFFS and EEPROM OK.
		#endif
		
		#warning You have to select Flash size 2MB and Minimal APP (1.3MB + 700KB) for some boards
  #endif
    
  #define USING_ESP32_C3        true
  
  #ifndef SHIELD_TYPE
  	#define SHIELD_TYPE         "ESP32_C3_W5500"
	#endif
  
  //#error ESP32_C3 not supported yet
  
#elif ( defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32_S3_BOX) || defined(ARDUINO_TINYS3) || \
        defined(ARDUINO_PROS3) || defined(ARDUINO_FEATHERS3) )
  #if (_ETHERNET_WEBSERVER_LOGLEVEL_ > 3)
  	#warning Using ESP32_S3. To install esp32-s3-support branch if using core v2.0.2-
  #endif
  
  #define USING_ESP32_S3        true
  
  #ifndef SHIELD_TYPE
  	#define SHIELD_TYPE         "ESP32_S3_W5500"
	#endif
 
#else
	#error This code is intended to run on the ESP32_S2/3, ESP32_C3 platform! Please check your Tools->Board setting.  
#endif

#define BOARD_NAME      ARDUINO_BOARD

//////////////////////////////////////////////////////////////

//#define CONFIG_ETH_SPI_ETHERNET_W5500				true

//////////////////////////////////////////////////////////////

#include <WiFi.h>
#include <WebServer.h> // Introduce corresponding libraries

#include <hal/spi_types.h>

//////////////////////////////////////////////////////////////

#if USING_ESP32_S3

	#if !defined(ETH_SPI_HOST)
		// Using SPI2 for ESP32_SC, but name is SPI3_HOST
		#define ETH_SPI_HOST            SPI3_HOST
	#endif

	#if !defined(SPI_CLOCK_MHZ)
		// Using 25MHz for W5500, 14MHz for W5100
		#define SPI_CLOCK_MHZ       25
	#endif

	#if !defined(INT_GPIO)
		#define INT_GPIO            4
	#endif

	#if !defined(MISO_GPIO)
		#define MISO_GPIO           13
	#endif

	#if !defined(MOSI_GPIO)
		#define MOSI_GPIO           11
	#endif

	#if !defined(SCK_GPIO)
		#define SCK_GPIO            12
	#endif

	#if !defined(CS_GPIO)
		#define CS_GPIO             10
	#endif

//////////////////////////////////////////////////////////////

#elif USING_ESP32_S2

	#if !defined(ETH_SPI_HOST)
		// Using SPI1 for ESP32_SC, but name is SPI2_HOST
		#define ETH_SPI_HOST            SPI3_HOST
	#endif

	#if !defined(SPI_CLOCK_MHZ)
		// Using 25MHz for W5500, 14MHz for W5100
		#define SPI_CLOCK_MHZ       25
	#endif

	#if !defined(INT_GPIO)
		#define INT_GPIO            4
	#endif

	#if !defined(MISO_GPIO)
		#define MISO_GPIO           37
	#endif

	#if !defined(MOSI_GPIO)
		#define MOSI_GPIO           35
	#endif

	#if !defined(SCK_GPIO)
		#define SCK_GPIO            36
	#endif

	#if !defined(CS_GPIO)
		#define CS_GPIO             34
	#endif

//////////////////////////////////////////////////////////////

#elif USING_ESP32_C3

	#if !defined(ETH_SPI_HOST)
		// Using SPI2 for ESP32_SC, but name is SPI3_HOST
		#define ETH_SPI_HOST            SPI2_HOST
	#endif

	#if !defined(SPI_CLOCK_MHZ)
		// Using 25MHz for W5500, 14MHz for W5100
		#define SPI_CLOCK_MHZ       25
	#endif

	#if !defined(INT_GPIO)
		#define INT_GPIO            10
	#endif

	#if !defined(MISO_GPIO)
		#define MISO_GPIO           5
	#endif

	#if !defined(MOSI_GPIO)
		#define MOSI_GPIO           6
	#endif

	#if !defined(SCK_GPIO)
		#define SCK_GPIO        		4    
	#endif

	#if !defined(CS_GPIO)
		#define CS_GPIO             7
	#endif
	
#endif

//////////////////////////////////////////////////////////////

extern bool ESP32_W5500_eth_connected;

extern void ESP32_W5500_onEvent();

extern void ESP32_W5500_waitForConnect();

extern bool ESP32_W5500_isConnected();

extern void ESP32_W5500_event(WiFiEvent_t event);

//////////////////////////////////////////////////////////////

#endif    // WEBSERVER_ESP32_SC_W5500_HPP
