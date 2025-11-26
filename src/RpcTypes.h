/**
 * RPC Arduino Toolkit - Core Types
 */

#ifndef RPC_TYPES_H
#define RPC_TYPES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "RpcConfig.h"

// ============================================================================
// Forward Declarations
// ============================================================================

class RpcTransport;
class RpcRequest;
class RpcResponse;

// ============================================================================
// Type Definitions
// ============================================================================

// Method handler function signature
// Takes JsonObject parameters and returns JsonVariant result
typedef std::function<JsonVariant(JsonObject)> RpcMethodHandler;

// Simple handler without parameters
typedef std::function<JsonVariant(void)> RpcSimpleHandler;

// ============================================================================
// RPC Request
// ============================================================================

class RpcRequest {
public:
    String jsonrpc;        // Always "2.0"
    String method;         // Method name
    JsonObject params;     // Method parameters
    JsonVariant id;        // Request ID (null for notifications)
    
    RpcRequest() : jsonrpc("2.0") {}
    
    bool isNotification() const {
        return id.isNull();
    }
    
    bool isValid() const {
        return jsonrpc == "2.0" && !method.isEmpty();
    }
};

// ============================================================================
// RPC Response
// ============================================================================

class RpcResponse {
private:
    StaticJsonDocument<RPC_JSON_DOC_SIZE> doc;
    bool _hasError;
    bool _isValid;
    
public:
    RpcResponse() : _hasError(false), _isValid(false) {}
    
    // Success response
    void setResult(JsonVariant result, JsonVariant id) {
        doc.clear();
        doc["jsonrpc"] = "2.0";
        doc["result"] = result;
        doc["id"] = id;
        _hasError = false;
        _isValid = true;
    }
    
    // Error response
    void setError(int code, const char* message, JsonVariant id) {
        doc.clear();
        doc["jsonrpc"] = "2.0";
        JsonObject error = doc.createNestedObject("error");
        error["code"] = code;
        error["message"] = message;
        doc["id"] = id;
        _hasError = true;
        _isValid = true;
    }
    
    // Parse from JSON string
    bool parse(const String& json) {
        DeserializationError error = deserializeJson(doc, json);
        if (error) {
            RPC_LOG_F("Failed to parse response: %s", error.c_str());
            _isValid = false;
            return false;
        }
        
        _hasError = doc.containsKey("error");
        _isValid = doc["jsonrpc"] == "2.0";
        return _isValid;
    }
    
    // Serialize to JSON string
    String toString() const {
        String output;
        serializeJson(doc, output);
        return output;
    }
    
    // Check if response has error
    bool hasError() const { return _hasError; }
    bool isSuccess() const { return _isValid && !_hasError; }
    bool isValid() const { return _isValid; }
    
    // Get result as specific type
    template<typename T>
    T result() const {
        if (_hasError) return T();
        return doc["result"].as<T>();
    }
    
    // Get result as JsonVariant
    JsonVariant result() const {
        return doc["result"];
    }
    
    // Get error code
    int errorCode() const {
        if (!_hasError) return 0;
        return doc["error"]["code"];
    }
    
    // Get error message
    String errorMessage() const {
        if (!_hasError) return "";
        return doc["error"]["message"].as<String>();
    }
    
    // Get ID
    JsonVariant id() const {
        return doc["id"];
    }
};

// ============================================================================
// RPC Error Helper
// ============================================================================

class RpcError {
public:
    static RpcResponse parseError(JsonVariant id) {
        RpcResponse resp;
        resp.setError(RPC_ERROR_PARSE, "Parse error", id);
        return resp;
    }
    
    static RpcResponse invalidRequest(JsonVariant id) {
        RpcResponse resp;
        resp.setError(RPC_ERROR_INVALID_REQ, "Invalid Request", id);
        return resp;
    }
    
    static RpcResponse methodNotFound(const char* method, JsonVariant id) {
        RpcResponse resp;
        String msg = "Method not found: ";
        msg += method;
        resp.setError(RPC_ERROR_METHOD_NOT_FOUND, msg.c_str(), id);
        return resp;
    }
    
    static RpcResponse invalidParams(JsonVariant id) {
        RpcResponse resp;
        resp.setError(RPC_ERROR_INVALID_PARAMS, "Invalid params", id);
        return resp;
    }
    
    static RpcResponse internalError(JsonVariant id) {
        RpcResponse resp;
        resp.setError(RPC_ERROR_INTERNAL, "Internal error", id);
        return resp;
    }
};

#endif // RPC_TYPES_H
