# Examples

This directory contains example sketches demonstrating the RPC Arduino Toolkit.

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
- HTTP transport
- Web-based RPC calls

**Hardware:** ESP32 or ESP8266

**Usage:** Access via HTTP POST to device IP

### WiFiClient (ESP32/ESP8266)
WiFi client calling remote RPC servers. Demonstrates:
- HTTP client
- Calling Node.js/PHP servers
- Cross-platform RPC

**Hardware:** ESP32 or ESP8266

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
