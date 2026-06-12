/**
 * RPCToolkit - Legacy Convenience Header
 * 
 * Compatibility umbrella header kept for sketches that used the repository
 * name before the Arduino/PlatformIO package name was finalized.
 *
 * Prefer including only the headers required by a sketch, such as
 * RpcClient.h plus a transport for clients or RpcServer.h plus a transport
 * for servers. New sketches that need the full toolkit surface can include
 * RPCToolkit.h.
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

#endif // RPC_ARDUINO_TOOLKIT_H
