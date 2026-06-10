/**
 * RPC Arduino Toolkit - Server Implementation
 */

#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include <ArduinoJson.h>
#include "RpcConfig.h"
#include "RpcTypes.h"
#include "RpcTransport.h"

// ============================================================================
// RPC Server
// ============================================================================

template<uint8_t MAX_METHODS = RPC_MAX_METHODS>
class RpcServer {
private:
    struct Method {
        char name[RPC_MAX_METHOD_NAME];
        RpcMethodHandler handler;
        RpcResponseHandler responseHandler;
        bool returnsResponse;
        bool active;
#if RPC_ENABLE_SCHEMA_SUPPORT
        char description[RPC_MAX_DESCRIPTION];
        bool exposeSchema;
#endif
    };

    Method methods[MAX_METHODS];
    uint8_t methodCount;

    // Parse request from a deserialized JSON value
    bool parseRequest(JsonVariant item, RpcRequest& req) {
        if (!item.is<JsonObject>()) {
            return false;
        }

        req.jsonrpc = item["jsonrpc"] | "";
        req.method = item["method"] | "";
        req.params = item["params"].as<JsonVariantConst>();
        req.id = item["id"];

        return req.isValid();
    }

    // Parse request from JSON
    bool parseRequest(const String& json, RpcRequest& req, StaticJsonDocument<RPC_JSON_DOC_SIZE>& doc) {
        DeserializationError error = deserializeJson(doc, json);
        if (error) {
            RPC_LOG_F("Parse error: %s", error.c_str());
            return false;
        }

        return parseRequest(doc.as<JsonVariant>(), req);
    }

    // Execute method
    RpcResponse executeMethod(RpcRequest& req, bool encodeSafe = false) {
        // Built-in introspection methods (memory-efficient)
        if (req.method == "__rpc.listMethods") {
            StaticJsonDocument<256> doc;
            JsonArray arr = doc.to<JsonArray>();

            for (uint8_t i = 0; i < MAX_METHODS; i++) {
                if (methods[i].active) {
                    arr.add(methods[i].name);
                }
            }

            RpcResponse resp;
            resp.setResult(doc.as<JsonVariant>(), req.id, encodeSafe);
            return resp;
        }

        if (req.method == "__rpc.version") {
            StaticJsonDocument<128> doc;
            doc["toolkit"] = "rpc-arduino-toolkit";
            doc["version"] = "1.0.0";
            doc["methodCount"] = methodCount;

            RpcResponse resp;
            resp.setResult(doc.as<JsonVariant>(), req.id, encodeSafe);
            return resp;
        }

#if RPC_ENABLE_SCHEMA_SUPPORT
        // __rpc.describe - Get method description and schema availability
        if (req.method == "__rpc.describe") {
            const char* methodName = req.params["method"] | "";

            if (strlen(methodName) == 0) {
                return RpcError::invalidParams(req.id);
            }

            // Prevent introspection of __rpc.* methods
            if (strncmp(methodName, "__rpc.", 6) == 0) {
                RpcResponse resp;
                resp.setError(RPC_ERROR_METHOD_NOT_FOUND, "Cannot describe introspection methods", req.id);
                return resp;
            }

            // Find method
            Method* method = nullptr;
            for (uint8_t i = 0; i < MAX_METHODS; i++) {
                if (methods[i].active && strcmp(methods[i].name, methodName) == 0) {
                    method = &methods[i];
                    break;
                }
            }

            if (!method) {
                return RpcError::methodNotFound(methodName, req.id);
            }

            // Check if schema is exposed
            if (!method->exposeSchema) {
                RpcResponse resp;
                resp.setError(RPC_ERROR_METHOD_NOT_FOUND, "Method schema not available", req.id);
                return resp;
            }

            StaticJsonDocument<256> doc;
            doc["name"] = method->name;
            doc["description"] = method->description;
            doc["exposeSchema"] = method->exposeSchema;

            RpcResponse resp;
            resp.setResult(doc.as<JsonVariant>(), req.id, encodeSafe);
            return resp;
        }
#endif

        // __rpc.capabilities - Get server capabilities
        if (req.method == "__rpc.capabilities") {
            StaticJsonDocument<256> doc;
            doc["batch"] = (RPC_ENABLE_BATCH != 0);
            doc["introspection"] = true;
            doc["safeMode"] = (RPC_ENABLE_SAFE_MODE != 0);
            doc["strictMode"] = (RPC_ENABLE_SAFE_MODE != 0) && (RPC_SAFE_STRICT_MODE != 0);
            doc["notifications"] = (RPC_ENABLE_NOTIFICATIONS != 0);
            doc["schemaSupport"] = (RPC_ENABLE_SCHEMA_SUPPORT != 0);
            doc["methodCount"] = methodCount;
            doc["maxMethods"] = MAX_METHODS;

            RpcResponse resp;
            resp.setResult(doc.as<JsonVariant>(), req.id, encodeSafe);
            return resp;
        }

        // Find method
        Method* method = nullptr;
        for (uint8_t i = 0; i < MAX_METHODS; i++) {
            if (methods[i].active && strcmp(methods[i].name, req.method.c_str()) == 0) {
                method = &methods[i];
                break;
            }
        }

        if (!method) {
            return RpcError::methodNotFound(req.method.c_str(), req.id);
        }

        // Execute handler
        try {
            if (method->returnsResponse) {
                return method->responseHandler(req, encodeSafe);
            } else {
                JsonVariant result = method->handler(req.params);
                RpcResponse resp;
                resp.setResult(result, req.id, encodeSafe);
                return resp;
            }
        } catch (...) {
            return RpcError::internalError(req.id);
        }
    }

#if RPC_ENABLE_BATCH
    String handleBatchRequest(JsonArray batch, bool encodeSafe = false) {
        if (batch.size() == 0) {
            RpcResponse resp = RpcError::invalidRequest(JsonVariant());
            return resp.toString();
        }

        String output;
        output.reserve(RPC_MAX_RESPONSE_SIZE);
        output += '[';
        bool hasResponse = false;

        for (JsonVariant item : batch) {
            String responseJson;
            bool includeResponse = true;

            if (!item.is<JsonObject>()) {
                responseJson = RpcError::invalidRequest(JsonVariant()).toString();
            } else {
                RpcRequest req;
                if (!parseRequest(item, req)) {
                    JsonVariant id = item["id"];
                    responseJson = RpcError::invalidRequest(id).toString();
                } else if (req.isNotification()) {
                    executeMethod(req, encodeSafe);
                    includeResponse = false;
                } else {
                    responseJson = executeMethod(req, encodeSafe).toString();
                }
            }

            if (!includeResponse) {
                continue;
            }

            if (hasResponse) {
                output += ',';
            }
            output += responseJson;
            hasResponse = true;
        }

        if (!hasResponse) {
            return "";
        }

        output += ']';
        return output;
    }
#endif

    void decodeParamsInPlace(JsonVariant root, bool decodeSafe) {
#if RPC_ENABLE_SAFE_MODE
        if (!decodeSafe) {
            return;
        }

        if (root.is<JsonArray>()) {
            for (JsonVariant item : root.as<JsonArray>()) {
                if (item.is<JsonObject>() && item.containsKey("params")) {
                    RpcSafe::decodeInPlace(item["params"]);
                }
            }
            return;
        }

        if (root.is<JsonObject>() && root.containsKey("params")) {
            RpcSafe::decodeInPlace(root["params"]);
        }
#else
        (void)root;
        (void)decodeSafe;
#endif
    }

    String missingSafeHeaderResponse(const String& json) {
        StaticJsonDocument<RPC_JSON_DOC_SIZE> doc;
        DeserializationError error = deserializeJson(doc, json);
        if (error) {
            RpcResponse resp = RpcError::parseError(JsonVariant());
            return resp.toString();
        }

        if (doc.is<JsonArray>()) {
#if RPC_ENABLE_BATCH
            JsonArray batch = doc.as<JsonArray>();
            if (batch.size() == 0) {
                RpcResponse resp = RpcError::invalidRequest(JsonVariant());
                return resp.toString();
            }

            String output;
            output.reserve(RPC_MAX_RESPONSE_SIZE);
            output += '[';
            bool hasResponse = false;

            for (JsonVariant item : batch) {
                bool includeResponse = true;
                JsonVariant id;

                if (item.is<JsonObject>()) {
                    id = item["id"];
                    includeResponse = !id.isNull();
                }

                if (!includeResponse) {
                    continue;
                }

                RpcResponse resp = RpcError::compatibilityError(id);
                if (hasResponse) {
                    output += ',';
                }
                output += resp.toString();
                hasResponse = true;
            }

            if (!hasResponse) {
                return "";
            }

            output += ']';
            return output;
#else
            RpcResponse resp = RpcError::compatibilityError(JsonVariant());
            return resp.toString();
#endif
        }

        if (!doc.is<JsonObject>()) {
            RpcResponse resp = RpcError::invalidRequest(JsonVariant());
            return resp.toString();
        }

        JsonVariant id = doc["id"];
        if (id.isNull()) {
            return "";
        }

        RpcResponse resp = RpcError::compatibilityError(id);
        return resp.toString();
    }

public:
    RpcServer() : methodCount(0) {
        for (uint8_t i = 0; i < MAX_METHODS; i++) {
            methods[i].active = false;
            methods[i].returnsResponse = false;
#if RPC_ENABLE_SCHEMA_SUPPORT
            methods[i].description[0] = '\0';
            methods[i].exposeSchema = false;
#endif
        }
    }

    /**
     * Register a method
     * @param name Method name
     * @param handler Function to handle the method
     * @return true if successful
     */
    bool addMethod(const char* name, RpcMethodHandler handler) {
        return addMethod(name, handler, "", false);
    }

#if RPC_ENABLE_SCHEMA_SUPPORT
    /**
     * Register a method with description and schema exposure
     * @param name Method name
     * @param handler Function to handle the method
     * @param description Method description (max RPC_MAX_DESCRIPTION chars)
     * @param exposeSchema Whether to expose schema via introspection
     * @return true if successful
     */
    bool addMethod(const char* name, RpcMethodHandler handler, const char* description, bool exposeSchema = false) {
#else
    bool addMethod(const char* name, RpcMethodHandler handler, const char* description = "", bool exposeSchema = false) {
#endif
        if (methodCount >= MAX_METHODS) {
            RPC_LOG("Max methods reached!");
            return false;
        }

        if (strlen(name) >= RPC_MAX_METHOD_NAME) {
            RPC_LOG("Method name too long!");
            return false;
        }

        // Find free slot
        for (uint8_t i = 0; i < MAX_METHODS; i++) {
            if (!methods[i].active) {
                strncpy(methods[i].name, name, RPC_MAX_METHOD_NAME - 1);
                methods[i].name[RPC_MAX_METHOD_NAME - 1] = '\0';
                methods[i].handler = handler;
                methods[i].responseHandler = nullptr;
                methods[i].returnsResponse = false;
                methods[i].active = true;
#if RPC_ENABLE_SCHEMA_SUPPORT
                strncpy(methods[i].description, description, RPC_MAX_DESCRIPTION - 1);
                methods[i].description[RPC_MAX_DESCRIPTION - 1] = '\0';
                methods[i].exposeSchema = exposeSchema;
#else
                (void)description;  // Suppress unused parameter warning
                (void)exposeSchema;
#endif
                methodCount++;

                RPC_LOG_F("Method registered: %s", name);
                return true;
            }
        }

        return false;
    }

    /**
     * Register a method that returns a complete JSON-RPC response.
     * Use this for custom errors with error.data or other advanced responses.
     */
    bool addResponseMethod(const char* name, RpcResponseHandler handler) {
        return addResponseMethod(name, handler, "", false);
    }

#if RPC_ENABLE_SCHEMA_SUPPORT
    bool addResponseMethod(const char* name, RpcResponseHandler handler, const char* description, bool exposeSchema = false) {
#else
    bool addResponseMethod(const char* name, RpcResponseHandler handler, const char* description = "", bool exposeSchema = false) {
#endif
        if (methodCount >= MAX_METHODS) {
            RPC_LOG("Max methods reached!");
            return false;
        }

        if (strlen(name) >= RPC_MAX_METHOD_NAME) {
            RPC_LOG("Method name too long!");
            return false;
        }

        for (uint8_t i = 0; i < MAX_METHODS; i++) {
            if (!methods[i].active) {
                strncpy(methods[i].name, name, RPC_MAX_METHOD_NAME - 1);
                methods[i].name[RPC_MAX_METHOD_NAME - 1] = '\0';
                methods[i].handler = nullptr;
                methods[i].responseHandler = handler;
                methods[i].returnsResponse = true;
                methods[i].active = true;
#if RPC_ENABLE_SCHEMA_SUPPORT
                strncpy(methods[i].description, description, RPC_MAX_DESCRIPTION - 1);
                methods[i].description[RPC_MAX_DESCRIPTION - 1] = '\0';
                methods[i].exposeSchema = exposeSchema;
#else
                (void)description;
                (void)exposeSchema;
#endif
                methodCount++;

                RPC_LOG_F("Response method registered: %s", name);
                return true;
            }
        }

        return false;
    }

    /**
     * Register a simple method (no parameters)
     */
    bool addMethod(const char* name, RpcSimpleHandler handler) {
        return addMethod(name, [handler](JsonVariantConst params) -> JsonVariant {
            (void)params;
            return handler();
        });
    }

    /**
     * Register a simple method with description/schema metadata
     */
    bool addMethod(const char* name, RpcSimpleHandler handler, const char* description, bool exposeSchema = false) {
        return addMethod(name, [handler](JsonVariantConst params) -> JsonVariant {
            (void)params;
            return handler();
        }, description, exposeSchema);
    }

    /**
     * Remove a method
     */
    bool removeMethod(const char* name) {
        for (uint8_t i = 0; i < MAX_METHODS; i++) {
            if (methods[i].active && strcmp(methods[i].name, name) == 0) {
                methods[i].active = false;
                methods[i].handler = nullptr;
                methods[i].responseHandler = nullptr;
                methods[i].returnsResponse = false;
                methodCount--;
                RPC_LOG_F("Method removed: %s", name);
                return true;
            }
        }
        return false;
    }

    /**
     * Handle request from transport
     */
    String handleRequest(RpcTransport& transport) {
        String json = transport.read();
        if (json.isEmpty()) {
            return "";
        }

        bool decodeSafe = false;
        bool encodeSafe = false;

#if RPC_ENABLE_SAFE_MODE
        encodeSafe = true;
        if (transport.isHttp()) {
            if ((RPC_SAFE_STRICT_MODE != 0) && !transport.hasClientSafeHeader()) {
                return missingSafeHeaderResponse(json);
            }
            decodeSafe = transport.hasClientSafeHeader() && transport.clientSafeEnabled();
        }
#endif

        return handleRequest(json, decodeSafe, encodeSafe);
    }

    /**
     * Handle request from JSON string
     */
    String handleRequest(const String& json) {
        return handleRequest(json, false, (RPC_ENABLE_SAFE_MODE != 0));
    }

    String handleRequest(const String& json, bool decodeSafe, bool encodeSafe) {
        StaticJsonDocument<RPC_JSON_DOC_SIZE> doc;
        DeserializationError error = deserializeJson(doc, json);
        if (error) {
            RPC_LOG_F("Parse error: %s", error.c_str());
            RpcResponse resp = RpcError::parseError(JsonVariant());
            return resp.toString();
        }

        decodeParamsInPlace(doc.as<JsonVariant>(), decodeSafe);

        if (doc.is<JsonArray>()) {
#if RPC_ENABLE_BATCH
            return handleBatchRequest(doc.as<JsonArray>(), encodeSafe);
#else
            RpcResponse resp = RpcError::invalidRequest(JsonVariant());
            return resp.toString();
#endif
        }

        RpcRequest req;
        if (!parseRequest(doc.as<JsonVariant>(), req)) {
            RpcResponse resp = RpcError::invalidRequest(JsonVariant());
            return resp.toString();
        }

        // Notification? (no response needed)
        if (req.isNotification()) {
            executeMethod(req, encodeSafe);
            return "";
        }

        // Execute and return response
        RpcResponse resp = executeMethod(req, encodeSafe);
        return resp.toString();
    }

    /**
     * Get number of registered methods
     */
    uint8_t getMethodCount() const {
        return methodCount;
    }
};

#endif // RPC_SERVER_H
