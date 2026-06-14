# Roadmap

## v1.0.0 - Initial Public Release

- [x] Core RPC client/server
- [x] Serial transport
- [x] HTTP client transport over Arduino Client-compatible sockets
- [x] HTTP server transport over Arduino Client-compatible sockets
- [x] Built-in introspection
- [x] Batch requests
- [x] Optional Safe Mode helper utilities
- [x] RPC Toolkit Safe Mode HTTP header negotiation
- [x] Recursive Safe Mode encode/decode for params and results
- [x] Safe Mode interoperability documentation and test matrix
- [x] Focused Safe Mode marker-like string round-trip test sketch
- [x] Basic examples for Serial, WiFi, introspection, and Safe Mode
- [x] Capture ESP32 Safe Mode interoperability validation evidence
- [x] Capture ESP8266 HTTP client runtime validation evidence
- [x] Prepare Arduino Library Manager-compliant package metadata
- [x] Prepare PlatformIO package metadata
- [x] Stabilize public API for initial 1.0.0 metadata
- [x] Publish first GitHub prerelease (`v1.0.0-rc.1`)

## Registry Publication

- [x] Publish tagged GitHub release
- [x] Arduino Library Manager submission accepted
- [x] Arduino Library Manager indexing complete
- [x] PlatformIO Registry publication

## Next Compatibility Work

- [ ] Review ArduinoJson 7 compatibility before expanding the supported dependency range
- [ ] Rerun reference Safe Mode interoperability checks before claiming additional ESP8266 reference/server coverage
- [ ] Optional schema-based params/result validation

## Future Transport And Discovery Work

- [ ] Bluetooth LE transport for ESP32
- [ ] LoRa transport
- [ ] WebSocket support
- [ ] mDNS discovery
- [ ] OTA updates integration

