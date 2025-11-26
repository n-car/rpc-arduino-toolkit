/**
 * RPC Arduino Toolkit - Main Header
 * 
 * Include this file to use the RPC library
 */

#ifndef RPC_ARDUINO_TOOLKIT_H
#define RPC_ARDUINO_TOOLKIT_H

// Core files
#include "RpcConfig.h"
#include "RpcTypes.h"
#include "RpcTransport.h"
#include "RpcSerialTransport.h"
#include "RpcServer.h"
#include "RpcClient.h"

// Platform-specific transports
#if RPC_HAS_WIFI
  #include "RpcWiFiTransport.h"
#endif

#if RPC_HAS_BLE
  #include "RpcBLETransport.h"
#endif

#endif // RPC_ARDUINO_TOOLKIT_H
