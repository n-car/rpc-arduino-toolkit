/**
 * Basic RPC Client Example - Serial
 * 
 * This example creates an RPC client that calls methods on a remote server
 * via Serial communication.
 * 
 * Hardware:
 * - Arduino Uno/Mega/Nano or compatible
 * 
 * Usage:
 * 1. Upload BasicServer sketch to one Arduino
 * 2. Upload this sketch to another Arduino
 * 3. Connect TX of client to RX of server
 * 4. Connect RX of client to TX of server
 * 5. Connect GND together
 * 6. Open Serial Monitor on client at 115200 baud
 */

#include <RpcClient.h>
#include <RpcSerialTransport.h>

// Use Serial1 for RPC communication (Serial for debugging)
// On Arduino Uno, you'll need SoftwareSerial
RpcSerialTransport transport(Serial);
RpcClient rpc(transport);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("RPC Client Starting...");
    Serial.println();
}

void loop() {
    // Test 1: Ping
    Serial.println("=== Test 1: Ping ===");
    RpcResponse resp = rpc.call("ping");
    if (resp.isSuccess()) {
        Serial.print("Result: ");
        Serial.println(resp.result<String>());
    } else {
        Serial.print("Error: ");
        Serial.println(resp.errorMessage());
    }
    Serial.println();
    
    delay(1000);
    
    // Test 2: Turn LED ON
    Serial.println("=== Test 2: LED ON ===");
    resp = rpc.call("setLED", "{\"state\":true}");
    if (resp.isSuccess()) {
        Serial.print("LED state: ");
        Serial.println(resp.result<bool>() ? "ON" : "OFF");
    } else {
        Serial.print("Error: ");
        Serial.println(resp.errorMessage());
    }
    Serial.println();
    
    delay(2000);
    
    // Test 3: Turn LED OFF
    Serial.println("=== Test 3: LED OFF ===");
    resp = rpc.call("setLED", "{\"state\":false}");
    if (resp.isSuccess()) {
        Serial.print("LED state: ");
        Serial.println(resp.result<bool>() ? "ON" : "OFF");
    } else {
        Serial.print("Error: ");
        Serial.println(resp.errorMessage());
    }
    Serial.println();
    
    delay(2000);
    
    // Test 4: Read analog pin
    Serial.println("=== Test 4: Read Analog ===");
    resp = rpc.call("readAnalog", "{\"pin\":0}");
    if (resp.isSuccess()) {
        Serial.print("Analog value: ");
        Serial.println(resp.result<int>());
    } else {
        Serial.print("Error: ");
        Serial.println(resp.errorMessage());
    }
    Serial.println();
    
    delay(2000);
    
    // Test 5: Get status
    Serial.println("=== Test 5: Get Status ===");
    resp = rpc.call("getStatus");
    if (resp.isSuccess()) {
        Serial.print("Uptime: ");
        Serial.println(resp.result()["uptime"].as<unsigned long>());
        Serial.print("Free memory: ");
        Serial.println(resp.result()["freeMem"].as<int>());
    } else {
        Serial.print("Error: ");
        Serial.println(resp.errorMessage());
    }
    Serial.println();
    
    delay(5000);
}
