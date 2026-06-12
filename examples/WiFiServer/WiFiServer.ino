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

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <RpcServer.h>
#include <RpcHttpServerTransport.h>

// WiFi credentials
const char* ssid = "YourSSID";
const char* password = "YourPassword";

// Create RPC server
RpcServer<9> rpc;
WiFiServer server(8080);

// Sensor simulation
float temperature = 25.0;
float humidity = 60.0;
StaticJsonDocument<512> rpcResultDoc;

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

JsonVariant makeBoolResult(bool value) {
    rpcResultDoc.clear();
    rpcResultDoc.set(value);
    return rpcResultDoc.as<JsonVariant>();
}

JsonVariant makeNumberResult(float value) {
    rpcResultDoc.clear();
    rpcResultDoc.set(value);
    return rpcResultDoc.as<JsonVariant>();
}

WiFiClient acceptRpcClient() {
    return server.accept();
}

const char* platformName() {
#if defined(ESP8266)
    return "ESP8266";
#elif defined(ESP32)
    return "ESP32";
#else
    return "Arduino";
#endif
}

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
    WiFiClient client = acceptRpcClient();

    if (client) {
        Serial.println("Client connected");

        // Create transport
        RpcHttpServerTransport transport(client);

        // Handle request
        String response = rpc.handleRequest(transport);
        transport.write(response);

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
        return makeStringResult("pong");
    });

    // LED control
    rpc.addMethod("setLED", [](JsonVariantConst params) -> JsonVariant {
        bool state = params["state"] | false;
        digitalWrite(2, state ? HIGH : LOW);

        Serial.print("LED ");
        Serial.println(state ? "ON" : "OFF");

        return makeBoolResult(state);
    });

    // Read temperature
    rpc.addMethod("readTemp", []() -> JsonVariant {
        Serial.print("Temperature: ");
        Serial.println(temperature);
        return makeNumberResult(temperature);
    });

    // Read humidity
    rpc.addMethod("readHumidity", []() -> JsonVariant {
        Serial.print("Humidity: ");
        Serial.println(humidity);
        return makeNumberResult(humidity);
    });

    // Get all sensors
    rpc.addMethod("getAllSensors", []() -> JsonVariant {
        rpcResultDoc.clear();
        rpcResultDoc["temperature"] = temperature;
        rpcResultDoc["humidity"] = humidity;
        rpcResultDoc["timestamp"] = millis();
        return rpcResultDoc.as<JsonVariant>();
    });

    // Get device info
    rpc.addMethod("getInfo", []() -> JsonVariant {
        rpcResultDoc.clear();
        rpcResultDoc["device"] = platformName();
        rpcResultDoc["firmware"] = "1.0.0";
        rpcResultDoc["uptime"] = millis();
        rpcResultDoc["freeHeap"] = ESP.getFreeHeap();
        rpcResultDoc["ip"] = WiFi.localIP().toString();
        rpcResultDoc["rssi"] = WiFi.RSSI();
        return rpcResultDoc.as<JsonVariant>();
    });

    // Echo test
    rpc.addMethod("echo", [](JsonVariantConst params) -> JsonVariant {
        String msg = params["message"] | "";
        Serial.print("Echo: ");
        Serial.println(msg);
        return makeStringResult(msg);
    });

    // Math operation
    rpc.addMethod("add", [](JsonVariantConst params) -> JsonVariant {
        float a = params["a"] | 0.0;
        float b = params["b"] | 0.0;
        return makeNumberResult(a + b);
    });

    // Array params example
    rpc.addMethod("sumArray", [](JsonVariantConst params) -> JsonVariant {
        JsonArrayConst values = params.as<JsonArrayConst>();
        float total = 0.0;

        for (JsonVariantConst value : values) {
            total += value.as<float>();
        }

        return makeNumberResult(total);
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
    Serial.println("  - sumArray");
}
