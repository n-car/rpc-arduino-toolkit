/**
 * RPC Arduino Toolkit - Serial Transport
 * 
 * Transport over Serial/UART
 */

#ifndef RPC_SERIAL_TRANSPORT_H
#define RPC_SERIAL_TRANSPORT_H

#include "RpcTransport.h"

class RpcSerialTransport : public RpcTransport {
private:
    Stream& serial;
    char buffer[RPC_MAX_REQUEST_SIZE];
    
public:
    explicit RpcSerialTransport(Stream& s) : serial(s) {
        setTimeout(RPC_SERIAL_TIMEOUT);
    }
    
    String read() override {
        if (!serial.available()) {
            return "";
        }
        
        // Read until newline or timeout
        size_t len = serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
        buffer[len] = '\0';
        
        String result(buffer);
        result.trim();
        
        RPC_LOG_F("Serial RX: %s", result.c_str());
        return result;
    }
    
    bool write(const String& data) override {
        RPC_LOG_F("Serial TX: %s", data.c_str());
        
        serial.println(data);
        serial.flush();
        return true;
    }
    
    bool available() override {
        return serial.available() > 0;
    }
};

#endif // RPC_SERIAL_TRANSPORT_H
