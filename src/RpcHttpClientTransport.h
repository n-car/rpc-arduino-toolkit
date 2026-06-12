/**
 * RPCToolkit - HTTP Client Transport
 *
 * Client-side JSON-RPC transport over an Arduino Client-compatible socket.
 * Works with WiFiClient, EthernetClient, and other Client implementations.
 */

#ifndef RPC_HTTP_CLIENT_TRANSPORT_H
#define RPC_HTTP_CLIENT_TRANSPORT_H

#include <Arduino.h>
#include <Client.h>
#include <stdlib.h>
#include "RpcTransport.h"

class RpcHttpClientTransport : public RpcTransport {
private:
    Client& client;
    const char* host;
    uint16_t port;
    const char* path;
    const char* userAgent;
    String responseBody;
    String lastErrorMessage;
    bool hasResponse;
    int httpStatus;
    unsigned long socketSettleDelay;
    bool remoteSafeFlag;
    bool remoteSafeHeaderSeen;

    void resetState() {
        responseBody = "";
        lastErrorMessage = "";
        hasResponse = false;
        httpStatus = 0;
        remoteSafeFlag = false;
        remoteSafeHeaderSeen = false;
    }

    bool waitForReadable(unsigned long start) {
        while (millis() - start < timeout) {
            if (client.available()) {
                return true;
            }
            if (!client.connected()) {
                return client.available() > 0;
            }
            delay(5);
        }
        return false;
    }

    bool readLine(String& line, unsigned long start) {
        if (!waitForReadable(start)) {
            return false;
        }

        line = client.readStringUntil('\n');
        line.trim();
        return true;
    }

    int parseHttpStatus(const String& statusLine) const {
        int firstSpace = statusLine.indexOf(' ');
        if (firstSpace < 0 || firstSpace + 4 > static_cast<int>(statusLine.length())) {
            return 0;
        }
        return statusLine.substring(firstSpace + 1, firstSpace + 4).toInt();
    }

    bool parseBoolHeaderValue(const String& line) const {
        int colon = line.indexOf(':');
        if (colon < 0) {
            return false;
        }

        String value = line.substring(colon + 1);
        value.trim();
        value.toLowerCase();
        return value == "true" || value == "1";
    }

    bool appendResponseChar(int value) {
        if (value < 0) {
            return true;
        }

        if (responseBody.length() >= RPC_MAX_RESPONSE_SIZE) {
            lastErrorMessage = "HTTP body too large";
            return false;
        }

        responseBody += static_cast<char>(value);
        return true;
    }

    bool readBodyByLength(size_t contentLength, unsigned long start) {
        responseBody.reserve(contentLength < RPC_MAX_RESPONSE_SIZE ? contentLength : RPC_MAX_RESPONSE_SIZE);
        unsigned long lastProgress = millis();

        while (responseBody.length() < contentLength && millis() - start < timeout) {
            if (!client.available()) {
                if (!client.connected() && millis() - lastProgress > 250) {
                    break;
                }
                delay(1);
                continue;
            }

            if (!appendResponseChar(client.read())) {
                return false;
            }
            lastProgress = millis();
#if defined(ESP8266)
            if ((responseBody.length() & 0x3f) == 0) {
                delay(0);
            }
#endif
        }

        if (responseBody.length() < contentLength) {
            lastErrorMessage = "HTTP body timeout";
            return false;
        }

        return true;
    }

    bool readBodyUntilClose(unsigned long start) {
        while (millis() - start < timeout) {
            while (client.available()) {
                if (!appendResponseChar(client.read())) {
                    return false;
                }
                start = millis();
            }

            if (!client.connected()) {
                return true;
            }
            delay(5);
        }

        lastErrorMessage = "HTTP body timeout";
        return false;
    }

    bool readChunkedBody(unsigned long start) {
        while (millis() - start < timeout) {
            String sizeLine;
            if (!readLine(sizeLine, start)) {
                lastErrorMessage = "HTTP chunk timeout";
                return false;
            }

            int separator = sizeLine.indexOf(';');
            if (separator >= 0) {
                sizeLine = sizeLine.substring(0, separator);
            }
            sizeLine.trim();

            size_t chunkSize = static_cast<size_t>(strtoul(sizeLine.c_str(), nullptr, 16));
            if (chunkSize == 0) {
                while (readLine(sizeLine, start) && !sizeLine.isEmpty()) {
                    // Discard trailing headers.
                }
                return true;
            }

            size_t readBytes = 0;
            while (readBytes < chunkSize && millis() - start < timeout) {
                if (!client.available()) {
                    if (!client.connected()) {
                        lastErrorMessage = "HTTP chunk ended early";
                        return false;
                    }
                    delay(1);
                    continue;
                }

                if (!appendResponseChar(client.read())) {
                    return false;
                }
                readBytes++;
            }

            if (readBytes < chunkSize) {
                lastErrorMessage = "HTTP chunk timeout";
                return false;
            }

            if (waitForReadable(start)) {
                client.readStringUntil('\n');
            }
        }

        lastErrorMessage = "HTTP chunk timeout";
        return false;
    }

public:
    RpcHttpClientTransport(Client& c, const char* h, uint16_t p, const char* requestPath = "/")
        : client(c),
          host(h),
          port(p),
          path(requestPath),
          userAgent("rpc-arduino-toolkit"),
          hasResponse(false),
          httpStatus(0),
          socketSettleDelay(0),
          remoteSafeFlag(false),
          remoteSafeHeaderSeen(false) {
        setTimeout(RPC_HTTP_TIMEOUT);
    }

    bool write(const String& data) override {
        resetState();

        client.stop();
        if (socketSettleDelay > 0) {
            delay(socketSettleDelay);
        }

        if (!client.connect(host, port)) {
            lastErrorMessage = "TCP connect failed";
            return false;
        }

        client.print("POST ");
        client.print(path);
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.print(host);
        client.print(":");
        client.println(port);
        client.print("User-Agent: ");
        client.println(userAgent);
        client.println("Content-Type: application/json");
        client.println("Accept: application/json");
        client.print("X-RPC-Safe-Enabled: ");
        client.println(RPC_ENABLE_SAFE_MODE ? "true" : "false");
        client.println("Connection: close");
        client.print("Content-Length: ");
        client.println(data.length());
        client.println();
        client.print(data);
        client.flush();

        unsigned long start = millis();
        String line;
        if (!readLine(line, start)) {
            lastErrorMessage = "HTTP response timeout";
            client.stop();
            return false;
        }

        httpStatus = parseHttpStatus(line);
        int contentLength = -1;
        bool isChunked = false;

        while (millis() - start < timeout) {
            if (!readLine(line, start)) {
                lastErrorMessage = "HTTP headers timeout";
                client.stop();
                return false;
            }
            if (line.isEmpty()) {
                break;
            }

            String lower = line;
            lower.toLowerCase();
            if (lower.startsWith("content-length:")) {
                contentLength = line.substring(line.indexOf(':') + 1).toInt();
            } else if (lower.startsWith("transfer-encoding:") && lower.indexOf("chunked") >= 0) {
                isChunked = true;
            } else if (lower.startsWith("x-rpc-safe-enabled:")) {
                remoteSafeHeaderSeen = true;
                remoteSafeFlag = parseBoolHeaderValue(line);
            }
        }

        bool bodyOk = isChunked
            ? readChunkedBody(start)
            : (contentLength >= 0
                ? readBodyByLength(static_cast<size_t>(contentLength), start)
                : readBodyUntilClose(start));

        client.stop();
        if (socketSettleDelay > 0) {
            delay(socketSettleDelay);
        }

        responseBody.trim();

        if (!bodyOk) {
            return false;
        }

        if (httpStatus < 200 || httpStatus >= 300) {
            lastErrorMessage = "HTTP status " + String(httpStatus);
            return false;
        }

        if (responseBody.isEmpty()) {
            lastErrorMessage = "Empty HTTP body";
            return false;
        }

        hasResponse = true;
        return true;
    }

    String read() override {
        hasResponse = false;
        String body = responseBody;
        responseBody = "";
        return body;
    }

    bool available() override {
        return hasResponse;
    }

    void setTimeout(unsigned long ms) override {
        RpcTransport::setTimeout(ms);
        client.setTimeout(ms);
    }

    void setSocketSettleDelay(unsigned long ms) {
        socketSettleDelay = ms;
    }

    const String& lastError() const {
        return lastErrorMessage;
    }

    int lastStatus() const {
        return httpStatus;
    }

    bool isHttp() const override {
        return true;
    }

    bool hasRemoteSafeHeader() const override {
        return remoteSafeHeaderSeen;
    }

    bool remoteSafeEnabled() const override {
        return remoteSafeFlag;
    }
};

#endif // RPC_HTTP_CLIENT_TRANSPORT_H
