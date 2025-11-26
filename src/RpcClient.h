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
    
    // Build request JSON
    String buildRequest(const char* method, const String& params, bool isNotification = false) {
        StaticJsonDocument<RPC_JSON_DOC_SIZE> doc;
        
        doc["jsonrpc"] = "2.0";
        doc["method"] = method;
        
        // Parse params if provided
        if (!params.isEmpty()) {
            if (params[0] == '{' || params[0] == '[') {
                // JSON params
                StaticJsonDocument<512> paramsDoc;
                deserializeJson(paramsDoc, params);
                doc["params"] = paramsDoc;
            } else {
                // String param (single value)
                doc["params"] = params;
            }
        }
        
        // Add ID unless notification
        if (!isNotification) {
            doc["id"] = requestId++;
        }
        
        String output;
        serializeJson(doc, output);
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
            resp.setError(RPC_ERROR_SERVER, "Failed to send request", nullptr);
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
        resp.setError(RPC_ERROR_SERVER, "Request timeout", nullptr);
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
