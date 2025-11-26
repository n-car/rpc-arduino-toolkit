/**
 * RPC Arduino Toolkit - Configuration
 * 
 * Adjust these settings based on your platform and requirements
 */

#ifndef RPC_CONFIG_H
#define RPC_CONFIG_H

// ============================================================================
// Memory Limits
// ============================================================================

// Maximum number of methods (adjust based on available RAM)
#ifndef RPC_MAX_METHODS
  #if defined(ESP32)
    #define RPC_MAX_METHODS 16
  #elif defined(ESP8266)
    #define RPC_MAX_METHODS 12
  #else
    #define RPC_MAX_METHODS 8
  #endif
#endif

// Maximum request/response size (bytes)
#ifndef RPC_MAX_REQUEST_SIZE
  #if defined(ESP32)
    #define RPC_MAX_REQUEST_SIZE 2048
  #elif defined(ESP8266)
    #define RPC_MAX_REQUEST_SIZE 1024
  #else
    #define RPC_MAX_REQUEST_SIZE 512
  #endif
#endif

#ifndef RPC_MAX_RESPONSE_SIZE
  #define RPC_MAX_RESPONSE_SIZE RPC_MAX_REQUEST_SIZE
#endif

// Maximum method name length
#ifndef RPC_MAX_METHOD_NAME
  #define RPC_MAX_METHOD_NAME 32
#endif

// ============================================================================
// Features (1 = enabled, 0 = disabled)
// ============================================================================

// Enable safe serialization (S: for strings, D: for dates)
#ifndef RPC_ENABLE_SAFE_MODE
  #define RPC_ENABLE_SAFE_MODE 0  // Disabled by default to save memory
#endif

// Enable batch requests
#ifndef RPC_ENABLE_BATCH
  #define RPC_ENABLE_BATCH 1
#endif

// Enable debug logging to Serial
#ifndef RPC_ENABLE_LOGGING
  #define RPC_ENABLE_LOGGING 0
#endif

// Enable method notifications (fire-and-forget)
#ifndef RPC_ENABLE_NOTIFICATIONS
  #define RPC_ENABLE_NOTIFICATIONS 1
#endif

// Enable schema support (adds description and exposeSchema per method)
#ifndef RPC_ENABLE_SCHEMA_SUPPORT
  #define RPC_ENABLE_SCHEMA_SUPPORT 1  // Enabled by default (minimal overhead)
#endif

// Maximum description length (if schema support enabled)
#ifndef RPC_MAX_DESCRIPTION
  #define RPC_MAX_DESCRIPTION 64
#endif

// ============================================================================
// Timeouts (milliseconds)
// ============================================================================

// Default client timeout
#ifndef RPC_DEFAULT_TIMEOUT
  #define RPC_DEFAULT_TIMEOUT 5000
#endif

// Serial read timeout
#ifndef RPC_SERIAL_TIMEOUT
  #define RPC_SERIAL_TIMEOUT 1000
#endif

// WiFi connection timeout
#ifndef RPC_WIFI_TIMEOUT
  #define RPC_WIFI_TIMEOUT 10000
#endif

// ============================================================================
// ArduinoJson Configuration
// ============================================================================

// Use StaticJsonDocument for better performance and predictable memory
// Size based on RPC_MAX_REQUEST_SIZE
#define RPC_JSON_DOC_SIZE (RPC_MAX_REQUEST_SIZE + 256)

// ============================================================================
// Logging Macros
// ============================================================================

#if RPC_ENABLE_LOGGING
  #define RPC_LOG(msg) Serial.print("[RPC] "); Serial.println(msg)
  #define RPC_LOG_F(fmt, ...) Serial.printf("[RPC] " fmt "\n", ##__VA_ARGS__)
#else
  #define RPC_LOG(msg)
  #define RPC_LOG_F(fmt, ...)
#endif

// ============================================================================
// Error Codes (JSON-RPC 2.0 standard)
// ============================================================================

#define RPC_ERROR_PARSE         -32700  // Parse error
#define RPC_ERROR_INVALID_REQ   -32600  // Invalid Request
#define RPC_ERROR_METHOD_NOT_FOUND -32601  // Method not found
#define RPC_ERROR_INVALID_PARAMS -32602  // Invalid params
#define RPC_ERROR_INTERNAL      -32603  // Internal error
#define RPC_ERROR_SERVER        -32000  // Server error

// ============================================================================
// Platform Detection
// ============================================================================

// Detect platform capabilities
#if defined(ESP32)
  #define RPC_HAS_WIFI 1
  #define RPC_HAS_BLE 1
  #define RPC_HAS_SPIFFS 1
#elif defined(ESP8266)
  #define RPC_HAS_WIFI 1
  #define RPC_HAS_BLE 0
  #define RPC_HAS_SPIFFS 1
#else
  #define RPC_HAS_WIFI 0
  #define RPC_HAS_BLE 0
  #define RPC_HAS_SPIFFS 0
#endif

#endif // RPC_CONFIG_H
