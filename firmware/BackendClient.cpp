#include "BackendClient.h"
#include "Logger.h"

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string.h>

namespace {

bool isPlaceholder(const char* value) {
  return strstr(value, "REPLACE_WITH_") != nullptr;
}

// Shared per-request setup: fresh TLS client + HTTPClient, common timeout.
// A new WiFiClientSecure per call is simpler than pooling one connection and
// is cheap enough at BACKEND_POLL_INTERVAL_MS-scale request rates; pooling
// would need its own reconnect/staleness handling for little benefit here.
void beginRequest(HTTPClient &http, WiFiClientSecure &tls, const String &url) {
  tls.setInsecure();  // see config.h TLS note
  http.begin(tls, url);
  http.setTimeout(BACKEND_HTTP_TIMEOUT_MS);
}

}  // namespace

void BackendClient::begin(MessageManager* messages) {
  _messages = messages;
  _lastPollMs = 0;
  _lastHeartbeatMs = 0;
  _configured = !isPlaceholder(BACKEND_DEVICE_ID) && !isPlaceholder(BACKEND_DEVICE_KEY);
  if (!_configured) {
    LOGLN("[BackendClient] BACKEND_DEVICE_ID/KEY still placeholders in config.h -- "
          "backend polling/heartbeat disabled until configured");
  } else {
    LOGLN("[BackendClient] ready");
  }
}

void BackendClient::update(DeviceState &state, bool wifiConnected) {
  if (!_configured || !wifiConnected) return;

  uint32_t now = millis();

  if (now - _lastPollMs >= BACKEND_POLL_INTERVAL_MS) {
    _lastPollMs = now;
    pollPendingMessage();
  }

  if (now - _lastHeartbeatMs >= BACKEND_HEARTBEAT_INTERVAL_MS) {
    _lastHeartbeatMs = now;
    sendHeartbeat(state);
  }
}

MessageSource BackendClient::parseSource(const char* s) {
  if (!s) return MessageSource::SYSTEM;
  if (strcmp(s, "APP") == 0) return MessageSource::APP;
  if (strcmp(s, "WHATSAPP") == 0) return MessageSource::WHATSAPP;
  if (strcmp(s, "SCHEDULED") == 0) return MessageSource::SCHEDULED;
  return MessageSource::SYSTEM;
}

MessagePriority BackendClient::parsePriority(const char* s) {
  if (!s) return MessagePriority::NORMAL;
  if (strcmp(s, "IMPORTANT") == 0) return MessagePriority::IMPORTANT;
  if (strcmp(s, "ALERT") == 0) return MessagePriority::ALERT;
  return MessagePriority::NORMAL;
}

void BackendClient::pollPendingMessage() {
  String url = String(BACKEND_BASE_URL) + "/api/v1/devices/" + BACKEND_DEVICE_ID + "/messages/pending";

  WiFiClientSecure tls;
  HTTPClient http;
  beginRequest(http, tls, url);
  http.addHeader("X-Device-Key", BACKEND_DEVICE_KEY);

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    if (code > 0) {
      LOGF("[BackendClient] pending-poll HTTP %d\n", code);
    } else {
      LOGF("[BackendClient] pending-poll failed: %s\n", http.errorToString(code).c_str());
    }
    http.end();
    return;
  }

  String body = http.getString();
  http.end();

  if (body.length() == 0 || body == "null") {
    return;  // nothing pending -- the normal case, per API contract
  }

  StaticJsonDocument<768> doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    LOGF("[BackendClient] pending-poll JSON parse failed: %s\n", err.c_str());
    return;
  }

  const char* messageId = doc["id"] | "";
  const char* source    = doc["source"] | "SYSTEM";
  const char* priority  = doc["priority"] | "NORMAL";
  const char* sender    = doc["sender"] | "";
  const char* title     = doc["title"] | "";
  const char* text      = doc["body"] | "";

  if (strlen(messageId) == 0) {
    LOGLN("[BackendClient] pending-poll response missing id, ignoring");
    return;
  }

  uint32_t localId = _messages->enqueue(
      parseSource(source), parsePriority(priority), sender, title, text);
  if (localId == 0) {
    LOGLN("[BackendClient] local queue full, dropped backend message "
          "(already marked DELIVERED server-side; acking anyway)");
  } else {
    LOGF("[BackendClient] enqueued backend message -> local id=%lu\n", (unsigned long)localId);
  }

  // Ack regardless of local enqueue outcome: get_pending_for_device() only
  // ever serves PENDING messages, and this one is already DELIVERED
  // server-side the moment the GET above returned it, so there is nothing
  // to preserve by withholding the ack (see class-level comment).
  String ackUrl = String(BACKEND_BASE_URL) + "/api/v1/messages/" + messageId + "/ack";
  WiFiClientSecure ackTls;
  HTTPClient ackHttp;
  beginRequest(ackHttp, ackTls, ackUrl);
  ackHttp.addHeader("X-Device-Id", BACKEND_DEVICE_ID);
  ackHttp.addHeader("X-Device-Key", BACKEND_DEVICE_KEY);
  ackHttp.addHeader("Content-Type", "application/json");

  int ackCode = ackHttp.POST("{}");
  if (ackCode != HTTP_CODE_OK) {
    LOGF("[BackendClient] ack HTTP %d for message %s\n", ackCode, messageId);
  }
  ackHttp.end();
}

void BackendClient::sendHeartbeat(DeviceState &state) {
  StaticJsonDocument<256> doc;
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["wifi_connected"] = (state.wifiState == WifiState::CONNECTED);
  doc["ble_connected"] = (state.bleState == BleState::CONNECTED);
  if (state.lastReading.valid) {
    doc["temperature_c"] = state.lastReading.temperatureC;
    doc["humidity_pct"] = state.lastReading.humidityPct;
  }

  String payload;
  serializeJson(doc, payload);

  String url = String(BACKEND_BASE_URL) + "/api/v1/devices/" + BACKEND_DEVICE_ID + "/heartbeat";
  WiFiClientSecure tls;
  HTTPClient http;
  beginRequest(http, tls, url);
  http.addHeader("X-Device-Key", BACKEND_DEVICE_KEY);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(payload);
  if (code != HTTP_CODE_OK) {
    if (code > 0) {
      LOGF("[BackendClient] heartbeat HTTP %d\n", code);
    } else {
      LOGF("[BackendClient] heartbeat failed: %s\n", http.errorToString(code).c_str());
    }
  } else {
    LOGLN("[BackendClient] heartbeat ok");
  }
  http.end();
}
