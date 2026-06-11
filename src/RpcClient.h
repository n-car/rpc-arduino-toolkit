/**
 * RPC Arduino Toolkit - Client Implementation
 */

#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#include <string.h>
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

#if RPC_ENABLE_SAFE_MODE
    static String encodeParamsForRequest(const String& params) {
        if (params.isEmpty()) {
            return params;
        }

        StaticJsonDocument<RPC_JSON_DOC_SIZE> source;
        DeserializationError error = deserializeJson(source, params);
        if (!error) {
            StaticJsonDocument<RPC_JSON_DOC_SIZE> encoded;
            RpcSafe::encodeValue(source.as<JsonVariantConst>(), encoded.to<JsonVariant>());

            String output;
            serializeJson(encoded, output);
            return output;
        }

        return RpcSafe::serializeString(params);
    }

    static String encodeBatchForRequest(const String& batch) {
        StaticJsonDocument<RPC_JSON_DOC_SIZE> source;
        DeserializationError error = deserializeJson(source, batch);
        if (error || !source.is<JsonArray>()) {
            return batch;
        }

        StaticJsonDocument<RPC_JSON_DOC_SIZE> encoded;
        JsonArray outputBatch = encoded.to<JsonArray>();

        for (JsonVariantConst item : source.as<JsonArrayConst>()) {
            if (!item.is<JsonObjectConst>()) {
                RpcSafe::encodeArrayElement(item, outputBatch);
                continue;
            }

            JsonObject outputItem = outputBatch.createNestedObject();
            for (JsonPairConst pair : item.as<JsonObjectConst>()) {
                const char* key = pair.key().c_str();
                if (strcmp(key, "params") == 0) {
                    RpcSafe::encodeObjectMember(pair.value(), outputItem, "params");
                } else {
                    outputItem[key].set(pair.value());
                }
            }
        }

        String output;
        serializeJson(encoded, output);
        return output;
    }

    bool shouldDecodeSafeResponse() const {
        return transport.isHttp() && transport.remoteSafeEnabled();
    }

    bool hasRequiredSafeResponseHeader() const {
        return !transport.isHttp() || transport.hasRemoteSafeHeader();
    }
#endif

    // Build request JSON
    String buildRequest(const char* method, const String& params, bool isNotification = false) {
#if RPC_ENABLE_SAFE_MODE
        String wireParams = encodeParamsForRequest(params);
#else
        const String& wireParams = params;
#endif

        String output;
        output.reserve(64 + strlen(method) + wireParams.length());
        output += "{\"jsonrpc\":\"2.0\",\"method\":";
        appendJsonString(output, method);

        // Parse params if provided
        if (!wireParams.isEmpty()) {
            output += ",\"params\":";
            if (isJsonParams(wireParams)) {
                output += wireParams;
            } else {
                appendJsonString(output, wireParams);
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
#if RPC_ENABLE_SAFE_MODE
                    if (!hasRequiredSafeResponseHeader()) {
                        resp.setError(
                            RPC_ERROR_INVALID_REQ,
                            "RPC Compatibility Error: Safe Mode response header missing.",
                            JsonVariant()
                        );
                        return resp;
                    }
                    resp.parse(responseJson, shouldDecodeSafeResponse());
#else
                    resp.parse(responseJson);
#endif
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
#if RPC_ENABLE_SAFE_MODE
        String wireBatch = encodeBatchForRequest(batch);
#else
        const String& wireBatch = batch;
#endif

        RPC_LOG_F("Client batch call: %s", wireBatch.c_str());

        if (!transport.write(wireBatch)) {
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
#if RPC_ENABLE_SAFE_MODE
                    if (!hasRequiredSafeResponseHeader()) {
                        resp.setError(RPC_ERROR_INVALID_REQ, "RPC Compatibility Error: Safe Mode response header missing.");
                        return resp;
                    }
                    resp.parse(responseJson, shouldDecodeSafeResponse());
#else
                    resp.parse(responseJson);
#endif
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
