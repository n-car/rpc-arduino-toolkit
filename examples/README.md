# Examples

This directory contains example sketches demonstrating RPCToolkit.

Examples intentionally use selective includes instead of umbrella headers such as `RPCToolkit.h` or `RpcArduinoToolkit.h` so sketches only pull the client, server, and transport pieces they use.

## Available Examples

### BasicServer
Simple RPC server that communicates over Serial. Demonstrates:
- Method registration
- LED control via RPC
- Analog pin reading
- Status reporting

**Hardware:** Arduino-compatible board with C++ `std::function` support

**Usage:** Open Serial Monitor, send JSON-RPC commands

### BasicClient
RPC client that calls methods on a remote server via Serial. Demonstrates:
- Making RPC calls
- Handling responses
- Error handling

**Hardware:** Arduino-compatible board with C++ `std::function` support

**Usage:** Connect to BasicServer via Serial

### WiFiServer (ESP32/ESP8266)
HTTP-based RPC server with WiFi. Demonstrates:
- WiFi connectivity
- HTTP server transport
- Web-based RPC calls

**Hardware:** ESP32 or ESP8266

**Usage:** Access via HTTP POST to device IP

### WiFiHttpClient (ESP32/ESP8266)
HTTP-based RPC client with WiFi. Demonstrates:
- WiFi connectivity
- `RpcHttpClientTransport`
- Standard JSON-RPC calls to a remote HTTP endpoint
- HTTP/RPC error reporting

**Hardware:** ESP32 or ESP8266

**Usage:** Update WiFi credentials and the remote JSON-RPC endpoint, then upload. The example calls `ping` and `add`; change those calls if your server exposes different methods. Compile with `RPC_ENABLE_SAFE_MODE=1` only when the remote endpoint supports RPC Toolkit Safe Mode.

`RpcHttpClientTransport` works with `WiFiClient`, `EthernetClient`, or another Arduino `Client` implementation.

### Introspection
Demonstrates built-in `__rpc.*` methods, including method listing, version metadata, description metadata, and capabilities.

### SafeMode
Demonstrates optional RPC Toolkit Safe Mode helper utilities and value-prefix conventions. HTTP transports perform Safe Mode header negotiation when `RPC_ENABLE_SAFE_MODE=1`; Serial examples do not require headers.

Safe Mode preserves marker intent over JSON, but ArduinoJson values remain JSON values. Date markers and BigInt markers are exposed as strings unless the sketch explicitly parses them with helper APIs.

### SafeModeInteropTest
Focused compile/runtime sketch for automatic Safe Mode source-datatype encoding. It verifies that normal strings starting with `S:`, `D:`, or ending with `n` are protected by the `S:` string prefix and round-trip as strings.

For HTTP interoperability testing against `rpc-express-toolkit` Safe Mode, see `../docs/SAFE_MODE_INTEROPERABILITY.md`.

## Running Examples

### Arduino IDE
1. Open File > Examples > RPCToolkit
2. Select the example you want
3. Upload to your board

### PlatformIO
Create a PlatformIO project for your board, add the GitHub `lib_deps` entry from the main README, and copy or open the example sketch you want to run.

For quick compile checks against a local checkout, use `pio ci` with the target board and the library path.

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
