/**
 * WiFi RPC Server Example - ESP32/ESP8266
 * 
 * This example creates an HTTP-based RPC server accessible over WiFi.
 * 
 * Hardware:
 * - ESP32 or ESP8266
 * - LED on GPIO 2 (built-in LED)
 * 
 * Usage:
 * 1. Update WiFi credentials below
 * 2. Upload sketch
 * 3. Open Serial Monitor to see IP address
 * 4. Send HTTP POST requests to http://YOUR_IP:8080
 */

#include <WiFi.h>
#include <RpcServer.h>
#include <RpcWiFiTransport.h>

// WiFi credentials
const char* ssid = "YourSSID";
const char* password = "YourPassword";

// Create RPC server
RpcServer<8> rpc;
WiFiServer server(8080);

// Sensor simulation
float temperature = 25.0;
float humidity = 60.0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=== RPC WiFi Server ===");
    
    // Setup GPIO
    pinMode(2, OUTPUT);
    
    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Port: 8080\n\n");
    
    // Register RPC methods
    registerMethods();
    
    // Start server
    server.begin();
    Serial.println("RPC Server Started!");
    Serial.println("\nTest with curl:");
    Serial.println("curl -X POST http://" + WiFi.localIP().toString() + ":8080 \\");
    Serial.println("  -H \"Content-Type: application/json\" \\");
    Serial.println("  -d '{\"jsonrpc\":\"2.0\",\"method\":\"ping\",\"id\":1}'");
    Serial.println();
}

void loop() {
    // Check for client connections
    WiFiClient client = server.available();
    
    if (client) {
        Serial.println("Client connected");
        
        // Create transport
        RpcWiFiTransport transport(client);
        
        // Handle request
        String response = rpc.handleRequest(transport);
        
        client.stop();
        Serial.println("Client disconnected\n");
        
        // Simulate sensor changes
        temperature += random(-10, 10) / 10.0;
        humidity += random(-5, 5) / 10.0;
    }
}

void registerMethods() {
    // Ping
    rpc.addMethod("ping", []() -> JsonVariant {
        return "pong";
    });
    
    // LED control
    rpc.addMethod("setLED", [](JsonObject params) -> JsonVariant {
        bool state = params["state"] | false;
        digitalWrite(2, state ? HIGH : LOW);
        
        Serial.print("LED ");
        Serial.println(state ? "ON" : "OFF");
        
        return state;
    });
    
    // Read temperature
    rpc.addMethod("readTemp", []() -> JsonVariant {
        Serial.print("Temperature: ");
        Serial.println(temperature);
        return temperature;
    });
    
    // Read humidity
    rpc.addMethod("readHumidity", []() -> JsonVariant {
        Serial.print("Humidity: ");
        Serial.println(humidity);
        return humidity;
    });
    
    // Get all sensors
    rpc.addMethod("getAllSensors", []() -> JsonVariant {
        StaticJsonDocument<128> doc;
        doc["temperature"] = temperature;
        doc["humidity"] = humidity;
        doc["timestamp"] = millis();
        return doc.as<JsonVariant>();
    });
    
    // Get device info
    rpc.addMethod("getInfo", []() -> JsonVariant {
        StaticJsonDocument<256> doc;
        doc["device"] = "ESP32";
        doc["firmware"] = "1.0.0";
        doc["uptime"] = millis();
        doc["freeHeap"] = ESP.getFreeHeap();
        doc["ip"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
        return doc.as<JsonVariant>();
    });
    
    // Echo test
    rpc.addMethod("echo", [](JsonObject params) -> JsonVariant {
        String msg = params["message"] | "";
        Serial.print("Echo: ");
        Serial.println(msg);
        return msg;
    });
    
    // Math operation
    rpc.addMethod("add", [](JsonObject params) -> JsonVariant {
        float a = params["a"] | 0.0;
        float b = params["b"] | 0.0;
        return a + b;
    });
    
    Serial.println("Registered methods:");
    Serial.println("  - ping");
    Serial.println("  - setLED");
    Serial.println("  - readTemp");
    Serial.println("  - readHumidity");
    Serial.println("  - getAllSensors");
    Serial.println("  - getInfo");
    Serial.println("  - echo");
    Serial.println("  - add");
}
