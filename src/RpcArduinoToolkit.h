/**
 * RPC Arduino Toolkit - Main Header
 * 
 * Convenience umbrella header.
 *
 * Prefer including only the headers required by a sketch, such as
 * RpcClient.h plus a transport for clients or RpcServer.h plus a transport
 * for servers. Include this file only when a sketch intentionally wants the
 * full toolkit surface.
 */

#ifndef RPC_ARDUINO_TOOLKIT_H
#define RPC_ARDUINO_TOOLKIT_H

// Core files
#include "RpcConfig.h"
#include "RpcTypes.h"
#include "RpcTransport.h"
#include "RpcSerialTransport.h"
#include "RpcHttpClientTransport.h"
#include "RpcHttpServerTransport.h"
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
