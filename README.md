# RPCToolkit

[![CI](https://github.com/n-car/rpc-arduino-toolkit/actions/workflows/ci.yml/badge.svg)](https://github.com/n-car/rpc-arduino-toolkit/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Status](https://img.shields.io/badge/status-stable-green.svg)](https://github.com/n-car/rpc-arduino-toolkit/releases/tag/v1.0.0)
[![Arduino Library](https://www.ardu-badge.com/badge/RPCToolkit.svg)](https://www.arduino.cc/reference/en/libraries/rpctoolkit/)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/n-car/library/RPCToolkit.svg)](https://registry.platformio.org/libraries/n-car/RPCToolkit)

Lightweight JSON-RPC 2.0 client and server library for ESP32, ESP8266, and compatible Arduino-style embedded targets. Packaged as `RPCToolkit` for Arduino and PlatformIO metadata, from the `rpc-arduino-toolkit` repository. Part of the RPC Toolkit ecosystem for compatible JSON-RPC integrations.

## Project Status

RPCToolkit 1.0.0 is the initial public release for ESP32/ESP8266-focused Arduino-compatible targets.

- Arduino Library Manager publication is complete as `RPCToolkit`.
- PlatformIO Registry publication is complete as `n-car/RPCToolkit`.
- GitHub installation remains available for development snapshots.
- RPC Toolkit Safe Mode HTTP interoperability is implemented.
- ESP32 Safe Mode HTTP interoperability has been physically validated against `rpc-express-toolkit`.
- ESP8266 HTTP client behavior has been physically validated against an ESP32 Arduino HTTP server with Safe Mode enabled.
- Safe Mode interoperability behavior and test coverage are tracked in [`docs/SAFE_MODE_INTEROPERABILITY.md`](docs/SAFE_MODE_INTEROPERABILITY.md).
- Registry publication for the initial public release is complete.

## Features

### Core Features
- **JSON-RPC 2.0 support** - Standard JSON-RPC 2.0 by default. Typed Safe Mode extensions only when explicitly enabled by compatible endpoints.
- **Client & Server** - Both RPC client and server implementations
- **Built-in Introspection** - API discovery with `__rpc.listMethods`, `__rpc.version`, `__rpc.describe`, and `__rpc.capabilities`
- **Multiple Transports** - Serial plus HTTP client/server transports over Arduino `Client` sockets
- **Memory-conscious** - Static allocation where practical, with ESP8266-specific heap-backed response buffers to reduce stack pressure
- **Cross-Platform** - Designed for standard JSON-RPC interoperability with compatible clients and servers

### Supported Platforms
- **ESP32** - Current primary validation target; WiFi/HTTP supported
- **ESP8266** - Declared supported architecture; WiFi/HTTP supported through Arduino `Client` sockets, with HTTP client runtime physically validated
- **Other Arduino-compatible cores with C++ `std::function` support** - Serial transport and core RPC types may work, but are not declared in the first registry metadata until validated
- **STM32 / RP2040** - Planned validation through the Arduino framework

AVR boards such as Uno, Mega, and Nano are not a validated target for the current implementation.
Library metadata currently declares `esp32` and `esp8266`.

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

See [`docs/TRANSPORTS.md`](docs/TRANSPORTS.md) for HTTP vs WiFi/Ethernet behavior, `RpcWiFiTransport` deprecation details, Safe Mode header negotiation, and custom transport guidance.

## Operational Notes

These are the main points to check before using the library in an application:

- Standard JSON-RPC 2.0 is the default. RPC Toolkit Safe Mode is opt-in with `RPC_ENABLE_SAFE_MODE=1`.
- HTTP transports are network-agnostic wrappers over Arduino `Client` sockets. Use `RpcHttpClientTransport` or `RpcHttpServerTransport` with `WiFiClient`, `EthernetClient`, or another compatible client.
- `RpcWiFiTransport` is only a deprecated compatibility alias for older sketches. New code should use `RpcHttpServerTransport`.
- Prefer selective includes over umbrella headers on constrained boards. `RPCToolkit.h` and `RpcArduinoToolkit.h` remain available for convenience.
- Server handlers return `JsonVariant` views. Any document that owns returned data must remain alive until the response is serialized.
- ESP8266 works, but stack and heap headroom are tighter than ESP32. Keep `RPC_MAX_REQUEST_SIZE`, response size, batch size, and local `StaticJsonDocument` instances sized for the real sketch, and measure heap/fragmentation on hardware.
- ArduinoJson has no native JavaScript `Date` or `BigInt` value types. Safe Mode date and BigInt markers are preserved as strings unless your sketch explicitly parses them.

## Installation

### Arduino IDE - Library Manager

Install `RPCToolkit` from Arduino IDE:

1. Open **Tools > Manage Libraries...**.
2. Search for `RPCToolkit`.
3. Install version `1.0.0` or newer.

### Arduino IDE - Manual GitHub Installation

For development snapshots, clone the repository directly:

```bash
cd ~/Arduino/libraries
git clone https://github.com/n-car/rpc-arduino-toolkit.git RPCToolkit
```

On Windows, the sketchbook libraries directory is usually `Documents/Arduino/libraries`.

### PlatformIO
Add to `platformio.ini`:
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    n-car/RPCToolkit@^1.0.0
```

For development directly from the repository, you can still use:

```ini
lib_deps =
    https://github.com/n-car/rpc-arduino-toolkit.git
```

### Selective Includes

For Arduino sketches, prefer including only the client, server, and transport headers you actually use. This keeps compile units smaller on constrained boards.

```cpp
// HTTP server
#include <RpcServer.h>
#include <RpcHttpServerTransport.h>

// HTTP client
#include <RpcClient.h>
#include <RpcHttpClientTransport.h>

// Serial client/server
#include <RpcClient.h>
#include <RpcServer.h>
#include <RpcSerialTransport.h>
```

`RPCToolkit.h` matches the package name and includes `RpcArduinoToolkit.h`. `RpcArduinoToolkit.h` remains available for compatibility. Examples use selective includes by default.

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

## API and Usage Details

The README keeps the first-run examples short. Detailed API signatures and
common patterns are maintained separately:

- [`docs/API_REFERENCE.md`](docs/API_REFERENCE.md) - `RpcServer`, `RpcClient`, responses, batches, notifications, custom errors, and introspection.
- [`docs/TRANSPORTS.md`](docs/TRANSPORTS.md) - Serial vs HTTP, WiFi/Ethernet usage, `RpcWiFiTransport` deprecation, Safe Mode headers, and custom transports.
- [`docs/SAFE_MODE_INTEROPERABILITY.md`](docs/SAFE_MODE_INTEROPERABILITY.md) - Safe Mode protocol expectations, JavaScript/Arduino type differences, and validation matrix.

## Memory And Resource Notes

Use `RpcServer<N>` to cap the number of methods at compile time, keep
application-owned `StaticJsonDocument` instances sized for the real payloads, and
ensure documents that own handler return values remain alive until serialization
is complete.

ESP8266 works, but has much tighter stack and heap headroom than ESP32. Measure
free heap and fragmentation in the final sketch when Safe Mode, HTTP, batch
requests, or larger JSON payloads are enabled.

See [`docs/MEMORY_METRICS.md`](docs/MEMORY_METRICS.md) for practical sizing
guidance, measured build deltas, and physical ESP32/ESP8266 heap notes.

## Safe Mode

Safe Mode is an optional RPC Toolkit extension for preserving application-level value types across JSON-RPC 2.0 boundaries.

JSON-RPC 2.0 remains the default wire format. When Safe Mode is disabled, values
are sent as plain JSON values without RPC Toolkit type prefixes.

Enable Safe Mode with:

```cpp
#define RPC_ENABLE_SAFE_MODE 1
```

Important behavior:

- HTTP transports negotiate Safe Mode with `X-RPC-Safe-Enabled`.
- Serial transport can use Safe Mode helper functions, but it does not support HTTP header negotiation.
- Automatic Safe Mode encoding is source-datatype based. ArduinoJson strings are encoded as strings even when they look like `S:`, `D:`, or BigInt marker values.
- ArduinoJson has no native JavaScript `Date` or `BigInt` value types. Date and BigInt markers are preserved as strings unless the sketch explicitly parses them with helper APIs.
- ESP32 Safe Mode HTTP interoperability has been physically validated against `rpc-express-toolkit`.
- ESP8266 HTTP client behavior has been physically validated against an ESP32 Arduino HTTP server with Safe Mode enabled.

See [`docs/SAFE_MODE_INTEROPERABILITY.md`](docs/SAFE_MODE_INTEROPERABILITY.md)
for protocol details, marker examples, strict-mode behavior, and the validation
matrix. See `examples/SafeMode/` and `examples/SafeModeInteropTest/` for Arduino
examples.

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
#define RPC_ENABLE_SAFE_MODE 0      // Enable RPC Toolkit Safe Mode
#define RPC_SAFE_STRICT_MODE 1      // Require Safe Mode HTTP header when Safe Mode is enabled
#define RPC_ENABLE_BATCH 1          // Enable JSON-RPC batch requests
#define RPC_ENABLE_LOGGING 0        // Enable debug logging
#define RPC_ENABLE_NOTIFICATIONS 1  // Enable fire-and-forget calls
#define RPC_ENABLE_SCHEMA_SUPPORT 1 // Enable method description metadata

// Timeouts
#define RPC_DEFAULT_TIMEOUT 5000    // Default timeout (ms)
#define RPC_SERIAL_TIMEOUT 1000     // Serial read timeout (ms)
```

## Cross-Platform Compatibility

Designed to work with standard JSON-RPC 2.0 clients and servers in the RPC Toolkit ecosystem by default. Optional HTTP Safe Mode interoperability follows the same header and value-prefix conventions used by `rpc-express-toolkit`. ESP32 has physical reference validation against `rpc-express-toolkit`; ESP8266 HTTP client behavior has physical board-to-board validation against an ESP32 Arduino server.

Safe Mode interoperability details are maintained in [`docs/SAFE_MODE_INTEROPERABILITY.md`](docs/SAFE_MODE_INTEROPERABILITY.md).

- **rpc-express-toolkit** (Node.js/Express)
- **rpc-php-toolkit** (PHP)
- **rpc-dotnet-toolkit** (.NET)
- **node-red-contrib-rpc-toolkit** (Node-RED)

### Example: ESP32/ESP8266 to Node.js Server

Use `RpcHttpClientTransport` to call a remote JSON-RPC HTTP endpoint from an ESP32 or ESP8266 client. See `examples/WiFiHttpClient/` for a complete sketch with platform-specific WiFi includes.

**HTTP Client:**
```cpp
#include <WiFi.h>
#include <RpcClient.h>
#include <RpcHttpClientTransport.h>

WiFiClient httpClient;
RpcHttpClientTransport transport(httpClient, "192.168.1.100", 3000, "/api");
RpcClient rpc(transport);

float result = rpc.call("add", "{\"a\":5,\"b\":3}").result<float>();
```

Physical reference and board-to-board interoperability results are tracked in the Safe Mode interoperability document.

**Node.js Server (Express):**
```javascript
const express = require('express');
const { RpcEndpoint } = require('rpc-express-toolkit');

const app = express();
app.use(express.json());

const rpc = new RpcEndpoint(app, {}, { endpoint: '/api' });

rpc.addMethod('add', (req, ctx, params) => {
    return params.a + params.b;
});

app.listen(3000);
```

For an RPC Toolkit Safe Mode HTTP endpoint, compile the Arduino sketch with `-DRPC_ENABLE_SAFE_MODE=1` and use `rpc-express-toolkit/safe` or an endpoint configured with `safeEnabled: true`:

```javascript
const { RpcSafeEndpoint } = require('rpc-express-toolkit/safe');
const rpc = new RpcSafeEndpoint(app, {}, { endpoint: '/api' });
```

Both sides exchange `X-RPC-Safe-Enabled` and recursively encode/decode params and results.

## Examples

See the `examples/` folder for complete working examples:

- **BasicServer** - Simple RPC server on Serial
- **BasicClient** - Simple RPC client on Serial
- **WiFiServer** - ESP32 HTTP RPC server
- **WiFiHttpClient** - ESP32/ESP8266 HTTP RPC client
- **Introspection** - Demonstrates __rpc.* methods and description metadata
- **SafeMode** - Optional Safe Mode helper utilities and value-prefix conventions
- **SafeModeInteropTest** - Focused Safe Mode marker-like string round-trip checks

## Related Projects

RPC Toolkit ecosystem projects. Standard JSON-RPC interoperability is the default compatibility target; optional Safe Mode HTTP interoperability follows the shared RPC Toolkit conventions. ESP32 Safe Mode HTTP interoperability has physical reference validation coverage; ESP8266 HTTP client behavior has physical board-to-board validation.

- [rpc-express-toolkit](https://github.com/n-car/rpc-express-toolkit) - Node.js/Express implementation
- [rpc-php-toolkit](https://github.com/n-car/rpc-php-toolkit) - PHP implementation
- [rpc-dotnet-toolkit](https://github.com/n-car/rpc-dotnet-toolkit) - .NET implementation
- [node-red-contrib-rpc-toolkit](https://github.com/n-car/node-red-contrib-rpc-toolkit) - Node-RED visual programming

## Development

### Build Examples

This repository does not require a bundled PlatformIO project to use the
examples. Open the sketches in Arduino IDE, or create a small PlatformIO project
that installs the library through the PlatformIO Registry entry or GitHub URL
shown above.

For ad-hoc PlatformIO checks, `pio ci` can compile a sketch against a selected
board and the local library path.

### Run Tests

```bash
# Native tests (host platform)
pio test -e native
```

## Roadmap

The initial `1.0.0` registry release is complete. Current follow-up work is
focused on ArduinoJson 7 compatibility review, additional ESP8266 reference
validation, optional schema validation, and future transport/discovery options.

See [`docs/ROADMAP.md`](docs/ROADMAP.md) for the full checklist.

## Contributing

Contributions, bug reports, and compatibility feedback are welcome. Please open an issue or pull request on GitHub.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Built on [ArduinoJson](https://arduinojson.org/) library
- Compatible with Arduino Core for ESP32/ESP8266
- Part of the RPC Toolkit ecosystem

---

**RPCToolkit** - JSON-RPC 2.0 for embedded projects.
