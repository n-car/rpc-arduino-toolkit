/**
 * RPC Arduino Toolkit - Introspection Example
 * 
 * This example demonstrates the built-in introspection methods
 * that allow clients to discover available RPC methods.
 * 
 * Available introspection methods:
 * - __rpc.listMethods: List all registered methods
 * - __rpc.version: Get server version and method count
 */

#include <RpcArduino.h>

// Create RPC server
RpcServer<16> rpc;

// Transport (Serial in this example)
RpcTransportSerial transport;

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    Serial.println("\n=== RPC Arduino Introspection Example ===");
    Serial.println("Registering methods...\n");
    
    // Register test methods
    rpc.addMethod("ping", []() -> JsonVariant {
        return "pong";
    });
    
    rpc.addMethod("add", [](JsonObject params) -> JsonVariant {
        int a = params["a"] | 0;
        int b = params["b"] | 0;
        return a + b;
    });
    
    rpc.addMethod("getUptime", []() -> JsonVariant {
        return millis();
    });
    
    rpc.addMethod("echo", [](JsonObject params) -> JsonVariant {
        String msg = params["message"] | "";
        return msg;
    });
    
    Serial.println("Methods registered:");
    Serial.println("  - ping");
    Serial.println("  - add");
    Serial.println("  - getUptime");
    Serial.println("  - echo");
    Serial.println();
    Serial.println("Built-in introspection methods:");
    Serial.println("  - __rpc.listMethods");
    Serial.println("  - __rpc.version");
    Serial.println();
    Serial.println("Ready! Send JSON-RPC requests via Serial...");
    Serial.println();
    Serial.println("Example requests:");
    Serial.println("{\"jsonrpc\":\"2.0\",\"method\":\"__rpc.listMethods\",\"id\":1}");
    Serial.println("{\"jsonrpc\":\"2.0\",\"method\":\"__rpc.version\",\"id\":2}");
    Serial.println("{\"jsonrpc\":\"2.0\",\"method\":\"ping\",\"id\":3}");
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
