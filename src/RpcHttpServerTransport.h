/**
 * RPC Arduino Toolkit - HTTP Server Transport
 *
 * Server-side JSON-RPC transport over an Arduino Client-compatible socket.
 * Works with WiFiClient, EthernetClient, and other Client implementations.
 */

#ifndef RPC_HTTP_SERVER_TRANSPORT_H
#define RPC_HTTP_SERVER_TRANSPORT_H

#include <Arduino.h>
#include <Client.h>
#include "RpcTransport.h"

class RpcHttpServerTransport : public RpcTransport {
private:
    Client& client;
    char buffer[RPC_MAX_REQUEST_SIZE];

    bool waitForReadable(unsigned long start) {
        while (!client.available() && client.connected() && (millis() - start < timeout)) {
            delay(1);
        }
        return client.available() > 0;
    }

    bool readHeaderLine(String& line, unsigned long start) {
        if (!waitForReadable(start)) {
            return false;
        }

        line = client.readStringUntil('\n');
        line.trim();
        return true;
    }

    size_t readBodyByLength(size_t contentLength, unsigned long start) {
        size_t len = 0;
        while (len < contentLength && len < sizeof(buffer) - 1 && (millis() - start < timeout)) {
            if (!client.available()) {
                if (!client.connected()) {
                    break;
                }
                delay(1);
                continue;
            }

            int value = client.read();
            if (value < 0) {
                continue;
            }
            buffer[len++] = static_cast<char>(value);
        }
        return len;
    }

    size_t readAvailableBody(unsigned long start) {
        size_t len = 0;
        while ((client.available() || client.connected()) && len < sizeof(buffer) - 1 &&
               (millis() - start < timeout)) {
            if (!client.available()) {
                delay(1);
                continue;
            }

            int value = client.read();
            if (value < 0) {
                continue;
            }

            buffer[len++] = static_cast<char>(value);
        }
        return len;
    }

public:
    explicit RpcHttpServerTransport(Client& c) : client(c) {
        setTimeout(RPC_HTTP_TIMEOUT);
    }

    String read() override {
        unsigned long start = millis();
        if (!waitForReadable(start)) {
            return "";
        }

        String line;
        if (!readHeaderLine(line, start)) {
            return "";
        }

        int contentLength = -1;
        bool isHttpRequest = line.startsWith("POST") || line.startsWith("GET");

        if (isHttpRequest) {
            while (millis() - start < timeout) {
                if (!readHeaderLine(line, start)) {
                    break;
                }
                if (line.isEmpty()) {
                    break;
                }

                String lower = line;
                lower.toLowerCase();
                if (lower.startsWith("content-length:")) {
                    contentLength = line.substring(line.indexOf(':') + 1).toInt();
                }
            }
        } else {
            line.toCharArray(buffer, sizeof(buffer));
            RPC_LOG_F("HTTP server RX: %s", buffer);
            return String(buffer);
        }

        size_t len = contentLength >= 0
            ? readBodyByLength(static_cast<size_t>(contentLength), start)
            : readAvailableBody(start);

        buffer[len] = '\0';

        String result(buffer);
        result.trim();

        RPC_LOG_F("HTTP server RX: %s", result.c_str());
        return result;
    }

    bool write(const String& data) override {
        RPC_LOG_F("HTTP server TX: %s", data.c_str());

        if (data.isEmpty()) {
            client.println("HTTP/1.1 204 No Content");
            client.println("Connection: close");
            client.println("Content-Length: 0");
            client.println();
            client.flush();
            return true;
        }

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.print("Content-Length: ");
        client.println(data.length());
        client.println();
        client.print(data);
        client.flush();

        return true;
    }

    bool available() override {
        return client.available() > 0;
    }
};

#endif // RPC_HTTP_SERVER_TRANSPORT_H
