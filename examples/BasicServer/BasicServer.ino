/**
 * Basic RPC Server Example - Serial
 * 
 * This example creates a simple RPC server that communicates over Serial.
 * You can control an LED and read analog values.
 * 
 * Hardware:
 * - Arduino Uno/Mega/Nano or compatible
 * - LED on pin 13 (or use built-in LED)
 * 
 * Usage:
 * 1. Upload this sketch
 * 2. Open Serial Monitor at 115200 baud
 * 3. Send JSON-RPC commands:
 *    {"jsonrpc":"2.0","method":"setLED","params":{"state":true},"id":1}
 *    {"jsonrpc":"2.0","method":"readAnalog","params":{"pin":0},"id":2}
 */

#include <RpcServer.h>
#include <RpcSerialTransport.h>

// Create RPC server with max 4 methods
RpcServer<4> rpc;

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    Serial.println("RPC Server Starting...");
    
    // Setup LED pin
    pinMode(LED_BUILTIN, OUTPUT);
    
    // Register LED control method
    rpc.addMethod("setLED", [](JsonObject params) -> JsonVariant {
        bool state = params["state"] | false;
        digitalWrite(LED_BUILTIN, state ? HIGH : LOW);
        
        Serial.print("LED ");
        Serial.println(state ? "ON" : "OFF");
        
        return state;
    });
    
    // Register analog read method
    rpc.addMethod("readAnalog", [](JsonObject params) -> JsonVariant {
        int pin = params["pin"] | 0;
        int value = analogRead(pin);
        
        Serial.print("Analog pin ");
        Serial.print(pin);
        Serial.print(": ");
        Serial.println(value);
        
        return value;
    });
    
    // Register ping method
    rpc.addMethod("ping", []() -> JsonVariant {
        return "pong";
    });
    
    // Register getStatus method
    rpc.addMethod("getStatus", []() -> JsonVariant {
        StaticJsonDocument<128> doc;
        doc["uptime"] = millis();
        doc["freeMem"] = freeMemory();
        return doc.as<JsonVariant>();
    });
    
    Serial.println("RPC Server Ready!");
    Serial.println("Registered methods:");
    Serial.println("  - setLED");
    Serial.println("  - readAnalog");
    Serial.println("  - ping");
    Serial.println("  - getStatus");
    Serial.println();
}

void loop() {
    // Check for incoming RPC requests
    if (Serial.available()) {
        RpcSerialTransport transport(Serial);
        String response = rpc.handleRequest(transport);
        
        // Send response if not a notification
        if (!response.isEmpty()) {
            Serial.println(response);
        }
    }
}

// Helper function to get free memory (for AVR boards)
int freeMemory() {
    #if defined(__AVR__)
        extern int __heap_start, *__brkval;
        int v;
        return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
    #else
        return -1; // Not implemented for this platform
    #endif
}
