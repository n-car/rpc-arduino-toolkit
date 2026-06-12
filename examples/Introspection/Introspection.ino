/**
 * RPCToolkit - Introspection Example
 *
 * This example demonstrates the built-in introspection methods
 * that allow clients to discover available RPC methods.
 *
 * Available introspection methods:
 * - __rpc.listMethods: List all registered methods
 * - __rpc.version: Get server version and method count
 * - __rpc.describe: Get description and schema info for a specific method
 * - __rpc.capabilities: Get server capabilities and features
 */

#include <RpcServer.h>
#include <RpcSerialTransport.h>

// Create RPC server
RpcServer<16> rpc;
RpcSerialTransport transport(Serial);
StaticJsonDocument<256> rpcResultDoc;

JsonVariant makeNumberResult(long value) {
    rpcResultDoc.clear();
    rpcResultDoc.set(value);
    return rpcResultDoc.as<JsonVariant>();
}

JsonVariant makeStringResult(const char* value) {
    rpcResultDoc.clear();
    rpcResultDoc.set(value);
    return rpcResultDoc.as<JsonVariant>();
}

JsonVariant makeStringResult(const String& value) {
    rpcResultDoc.clear();
    rpcResultDoc.set(value);
    return rpcResultDoc.as<JsonVariant>();
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("\n=== RPC Arduino Introspection Example ===");
    Serial.println("Registering methods...\n");

    // Register test methods (simple without schema)
    rpc.addMethod("ping", []() -> JsonVariant {
        return makeStringResult("pong");
    });

    // Register method with description and schema exposure
    rpc.addMethod("add", [](JsonVariantConst params) -> JsonVariant {
        int a = params["a"] | 0;
        int b = params["b"] | 0;
        return makeNumberResult(a + b);
    }, "Add two numbers", true);  // Description + expose schema

    rpc.addMethod("getUptime", []() -> JsonVariant {
        return makeNumberResult(millis());
    }, "Get system uptime in milliseconds", true);

    rpc.addMethod("echo", [](JsonVariantConst params) -> JsonVariant {
        String msg = params["message"] | "";
        return makeStringResult(msg);
    }, "Echo back the message parameter", true);

    rpc.addMethod("multiply", [](JsonVariantConst params) -> JsonVariant {
        int a = params["a"] | 1;
        int b = params["b"] | 1;
        return makeNumberResult(a * b);
    }, "Multiply two numbers", true);

    Serial.println("Methods registered:");
    Serial.println("  - ping (no schema)");
    Serial.println("  - add (with schema)");
    Serial.println("  - getUptime (with schema)");
    Serial.println("  - echo (with schema)");
    Serial.println("  - multiply (with schema)");
    Serial.println();
    Serial.println("Built-in introspection methods:");
    Serial.println("  - __rpc.listMethods");
    Serial.println("  - __rpc.version");
    Serial.println("  - __rpc.describe");
    Serial.println("  - __rpc.capabilities");
    Serial.println();
    Serial.println("Ready! Send JSON-RPC requests via Serial...");
    Serial.println();
    Serial.println("Example requests:");
    Serial.println("{\"jsonrpc\":\"2.0\",\"method\":\"__rpc.listMethods\",\"id\":1}");
    Serial.println("{\"jsonrpc\":\"2.0\",\"method\":\"__rpc.version\",\"id\":2}");
    Serial.println("{\"jsonrpc\":\"2.0\",\"method\":\"__rpc.capabilities\",\"id\":3}");
    Serial.println("{\"jsonrpc\":\"2.0\",\"method\":\"__rpc.describe\",\"params\":{\"method\":\"add\"},\"id\":4}");
    Serial.println("{\"jsonrpc\":\"2.0\",\"method\":\"add\",\"params\":{\"a\":5,\"b\":3},\"id\":5}");
    Serial.println();
}

void loop() {
    // Handle RPC requests
    String response = rpc.handleRequest(transport);

    if (response.length() > 0) {
        Serial.println(response);
    }

    delay(10);
}
