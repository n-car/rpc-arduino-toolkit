# API Reference

This reference covers the public API surface and the most common usage patterns.
For transport-specific behavior, see [TRANSPORTS.md](TRANSPORTS.md). For Safe
Mode behavior, see [SAFE_MODE_INTEROPERABILITY.md](SAFE_MODE_INTEROPERABILITY.md).

## Includes

Prefer selective includes on constrained boards:

```cpp
// HTTP server
#include <RpcServer.h>
#include <RpcHttpServerTransport.h>

// HTTP client
#include <RpcClient.h>
#include <RpcHttpClientTransport.h>

// Serial client/server
#include <RpcClient.h>
#include <RpcServer.h>
#include <RpcSerialTransport.h>
```

`RpcArduinoToolkit.h` remains available as a convenience umbrella header, but
examples use selective includes by default.

## RpcServer

```cpp
template<uint8_t MAX_METHODS>
class RpcServer {
public:
    bool addMethod(const char* name, RpcMethodHandler handler);
    bool addMethod(const char* name, RpcMethodHandler handler, const char* description, bool exposeSchema = false);
    bool addMethod(const char* name, RpcSimpleHandler handler);
    bool addMethod(const char* name, RpcSimpleHandler handler, const char* description, bool exposeSchema = false);
    bool addResponseMethod(const char* name, RpcResponseHandler handler);
    bool addResponseMethod(const char* name, RpcResponseHandler handler, const char* description, bool exposeSchema = false);

    String handleRequest(RpcTransport& transport);
    String handleRequest(const String& json);

    bool removeMethod(const char* name);
};
```

Handler signatures:

```cpp
JsonVariant handler(JsonVariantConst params);
RpcResponse responseHandler(RpcRequest& req, bool encodeSafe);
```

Use `addMethod(...)` for normal JSON-RPC `result` responses. Use
`addResponseMethod(...)` when a method must return a complete `RpcResponse`, such
as a custom JSON-RPC error with `error.data`.

## RpcClient

```cpp
class RpcClient {
public:
    RpcClient(RpcTransport& transport);

    RpcResponse call(const char* method, const String& params = "");
    RpcResponse call(const char* method, JsonObject params);

    void notify(const char* method, const String& params = "");
    void notify(const char* method, JsonObject params);

    RpcBatchResponse callBatch(const String& batch);

    void setTimeout(unsigned long ms);
    unsigned long getTimeout() const;
};
```

## RpcTransport

```cpp
class RpcTransport {
public:
    virtual ~RpcTransport();

    virtual String read() = 0;
    virtual bool write(const String& data) = 0;
    virtual bool available() = 0;

    virtual void setTimeout(unsigned long ms);

    virtual bool isHttp() const;
    virtual bool hasClientSafeHeader() const;
    virtual bool clientSafeEnabled() const;
    virtual bool hasRemoteSafeHeader() const;
    virtual bool remoteSafeEnabled() const;
};
```

Non-HTTP transports normally keep the Safe Mode header hooks at their default
values.

## RpcHttpServerTransport

```cpp
class RpcHttpServerTransport : public RpcTransport {
public:
    explicit RpcHttpServerTransport(Client& client);

    String read() override;
    bool write(const String& data) override;
    bool available() override;
    bool isHttp() const override;
    bool hasClientSafeHeader() const override;
    bool clientSafeEnabled() const override;
};
```

`RpcWiFiTransport` remains available as a deprecated compatibility wrapper for
existing sketches. New code should use `RpcHttpServerTransport`.

## RpcHttpClientTransport

```cpp
class RpcHttpClientTransport : public RpcTransport {
public:
    RpcHttpClientTransport(Client& client, const char* host, uint16_t port, const char* path = "/");

    bool write(const String& data) override;
    String read() override;
    bool available() override;
    void setTimeout(unsigned long ms) override;
    void setSocketSettleDelay(unsigned long ms);
    const String& lastError() const;
    int lastStatus() const;
    bool isHttp() const override;
    bool hasRemoteSafeHeader() const override;
    bool remoteSafeEnabled() const override;
};
```

## RpcResponse

```cpp
class RpcResponse {
public:
    bool isValid() const;
    bool isSuccess() const;
    bool hasError() const;

    template<typename T>
    T result() const;

    JsonVariantConst result() const;

    int errorCode() const;
    String errorMessage() const;
    JsonVariantConst errorData() const;
};
```

## RpcBatchResponse

```cpp
class RpcBatchResponse {
public:
    bool isValid() const;
    bool isBatch() const;
    bool isSuccess() const;
    size_t count() const;

    JsonVariantConst response(size_t index) const;
    JsonVariantConst result(size_t index) const;
    bool hasError(size_t index) const;
    int errorCode(size_t index) const;
    String errorMessage(size_t index) const;
    JsonVariantConst id(size_t index) const;
};
```

## RpcError

```cpp
class RpcError {
public:
    static RpcResponse custom(int code, const char* message, JsonVariantConst data, JsonVariant id, bool encodeSafe = false);
    static RpcResponse custom(int code, const String& message, JsonVariantConst data, JsonVariant id, bool encodeSafe = false);
};
```

When Safe Mode is enabled, `RpcError::custom(...)` encodes `error.data`
recursively when `encodeSafe` is `true`.

## Server Params

Server method handlers receive `JsonVariantConst` params, so they can read
standard JSON-RPC object params and array params without copying.

```cpp
rpc.addMethod("sum", [](JsonVariantConst params) -> JsonVariant {
    JsonArrayConst values = params.as<JsonArrayConst>();
    int total = 0;

    for (JsonVariantConst value : values) {
        total += value.as<int>();
    }

    static StaticJsonDocument<64> resultDoc;
    resultDoc.clear();
    resultDoc.set(total);
    return resultDoc.as<JsonVariant>();
});
```

## Handler Result Storage

Handlers return `JsonVariant` views. Store owned scalar/object results in a
document that remains alive until the response is serialized.

```cpp
StaticJsonDocument<64> resultDoc;

JsonVariant makeNumberResult(int value) {
    resultDoc.clear();
    resultDoc.set(value);
    return resultDoc.as<JsonVariant>();
}

rpc.addMethod("answer", []() -> JsonVariant {
    return makeNumberResult(42);
});
```

## Batch Requests

```cpp
String batch = "[{\"jsonrpc\":\"2.0\",\"method\":\"readTemp\",\"id\":1},"
               "{\"jsonrpc\":\"2.0\",\"method\":\"readHumidity\",\"id\":2}]";

RpcBatchResponse batchResp = rpc.callBatch(batch);

if (batchResp.isValid() && batchResp.count() == 2) {
    float temp = batchResp.result(0).as<float>();
    float humidity = batchResp.result(1).as<float>();
}
```

`RpcServer` also accepts JSON-RPC batch request arrays when `RPC_ENABLE_BATCH` is
enabled. Notifications inside a batch are executed without response entries. If
all batch items are notifications, HTTP transports return `204 No Content`.

## Notifications

```cpp
rpc.notify("logEvent", "{\"level\":\"info\",\"msg\":\"Sensor read\"}");
```

Notifications are fire-and-forget calls and do not produce JSON-RPC response
bodies.

## Error Handling

```cpp
RpcResponse resp = rpc.call("unknownMethod");

if (resp.hasError()) {
    Serial.print("Error code: ");
    Serial.println(resp.errorCode());
    Serial.print("Error message: ");
    Serial.println(resp.errorMessage());
}
```

For application errors with `error.data`, use `addResponseMethod(...)`:

```cpp
StaticJsonDocument<256> errorDataDoc;

rpc.addResponseMethod("domainError",
    [](RpcRequest& req, bool encodeSafe) -> RpcResponse {
        errorDataDoc.clear();
        JsonObject data = errorDataDoc.to<JsonObject>();
        data["reason"] = "intentional-test-error";
        data["markerString"] = "S:error-data-literal";

        return RpcError::custom(
            -32042,
            "Domain failure",
            data,
            req.id,
            encodeSafe);
    },
    "Throw a domain JSON-RPC error",
    true);
```

## Built-In Introspection

The RPC server includes built-in introspection methods for API discovery:

```cpp
RpcResponse resp = rpc.call("__rpc.listMethods");
// ["ping", "setLED", "readTemp", ...]

resp = rpc.call("__rpc.version");
// {"toolkit":"rpc-arduino-toolkit","version":"1.0.0","methodCount":3}

resp = rpc.call("__rpc.describe", "{\"method\":\"add\"}");
// {"name":"add","description":"Add two numbers","exposeSchema":true}

resp = rpc.call("__rpc.capabilities");
// {"batch":true,"introspection":true,"safeMode":false,"strictMode":false,...}
```

Description metadata support is optional. Disable
`RPC_ENABLE_SCHEMA_SUPPORT` when metadata is not needed.

```cpp
rpc.addMethod("add", [](JsonVariantConst params) -> JsonVariant {
    int a = params["a"] | 0;
    int b = params["b"] | 0;
    return makeNumberResult(a + b);
}, "Add two numbers", true);
```

See `examples/Introspection/` for a complete Arduino example.
