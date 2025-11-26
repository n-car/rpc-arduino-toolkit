/**
 * RPC Arduino Toolkit - Transport Layer
 * 
 * Abstract base class for different transport mechanisms
 */

#ifndef RPC_TRANSPORT_H
#define RPC_TRANSPORT_H

#include <Arduino.h>
#include "RpcConfig.h"

// ============================================================================
// RPC Transport Base Class
// ============================================================================

class RpcTransport {
public:
    virtual ~RpcTransport() {}
    
    /**
     * Read data from transport
     * @return JSON string or empty if no data
     */
    virtual String read() = 0;
    
    /**
     * Write data to transport
     * @param data JSON string to send
     * @return true if successful
     */
    virtual bool write(const String& data) = 0;
    
    /**
     * Check if transport is available/connected
     * @return true if ready
     */
    virtual bool available() = 0;
    
    /**
     * Set timeout for read operations
     * @param ms timeout in milliseconds
     */
    virtual void setTimeout(unsigned long ms) {
        timeout = ms;
    }
    
protected:
    unsigned long timeout = RPC_DEFAULT_TIMEOUT;
};

#endif // RPC_TRANSPORT_H
