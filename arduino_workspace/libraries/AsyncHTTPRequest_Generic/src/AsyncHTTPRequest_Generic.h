/****************************************************************************************************************************
  AsyncHTTPRequest_Generic.h - Dead simple AsyncHTTPRequest for ESP8266, ESP32 and currently STM32 with built-in LAN8742A Ethernet
  
  For ESP8266, ESP32 and STM32 with built-in LAN8742A Ethernet (Nucleo-144, DISCOVERY, etc)
  
  AsyncHTTPRequest_STM32 is a library for the ESP8266, ESP32 and currently STM32 run built-in Ethernet WebServer
  
  Based on and modified from asyncHTTPrequest Library (https://github.com/boblemaire/asyncHTTPrequest)
  
  Built by Khoi Hoang https://github.com/khoih-prog/AsyncHTTPRequest_Generic
  Licensed under MIT license
  
  Copyright (C) <2018>  <Bob Lemaire, IoTaWatt, Inc.>
  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License 
  as published bythe Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
  You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.  
 
  Version: 1.8.2
  
  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0    K Hoang     14/09/2020 Initial coding to add support to STM32 using built-in Ethernet (Nucleo-144, DISCOVERY, etc).
  ...
  1.7.0    K Hoang     10/02/2022 Add support to new ESP32-S3. Add LittleFS support to ESP32-C3. Use core LittleFS
  1.7.1    K Hoang     25/02/2022 Add example AsyncHTTPRequest_ESP_Multi to demo connection to multiple addresses
  1.8.0    K Hoang     13/04/2022 Add support to ESP8266 using W5x00 with lwip_W5100 or lwip_W5500 library
  1.8.1    K Hoang     13/04/2022 Add support to ESP8266 using ENC28J60 with lwip_enc28j60 library
  1.8.2    K Hoang     10/08/2022 Fix library.properties to remove unavailable items from depends
 *****************************************************************************************************************************/

#pragma once

#ifndef ASYNC_HTTP_REQUEST_GENERIC_H
#define ASYNC_HTTP_REQUEST_GENERIC_H

#include "AsyncHTTPRequest_Generic.hpp"
#include "AsyncHTTPRequest_Impl_Generic.h"

#endif    // ASYNC_HTTP_REQUEST_GENERIC_H
