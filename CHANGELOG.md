# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.1] - 2026-10-12

### Fixed

- Validate JSON-RPC envelopes before dispatch. Explicit `id: null` receives a response; only a missing ID denotes a notification. Invalid params and IDs return `-32600` with `id: null`, including inside batches.
- Accept HTTP 204 as successful completion of notification-only batches, without hiding missing responses to calls or accepting empty HTTP 200 responses.
- Preserve notification and invalid-envelope behavior when strict Safe Mode headers are missing.
- Use heap-backed response and batch-response documents on ESP32, as on ESP8266, to prevent response temporaries from overflowing the default task stack.

### Tests and documentation

- Add release-build-safe protocol checks and four PlatformIO test environments. Hardware checks on 2026-10-12 passed 54 standard and 57 Safe Mode checks on each board, plus 23 HTTP checks per direction/mode between ESP32 and ESP8266.
- Update API, transport, memory, and interoperability documentation. These are focused regression results, not a long-duration or exhaustive memory test.

## [1.1.0] - 2026-09-24

### Added
- Added `RPC_ENABLE_INTROSPECTION` so production firmware can omit built-in `__rpc.*` methods.
- Initial project structure
- RpcServer class for handling JSON-RPC 2.0 requests
- RpcClient class for making JSON-RPC 2.0 calls
- RpcTransport abstract base class
- RpcSerialTransport for Serial/UART communication
- RpcWiFiTransport for ESP32/ESP8266 WiFi
- Memory-efficient static allocation
- Configurable features via RpcConfig.h
- Basic examples (BasicServer, BasicClient)
- ArduinoJson 6.x integration
- Support for Arduino, ESP32, ESP8266

### Changed
- N/A

### Deprecated
- N/A

### Removed
- N/A

### Fixed
- N/A

### Security
- N/A

## [1.0.0] - TBD

Initial release with core features:
- JSON-RPC 2.0 compliance
- Client and Server implementations
- Serial transport
- WiFi transport (ESP32/ESP8266)
- Memory-efficient design
- Cross-platform compatibility
