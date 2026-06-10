/**
 * RPC Arduino Toolkit - Client Implementation
 */

#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#include <ArduinoJson.h>
#include "RpcConfig.h"
#include "RpcTypes.h"
#include "RpcTransport.h"

class RpcClient {
private:
    RpcTransport& transport;
    unsigned long timeout;
    uint32_t requestId;

    static void appendJsonString(String& output, const char* value) {
        output += '"';
        if (value != nullptr) {
            for (const char* p = value; *p != '\0'; ++p) {
                unsigned char c = static_cast<unsigned char>(*p);
                switch (c) {
                    case '"': output += "\\\""; break;
                    case '\\': output += "\\\\"; break;
                    case '\b': output += "\\b"; break;
                    case '\f': output += "\\f"; break;
                    case '\n': output += "\\n"; break;
                    case '\r': output += "\\r"; break;
                    case '\t': output += "\\t"; break;
                    default:
                        if (c < 0x20) {
                            char escaped[7];
                            snprintf(escaped, sizeof(escaped), "\\u%04x", c);
                            output += escaped;
                        } else {
                            output += static_cast<char>(c);
                        }
                        break;
                }
            }
        }
        output += '"';
    }

    static void appendJsonString(String& output, const String& value) {
        appendJsonString(output, value.c_str());
    }

    static bool isJsonParams(const String& params) {
        for (size_t i = 0; i < params.length(); ++i) {
            char c = params[i];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                continue;
            }
            return c == '{' || c == '[';
        }
        return false;
    }

    // Build request JSON
    String buildRequest(const char* method, const String& params, bool isNotification = false) {
        String output;
        output.reserve(64 + strlen(method) + params.length());
        output += "{\"jsonrpc\":\"2.0\",\"method\":";
        appendJsonString(output, method);

        // Parse params if provided
        if (!params.isEmpty()) {
            output += ",\"params\":";
            if (isJsonParams(params)) {
                output += params;
            } else {
                appendJsonString(output, params);
            }
        }

        // Add ID unless notification
        if (!isNotification) {
            output += ",\"id\":";
            output += requestId++;
        }

        output += "}";
        return output;
    }

public:
    explicit RpcClient(RpcTransport& t)
        : transport(t), timeout(RPC_DEFAULT_TIMEOUT), requestId(1) {}

    /**
     * Call remote method
     * @param method Method name
     * @param params Parameters as JSON string
     * @return RpcResponse object
     */
    RpcResponse call(const char* method, const String& params = "") {
        String request = buildRequest(method, params);

        RPC_LOG_F("Client call: %s", request.c_str());

        // Send request
        if (!transport.write(request)) {
            RpcResponse resp;
            resp.setError(RPC_ERROR_SERVER, "Failed to send request", JsonVariant());
            return resp;
        }

        // Wait for response
        unsigned long start = millis();
        while (millis() - start < timeout) {
            if (transport.available()) {
                String responseJson = transport.read();
                if (!responseJson.isEmpty()) {
                    RPC_LOG_F("Client response: %s", responseJson.c_str());

                    RpcResponse resp;
                    resp.parse(responseJson);
                    return resp;
                }
            }
            delay(10);
        }

        // Timeout
        RpcResponse resp;
        resp.setError(RPC_ERROR_SERVER, "Request timeout", JsonVariant());
        return resp;
    }

    /**
     * Call method with JsonObject params
     */
    RpcResponse call(const char* method, JsonObject params) {
        String paramsStr;
        serializeJson(params, paramsStr);
        return call(method, paramsStr);
    }

    /**
     * Send a JSON-RPC batch request
     * @param batch Batch request as JSON array string
     * @return RpcBatchResponse object
     */
    RpcBatchResponse callBatch(const String& batch) {
        RPC_LOG_F("Client batch call: %s", batch.c_str());

        if (!transport.write(batch)) {
            RpcBatchResponse resp;
            resp.setError(RPC_ERROR_SERVER, "Failed to send batch request");
            return resp;
        }

        unsigned long start = millis();
        while (millis() - start < timeout) {
            if (transport.available()) {
                String responseJson = transport.read();
                if (!responseJson.isEmpty()) {
                    RPC_LOG_F("Client batch response: %s", responseJson.c_str());

                    RpcBatchResponse resp;
                    resp.parse(responseJson);
                    return resp;
                }
            }
            delay(10);
        }

        RpcBatchResponse resp;
        resp.setError(RPC_ERROR_SERVER, "Batch request timeout");
        return resp;
    }

    /**
     * Send notification (no response expected)
     */
    void notify(const char* method, const String& params = "") {
        String request = buildRequest(method, params, true);
        RPC_LOG_F("Client notify: %s", request.c_str());
        transport.write(request);
    }

    /**
     * Send notification with JsonObject params
     */
    void notify(const char* method, JsonObject params) {
        String paramsStr;
        serializeJson(params, paramsStr);
        notify(method, paramsStr);
    }

    /**
     * Set request timeout
     */
    void setTimeout(unsigned long ms) {
        timeout = ms;
        transport.setTimeout(ms);
    }

    /**
     * Get current timeout
     */
    unsigned long getTimeout() const {
        return timeout;
    }
};

#endif // RPC_CLIENT_H
