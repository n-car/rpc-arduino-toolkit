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
class RpcBatchResponse;

// ============================================================================
// Type Definitions
// ============================================================================

// Method handler function signature
// Takes read-only JSON-RPC parameters and returns JsonVariant result
typedef std::function<JsonVariant(JsonVariantConst)> RpcMethodHandler;

// Simple handler without parameters
typedef std::function<JsonVariant(void)> RpcSimpleHandler;

// ============================================================================
// RPC Request
// ============================================================================

class RpcRequest {
public:
    String jsonrpc;        // Always "2.0"
    String method;         // Method name
    JsonVariantConst params; // Method parameters
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
// Safe Serialization Helpers (if RPC_ENABLE_SAFE_MODE)
// ============================================================================

#if RPC_ENABLE_SAFE_MODE

class RpcSafe {
public:
    /**
     * Serialize a string with S: prefix for safe mode
     */
    static String serializeString(const String& value) {
        return "S:" + value;
    }

    /**
     * Serialize a date/timestamp with D: prefix (ISO 8601 or timestamp string)
     * @param timestamp Unix timestamp in seconds
     */
    static String serializeDate(unsigned long timestamp) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "D:%lu", timestamp);
        return String(buffer);
    }

    /**
     * Serialize a large integer with 'n' suffix (BigInt equivalent)
     */
    static String serializeBigInt(long long value) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%lldn", value);
        return String(buffer);
    }

    /**
     * Deserialize a safe string (remove S: prefix)
     */
    static String deserializeString(const String& value) {
        if (value.startsWith("S:")) {
            return value.substring(2);
        }
        return value;
    }

    /**
     * Deserialize a safe date (remove D: prefix and parse)
     */
    static unsigned long deserializeDate(const String& value) {
        if (value.startsWith("D:")) {
            return value.substring(2).toInt();
        }
        return 0;
    }

    /**
     * Check if string is a safe string
     */
    static bool isSafeString(const String& value) {
        return value.startsWith("S:");
    }

    /**
     * Check if string is a safe date
     */
    static bool isSafeDate(const String& value) {
        return value.startsWith("D:");
    }

    /**
     * Check if string is a BigInt marker
     */
    static bool isBigInt(const String& value) {
        if (value.length() < 2 || !value.endsWith("n")) {
            return false;
        }

        size_t start = 0;
        if (value[0] == '-') {
            start = 1;
            if (value.length() < 3) {
                return false;
            }
        }

        for (size_t i = start; i < value.length() - 1; ++i) {
            if (!isDigit(value[i])) {
                return false;
            }
        }

        return true;
    }

    /**
     * Deserialize a BigInt marker (remove 'n' suffix)
     */
    static long long deserializeBigInt(const String& value) {
        if (isBigInt(value)) {
            String numStr = value.substring(0, value.length() - 1);
            return atoll(numStr.c_str());
        }
        return 0;
    }

    /**
     * Check if a string already looks like explicit/manual Safe Mode helper output.
     * Automatic recursive encoding does not use this because it is datatype-based.
     */
    static bool isEncodedString(const String& value) {
        return isSafeString(value) || isSafeDate(value) || isBigInt(value);
    }

    static void encodeObjectMember(JsonVariantConst source, JsonObject object, const char* key) {
        if (source.isNull()) {
            object[key] = nullptr;
            return;
        }

        if (source.is<JsonObjectConst>()) {
            JsonObject nestedObject = object.createNestedObject(key);
            for (JsonPairConst pair : source.as<JsonObjectConst>()) {
                encodeObjectMember(pair.value(), nestedObject, pair.key().c_str());
            }
            return;
        }

        if (source.is<JsonArrayConst>()) {
            JsonArray nestedArray = object.createNestedArray(key);
            for (JsonVariantConst item : source.as<JsonArrayConst>()) {
                encodeArrayElement(item, nestedArray);
            }
            return;
        }

        if (source.is<const char*>()) {
            String value = source.as<String>();
            object[key] = serializeString(value);
            return;
        }

        object[key].set(source);
    }

    static void encodeArrayElement(JsonVariantConst source, JsonArray array) {
        if (source.isNull()) {
            array.add(nullptr);
            return;
        }

        if (source.is<JsonObjectConst>()) {
            JsonObject object = array.createNestedObject();
            for (JsonPairConst pair : source.as<JsonObjectConst>()) {
                encodeObjectMember(pair.value(), object, pair.key().c_str());
            }
            return;
        }

        if (source.is<JsonArrayConst>()) {
            JsonArray nestedArray = array.createNestedArray();
            for (JsonVariantConst item : source.as<JsonArrayConst>()) {
                encodeArrayElement(item, nestedArray);
            }
            return;
        }

        if (source.is<const char*>()) {
            String value = source.as<String>();
            array.add(serializeString(value));
            return;
        }

        array.add(source);
    }

    /**
     * Recursively encode JSON values using RPC Toolkit Safe Mode conventions.
     * Encoding is source-datatype based: every JSON string gets an S: prefix.
     */
    static void encodeValue(JsonVariantConst source, JsonVariant target) {
        if (source.isNull()) {
            target.set(nullptr);
            return;
        }

        if (source.is<JsonObjectConst>()) {
            JsonObject object = target.to<JsonObject>();
            for (JsonPairConst pair : source.as<JsonObjectConst>()) {
                encodeObjectMember(pair.value(), object, pair.key().c_str());
            }
            return;
        }

        if (source.is<JsonArrayConst>()) {
            JsonArray array = target.to<JsonArray>();
            for (JsonVariantConst item : source.as<JsonArrayConst>()) {
                encodeArrayElement(item, array);
            }
            return;
        }

        if (source.is<const char*>()) {
            String value = source.as<String>();
            target.set(serializeString(value));
            return;
        }

        target.set(source);
    }

    /**
     * Recursively decode JSON values that came from a Safe Mode peer.
     * D: and BigInt values are kept as strings in generic JSON to avoid overflow.
     */
    static void decodeInPlace(JsonVariant value) {
        if (value.isNull()) {
            return;
        }

        if (value.is<JsonObject>()) {
            for (JsonPair pair : value.as<JsonObject>()) {
                decodeInPlace(pair.value());
            }
            return;
        }

        if (value.is<JsonArray>()) {
            for (JsonVariant item : value.as<JsonArray>()) {
                decodeInPlace(item);
            }
            return;
        }

        if (value.is<const char*>()) {
            String text = value.as<String>();
            if (isSafeString(text) || isSafeDate(text)) {
                value.set(text.substring(2));
            } else if (isBigInt(text)) {
                value.set(text);
            }
        }
    }
};

#endif // RPC_ENABLE_SAFE_MODE

// ============================================================================
// RPC Response
// ============================================================================

class RpcResponse {
private:
#if defined(ESP8266)
    DynamicJsonDocument doc;
#else
    StaticJsonDocument<RPC_JSON_DOC_SIZE> doc;
#endif
    bool _hasError;
    bool _isValid;

public:
    RpcResponse()
#if defined(ESP8266)
        : doc(RPC_JSON_DOC_SIZE), _hasError(false), _isValid(false) {}
#else
        : _hasError(false), _isValid(false) {}
#endif

    RpcResponse(const RpcResponse& other)
#if defined(ESP8266)
        : doc(RPC_JSON_DOC_SIZE), _hasError(other._hasError), _isValid(other._isValid) {
#else
        : _hasError(other._hasError), _isValid(other._isValid) {
#endif
        doc.set(other.doc);
    }

    RpcResponse& operator=(const RpcResponse& other) {
        if (this != &other) {
            doc.set(other.doc);
            _hasError = other._hasError;
            _isValid = other._isValid;
        }
        return *this;
    }

    // Success response
    void setResult(JsonVariant result, JsonVariant id) {
        setResult(result, id, false);
    }

    void setResult(JsonVariant result, JsonVariant id, bool encodeSafe) {
        doc.clear();
        JsonObject root = doc.to<JsonObject>();
        root["jsonrpc"] = "2.0";
#if RPC_ENABLE_SAFE_MODE
        if (encodeSafe) {
            RpcSafe::encodeObjectMember(result.as<JsonVariantConst>(), root, "result");
        } else
#endif
        {
            root["result"].set(result);
        }
        root["id"].set(id);
        _hasError = false;
        _isValid = true;
    }

    // Error response
    void setError(int code, const char* message, JsonVariant id) {
        doc.clear();
        JsonObject root = doc.to<JsonObject>();
        root["jsonrpc"] = "2.0";
        JsonObject error = root.createNestedObject("error");
        error["code"] = code;
        error["message"] = message;
        root["id"].set(id);
        _hasError = true;
        _isValid = true;
    }

    void setError(int code, const String& message, JsonVariant id) {
        doc.clear();
        JsonObject root = doc.to<JsonObject>();
        root["jsonrpc"] = "2.0";
        JsonObject error = root.createNestedObject("error");
        error["code"] = code;
        error["message"] = message;
        root["id"].set(id);
        _hasError = true;
        _isValid = true;
    }

    void setError(int code, const char* message, JsonVariantConst data, JsonVariant id, bool encodeSafe = false) {
        doc.clear();
        JsonObject root = doc.to<JsonObject>();
        root["jsonrpc"] = "2.0";
        JsonObject error = root.createNestedObject("error");
        error["code"] = code;
        error["message"] = message;
        if (!data.isNull()) {
#if RPC_ENABLE_SAFE_MODE
            if (encodeSafe) {
                RpcSafe::encodeObjectMember(data, error, "data");
            } else
#endif
            {
                error["data"].set(data);
            }
        }
        root["id"].set(id);
        _hasError = true;
        _isValid = true;
    }

    void setError(int code, const String& message, JsonVariantConst data, JsonVariant id, bool encodeSafe = false) {
        doc.clear();
        JsonObject root = doc.to<JsonObject>();
        root["jsonrpc"] = "2.0";
        JsonObject error = root.createNestedObject("error");
        error["code"] = code;
        error["message"] = message;
        if (!data.isNull()) {
#if RPC_ENABLE_SAFE_MODE
            if (encodeSafe) {
                RpcSafe::encodeObjectMember(data, error, "data");
            } else
#endif
            {
                error["data"].set(data);
            }
        }
        root["id"].set(id);
        _hasError = true;
        _isValid = true;
    }

    // Parse from JSON string
    bool parse(const String& json) {
        return parse(json, false);
    }

    bool parse(const String& json, bool decodeSafe) {
        DeserializationError error = deserializeJson(doc, json);
        if (error) {
            RPC_LOG_F("Failed to parse response: %s", error.c_str());
            _isValid = false;
            return false;
        }

#if RPC_ENABLE_SAFE_MODE
        if (decodeSafe) {
            if (doc.containsKey("result")) {
                RpcSafe::decodeInPlace(doc["result"]);
            }
            if (doc.containsKey("error") && doc["error"].containsKey("data")) {
                RpcSafe::decodeInPlace(doc["error"]["data"]);
            }
        }
#else
        (void)decodeSafe;
#endif

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

    // Get result as read-only JsonVariant
    JsonVariantConst result() const {
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

    JsonVariantConst errorData() const {
        if (!_hasError) return JsonVariantConst();
        return doc["error"]["data"];
    }

    // Get ID
    JsonVariantConst id() const {
        return doc["id"];
    }
};

// Handler that can return a complete JSON-RPC response, including errors.
typedef std::function<RpcResponse(RpcRequest&, bool)> RpcResponseHandler;

// ============================================================================
// RPC Batch Response
// ============================================================================

class RpcBatchResponse {
private:
#if defined(ESP8266)
    DynamicJsonDocument doc;
#else
    StaticJsonDocument<RPC_JSON_DOC_SIZE> doc;
#endif
    bool _isValid;
    bool _isBatch;

    JsonVariantConst itemAt(size_t index) const {
        if (!_isValid) {
            return JsonVariantConst();
        }

        if (_isBatch) {
            JsonArrayConst arr = doc.as<JsonArrayConst>();
            return arr[index];
        }

        return index == 0 ? doc.as<JsonVariantConst>() : JsonVariantConst();
    }

public:
    RpcBatchResponse()
#if defined(ESP8266)
        : doc(RPC_JSON_DOC_SIZE), _isValid(false), _isBatch(false) {}
#else
        : _isValid(false), _isBatch(false) {}
#endif

    // Parse from JSON string. Valid batch responses are arrays; a single
    // JSON-RPC error object is accepted for invalid batch requests.
    bool parse(const String& json) {
        return parse(json, false);
    }

    bool parse(const String& json, bool decodeSafe) {
        DeserializationError error = deserializeJson(doc, json);
        if (error) {
            RPC_LOG_F("Failed to parse batch response: %s", error.c_str());
            _isValid = false;
            _isBatch = false;
            return false;
        }

#if RPC_ENABLE_SAFE_MODE
        if (decodeSafe) {
            if (doc.is<JsonArray>()) {
                for (JsonVariant item : doc.as<JsonArray>()) {
                    if (item.containsKey("result")) {
                        RpcSafe::decodeInPlace(item["result"]);
                    }
                    if (item.containsKey("error") && item["error"].containsKey("data")) {
                        RpcSafe::decodeInPlace(item["error"]["data"]);
                    }
                }
            } else {
                if (doc.containsKey("result")) {
                    RpcSafe::decodeInPlace(doc["result"]);
                }
                if (doc.containsKey("error") && doc["error"].containsKey("data")) {
                    RpcSafe::decodeInPlace(doc["error"]["data"]);
                }
            }
        }
#else
        (void)decodeSafe;
#endif

        if (doc.is<JsonArray>()) {
            _isValid = true;
            _isBatch = true;
            return true;
        }

        if (doc.is<JsonObject>() && doc["jsonrpc"] == "2.0" && doc.containsKey("error")) {
            _isValid = true;
            _isBatch = false;
            return true;
        }

        _isValid = false;
        _isBatch = false;
        return false;
    }

    // Error response for local transport failures/timeouts.
    void setError(int code, const char* message) {
        doc.clear();
        doc["jsonrpc"] = "2.0";
        JsonObject error = doc.createNestedObject("error");
        error["code"] = code;
        error["message"] = message;
        doc["id"] = JsonVariant();
        _isValid = true;
        _isBatch = false;
    }

    String toString() const {
        String output;
        serializeJson(doc, output);
        return output;
    }

    bool isValid() const { return _isValid; }
    bool isBatch() const { return _isValid && _isBatch; }
    bool isSuccess() const {
        if (!_isValid || count() == 0) {
            return false;
        }

        for (size_t i = 0; i < count(); ++i) {
            if (hasError(i)) {
                return false;
            }
        }
        return true;
    }

    size_t count() const {
        if (!_isValid) {
            return 0;
        }

        if (_isBatch) {
            JsonArrayConst arr = doc.as<JsonArrayConst>();
            return arr.size();
        }

        return 1;
    }

    JsonVariantConst response(size_t index) const {
        return itemAt(index);
    }

    JsonVariantConst result(size_t index) const {
        JsonVariantConst item = itemAt(index);
        return item["result"];
    }

    bool hasError(size_t index) const {
        JsonVariantConst item = itemAt(index);
        return !item.isNull() && item.containsKey("error");
    }

    int errorCode(size_t index) const {
        JsonVariantConst item = itemAt(index);
        if (item.isNull() || !item.containsKey("error")) return 0;
        return item["error"]["code"] | 0;
    }

    String errorMessage(size_t index) const {
        JsonVariantConst item = itemAt(index);
        if (item.isNull() || !item.containsKey("error")) return "";
        return item["error"]["message"].as<String>();
    }

    JsonVariantConst id(size_t index) const {
        JsonVariantConst item = itemAt(index);
        return item["id"];
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

    static RpcResponse compatibilityError(JsonVariant id) {
        RpcResponse resp;
        resp.setError(
            RPC_ERROR_INVALID_REQ,
            "RPC Compatibility Error: Server requires safe serialization header but client did not provide it.",
            id
        );
        return resp;
    }

    static RpcResponse methodNotFound(const char* method, JsonVariant id) {
        RpcResponse resp;
        String msg = "Method not found: ";
        msg += method;
        resp.setError(RPC_ERROR_METHOD_NOT_FOUND, msg, id);
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

    static RpcResponse custom(int code, const char* message, JsonVariantConst data, JsonVariant id, bool encodeSafe = false) {
        RpcResponse resp;
        resp.setError(code, message, data, id, encodeSafe);
        return resp;
    }

    static RpcResponse custom(int code, const String& message, JsonVariantConst data, JsonVariant id, bool encodeSafe = false) {
        RpcResponse resp;
        resp.setError(code, message, data, id, encodeSafe);
        return resp;
    }
};

#endif // RPC_TYPES_H
