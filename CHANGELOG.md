# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
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
