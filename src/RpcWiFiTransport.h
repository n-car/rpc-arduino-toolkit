/**
 * RPC Arduino Toolkit - WiFi Transport
 * 
 * Transport over WiFi (ESP32/ESP8266)
 */

#ifndef RPC_WIFI_TRANSPORT_H
#define RPC_WIFI_TRANSPORT_H

#if RPC_HAS_WIFI

#include "RpcTransport.h"

#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

class RpcWiFiTransport : public RpcTransport {
private:
    WiFiClient& client;
    char buffer[RPC_MAX_REQUEST_SIZE];
    
public:
    explicit RpcWiFiTransport(WiFiClient& c) : client(c) {
        setTimeout(RPC_WIFI_TIMEOUT);
    }
    
    String read() override {
        if (!client.available()) {
            return "";
        }
        
        // Read HTTP request line (for servers)
        String line = client.readStringUntil('\n');
        line.trim();
        
        // If it's an HTTP request, skip headers
        if (line.startsWith("POST") || line.startsWith("GET")) {
            // Skip headers until empty line
            while (client.available()) {
                line = client.readStringUntil('\n');
                line.trim();
                if (line.isEmpty()) break;
            }
        }
        
        // Read JSON body
        size_t len = 0;
        unsigned long start = millis();
        while (client.available() && len < sizeof(buffer) - 1 && (millis() - start < timeout)) {
            buffer[len++] = client.read();
        }
        buffer[len] = '\0';
        
        String result(buffer);
        result.trim();
        
        RPC_LOG_F("WiFi RX: %s", result.c_str());
        return result;
    }
    
    bool write(const String& data) override {
        RPC_LOG_F("WiFi TX: %s", data.c_str());
        
        // Send as HTTP response
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.print("Content-Length: ");
        client.println(data.length());
        client.println();
        client.println(data);
        client.flush();
        
        return true;
    }
    
    bool available() override {
        return client.connected() && client.available();
    }
};

#endif // RPC_HAS_WIFI

#endif // RPC_WIFI_TRANSPORT_H
