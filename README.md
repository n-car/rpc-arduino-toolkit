# RPC Arduino Toolkit

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Arduino](https://img.shields.io/badge/Arduino-Compatible-green.svg)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)

Lightweight JSON-RPC 2.0 client and server library for Arduino, ESP32, ESP8266, and other embedded platforms. Part of the RPC Toolkit ecosystem for compatible JSON-RPC integrations.

## Project Status

RPC Arduino Toolkit is an early public version. Version 1.0.0 is currently in development as the initial public release candidate.

- The API may still change before the first official release.
- GitHub installation is currently the recommended method.
- Arduino Library Manager and PlatformIO Registry publication are planned after the first official release is ready.

## Features

### Core Features
- **JSON-RPC 2.0 support** - Standard JSON-RPC 2.0 by default. Typed Safe Mode extensions only when explicitly enabled by compatible endpoints.
- **Client & Server** - Both RPC client and server implementations
- **Built-in Introspection** - API discovery with `__rpc.listMethods`, `__rpc.version`, `__rpc.describe`, and `__rpc.capabilities`
- **Multiple Transports** - Serial plus HTTP client/server transports over Arduino `Client` sockets
- **Memory Efficient** - Static allocation, minimal RAM usage
- **Cross-Platform** - Designed for standard JSON-RPC interoperability with compatible clients and servers

### Supported Platforms
- **ESP32** - Current validation target; WiFi/HTTP supported
- **ESP8266** - WiFi/HTTP supported by design through Arduino `Client` sockets
- **Arduino-compatible cores with C++ `std::function` support** - Serial transport and core RPC types
- **STM32 / RP2040** - Planned validation through the Arduino framework

AVR boards such as Uno, Mega, and Nano are not a validated target for the current implementation.

### Supported Transports

Current:
- **Serial/UART** - USB and hardware serial
- **HTTP client transport** - `RpcHttpClientTransport` over Arduino `Client` sockets for calling remote JSON-RPC HTTP endpoints
- **HTTP server transport** - `RpcHttpServerTransport` over Arduino `Client` sockets, usable with `WiFiClient`, `EthernetClient`, and compatible clients

Compatibility:
- **RpcWiFiTransport** - deprecated compatibility wrapper for `RpcHttpServerTransport`

Planned:
- **Bluetooth LE** - ESP32 BLE transport
- **LoRa** - Long-range IoT communication
- **WebSocket** - Network transport option
- **mDNS discovery** - Device/service discovery for local networks

## Installation

### Arduino IDE - Manual GitHub Installation

The library is not yet published in Arduino Library Manager. Until the first official release is indexed, install it manually from GitHub:

1. Download the repository ZIP from GitHub using **Code > Download ZIP**.
2. Extract it into your Arduino sketchbook `libraries` directory.
3. Rename the extracted folder to `RpcArduinoToolkit` if needed.
4. Restart Arduino IDE.

You can also clone the repository directly:

```bash
cd ~/Arduino/libraries
git clone https://github.com/n-car/rpc-arduino-toolkit.git RpcArduinoToolkit
```

On Windows, the sketchbook libraries directory is usually `Documents/Arduino/libraries`.

### Arduino Library Manager - Planned

Arduino Library Manager installation is planned for a future official release. For now, use the manual GitHub installation method above.

### PlatformIO
Add to `platformio.ini`:
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    https://github.com/n-car/rpc-arduino-toolkit.git
```

The PlatformIO Registry name will be used only after the library is officially published there.

## Quick Start

### Server Example (ESP32 - WiFi)

```cpp
#include <WiFi.h>
#include <RpcServer.h>
#include <RpcHttpServerTransport.h>

// Create server with max 8 methods
RpcServer<8> rpc;
WiFiServer server(8080);
StaticJsonDocument<128> resultDoc;

JsonVariant makeBoolResult(bool value) {
    resultDoc.clear();
    resultDoc.set(value);
    return resultDoc.as<JsonVariant>();
}

JsonVariant makeNumberResult(float value) {
    resultDoc.clear();
    resultDoc.set(value);
    return resultDoc.as<JsonVariant>();
}

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin("YourSSID", "YourPassword");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    Serial.println(WiFi.localIP());

    // Register methods
    rpc.addMethod("led", [](JsonVariantConst params) -> JsonVariant {
        int pin = params["pin"];
        bool state = params["state"];
        digitalWrite(pin, state ? HIGH : LOW);
        return makeBoolResult(true);
    });

    rpc.addMethod("readTemp", []() -> JsonVariant {
        // Read temperature sensor (example)
        float temp = analogRead(A0) * 0.1;
        return makeNumberResult(temp);
    });

    // Start server
    server.begin();
}

void loop() {
    WiFiClient client = server.accept();
    if (client) {
        RpcHttpServerTransport transport(client);
        String response = rpc.handleRequest(transport);
        transport.write(response);
        client.stop();
    }
}
```

### Client Example (Arduino - Serial)

```cpp
#include <RpcClient.h>
#include <RpcSerialTransport.h>

RpcSerialTransport transport(Serial);
RpcClient rpc(transport);

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    // Call remote method
    RpcResponse resp = rpc.call("readTemp");

    if (resp.isSuccess()) {
        float temp = resp.result<float>();
        Serial.print("Temperature: ");
        Serial.println(temp);

        // Control LED based on temperature
        if (temp > 30.0) {
            rpc.call("led", "{\"pin\":13,\"state\":true}");
        }
    }

    delay(1000);
}
```

### Server Example (Arduino - Serial)

```cpp
#include <RpcServer.h>
#include <RpcSerialTransport.h>

RpcServer<4> rpc;
StaticJsonDocument<64> resultDoc;

JsonVariant makeBoolResult(bool value) {
    resultDoc.clear();
    resultDoc.set(value);
    return resultDoc.as<JsonVariant>();
}

JsonVariant makeNumberResult(int value) {
    resultDoc.clear();
    resultDoc.set(value);
    return resultDoc.as<JsonVariant>();
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // Register LED control
    rpc.addMethod("setLED", [](JsonVariantConst params) -> JsonVariant {
        bool state = params["state"];
        digitalWrite(LED_BUILTIN, state ? HIGH : LOW);
        return makeBoolResult(state);
    });

    // Register analog read
    rpc.addMethod("readAnalog", [](JsonVariantConst params) -> JsonVariant {
        int pin = params["pin"];
        return makeNumberResult(analogRead(pin));
    });
}

void loop() {
    if (Serial.available()) {
        RpcSerialTransport transport(Serial);
        String response = rpc.handleRequest(transport);
        Serial.println(response);
    }
}
```

## Advanced Usage

### Server Params

Server method handlers receive `JsonVariantConst` params, so they can read standard JSON-RPC object params and array params without copying.

```cpp
rpc.addMethod("sum", [](JsonVariantConst params) -> JsonVariant {
    JsonArrayConst values = params.as<JsonArrayConst>();
    int total = 0;

    for (JsonVariantConst value : values) {
        total += value.as<int>();
    }

    // Store owned results in a document that remains alive until serialization.
    static StaticJsonDocument<64> resultDoc;
    resultDoc.clear();
    resultDoc.set(total);
    return resultDoc.as<JsonVariant>();
});
```

### Batch Requests

```cpp
// Client sends multiple requests at once
String batch = "[{\"jsonrpc\":\"2.0\",\"method\":\"readTemp\",\"id\":1},"
               "{\"jsonrpc\":\"2.0\",\"method\":\"readHumidity\",\"id\":2}]";

RpcBatchResponse batchResp = rpc.callBatch(batch);

if (batchResp.isValid() && batchResp.count() == 2) {
    float temp = batchResp.result(0).as<float>();
    float humidity = batchResp.result(1).as<float>();
}
```

`RpcServer` also accepts JSON-RPC batch request arrays when `RPC_ENABLE_BATCH` is enabled. Notifications inside a batch are executed without response entries; if all batch items are notifications, HTTP transports return `204 No Content`. Batch handling is implemented and should receive more field testing before the first official release.

### Notifications (No Response)

```cpp
// Fire-and-forget call
rpc.notify("logEvent", "{\"level\":\"info\",\"msg\":\"Sensor read\"}");
```

### Error Handling

```cpp
RpcResponse resp = rpc.call("unknownMethod");

if (resp.hasError()) {
    Serial.print("Error code: ");
    Serial.println(resp.errorCode());
    Serial.print("Error message: ");
    Serial.println(resp.errorMessage());
}
```

### Built-in Introspection Methods

The RPC server includes built-in introspection methods for API discovery (memory-optimized for embedded platforms):

```cpp
// __rpc.listMethods - List all registered methods (excludes __rpc.*)
RpcResponse resp = rpc.call("__rpc.listMethods");
// Result: ["ping", "setLED", "readTemp", ...]

// __rpc.version - Get server version and method count
resp = rpc.call("__rpc.version");
// Result: {"toolkit":"rpc-arduino-toolkit","version":"1.0.0","methodCount":3}

// __rpc.describe - Get method description metadata (requires RPC_ENABLE_SCHEMA_SUPPORT)
resp = rpc.call("__rpc.describe", "{\"method\":\"add\"}");
// Result: {"name":"add","description":"Add two numbers","exposeSchema":true}

// __rpc.capabilities - Get server capabilities
resp = rpc.call("__rpc.capabilities");
// Result: {"batch":true,"introspection":true,"safeMode":false,"notifications":true,"schemaSupport":true,"methodCount":5,"maxMethods":8}
```

**Features:**
- Automatically available on all RPC servers
- No registration needed - built into `executeMethod()`
- Minimal memory footprint (~800 bytes)
- Usable by standard JSON-RPC clients
- Description metadata support optional (can be disabled to save ~200 bytes per method)

**Register methods with description metadata:**

```cpp
StaticJsonDocument<64> resultDoc;

JsonVariant makeStringResult(const char* value) {
    resultDoc.clear();
    resultDoc.set(value);
    return resultDoc.as<JsonVariant>();
}

JsonVariant makeNumberResult(int value) {
    resultDoc.clear();
    resultDoc.set(value);
    return resultDoc.as<JsonVariant>();
}

// Simple method without metadata
rpc.addMethod("ping", []() -> JsonVariant {
    return makeStringResult("pong");
});

// Method with description metadata exposure
rpc.addMethod("add", [](JsonVariantConst params) -> JsonVariant {
    int a = params["a"] | 0;
    int b = params["b"] | 0;
    return makeNumberResult(a + b);
}, "Add two numbers", true);  // description, exposeSchema
```

**Example usage with Node.js client:**

```javascript
const { RpcClient } = require('rpc-express-toolkit');
const client = new RpcClient('http://esp32.local:8080');

// Discover available methods
const methods = await client.call('__rpc.listMethods');
console.log('Available methods:', methods);

// Get server info
const version = await client.call('__rpc.version');
console.log(`Server: ${version.toolkit} v${version.version}`);
console.log(`Methods: ${version.methodCount}`);

// Get method details
const methodInfo = await client.call('__rpc.describe', { method: 'add' });
console.log(`Method: ${methodInfo.name}`);
console.log(`Description: ${methodInfo.description}`);

// Get server capabilities
const caps = await client.call('__rpc.capabilities');
console.log('Capabilities:', caps);
```

See `examples/Introspection/` for complete Arduino example.

### Custom Transport

```cpp
class MyCustomTransport : public RpcTransport {
public:
    String read() override {
        // Read from your custom interface
        return readFromCustomInterface();
    }

    void write(const String& data) override {
        // Write to your custom interface
        writeToCustomInterface(data);
    }
};
```

## Memory Optimization

### Static Allocation

```cpp
// Specify max methods at compile time
RpcServer<8> server;  // 8 methods max

// Use StaticJsonDocument for predictable memory
StaticJsonDocument<512> doc;
```

### Handler Result Storage

Handlers return `JsonVariant` views. Store owned scalar/object results in a document that remains alive until the response is serialized:

```cpp
StaticJsonDocument<64> resultDoc;

JsonVariant makeNumberResult(int value) {
    resultDoc.clear();
    resultDoc.set(value);
    return resultDoc.as<JsonVariant>();
}

rpc.addMethod("answer", []() -> JsonVariant {
    return makeNumberResult(42);
});
```

### Disable Features

```cpp
// In RpcConfig.h or build flags
#define RPC_ENABLE_SAFE_MODE 0      // Disable safe mode (save ~1KB)
#define RPC_ENABLE_SCHEMA_SUPPORT 0 // Disable description metadata (save ~200B/method)
#define RPC_MAX_METHOD_NAME 16      // Limit method name length
```

## Safe Mode

Safe Mode is an optional RPC Toolkit extension for preserving application-level value types across JSON-RPC 2.0 boundaries.

JSON-RPC 2.0 remains the default wire format. When Safe Mode is disabled, values are sent as plain JSON values without RPC Toolkit type prefixes.

Optional Safe Mode helper utilities are currently available in this library. Automatic end-to-end Safe Mode encoding/decoding with the other RPC Toolkit implementations is planned for a future release.

This means that `rpc-arduino-toolkit` is currently interoperable with standard JSON-RPC 2.0 clients and servers by default. Full Safe Mode interoperability requires both endpoints to implement and enable the same RPC Toolkit Safe Mode conventions.

Schema validation is planned as an optional layer. The goal is to let endpoints describe expected params and results, optionally validate calls, and preserve type intent when Safe Mode is enabled.

### Safe Mode Helper Utilities

Safe Mode helpers can add prefixes to disambiguate application-level types when serializing values (disabled by default to save memory):

```cpp
// Enable in RpcConfig.h
#define RPC_ENABLE_SAFE_MODE 1

// Use RpcSafe helper class
#include <RpcTypes.h>

// Serialize with prefixes
String safeStr = RpcSafe::serializeString("hello");      // "S:hello"
String safeDate = RpcSafe::serializeDate(1234567890);    // "D:1234567890"
String safeBigInt = RpcSafe::serializeBigInt(9999999);   // "9999999n"

// Deserialize
String str = RpcSafe::deserializeString("S:hello");      // "hello"
unsigned long ts = RpcSafe::deserializeDate("D:1234567890");  // 1234567890
long long num = RpcSafe::deserializeBigInt("9999999n");  // 9999999

// Check type
bool isStr = RpcSafe::isSafeString("S:test");    // true
bool isDate = RpcSafe::isSafeDate("D:12345");    // true
bool isBigInt = RpcSafe::isBigInt("123n");       // true
```

**Safe Mode Helpers:**
- String prefix: `S:` - Distinguishes strings from other types
- Date prefix: `D:` - Marks timestamps/dates
- BigInt suffix: `n` - Marks large integers (like JavaScript BigInt)
- Automatic end-to-end Safe Mode interoperability with other RPC Toolkit implementations is planned
- Helps preserve type intent when both endpoints use the same conventions
- Disabled by default (enable with `RPC_ENABLE_SAFE_MODE=1`)

See `examples/SafeMode/` for complete example.

## Configuration

### RpcConfig.h

```cpp
// Memory limits
#define RPC_MAX_METHODS 8           // Maximum registered methods
#define RPC_MAX_REQUEST_SIZE 512    // Max JSON request size
#define RPC_MAX_RESPONSE_SIZE 512   // Max JSON response size
#define RPC_MAX_METHOD_NAME 32      // Max method name length
#define RPC_MAX_DESCRIPTION 64      // Max description metadata length

// Features
#define RPC_ENABLE_SAFE_MODE 0      // Enable Safe Mode helper serialization (S:, D:, n)
#define RPC_ENABLE_BATCH 1          // Enable JSON-RPC batch requests
#define RPC_ENABLE_LOGGING 0        // Enable debug logging
#define RPC_ENABLE_NOTIFICATIONS 1  // Enable fire-and-forget calls
#define RPC_ENABLE_SCHEMA_SUPPORT 1 // Enable method description metadata

// Timeouts
#define RPC_DEFAULT_TIMEOUT 5000    // Default timeout (ms)
#define RPC_SERIAL_TIMEOUT 1000     // Serial read timeout (ms)
```

## Memory Usage

| Platform | Flash (Code) | RAM (Static) | RAM (Runtime) | Features |
|----------|--------------|--------------|---------------|----------|
| ESP32 | ~12KB | ~300B | ~1KB | Full |
| ESP8266 | ~10KB | ~250B | ~800B | Full |

**Feature Impact:**
- `RPC_ENABLE_SCHEMA_SUPPORT=1`: +200 bytes per method (description metadata storage)
- `RPC_ENABLE_SAFE_MODE=1`: +1KB Flash, +50 bytes RAM

*Note: Values depend on enabled features and registered methods*

## Cross-Platform Compatibility

Designed to work with standard JSON-RPC 2.0 clients and servers in the RPC Toolkit ecosystem by default. Safe Mode interoperability across implementations is planned and requires compatible endpoints to enable the same conventions.

- **rpc-express-toolkit** (Node.js/Express)
- **rpc-php-toolkit** (PHP)
- **rpc-dotnet-toolkit** (.NET)
- **node-red-contrib-rpc-toolkit** (Node-RED)

### Example: ESP32 to Node.js Server

Use `RpcHttpClientTransport` to call a remote JSON-RPC HTTP endpoint from an ESP32 client.

**ESP32 Client:**
```cpp
#include <WiFi.h>
#include <RpcClient.h>
#include <RpcHttpClientTransport.h>

WiFiClient httpClient;
RpcHttpClientTransport transport(httpClient, "192.168.1.100", 3000, "/api");
RpcClient rpc(transport);

float result = rpc.call("add", "{\"a\":5,\"b\":3}").result<float>();
```

Physical ESP32 interoperability tests are maintained separately during development.

**Node.js Server (Express):**
```javascript
const { RpcEndpoint } = require('rpc-express-toolkit');
const rpc = new RpcEndpoint('/api/rpc');

rpc.addMethod('add', (params) => {
    return params.a + params.b;
});
```

## Examples

See the `examples/` folder for complete working examples:

- **BasicServer** - Simple RPC server on Serial
- **BasicClient** - Simple RPC client on Serial
- **WiFiServer** - ESP32 HTTP RPC server
- **Introspection** - Demonstrates __rpc.* methods and description metadata
- **SafeMode** - Optional helper utilities for S:, D:, and n value prefixes

## API Reference

### RpcServer

```cpp
template<uint8_t MAX_METHODS>
class RpcServer {
public:
    // Register a method
    bool addMethod(const char* name, RpcMethodHandler handler);
    bool addMethod(const char* name, RpcMethodHandler handler, const char* description, bool exposeSchema = false);
    bool addMethod(const char* name, RpcSimpleHandler handler);
    bool addMethod(const char* name, RpcSimpleHandler handler, const char* description, bool exposeSchema = false);

    // Handler signature
    // JsonVariant handler(JsonVariantConst params);

    // Handle incoming request
    String handleRequest(RpcTransport& transport);
    String handleRequest(const String& json);

    // Remove a method
    bool removeMethod(const char* name);
};
```

### RpcClient

```cpp
class RpcClient {
public:
    RpcClient(RpcTransport& transport);

    // Call remote method
    RpcResponse call(const char* method, const String& params = "");
    RpcResponse call(const char* method, JsonObject params);

    // Send notification (no response)
    void notify(const char* method, const String& params = "");

    // Batch request
    RpcBatchResponse callBatch(const String& batch);

    // Set timeout
    void setTimeout(unsigned long ms);
};
```

### RpcHttpServerTransport

```cpp
class RpcHttpServerTransport : public RpcTransport {
public:
    explicit RpcHttpServerTransport(Client& client);
};
```

`RpcWiFiTransport` remains available as a deprecated compatibility wrapper for existing sketches. New code should use `RpcHttpServerTransport`.

### RpcHttpClientTransport

```cpp
class RpcHttpClientTransport : public RpcTransport {
public:
    RpcHttpClientTransport(Client& client, const char* host, uint16_t port, const char* path = "/");

    void setSocketSettleDelay(unsigned long ms);
    const String& lastError() const;
    int lastStatus() const;
};
```

### RpcBatchResponse

```cpp
class RpcBatchResponse {
public:
    bool isValid() const;
    bool isBatch() const;
    bool isSuccess() const;
    size_t count() const;

    JsonVariantConst response(size_t index) const;
    JsonVariantConst result(size_t index) const;
    bool hasError(size_t index) const;
    int errorCode(size_t index) const;
    String errorMessage(size_t index) const;
};
```

### RpcResponse

```cpp
class RpcResponse {
public:
    bool isSuccess() const;
    bool hasError() const;

    // Get result
    template<typename T>
    T result() const;

    // Get error
    int errorCode() const;
    String errorMessage() const;
};
```

## Related Projects

RPC Toolkit ecosystem projects. Standard JSON-RPC interoperability is the current/default compatibility target; Safe Mode interoperability is planned.

- [rpc-express-toolkit](https://github.com/n-car/rpc-express-toolkit) - Node.js/Express implementation
- [rpc-php-toolkit](https://github.com/n-car/rpc-php-toolkit) - PHP implementation
- [rpc-dotnet-toolkit](https://github.com/n-car/rpc-dotnet-toolkit) - .NET implementation
- [node-red-contrib-rpc-toolkit](https://github.com/n-car/node-red-contrib-rpc-toolkit) - Node-RED visual programming

## Development

### Build Examples

```bash
# Using PlatformIO
pio run -e esp32dev
pio run -e esp8266

# Upload to device
pio run -e esp32dev --target upload
```

### Run Tests

```bash
# Native tests (host platform)
pio test -e native
```

## Roadmap

### v1.0.0 - Initial Public Release Candidate
- [x] Core RPC client/server
- [x] Serial transport
- [x] HTTP client transport over Arduino Client-compatible sockets
- [x] HTTP server transport over Arduino Client-compatible sockets
- [x] Built-in introspection
- [x] Batch requests
- [x] Optional Safe Mode helper utilities
- [x] Basic examples for Serial, WiFi, introspection, and Safe Mode
- [ ] Finalize public API before official release
- [ ] Publish first GitHub release

### Planned After v1.0.0
- [ ] Arduino Library Manager publication
- [ ] PlatformIO Registry publication
- [ ] Bluetooth LE transport (ESP32)
- [ ] End-to-end Safe Mode encoding/decoding across RPC Toolkit implementations
- [ ] Optional schema-based params/result validation

### Future Transport and Discovery Work
- [ ] LoRa transport
- [ ] WebSocket support
- [ ] mDNS discovery
- [ ] OTA updates integration

## Contributing

Contributions, bug reports, and compatibility feedback are welcome. Please open an issue or pull request on GitHub.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Built on [ArduinoJson](https://arduinojson.org/) library
- Compatible with Arduino Core for ESP32/ESP8266
- Part of the RPC Toolkit ecosystem

---

**RPC Arduino Toolkit** - JSON-RPC 2.0 for embedded projects.
