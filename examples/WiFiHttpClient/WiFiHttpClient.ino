/**
 * RPC Arduino Toolkit - WiFi HTTP Client Example
 *
 * Calls a JSON-RPC HTTP endpoint over WiFi from ESP32 or ESP8266.
 *
 * Standard JSON-RPC 2.0 is used by default. If the remote endpoint supports
 * RPC Toolkit Safe Mode, compile with:
 *   -DRPC_ENABLE_SAFE_MODE=1
 *
 * Update the WiFi credentials and endpoint before uploading.
 * The remote endpoint should expose `ping` and `add` methods, or you can
 * replace the calls below with methods from your own server.
 */

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include <RpcClient.h>
#include <RpcHttpClientTransport.h>

const char* ssid = "YourSSID";
const char* password = "YourPassword";

const char* rpcHost = "192.168.1.100";
const uint16_t rpcPort = 3000;
const char* rpcPath = "/api";

WiFiClient httpClient;
RpcHttpClientTransport transport(httpClient, rpcHost, rpcPort, rpcPath);
RpcClient rpc(transport);

const unsigned long callIntervalMs = 5000;
unsigned long lastCall = 0;

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    Serial.print("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
#if defined(ESP8266)
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
#else
    WiFi.setSleep(false);
#endif
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void printRpcError(const char* method, const RpcResponse& response) {
    Serial.print(method);
    Serial.print(" failed");

    if (response.hasError()) {
        Serial.print(" rpcCode=");
        Serial.print(response.errorCode());
        Serial.print(" rpcMessage=");
        Serial.print(response.errorMessage());
    }

    if (transport.lastStatus() != 0) {
        Serial.print(" httpStatus=");
        Serial.print(transport.lastStatus());
    }

    if (transport.lastError().length() > 0) {
        Serial.print(" transport=");
        Serial.print(transport.lastError());
    }

    Serial.println();
}

void callPing() {
    RpcResponse response = rpc.call("ping");

    if (!response.isSuccess()) {
        printRpcError("ping", response);
        return;
    }

    Serial.print("ping result: ");
    Serial.println(response.result().as<const char*>());
}

void callAdd() {
    RpcResponse response = rpc.call("add", "{\"a\":2,\"b\":3}");

    if (!response.isSuccess()) {
        printRpcError("add", response);
        return;
    }

    Serial.print("add result: ");
    Serial.println(response.result().as<float>());
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=== RPC WiFi HTTP Client ===");
#if RPC_ENABLE_SAFE_MODE
    Serial.println("Safe Mode: enabled");
#else
    Serial.println("Safe Mode: disabled");
#endif

    connectWiFi();
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    if (millis() - lastCall >= callIntervalMs) {
        lastCall = millis();
        callPing();
        callAdd();
        Serial.println();
    }
}
