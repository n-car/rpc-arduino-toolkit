// Self-contained regression sketch: fake sockets only, no WiFi or peripheral I/O.
// Build with RPC_ENABLE_SAFE_MODE=0 and 1 to cover both protocol modes.
// Monitor serial at 115200; completion is RPC_PROTOCOL PASS or FAIL, followed
// by periodic heartbeats. Checks remain active in release/NDEBUG builds.
#include <Arduino.h>
#include <ArduinoJson.h>
#include <RpcClient.h>
#include <RpcHttpClientTransport.h>
#include <RpcServer.h>

unsigned int rpcChecks = 0;
unsigned int rpcStages = 0;
bool rpcFailed = false;
const char* rpcStage = "startup";

bool rpcCheck(bool condition, const char* expression, unsigned int line) {
    ++rpcChecks;
    if (condition) return true;
    rpcFailed = true;
    Serial.printf("RPC_PROTOCOL CHECK_FAIL stage=%s check=%u line=%u expression=%s\n",
                  rpcStage, rpcChecks, line, expression);
    Serial.flush();
    return false;
}

// Returning avoids unsafe follow-up assertions after a parse/transport failure.
// Do not replace this with assert(): NDEBUG disables checks and may also
// remove test operations, depending on the Arduino core's assert definition.
#define RPC_CHECK(expression) do { \
    if (!rpcCheck(static_cast<bool>(expression), #expression, __LINE__)) return; \
} while (0)

class FakeHttpSocket : public Client {
    String response;
    size_t position = 0;
    bool open = false;
public:
    explicit FakeHttpSocket(const char* value) : response(value) {}
    int connect(IPAddress, uint16_t) override { position = 0; open = true; return 1; }
    int connect(const char*, uint16_t) override { position = 0; open = true; return 1; }
    size_t write(uint8_t) override { return 1; }
    size_t write(const uint8_t*, size_t size) override { return size; }
    int available() override { return open ? response.length() - position : 0; }
    int read() override { return available() ? response[position++] : -1; }
    int read(uint8_t* buffer, size_t size) override {
        size_t count = 0;
        while (count < size && available()) buffer[count++] = read();
        return count;
    }
    int peek() override { return available() ? response[position] : -1; }
    void flush() override {}
    void stop() override { open = false; }
    uint8_t connected() override { return open && position < response.length(); }
    operator bool() override { return open; }
};

class FakeTransport : public RpcTransport {
public:
    String input;
    bool http;
    explicit FakeTransport(const String& value = "", bool isHttp = false) : input(value), http(isHttp) {}
    String read() override { return input; }
    bool write(const String&) override { return true; }
    bool available() override { return false; }
    bool isHttp() const override { return http; }
};

void testServerEnvelope() {
    RpcServer<1> server;
    int calls = 0;
    server.addResponseMethod("echo", [&calls](RpcRequest& request, bool encodeSafe) {
        ++calls;
        RpcResponse response;
        DynamicJsonDocument result(256);
        result.set(request.params);
        response.setResult(result.as<JsonVariant>(), request.id, encodeSafe);
        return response;
    });

    const char* invalid[] = {
        "{}", "null", "1", "{\"method\":\"echo\"}",
        "{\"jsonrpc\":\"1.0\",\"method\":\"echo\"}",
        "{\"jsonrpc\":\"2.0\",\"method\":true}",
        "{\"jsonrpc\":\"2.0\",\"method\":\"echo\",\"id\":true}",
        "{\"jsonrpc\":\"2.0\",\"method\":\"echo\",\"id\":{}}",
        "{\"jsonrpc\":\"2.0\",\"method\":\"echo\",\"id\":[]}",
        "{\"jsonrpc\":\"2.0\",\"method\":\"echo\",\"params\":null}",
        "{\"jsonrpc\":\"2.0\",\"method\":\"echo\",\"params\":true}",
        "{\"jsonrpc\":\"2.0\",\"method\":\"echo\",\"params\":1,\"id\":7}"
    };
    DynamicJsonDocument parsed(2048);
    for (const char* request : invalid) {
        RPC_CHECK(!deserializeJson(parsed, server.handleRequest(request)));
        RPC_CHECK(parsed["error"]["code"] == -32600);
        RPC_CHECK(parsed.containsKey("id") && parsed["id"].isNull());
    }
    RPC_CHECK(calls == 0);
    RPC_CHECK(!deserializeJson(parsed, server.handleRequest(
        "{\"jsonrpc\":\"2.0\",\"method\":\"echo\",\"params\":{},\"id\":null}")));
    RPC_CHECK(parsed.containsKey("result") && parsed["result"].is<JsonObject>());
    RPC_CHECK(parsed.containsKey("id") && parsed["id"].isNull());
    RPC_CHECK(calls == 1);
    RPC_CHECK(server.handleRequest("{\"jsonrpc\":\"2.0\",\"method\":\"echo\"}").isEmpty());
    RPC_CHECK(calls == 2);
    RPC_CHECK(!deserializeJson(parsed, server.handleRequest(
        "[{}, {\"jsonrpc\":\"2.0\",\"method\":\"echo\"},"
        "{\"jsonrpc\":\"2.0\",\"method\":\"echo\",\"params\":[],\"id\":null}]")));
    RPC_CHECK(parsed.is<JsonArray>() && parsed.size() == 2);
    RPC_CHECK(parsed[0]["error"]["code"] == -32600);
    RPC_CHECK(parsed[1]["id"].isNull() && parsed[1]["result"].is<JsonArray>());
#if RPC_ENABLE_SAFE_MODE
    FakeTransport missingHeader("[{}, {\"jsonrpc\":\"2.0\",\"method\":\"echo\",\"id\":null},"
                                "{\"jsonrpc\":\"2.0\",\"method\":\"echo\"}]", true);
    RPC_CHECK(!deserializeJson(parsed, server.handleRequest(missingHeader)));
    RPC_CHECK(parsed.size() == 2);
    RPC_CHECK(parsed[0]["error"]["code"] == -32600 && parsed[1]["error"]["code"] == -32600);
#endif
}

void testNoContentBatch() {
    FakeHttpSocket socket("HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n");
    RpcHttpClientTransport transport(socket, "unused.invalid", 80, "/rpc");
    transport.setSocketSettleDelay(0);
    RpcClient client(transport);
    client.setTimeout(10);
    const char* notifications = "[{\"jsonrpc\":\"2.0\",\"method\":\"echo\"}]";
    RpcBatchResponse response = client.callBatch(notifications);
    RPC_CHECK(transport.lastStatus() == 204 && transport.lastError().isEmpty());
    RPC_CHECK(response.isValid() && response.isSuccess() && response.count() == 0);
    // A 204 must never hide missing responses to actual calls or invalid items.
    RPC_CHECK(!client.callBatch("[{\"jsonrpc\":\"2.0\",\"method\":\"echo\",\"id\":null}]").isSuccess());
    RPC_CHECK(!client.callBatch("[{}]").isSuccess());
    RPC_CHECK(!client.callBatch("[]").isSuccess());
    FakeTransport serial;
    RpcClient serialClient(serial);
    RPC_CHECK(serialClient.callBatch(notifications).isSuccess());
    // An empty 200 remains a transport error, not a successful notification ack.
    FakeHttpSocket emptySocket("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
    RpcHttpClientTransport emptyTransport(emptySocket, "unused.invalid", 80, "/rpc");
    emptyTransport.setSocketSettleDelay(0);
    RPC_CHECK(!emptyTransport.write(notifications));
}

void setup() {
    Serial.begin(115200);
    delay(500);
#if defined(NDEBUG)
    const unsigned int ndebug = 1;
#else
    const unsigned int ndebug = 0;
#endif
    Serial.printf("RPC_PROTOCOL BEGIN safe=%u ndebug=%u checks_always_enabled=1\n",
                  static_cast<unsigned int>(RPC_ENABLE_SAFE_MODE != 0), ndebug);
    rpcStage = "envelope";
    Serial.printf("RPC_PROTOCOL STAGE_BEGIN name=%s\n", rpcStage);
    testServerEnvelope();
    if (!rpcFailed) {
        ++rpcStages;
        Serial.printf("RPC_PROTOCOL STAGE_PASS name=%s checks=%u\n", rpcStage, rpcChecks);
        rpcStage = "no_content_batch";
        Serial.printf("RPC_PROTOCOL STAGE_BEGIN name=%s\n", rpcStage);
        testNoContentBatch();
        if (!rpcFailed) {
            ++rpcStages;
            Serial.printf("RPC_PROTOCOL STAGE_PASS name=%s checks=%u\n", rpcStage, rpcChecks);
        }
    }
    const unsigned int expectedChecks = RPC_ENABLE_SAFE_MODE ? 57 : 54;
    if (rpcChecks != expectedChecks || rpcStages != 2) rpcFailed = true;
    Serial.printf("RPC_PROTOCOL %s safe=%u checks=%u expected=%u stages=%u\n",
                  rpcFailed ? "FAIL" : "PASS", static_cast<unsigned int>(RPC_ENABLE_SAFE_MODE != 0),
                  rpcChecks, expectedChecks, rpcStages);
    Serial.flush();
}

void loop() {
    static unsigned long lastHeartbeat = 0;
    const unsigned long now = millis();
    if (now - lastHeartbeat >= 2000) {
        lastHeartbeat = now;
        Serial.printf("RPC_PROTOCOL HEARTBEAT status=%s safe=%u checks=%u uptime_ms=%lu\n",
                      rpcFailed ? "FAIL" : "PASS", static_cast<unsigned int>(RPC_ENABLE_SAFE_MODE != 0),
                      rpcChecks, now);
    }
    delay(10);
}
