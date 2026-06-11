# Memory Metrics

Measured on 2026-06-11 with PlatformIO build scenarios and physical ESP32
board-to-board heap testing.

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

## Current Limitations

- ESP8266 values above are compile/link metrics only; physical runtime heap
  validation is still pending.
- Runtime heap values include Arduino core, WiFi stack, ArduinoJson, test code,
  Serial logging, Safe Mode, schema metadata on the server, and test methods.
- Build deltas are useful for trend comparisons, but application firmware should
  still be measured with its real methods, documents, and transports.
