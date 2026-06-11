# Safe Mode Interoperability Test Plan

This plan tracks `rpc-arduino-toolkit` interoperability with `rpc-express-toolkit` Safe Mode endpoints.

`rpc-express-toolkit` is the reference implementation for HTTP Safe Mode behavior in these tests. The Arduino library must remain standard JSON-RPC 2.0 compatible by default, and Safe Mode must only affect HTTP requests/responses when it is explicitly enabled.

## Scope

Covered:
- Arduino HTTP client to Node.js `RpcSafeEndpoint`.
- Node.js `RpcSafeClient` to Arduino HTTP server.
- Standard `safe=false` clients calling `safe=true` servers.
- Missing `X-RPC-Safe-Enabled` header with strict mode enabled.
- Recursive object and array params/results.
- Strings beginning with `S:` and `D:`.
- ISO date strings.
- BigInt marker strings ending with `n`.
- Batch requests and notifications.
- JSON-RPC errors with `error.data`.

Not covered in this pass:
- Schema validation.
- Bluetooth LE, LoRa, WebSocket, or mDNS transport tests.
- AVR validation.

Status terms used below:
- **Covered by sketch**: exercised by a public Arduino example/test sketch, but not necessarily over HTTP.
- **Physical evidence needed**: the test requires ESP32/ESP8266 hardware and a live Node.js endpoint; capture serial/LAN logs and pass/fail summary before treating it as release evidence.
- **Implementation difference**: behavior is intentionally different between JavaScript and Arduino because ArduinoJson has no native `Date` or `BigInt` value types.

## Protocol Expectations

- Standard JSON-RPC 2.0 remains the default. `RPC_ENABLE_SAFE_MODE` defaults to `0`.
- Safe Mode over HTTP is opt-in and uses `X-RPC-Safe-Enabled: true|false`.
- `RpcHttpClientTransport` sends the header and records the server response header.
- `RpcHttpServerTransport` reads the client header and sends its local Safe Mode state in the response header.
- With `RPC_ENABLE_SAFE_MODE=1` and `RPC_SAFE_STRICT_MODE=1`, Arduino HTTP server requests without `X-RPC-Safe-Enabled` return JSON-RPC error `-32600` for calls with ids. Notifications return no JSON-RPC body. Batch notifications produce no response entries.
- `rpc-express-toolkit` strict-mode compatibility errors include diagnostic `error.data`. Arduino strict-mode compatibility errors currently return code/message only. This is a known implementation difference, not a failure of `error.data` support for application errors.
- Automatic Safe Mode encoding is source-datatype based. Every ArduinoJson string is encoded with `S:`, including strings that already start with `S:`, start with `D:`, or end with `n`.
- Serial transport can use Safe Mode helper encoding/decoding, but it does not support HTTP header negotiation and must not require Safe Mode headers.
- Arduino keeps BigInt marker values as strings such as `"9007199254740993n"`. It does not expose JavaScript `BigInt` semantics.
- Arduino decodes `D:<value>` markers by removing the `D:` prefix and keeping the value as a string in generic JSON results/params. Date helper APIs may parse timestamp strings explicitly when sketches call them.
- Helper output such as `RpcSafe::serializeDate()` is treated as a normal string if inserted into ArduinoJson and passed through automatic Safe Mode encoding.
- A standard or `safe=false` client may call a strict Safe Mode server as long as it sends `X-RPC-Safe-Enabled: false`. The server may still answer with `X-RPC-Safe-Enabled: true` and Safe Mode encoded response values when the server itself has Safe Mode enabled.

## Reference Test Setup

### Node.js Safe Endpoint

Use `rpc-express-toolkit/safe` so the Node side is the Safe Mode reference implementation. The current reference version used for this plan is `rpc-express-toolkit` 4.3.x or later.

Prefer `new RpcSafeEndpoint(...)`; `createSafeEndpoint(...)` is kept by `rpc-express-toolkit` for compatibility but is deprecated.

```js
const express = require('express');
const { RpcSafeEndpoint } = require('rpc-express-toolkit/safe');

const app = express();
app.use(express.json());

const context = {
  notifications: [],
};

const rpc = new RpcSafeEndpoint(app, context, {
  endpoint: '/api',
  strictMode: true,
});

rpc.addMethod('ping', () => 'pong');
rpc.addMethod('echo', (req, ctx, params) => params);
rpc.addMethod('types', () => ({
  stringValue: 'hello',
  sPrefix: 'S:literal',
  dPrefix: 'D:literal',
  isoDateString: '2026-06-10T12:34:56.000Z',
  dateValue: new Date('2026-06-10T12:34:56.000Z'),
  bigintValue: 9007199254740993n,
  bigintMarkerString: '9007199254740993n',
  nested: {
    array: ['alpha', 'S:beta', { value: 'D:gamma' }],
  },
}));

rpc.addMethod('domainError', () => {
  const error = new Error('Domain failure from rpc-express-toolkit');
  error.code = -32042;
  error.data = {
    reason: 'intentional-test-error',
    marker: 'S:error-data',
    nested: {
      date: new Date('2026-06-10T12:34:56.000Z'),
      bigint: 9007199254740993n,
    },
  };
  throw error;
});

rpc.addMethod('notify.record', (req, ctx, params) => {
  ctx.notifications.push(params);
  return { count: ctx.notifications.length };
});

rpc.addMethod('notify.stats', (req, ctx) => ({
  count: ctx.notifications.length,
  last: ctx.notifications[ctx.notifications.length - 1] || null,
}));

app.listen(3000, '0.0.0.0');
```

### Arduino HTTP Client Firmware

Build an ESP32/ESP8266 client sketch with:

```ini
build_flags =
    -DRPC_ENABLE_SAFE_MODE=1
    -DRPC_SAFE_STRICT_MODE=1
```

Use `RpcHttpClientTransport` pointing at the PC LAN address, not `127.0.0.1`.

### Arduino HTTP Server Firmware

Build an ESP32/ESP8266 server sketch with:

```ini
build_flags =
    -DRPC_ENABLE_SAFE_MODE=1
    -DRPC_SAFE_STRICT_MODE=1
```

Expose at least these methods:
- `ping`
- `echo`
- `types`
- `domainError` through `addResponseMethod`, returning JSON-RPC error `-32042` with `error.data`
- `notify.record`
- `notify.stats`

## Local Executable Checks

These checks do not replace physical HTTP interoperability tests, but they prevent regressions in the Arduino Safe Mode encoder/decoder.

Compile the focused sketch:

```bash
pio ci --lib=. --board esp32dev \
  --project-option "build_flags=-DRPC_ENABLE_SAFE_MODE=1 -DRPC_SAFE_STRICT_MODE=1" \
  examples/SafeModeInteropTest/SafeModeInteropTest.ino
```

Then run the sketch on hardware and confirm Serial output ends with:

```text
SafeModeInteropTest PASS
```

Covered by this sketch:
- Recursive objects and arrays.
- Literal strings beginning with `S:` and `D:`.
- Literal strings ending with `n`.
- Date markers decoded to strings.
- BigInt markers preserved as strings.
- Helper output re-encoded as ordinary strings when it is stored in ArduinoJson before automatic encoding.

Not covered by this sketch:
- HTTP header negotiation.
- `RpcSafeEndpoint` / `RpcSafeClient` interoperability.
- Batch HTTP requests.
- JSON-RPC notification transport behavior.
- Physical ESP32/ESP8266 WiFi behavior.

## Test Matrix

| ID | Direction | Case | Expected Result | Status |
| --- | --- | --- | --- | --- |
| SM-01 | Arduino HTTP client -> Node `RpcSafeEndpoint` | `ping` with no params | Result is `pong`; request and response include Safe Mode header | Physical evidence needed |
| SM-02 | Arduino HTTP client -> Node `RpcSafeEndpoint` | Recursive object/array params through `echo` | Nested strings decode without `S:` prefix; arrays/objects preserve shape | Physical evidence needed |
| SM-03 | Arduino HTTP client -> Node `RpcSafeEndpoint` | Strings beginning with `S:` and `D:` | Literal prefixes are sent as `S:S:...` and `S:D:...` on the wire and round-trip as string values | Physical evidence needed |
| SM-04 | Arduino HTTP client -> Node `RpcSafeEndpoint` | ISO date string param | Value remains an ISO string on Arduino; Node may hydrate Date only for `D:` markers | Physical evidence needed |
| SM-05 | Arduino HTTP client -> Node `RpcSafeEndpoint` | BigInt-looking string ending in `n` | Literal string is sent as `S:9007199254740993n` and round-trips as a string, not JavaScript `BigInt` | Physical evidence needed |
| SM-06 | Arduino HTTP client -> Node `RpcSafeEndpoint` | JSON-RPC application error with `error.data` | Arduino receives error code/message and decoded nested `error.data` | Physical evidence needed |
| SM-07 | Arduino HTTP client -> Node `RpcSafeEndpoint` | Batch with success, domain error, and method-not-found | `RpcBatchResponse` exposes each item in order; error items keep code/message/data | Physical evidence needed |
| SM-08 | Arduino HTTP client -> Node `RpcSafeEndpoint` | Notification `notify.record` | No response body is required; follow-up `notify.stats` confirms execution | Physical evidence needed |
| SM-09 | Node `RpcSafeClient` -> Arduino HTTP server | `ping` with no params | Result is `pong`; request and response include Safe Mode header | Physical evidence needed |
| SM-10 | Node `RpcSafeClient` -> Arduino HTTP server | Recursive object/array params through `echo` | Arduino decodes params recursively; Node receives recursively encoded result | Physical evidence needed |
| SM-11 | Node `RpcSafeClient` -> Arduino HTTP server | Strings beginning with `S:` and `D:` | Literal prefixes arrive as string values after one `S:` layer is decoded | Physical evidence needed |
| SM-12 | Node `RpcSafeClient` -> Arduino HTTP server | ISO date string param | Arduino sees a string; it does not create a Date object | Physical evidence needed |
| SM-13 | Node `RpcSafeClient` -> Arduino HTTP server | BigInt value and BigInt-looking string | Arduino stores both as strings in generic JSON; literal BigInt-looking strings must remain strings when echoed | Physical evidence needed |
| SM-14 | Node `RpcSafeClient` -> Arduino HTTP server | JSON-RPC application error with `error.data` | Node receives decoded `error.data` from Arduino response | Physical evidence needed |
| SM-15 | Node `RpcSafeClient` -> Arduino HTTP server | Batch with success, error, and notification | Response includes only calls with ids; notification executes without response entry | Physical evidence needed |
| SM-16 | Node standard `RpcClient` or raw client -> Safe Mode server | `safe=false` client with header `false` | Server accepts request because header is present; response header reports server Safe Mode state | Physical evidence needed |
| SM-17 | Raw HTTP client -> strict Safe Mode server | Missing `X-RPC-Safe-Enabled` header, request with id | JSON-RPC error `-32600` with compatibility message. Node includes diagnostic `error.data`; Arduino currently does not. | Can be tested with `curl` |
| SM-18 | Raw HTTP client -> strict Safe Mode server | Missing header notification | HTTP `204 No Content` or otherwise empty body; method must not return a JSON-RPC response body | Can be tested with `curl` |
| SM-19 | Raw HTTP client -> strict Safe Mode server | Missing header batch with ids and notifications | Error entries for calls with ids; no entries for notifications. Node entries include diagnostic `error.data`; Arduino entries currently do not. | Can be tested with `curl` |
| SM-20 | Arduino local sketch | Recursive Safe Mode encode/decode | `SafeModeInteropTest` prints `PASS` | Covered by sketch |
| SM-21 | Arduino local sketch | Literal `S:` / `D:` / `n` marker-like strings | Strings are protected by `S:` during encoding and round-trip as strings | Covered by sketch |
| SM-22 | Arduino local sketch | Actual `D:` and BigInt markers from peers | Date marker loses `D:` prefix and remains a string; BigInt marker remains a string with `n` | Covered by sketch |
| SM-23 | Arduino local sketch | Helper output inserted into ArduinoJson | Helper marker strings are encoded as ordinary strings by automatic Safe Mode encoding | Covered by sketch |

## Raw HTTP Strict Mode Checks

Use these against a strict Safe Mode server.

Missing header with id:

```bash
curl -sS -X POST http://DEVICE_OR_NODE_HOST:PORT/api \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"ping","id":1}'
```

Expected:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "error": {
    "code": -32600,
    "message": "RPC Compatibility Error: Server requires safe serialization header but client did not provide it."
  }
}
```

For `rpc-express-toolkit`, the same error also includes diagnostic `error.data` with fields such as `serverSafeEnabled`, `requiredHeader`, and `strictMode`. Arduino strict-mode compatibility errors currently return code/message only.

Safe header explicitly false:

```bash
curl -sS -X POST http://DEVICE_OR_NODE_HOST:PORT/api \
  -H "Content-Type: application/json" \
  -H "X-RPC-Safe-Enabled: false" \
  -d '{"jsonrpc":"2.0","method":"echo","params":{"message":"plain"},"id":2}'
```

Expected:
- Request is accepted.
- Params are interpreted as standard JSON.
- Response includes `X-RPC-Safe-Enabled: true` when the server was built/configured with Safe Mode enabled.
- Response values may be Safe Mode encoded on the wire because the server is Safe Mode enabled. Clients that understand the response header should decode them.

Notification without header:

```bash
curl -i -X POST http://DEVICE_OR_NODE_HOST:PORT/api \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"notify.record","params":{"event":"missing-header-notify"}}'
```

Expected:
- No JSON-RPC response body.
- HTTP status is `204` or otherwise an empty response appropriate for the transport.

Batch without header:

```bash
curl -sS -X POST http://DEVICE_OR_NODE_HOST:PORT/api \
  -H "Content-Type: application/json" \
  -d '[
    {"jsonrpc":"2.0","method":"ping","id":101},
    {"jsonrpc":"2.0","method":"notify.record","params":{"event":"batch-notify"}},
    {"jsonrpc":"2.0","method":"echo","params":{"value":"test"},"id":102}
  ]'
```

Expected:
- Response is a JSON array.
- Entries with ids return `-32600`.
- Notification has no response entry.
- Node entries include diagnostic `error.data`; Arduino entries currently include code/message only.

## Value Round-Trip Cases

Use these payloads in both directions through `echo` using Safe Mode-aware clients. If sending raw HTTP with `X-RPC-Safe-Enabled: true`, literal marker-like strings must already be protected with `S:`; otherwise the receiver may correctly treat them as typed markers.

```json
{
  "plain": "hello",
  "sPrefix": "S:literal",
  "dPrefix": "D:literal",
  "isoDateString": "2026-06-10T12:34:56.000Z",
  "bigintMarker": "9007199254740993n",
  "nested": {
    "items": [
      "alpha",
      "S:beta",
      "D:gamma",
      "2026-06-10T12:34:56.000Z",
      "9007199254740993n",
      {
        "deep": ["S:delta", "9007199254740994n"]
      }
    ]
  }
}
```

Expected Arduino-side interpretation:
- `plain`, `sPrefix`, `dPrefix`, and `isoDateString` are strings.
- `bigintMarker` and nested `n` marker values are strings, including the trailing `n`.
- Arrays and objects preserve their shape.
- Arduino does not expose JavaScript `Date` or `BigInt` values.

Expected Node-side interpretation:
- Literal strings beginning with `S:` or `D:` must still round-trip as strings when they were originally strings.
- Literal strings ending with `n`, such as `"9007199254740993n"`, must still round-trip as strings when they were originally strings.
- `RpcSafeClient` hydrates actual `D:` markers to `Date` and actual `n` markers to JavaScript `BigInt` only when those markers came from typed Date/BigInt values or explicit marker output, not from literal strings protected by `S:`.

Expected Arduino automatic wire encoding for literal strings:

| Source string | Safe Mode wire value |
| --- | --- |
| `"hello"` | `"S:hello"` |
| `"S:literal"` | `"S:S:literal"` |
| `"D:literal"` | `"S:D:literal"` |
| `"2026-06-10T12:34:56.000Z"` | `"S:2026-06-10T12:34:56.000Z"` |
| `"9007199254740993n"` | `"S:9007199254740993n"` |

## Error Data Case

Method:

```js
rpc.addMethod('domainError', () => {
  const error = new Error('Domain failure');
  error.code = -32042;
  error.data = {
    text: 'S:error-data-literal',
    isoDateString: '2026-06-10T12:34:56.000Z',
    bigintMarker: '9007199254740993n',
    nested: {
      list: ['D:nested-literal', '9007199254740994n'],
    },
  };
  throw error;
});
```

Expected:
- JSON-RPC response has `error.code === -32042`.
- `error.data` is recursively decoded on the receiving side.
- Arduino preserves BigInt marker strings.
- This case validates application-level `error.data`. It is separate from strict-mode compatibility errors, where Arduino currently returns code/message without diagnostic `error.data`.

## Release Gate

Before claiming full Safe Mode interoperability in the public release:
- Capture or rerun the full matrix on ESP32 and save serial/LAN logs with the result summary.
- Capture or rerun at least HTTP client and HTTP server smoke tests on ESP8266.
- Capture firmware commit, `rpc-express-toolkit` version/commit, board model, Arduino core/platform version, and pass/fail summary.
- Confirm marker-like literal strings pass both Arduino-client-to-Node and Node-client-to-Arduino echo tests.
