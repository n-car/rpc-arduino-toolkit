# RPC Arduino Toolkit

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Arduino](https://img.shields.io/badge/Arduino-Compatible-green.svg)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)

Lightweight JSON-RPC 2.0 client and server library for Arduino, ESP32, ESP8266, and other embedded platforms. Part of the RPC Toolkit ecosystem with cross-platform compatibility.

## 🎯 Features

### Core Features
- **JSON-RPC 2.0 Compliance** - Full spec support with minimal memory footprint
- **Client & Server** - Both RPC client and server implementations
- **Built-in Introspection** - API discovery with `__rpc.listMethods` and `__rpc.version`
- **Multiple Transports** - Serial, WiFi, Bluetooth LE, LoRa
- **Memory Efficient** - Static allocation, minimal RAM usage
- **Cross-Platform** - Works with Node.js, PHP, and .NET servers

### Supported Platforms
- ✅ **Arduino** (Uno, Mega, Nano, etc.) - 2KB+ RAM
- ✅ **ESP32** - WiFi/BLE integrated, 520KB RAM
- ✅ **ESP8266** - WiFi, 80KB RAM
- ✅ **STM32** - Via Arduino framework
- ✅ **Raspberry Pi Pico** (RP2040)

### Transport Options
- **Serial/UART** - USB, hardware serial
- **WiFi** - ESP32/ESP8266 HTTP client/server
- **Bluetooth LE** - ESP32 BLE
- **LoRa** - Long-range IoT communication (optional)

## 📦 Installation

### Arduino IDE
1. Open Arduino IDE
2. Go to **Sketch > Include Library > Manage Libraries**
3. Search for "RPC Arduino Toolkit"
4. Click **Install**

### PlatformIO
Add to `platformio.ini`:
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    n-car/RpcArduinoToolkit
```

### Manual Installation
1. Download the latest release
2. Extract to `Arduino/libraries/RpcArduinoToolkit`
3. Restart Arduino IDE

## 🚀 Quick Start

### Server Example (ESP32 - WiFi)

```cpp
#include <WiFi.h>
#include <RpcServer.h>
#include <RpcWiFiTransport.h>

// Create server with max 8 methods
RpcServer<8> rpc;
WiFiServer server(8080);

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
    rpc.addMethod("led", [](JsonObject params) -> JsonVariant {
        int pin = params["pin"];
        bool state = params["state"];
        digitalWrite(pin, state ? HIGH : LOW);
        return true;
    });
    
    rpc.addMethod("readTemp", []() -> JsonVariant {
        // Read temperature sensor (example)
        float temp = analogRead(A0) * 0.1;
        return temp;
    });
    
    // Start server
    server.begin();
}

void loop() {
    WiFiClient client = server.available();
    if (client) {
        RpcWiFiTransport transport(client);
        String response = rpc.handleRequest(transport);
        client.print(response);
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

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    
    // Register LED control
    rpc.addMethod("setLED", [](JsonObject params) -> JsonVariant {
        bool state = params["state"];
        digitalWrite(LED_BUILTIN, state ? HIGH : LOW);
        return state;
    });
    
    // Register analog read
    rpc.addMethod("readAnalog", [](JsonObject params) -> JsonVariant {
        int pin = params["pin"];
        return analogRead(pin);
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

## 📚 Advanced Usage

### Batch Requests

```cpp
// Client sends multiple requests at once
String batch = "[{\"jsonrpc\":\"2.0\",\"method\":\"readTemp\",\"id\":1},"
               "{\"jsonrpc\":\"2.0\",\"method\":\"readHumidity\",\"id\":2}]";
               
RpcResponse resp = rpc.callBatch(batch);
```

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

// __rpc.describe - Get method description and schema info (requires RPC_ENABLE_SCHEMA_SUPPORT)
resp = rpc.call("__rpc.describe", "{\"method\":\"add\"}");
// Result: {"name":"add","description":"Add two numbers","exposeSchema":true}

// __rpc.capabilities - Get server capabilities
resp = rpc.call("__rpc.capabilities");
// Result: {"batch":true,"introspection":true,"safeMode":false,"schemaSupport":true,"methodCount":5,"maxMethods":8}
```

**Features:**
- Automatically available on all RPC servers
- No registration needed - built into `executeMethod()`
- Minimal memory footprint (~800 bytes)
- Compatible with cross-platform toolkits
- Schema support optional (can be disabled to save ~200 bytes per method)

**Register methods with schema information:**

```cpp
// Simple method without schema
rpc.addMethod("ping", []() -> JsonVariant {
    return "pong";
});

// Method with description and schema exposure
rpc.addMethod("add", [](JsonObject params) -> JsonVariant {
    int a = params["a"] | 0;
    int b = params["b"] | 0;
    return a + b;
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

## 🎨 Memory Optimization

### Static Allocation

```cpp
// Specify max methods at compile time
RpcServer<8> server;  // 8 methods max

// Use StaticJsonDocument for predictable memory
StaticJsonDocument<512> doc;
```

### Flash Storage (PROGMEM)

```cpp
// Store strings in flash memory
const char METHOD_NAME[] PROGMEM = "myMethod";

rpc.addMethod(FPSTR(METHOD_NAME), []() {
    return 42;
});
```

### Disable Features

```cpp
// In RpcConfig.h or build flags
#define RPC_ENABLE_SAFE_MODE 0      // Disable safe mode (save ~1KB)
#define RPC_ENABLE_BATCH 0          // Disable batch (save ~500B)
#define RPC_ENABLE_SCHEMA_SUPPORT 0 // Disable schema support (save ~200B/method)
#define RPC_MAX_METHOD_NAME 16      // Limit method name length
#define RPC_MAX_PARAMS_SIZE 256     // Limit params size
```

### Safe Mode Serialization

Safe mode adds prefixes to disambiguate types when serializing to JSON (disabled by default to save memory):

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

**Safe Mode Features:**
- String prefix: `S:` - Distinguishes strings from other types
- Date prefix: `D:` - Marks timestamps/dates
- BigInt suffix: `n` - Marks large integers (like JavaScript BigInt)
- Compatible with Express, PHP, .NET, Java implementations
- Prevents type confusion in JSON serialization
- Disabled by default (enable with `RPC_ENABLE_SAFE_MODE=1`)

See `examples/SafeMode/` for complete example.

## 🔧 Configuration

### RpcConfig.h

```cpp
// Memory limits
#define RPC_MAX_METHODS 8           // Maximum registered methods
#define RPC_MAX_REQUEST_SIZE 512    // Max JSON request size
#define RPC_MAX_RESPONSE_SIZE 512   // Max JSON response size
#define RPC_MAX_METHOD_NAME 32      // Max method name length
#define RPC_MAX_DESCRIPTION 64      // Max description length (schema support)

// Features
#define RPC_ENABLE_SAFE_MODE 0      // Enable safe serialization (S:, D:, n)
#define RPC_ENABLE_BATCH 1          // Enable batch requests
#define RPC_ENABLE_LOGGING 0        // Enable debug logging
#define RPC_ENABLE_NOTIFICATIONS 1  // Enable fire-and-forget calls
#define RPC_ENABLE_SCHEMA_SUPPORT 1 // Enable method descriptions

// Timeouts
#define RPC_DEFAULT_TIMEOUT 5000    // Default timeout (ms)
#define RPC_SERIAL_TIMEOUT 1000     // Serial read timeout (ms)
```

## 📊 Memory Usage

| Platform | Flash (Code) | RAM (Static) | RAM (Runtime) | Features |
|----------|--------------|--------------|---------------|----------|
| Arduino Uno | ~8KB | ~200B | ~512B | Basic |
| Arduino Uno + Schema | ~9KB | ~400B | ~512B | +Schema |
| ESP32 | ~12KB | ~300B | ~1KB | Full |
| ESP8266 | ~10KB | ~250B | ~800B | Full |

**Feature Impact:**
- `RPC_ENABLE_SCHEMA_SUPPORT=1`: +200 bytes per method (description storage)
- `RPC_ENABLE_SAFE_MODE=1`: +1KB Flash, +50 bytes RAM
- `RPC_ENABLE_BATCH=1`: +500 bytes Flash

*Note: Values depend on enabled features and registered methods*

## 🌐 Cross-Platform Compatibility

Works seamlessly with:
- ✅ **rpc-express-toolkit** (Node.js/Express)
- ✅ **rpc-php-toolkit** (PHP)
- ✅ **rpc-dotnet-toolkit** (.NET)

### Example: ESP32 → Node.js Server

**ESP32 Client:**
```cpp
WiFiClient client;
client.connect("192.168.1.100", 3000);

RpcWiFiTransport transport(client);
RpcClient rpc(transport);

float result = rpc.call("add", "{\"a\":5,\"b\":3}").result<float>();
```

**Node.js Server (Express):**
```javascript
const { RpcEndpoint } = require('rpc-express-toolkit');
const rpc = new RpcEndpoint('/api/rpc');

rpc.addMethod('add', (params) => {
    return params.a + params.b;
});
```

## 🧪 Examples

See the `examples/` folder for complete working examples:

- **BasicServer** - Simple RPC server on Serial
- **WiFiServer** - ESP32 HTTP RPC server
- **WiFiClient** - ESP32 calling remote server
- **SerialBridge** - Arduino ↔ ESP32 bridge
- **SensorHub** - Multi-sensor data collection
- **BLEServer** - Bluetooth LE RPC server (ESP32)
- **LoRaNode** - Long-range IoT node
- **Introspection** - Demonstrates __rpc.* methods and schema support
- **SafeMode** - Safe serialization with S:, D:, n prefixes

## 📖 API Reference

### RpcServer

```cpp
template<uint8_t MAX_METHODS>
class RpcServer {
public:
    // Register a method
    bool addMethod(const char* name, RpcMethodHandler handler);
    
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
    RpcResponse callBatch(const String& batch);
    
    // Set timeout
    void setTimeout(unsigned long ms);
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

## 🔗 Related Projects

- [rpc-express-toolkit](https://github.com/n-car/rpc-express-toolkit) - Node.js/Express implementation
- [rpc-php-toolkit](https://github.com/n-car/rpc-php-toolkit) - PHP implementation
- [rpc-dotnet-toolkit](https://github.com/n-car/rpc-dotnet-toolkit) - .NET implementation
- [rpc-java-toolkit](https://github.com/n-car/rpc-java-toolkit) - Java & Android implementation
- [node-red-contrib-rpc-toolkit](https://github.com/n-car/node-red-contrib-rpc-toolkit) - Node-RED visual programming
- [rpc-arduino-toolkit](https://github.com/n-car/rpc-arduino-toolkit) - Arduino/ESP32 implementation (you are here)

## 🛠️ Development

### Build Examples

```bash
# Using PlatformIO
pio run -e esp32dev
pio run -e arduino_uno
pio run -e esp8266

# Upload to device
pio run -e esp32dev --target upload
```

### Run Tests

```bash
# Native tests (host platform)
pio test -e native
```

## 📝 Roadmap

### v1.0.0 (Current Development)
- [x] Core RPC client/server
- [x] Serial transport
- [x] Basic JSON parser
- [ ] WiFi transport (ESP32/ESP8266)
- [ ] Complete examples
- [ ] Documentation

### v1.1.0
- [ ] Bluetooth LE transport (ESP32)
- [ ] Safe Mode support
- [ ] Batch requests
- [ ] Schema validation

### v1.2.0
- [ ] LoRa transport
- [ ] WebSocket support
- [ ] mDNS discovery
- [ ] OTA updates integration

## 🤝 Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Built on [ArduinoJson](https://arduinojson.org/) library
- Compatible with Arduino Core for ESP32/ESP8266
- Part of the RPC Toolkit ecosystem

---

**RPC Arduino Toolkit** - Bring JSON-RPC 2.0 to your embedded projects.
