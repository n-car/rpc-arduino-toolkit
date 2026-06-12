# Transports

`rpc-arduino-toolkit` separates JSON-RPC behavior from the byte transport used
to move requests and responses.

## Current Transports

| Transport | Class | Status | Notes |
| --- | --- | --- | --- |
| Serial/UART | `RpcSerialTransport` | Current | Works over USB serial and hardware serial. No HTTP headers. |
| HTTP client | `RpcHttpClientTransport` | Current | Calls remote JSON-RPC HTTP endpoints over an Arduino `Client`. |
| HTTP server | `RpcHttpServerTransport` | Current | Handles JSON-RPC HTTP requests over an accepted Arduino `Client`. |
| WiFi server alias | `RpcWiFiTransport` | Deprecated compatibility alias | Wrapper for `RpcHttpServerTransport`; new code should not use it. |

Planned transports and discovery work:

- Bluetooth LE transport for ESP32.
- LoRa transport.
- WebSocket transport.
- mDNS discovery.

## HTTP, WiFi, and Ethernet

HTTP is the RPC transport. WiFi, Ethernet, and similar libraries provide the
socket object used by the HTTP transport.

Use `RpcHttpClientTransport` or `RpcHttpServerTransport` with any Arduino
`Client` implementation that behaves like `WiFiClient` or `EthernetClient`.

Examples:

```cpp
WiFiClient httpClient;
RpcHttpClientTransport transport(httpClient, "192.168.1.100", 3000, "/api");
RpcClient rpc(transport);
```

```cpp
WiFiClient client = server.accept();
RpcHttpServerTransport transport(client);
String response = rpc.handleRequest(transport);
transport.write(response);
client.stop();
```

The class names intentionally say `Http`, not `WiFi`, because the same transport
can be used over WiFi, Ethernet, or another compatible `Client`.

## Deprecated RpcWiFiTransport

`RpcWiFiTransport` is kept only so older sketches can still compile. It is a
server-side compatibility wrapper around `RpcHttpServerTransport`.

New sketches should use:

```cpp
#include <RpcHttpServerTransport.h>
```

instead of:

```cpp
#include <RpcWiFiTransport.h>
```

## Serial Transport

`RpcSerialTransport` reads and writes plain JSON-RPC payloads through a
`Stream`, usually `Serial`.

```cpp
RpcSerialTransport transport(Serial);
RpcServer<4> rpc;

void loop() {
    if (Serial.available()) {
        String response = rpc.handleRequest(transport);
        Serial.println(response);
    }
}
```

Serial transport can use Safe Mode helper utilities if your sketch explicitly
serializes marker strings, but it does not negotiate `X-RPC-Safe-Enabled`
because that is an HTTP header.

## Safe Mode Header Negotiation

Safe Mode over HTTP uses:

```http
X-RPC-Safe-Enabled: true
```

HTTP transports handle this header when `RPC_ENABLE_SAFE_MODE=1`:

- `RpcHttpClientTransport` sends the local Safe Mode state and records the
  server response header.
- `RpcHttpServerTransport` records the client header and includes the server
  Safe Mode state in responses.
- `RpcServer` and `RpcClient` use the transport header state to decide whether
  params, results, and `error.data` need recursive Safe Mode encode/decode.

When `RPC_ENABLE_SAFE_MODE=0`, HTTP transports still send the header with
`false`.

With `RPC_ENABLE_SAFE_MODE=1` and `RPC_SAFE_STRICT_MODE=1`, HTTP server requests
without `X-RPC-Safe-Enabled` are rejected with JSON-RPC error `-32600` for calls
with ids. Notifications do not produce JSON-RPC response bodies.

See [SAFE_MODE_INTEROPERABILITY.md](SAFE_MODE_INTEROPERABILITY.md) for the full
Safe Mode behavior and validation matrix.

## Custom Transports

Implement `RpcTransport` when you need to move JSON-RPC payloads over another
interface.

```cpp
class MyCustomTransport : public RpcTransport {
public:
    String read() override {
        return readFromCustomInterface();
    }

    bool write(const String& data) override {
        return writeToCustomInterface(data);
    }

    bool available() override {
        return customInterfaceHasData();
    }
};
```

Custom non-HTTP transports normally keep `isHttp()` false and therefore do not
participate in HTTP Safe Mode header negotiation.
