/**
 * RPCToolkit - Deprecated WiFi Transport Name
 *
 * Use RpcHttpServerTransport for new code. This wrapper is kept for sketches
 * that used the old WiFi-specific name before the HTTP transport was renamed.
 */

#ifndef RPC_WIFI_TRANSPORT_H
#define RPC_WIFI_TRANSPORT_H

#include "RpcConfig.h"
#include "RpcHttpServerTransport.h"

#if RPC_HAS_WIFI

#if defined(__GNUC__)
  #define RPC_DEPRECATED_TRANSPORT(message) __attribute__((deprecated(message)))
#else
  #define RPC_DEPRECATED_TRANSPORT(message)
#endif

class RPC_DEPRECATED_TRANSPORT("Use RpcHttpServerTransport instead.") RpcWiFiTransport : public RpcHttpServerTransport {
public:
    explicit RpcWiFiTransport(Client& c) : RpcHttpServerTransport(c) {}
};

#undef RPC_DEPRECATED_TRANSPORT

#endif // RPC_HAS_WIFI

#endif // RPC_WIFI_TRANSPORT_H
