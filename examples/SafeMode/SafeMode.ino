/**
 * RPC Arduino Toolkit - Safe Mode Example
 *
 * This example demonstrates safe serialization with S: prefixes for strings,
 * D: prefixes for dates/timestamps, and 'n' suffixes for large integers.
 *
 * To enable safe mode, define RPC_ENABLE_SAFE_MODE=1 in RpcConfig.h
 * or add build flag: -DRPC_ENABLE_SAFE_MODE=1
 */

#include <RpcServer.h>
#include <RpcSerialTransport.h>

// Create RPC server
RpcServer<8> rpc;
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
        return makeStringResult("Hello from Arduino");
    }, "Returns a string", true);

    rpc.addMethod("getTimestamp", []() -> JsonVariant {
        // Return current timestamp (millis since boot)
#if RPC_ENABLE_SAFE_MODE
        return makeStringResult(RpcSafe::serializeDate(millis() / 1000));
#else
        return makeNumberResult(millis() / 1000);
#endif
    }, "Returns current timestamp", true);

    rpc.addMethod("getBigNumber", []() -> JsonVariant {
        // Return a large number
        long long bigNum = 9007199254740992LL;  // Larger than safe JS integer
#if RPC_ENABLE_SAFE_MODE
        return makeStringResult(RpcSafe::serializeBigInt(bigNum));
#else
        return makeNumberResult((long)bigNum);  // Might lose precision on Arduino
#endif
    }, "Returns a large number", true);

    rpc.addMethod("echo", [](JsonVariantConst params) -> JsonVariant {
        String msg = params["message"] | "";

#if RPC_ENABLE_SAFE_MODE
        // Deserialize if it's a safe string
        if (RpcSafe::isSafeString(msg)) {
            msg = RpcSafe::deserializeString(msg);
        }

        // Return with safe prefix
        return makeStringResult(RpcSafe::serializeString(msg));
#else
        return makeStringResult(msg);
#endif
    }, "Echo message with safe serialization", true);

    rpc.addMethod("processData", [](JsonVariantConst params) -> JsonVariant {
        rpcResultDoc.clear();

        // Process different data types
        String text = params["text"] | "";
        int timestamp = params["timestamp"] | 0;

#if RPC_ENABLE_SAFE_MODE
        // Deserialize safe strings
        if (RpcSafe::isSafeString(text)) {
            text = RpcSafe::deserializeString(text);
        }

        // Create safe response
        text.toUpperCase();
        rpcResultDoc["processedText"] = RpcSafe::serializeString(text);
        rpcResultDoc["receivedAt"] = RpcSafe::serializeDate(millis() / 1000);
        rpcResultDoc["inputTimestamp"] = timestamp;
#else
        text.toUpperCase();
        rpcResultDoc["processedText"] = text;
        rpcResultDoc["receivedAt"] = millis() / 1000;
        rpcResultDoc["inputTimestamp"] = timestamp;
#endif

        return rpcResultDoc.as<JsonVariant>();
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
