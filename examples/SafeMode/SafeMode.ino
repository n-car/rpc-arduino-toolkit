/**
 * RPC Arduino Toolkit - Safe Mode Example
 * 
 * This example demonstrates safe serialization with S: prefixes for strings,
 * D: prefixes for dates/timestamps, and 'n' suffixes for large integers.
 * 
 * To enable safe mode, define RPC_ENABLE_SAFE_MODE=1 in RpcConfig.h
 * or add build flag: -DRPC_ENABLE_SAFE_MODE=1
 */

#include <RpcArduino.h>

// Create RPC server
RpcServer<8> rpc;

// Transport (Serial in this example)
RpcTransportSerial transport;

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    Serial.println("\n=== RPC Arduino Safe Mode Example ===");
    
#if RPC_ENABLE_SAFE_MODE
    Serial.println("Safe Mode: ENABLED");
    Serial.println("Strings will have S: prefix, timestamps D: prefix, large numbers 'n' suffix");
#else
    Serial.println("Safe Mode: DISABLED");
    Serial.println("To enable, set RPC_ENABLE_SAFE_MODE=1 in RpcConfig.h");
#endif
    
    Serial.println();
    
    // Register methods that demonstrate safe serialization
    rpc.addMethod("getString", []() -> JsonVariant {
        return "Hello from Arduino";
    }, "Returns a string", true);
    
    rpc.addMethod("getTimestamp", []() -> JsonVariant {
        // Return current timestamp (millis since boot)
#if RPC_ENABLE_SAFE_MODE
        return RpcSafe::serializeDate(millis() / 1000);
#else
        return millis() / 1000;
#endif
    }, "Returns current timestamp", true);
    
    rpc.addMethod("getBigNumber", []() -> JsonVariant {
        // Return a large number
        long long bigNum = 9007199254740992LL;  // Larger than safe JS integer
#if RPC_ENABLE_SAFE_MODE
        return RpcSafe::serializeBigInt(bigNum);
#else
        return (long)bigNum;  // Might lose precision on Arduino
#endif
    }, "Returns a large number", true);
    
    rpc.addMethod("echo", [](JsonObject params) -> JsonVariant {
        String msg = params["message"] | "";
        
#if RPC_ENABLE_SAFE_MODE
        // Deserialize if it's a safe string
        if (RpcSafe::isSafeString(msg)) {
            msg = RpcSafe::deserializeString(msg);
        }
        
        // Return with safe prefix
        return RpcSafe::serializeString(msg);
#else
        return msg;
#endif
    }, "Echo message with safe serialization", true);
    
    rpc.addMethod("processData", [](JsonObject params) -> JsonVariant {
        StaticJsonDocument<256> result;
        
        // Process different data types
        String text = params["text"] | "";
        int timestamp = params["timestamp"] | 0;
        
#if RPC_ENABLE_SAFE_MODE
        // Deserialize safe strings
        if (RpcSafe::isSafeString(text)) {
            text = RpcSafe::deserializeString(text);
        }
        
        // Create safe response
        result["processedText"] = RpcSafe::serializeString(text.toUpperCase());
        result["receivedAt"] = RpcSafe::serializeDate(millis() / 1000);
        result["inputTimestamp"] = timestamp;
#else
        result["processedText"] = text.toUpperCase();
        result["receivedAt"] = millis() / 1000;
        result["inputTimestamp"] = timestamp;
#endif
        
        return result.as<JsonVariant>();
    }, "Process data with safe serialization", true);
    
    Serial.println("Methods registered:");
    Serial.println("  - getString");
    Serial.println("  - getTimestamp");
    Serial.println("  - getBigNumber");
    Serial.println("  - echo");
    Serial.println("  - processData");
    Serial.println();
    Serial.println("Ready! Send JSON-RPC requests via Serial...");
    Serial.println();
    Serial.println("Example requests:");
    Serial.println("{\"jsonrpc\":\"2.0\",\"method\":\"getString\",\"id\":1}");
    Serial.println("{\"jsonrpc\":\"2.0\",\"method\":\"getTimestamp\",\"id\":2}");
    Serial.println("{\"jsonrpc\":\"2.0\",\"method\":\"getBigNumber\",\"id\":3}");
    Serial.println("{\"jsonrpc\":\"2.0\",\"method\":\"echo\",\"params\":{\"message\":\"test\"},\"id\":4}");
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
