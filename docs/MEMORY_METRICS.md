# Memory Metrics

Measured with PlatformIO build scenarios and physical WiFi/HTTP heap testing:

- 2026-06-11: build deltas and ESP32 board-to-board heap testing.
- 2026-06-12: ESP8266EX HTTP client heap testing against an ESP32 Arduino HTTP server.

These values are intended as representative engineering measurements, not fixed
guarantees. Actual memory usage depends on the board core, ArduinoJson version,
compiler and linker settings, enabled feature flags, transport, registered
methods, JSON document sizes, and surrounding sketch code.

## Build Delta Method

Build metrics are generated from minimal firmware scenarios and compared against
a baseline sketch for the same board/core. Deltas include transitive code pulled
by headers, templates, ArduinoJson, WiFi core classes, and scenario code. They
are not isolated object-file sizes for the library alone.

Tooling:

- PlatformIO Core 6.1.19
- ArduinoJson 6.21.0 in the metrics scenarios
- ESP32 board: PlatformIO `esp32dev`
- ESP8266 board: PlatformIO `nodemcuv2`

## Build Deltas

### ESP32 `esp32dev`

| Scenario | RAM Used | Static RAM Delta | Flash Used | Flash Delta |
| --- | ---: | ---: | ---: | ---: |
| baseline | 45,232 B | 0 B | 884,428 B | 0 B |
| JSON only | 45,536 B | +304 B | 884,876 B | +448 B |
| RPC types | 47,888 B | +2,656 B | 888,304 B | +3,876 B |
| Serial client | 47,608 B | +2,376 B | 893,900 B | +9,472 B |
| Serial server | 48,128 B | +2,896 B | 899,156 B | +14,728 B |
| HTTP client | 46,928 B | +1,696 B | 911,948 B | +27,520 B |
| HTTP server | 49,440 B | +4,208 B | 915,320 B | +30,892 B |
| Safe HTTP client | 46,928 B | +1,696 B | 915,824 B | +31,396 B |
| Safe HTTP server | 49,440 B | +4,208 B | 918,016 B | +33,588 B |
| Safe HTTP server, schema disabled | 49,184 B | +3,952 B | 917,212 B | +32,784 B |
| Safe HTTP server, batch disabled | 49,440 B | +4,208 B | 916,740 B | +32,312 B |

### ESP8266 `nodemcuv2`

| Scenario | RAM Used | Static RAM Delta | Flash Used | Flash Delta |
| --- | ---: | ---: | ---: | ---: |
| baseline | 28,132 B | 0 B | 265,891 B | 0 B |
| JSON only | 28,448 B | +316 B | 266,407 B | +516 B |
| RPC types | 29,964 B | +1,832 B | 269,747 B | +3,856 B |
| Serial client | 29,844 B | +1,712 B | 275,819 B | +9,928 B |
| Serial server | 30,764 B | +2,632 B | 280,059 B | +14,168 B |
| HTTP client | 29,384 B | +1,252 B | 287,451 B | +21,560 B |
| HTTP server | 31,056 B | +2,924 B | 289,659 B | +23,768 B |
| Safe HTTP client | 29,464 B | +1,332 B | 290,091 B | +24,200 B |
| Safe HTTP server | 31,056 B | +2,924 B | 291,491 B | +25,600 B |
| Safe HTTP server, schema disabled | 30,672 B | +2,540 B | 290,907 B | +25,016 B |
| Safe HTTP server, batch disabled | 31,056 B | +2,924 B | 290,723 B | +24,832 B |

## ESP32 Runtime Heap

Runtime heap was measured on two physical ESP32 boards running the WiFi HTTP
client/server interoperability firmware with Safe Mode enabled.

Client-side full suite result:

```text
SUMMARY pass=21 fail=0 gap=0
```

Client heap:

| Metric | Value |
| --- | ---: |
| Heap size | 333,172 B |
| Suite start free heap | 228,520 B |
| Suite end free heap | 227,280 B |
| Lowest observed `minFree` | 220,924 B |
| Max allocatable block | 110,580 B |
| Low-water delta from suite start | 7,596 B |

Server heap:

| Metric | Value |
| --- | ---: |
| Heap size | 330,084 B |
| Setup start free heap | 277,556 B |
| After WiFi free heap | 225,436 B |
| After method registration free heap | 225,436 B |
| Lowest observed request `minFree` | 212,328 B |
| Lowest printed request-after free heap | 214,676 B |
| Max allocatable block | 110,580 B |
| HTTP requests during suite | 36 |
| Low-water delta from after-WiFi free heap | 13,108 B |

## ESP8266 Runtime Heap

Runtime heap was measured on a physical ESP8266EX running the WiFi HTTP client
interoperability firmware against an ESP32 Arduino HTTP server with Safe Mode
enabled.

Client-side full suite result:

```text
SUMMARY pass=21 fail=0 gap=0
```

Client heap:

| Metric | Value |
| --- | ---: |
| Suite start free heap | 47,480 B |
| Suite start max free block | 47,032 B |
| Suite start fragmentation | 1% |
| Suite end free heap | 45,928 B |
| Suite end max free block | 44,040 B |
| Suite end fragmentation | 5% |
| Free heap delta during suite | 1,552 B |

ESP8266 does not expose the same `minFree` and heap-size APIs as ESP32 in this
test sketch, so the table records the serial snapshots printed by the firmware
before and after the suite.

## Current Limitations

- Runtime heap values include Arduino core, WiFi stack, ArduinoJson, test code,
  Serial logging, Safe Mode, schema metadata on the server, and test methods.
- Build deltas are useful for trend comparisons, but application firmware should
  still be measured with its real methods, documents, and transports.
- The captured ESP8266 runtime result covers the HTTP client path. ESP8266 HTTP
  server applications should still be measured on the target board with their
  real handlers and response sizes.
