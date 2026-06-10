# Examples

This directory contains example sketches demonstrating the RPC Arduino Toolkit.

Examples intentionally use selective includes instead of `RpcArduinoToolkit.h` so sketches only pull the client, server, and transport pieces they use.

## Available Examples

### BasicServer
Simple RPC server that communicates over Serial. Demonstrates:
- Method registration
- LED control via RPC
- Analog pin reading
- Status reporting

**Hardware:** Any Arduino board

**Usage:** Open Serial Monitor, send JSON-RPC commands

### BasicClient
RPC client that calls methods on a remote server via Serial. Demonstrates:
- Making RPC calls
- Handling responses
- Error handling

**Hardware:** Any Arduino board

**Usage:** Connect to BasicServer via Serial

### WiFiServer (ESP32/ESP8266)
HTTP-based RPC server with WiFi. Demonstrates:
- WiFi connectivity
- HTTP server transport
- Web-based RPC calls

**Hardware:** ESP32 or ESP8266

**Usage:** Access via HTTP POST to device IP

### HTTP Client
Use `RpcHttpClientTransport` with `WiFiClient`, `EthernetClient`, or another Arduino `Client` implementation to call remote JSON-RPC HTTP endpoints.

### Introspection
Demonstrates built-in `__rpc.*` methods, including method listing, version metadata, description metadata, and capabilities.

### SafeMode
Demonstrates optional RPC Toolkit Safe Mode helper utilities and value-prefix conventions. HTTP transports perform Safe Mode header negotiation when `RPC_ENABLE_SAFE_MODE=1`; Serial examples do not require headers.

### SafeModeInteropTest
Focused compile/runtime sketch for automatic Safe Mode source-datatype encoding. It verifies that normal strings starting with `S:`, `D:`, or ending with `n` are protected by the `S:` string prefix and round-trip as strings.

For HTTP interoperability testing against `rpc-express-toolkit` Safe Mode, see `../docs/SAFE_MODE_INTEROPERABILITY_TEST_PLAN.md`.

## Running Examples

### Arduino IDE
1. Open File > Examples > RPC Arduino Toolkit
2. Select the example you want
3. Upload to your board

### PlatformIO
```bash
cd examples/BasicServer
pio run --target upload
```

## Example JSON-RPC Commands

### Turn LED ON
```json
{"jsonrpc":"2.0","method":"setLED","params":{"state":true},"id":1}
```

### Read Analog Pin
```json
{"jsonrpc":"2.0","method":"readAnalog","params":{"pin":0},"id":2}
```

### Ping
```json
{"jsonrpc":"2.0","method":"ping","id":3}
```

### Get Status
```json
{"jsonrpc":"2.0","method":"getStatus","id":4}
```

## Testing with curl

For WiFi examples:
```bash
curl -X POST http://192.168.1.100:8080 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"ping","id":1}'
```
