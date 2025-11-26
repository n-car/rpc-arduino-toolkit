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
        bool active;
#if RPC_ENABLE_SCHEMA_SUPPORT
        char description[RPC_MAX_DESCRIPTION];
        bool exposeSchema;
#endif
    };
    
    Method methods[MAX_METHODS];
    uint8_t methodCount;
    
    // Parse request from JSON
    bool parseRequest(const String& json, RpcRequest& req, StaticJsonDocument<RPC_JSON_DOC_SIZE>& doc) {
        DeserializationError error = deserializeJson(doc, json);
        if (error) {
            RPC_LOG_F("Parse error: %s", error.c_str());
            return false;
        }
        
        req.jsonrpc = doc["jsonrpc"] | "";
        req.method = doc["method"] | "";
        req.params = doc["params"].as<JsonObject>();
        req.id = doc["id"];
        
        return req.isValid();
    }
    
    // Execute method
    RpcResponse executeMethod(RpcRequest& req) {
        RpcResponse resp;
        
        // Built-in introspection methods (memory-efficient)
        if (req.method == "__rpc.listMethods") {
            StaticJsonDocument<256> doc;
            JsonArray arr = doc.to<JsonArray>();
            
            for (uint8_t i = 0; i < MAX_METHODS; i++) {
                if (methods[i].active) {
                    arr.add(methods[i].name);
                }
            }
            
            resp.setResult(doc.as<JsonVariant>(), req.id);
            return resp;
        }
        
        if (req.method == "__rpc.version") {
            StaticJsonDocument<128> doc;
            doc["toolkit"] = "rpc-arduino-toolkit";
            doc["version"] = "1.0.0";
            doc["methodCount"] = methodCount;
            
            resp.setResult(doc.as<JsonVariant>(), req.id);
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
            
            resp.setResult(doc.as<JsonVariant>(), req.id);
            return resp;
        }
#endif
        
        // __rpc.capabilities - Get server capabilities
        if (req.method == "__rpc.capabilities") {
            StaticJsonDocument<256> doc;
            doc["batch"] = RPC_ENABLE_BATCH;
            doc["introspection"] = true;
            doc["safeMode"] = RPC_ENABLE_SAFE_MODE;
            doc["notifications"] = RPC_ENABLE_NOTIFICATIONS;
            doc["schemaSupport"] = RPC_ENABLE_SCHEMA_SUPPORT;
            doc["methodCount"] = methodCount;
            doc["maxMethods"] = MAX_METHODS;
            
            resp.setResult(doc.as<JsonVariant>(), req.id);
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
            JsonVariant result = method->handler(req.params);
            resp.setResult(result, req.id);
        } catch (...) {
            return RpcError::internalError(req.id);
        }
        
        return resp;
    }
    
public:
    RpcServer() : methodCount(0) {
        for (uint8_t i = 0; i < MAX_METHODS; i++) {
            methods[i].active = false;
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
     * Register a simple method (no parameters)
     */
    bool addMethod(const char* name, RpcSimpleHandler handler) {
        return addMethod(name, [handler](JsonObject params) -> JsonVariant {
            return handler();
        });
    }
    
    /**
     * Remove a method
     */
    bool removeMethod(const char* name) {
        for (uint8_t i = 0; i < MAX_METHODS; i++) {
            if (methods[i].active && strcmp(methods[i].name, name) == 0) {
                methods[i].active = false;
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
        
        return handleRequest(json);
    }
    
    /**
     * Handle request from JSON string
     */
    String handleRequest(const String& json) {
        StaticJsonDocument<RPC_JSON_DOC_SIZE> doc;
        RpcRequest req;
        
        // Parse request
        if (!parseRequest(json, req, doc)) {
            RpcResponse resp = RpcError::parseError(nullptr);
            return resp.toString();
        }
        
        // Notification? (no response needed)
        if (req.isNotification()) {
            executeMethod(req);
            return "";
        }
        
        // Execute and return response
        RpcResponse resp = executeMethod(req);
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
