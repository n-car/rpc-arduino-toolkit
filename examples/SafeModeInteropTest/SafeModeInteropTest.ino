/**
 * RPC Arduino Toolkit - Safe Mode Interoperability Test
 *
 * Compile with:
 *   -DRPC_ENABLE_SAFE_MODE=1
 *   -DRPC_SAFE_STRICT_MODE=1
 *
 * This sketch verifies that automatic Safe Mode encoding is based on the
 * ArduinoJson source datatype. Every JSON string is encoded with S:, even
 * when the string already looks like a Safe Mode marker.
 */

#include <string.h>
#include <RpcTypes.h>

#if !RPC_ENABLE_SAFE_MODE
#error "SafeModeInteropTest requires RPC_ENABLE_SAFE_MODE=1"
#endif

StaticJsonDocument<1024> sourceDoc;
StaticJsonDocument<1024> encodedDoc;
StaticJsonDocument<1024> decodedDoc;

bool expectString(JsonVariantConst value, const char* expected, const char* label) {
    const char* actual = value.as<const char*>();
    if (actual == nullptr || strcmp(actual, expected) != 0) {
        Serial.print("FAIL ");
        Serial.print(label);
        Serial.print(": expected ");
        Serial.print(expected);
        Serial.print(", got ");
        Serial.println(actual == nullptr ? "<null>" : actual);
        return false;
    }

    return true;
}

bool runEncodingChecks() {
    sourceDoc.clear();
    JsonObject root = sourceDoc.to<JsonObject>();
    root["plain"] = "hello";
    root["safePrefix"] = "S:literal";
    root["datePrefix"] = "D:literal";
    root["isoDateString"] = "2026-06-10T12:34:56.000Z";
    root["bigintLikeString"] = "9007199254740993n";
    root["helperStringOutput"] = RpcSafe::serializeString("manual");
    root["helperDateOutput"] = RpcSafe::serializeDate(1234);

    JsonObject nested = root.createNestedObject("nested");
    nested["value"] = "S:nested";
    JsonArray values = nested.createNestedArray("values");
    values.add("hello");
    values.add("D:literal");
    values.add("9007199254740993n");
    JsonObject deep = values.createNestedObject();
    deep["value"] = "S:deep";

    encodedDoc.clear();
    RpcSafe::encodeValue(sourceDoc.as<JsonVariantConst>(), encodedDoc.to<JsonVariant>());

    bool ok = true;
    ok = expectString(encodedDoc["plain"], "S:hello", "encode plain") && ok;
    ok = expectString(encodedDoc["safePrefix"], "S:S:literal", "encode S literal") && ok;
    ok = expectString(encodedDoc["datePrefix"], "S:D:literal", "encode D literal") && ok;
    ok = expectString(
        encodedDoc["isoDateString"],
        "S:2026-06-10T12:34:56.000Z",
        "encode ISO date string"
    ) && ok;
    ok = expectString(
        encodedDoc["bigintLikeString"],
        "S:9007199254740993n",
        "encode bigint-looking string"
    ) && ok;
    ok = expectString(encodedDoc["helperStringOutput"], "S:S:manual", "encode helper string output") && ok;
    ok = expectString(encodedDoc["helperDateOutput"], "S:D:1234", "encode helper date output") && ok;
    ok = expectString(encodedDoc["nested"]["value"], "S:S:nested", "encode nested S literal") && ok;
    ok = expectString(encodedDoc["nested"]["values"][0], "S:hello", "encode nested array plain") && ok;
    ok = expectString(encodedDoc["nested"]["values"][1], "S:D:literal", "encode nested array D literal") && ok;
    ok = expectString(
        encodedDoc["nested"]["values"][2],
        "S:9007199254740993n",
        "encode nested array bigint-looking string"
    ) && ok;
    ok = expectString(encodedDoc["nested"]["values"][3]["value"], "S:S:deep", "encode deep S literal") && ok;

    return ok;
}

bool runDecodingChecks() {
    decodedDoc.clear();
    JsonObject root = decodedDoc.to<JsonObject>();
    root["plain"] = "S:hello";
    root["safePrefix"] = "S:S:literal";
    root["datePrefix"] = "S:D:literal";
    root["bigintLikeString"] = "S:9007199254740993n";
    root["dateMarker"] = "D:2026-06-10T12:34:56.000Z";
    root["bigintMarker"] = "9007199254740993n";
    root["negativeBigintMarker"] = "-9007199254740993n";

    JsonObject nested = root.createNestedObject("nested");
    JsonArray values = nested.createNestedArray("values");
    values.add("S:S:nested");
    values.add("S:D:nested");
    values.add("S:9007199254740994n");
    values.add("D:2026-06-10T12:34:56.000Z");
    values.add("9007199254740994n");

    RpcSafe::decodeInPlace(decodedDoc.as<JsonVariant>());

    bool ok = true;
    ok = expectString(decodedDoc["plain"], "hello", "decode plain") && ok;
    ok = expectString(decodedDoc["safePrefix"], "S:literal", "decode S literal") && ok;
    ok = expectString(decodedDoc["datePrefix"], "D:literal", "decode D literal") && ok;
    ok = expectString(
        decodedDoc["bigintLikeString"],
        "9007199254740993n",
        "decode S-prefixed bigint-looking string"
    ) && ok;
    ok = expectString(
        decodedDoc["dateMarker"],
        "2026-06-10T12:34:56.000Z",
        "decode date marker"
    ) && ok;
    ok = expectString(decodedDoc["bigintMarker"], "9007199254740993n", "decode bigint marker") && ok;
    ok = expectString(
        decodedDoc["negativeBigintMarker"],
        "-9007199254740993n",
        "decode negative bigint marker"
    ) && ok;
    ok = expectString(decodedDoc["nested"]["values"][0], "S:nested", "decode nested S literal") && ok;
    ok = expectString(decodedDoc["nested"]["values"][1], "D:nested", "decode nested D literal") && ok;
    ok = expectString(
        decodedDoc["nested"]["values"][2],
        "9007199254740994n",
        "decode nested S-prefixed bigint-looking string"
    ) && ok;
    ok = expectString(
        decodedDoc["nested"]["values"][3],
        "2026-06-10T12:34:56.000Z",
        "decode nested date marker"
    ) && ok;
    ok = expectString(decodedDoc["nested"]["values"][4], "9007199254740994n", "decode nested bigint marker") && ok;

    return ok;
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    bool ok = runEncodingChecks();
    ok = runDecodingChecks() && ok;

    Serial.println(ok ? "SafeModeInteropTest PASS" : "SafeModeInteropTest FAIL");

    if (!ok) {
        while (true) {
            delay(1000);
        }
    }
}

void loop() {
}
