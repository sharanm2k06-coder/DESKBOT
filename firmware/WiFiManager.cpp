#include "WiFiManager.h"
#include "Logger.h"
#include <WiFi.h>

void WiFiManagerNB::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // clear any stale state from a previous run
  _phase = Phase::IDLE;
  _phaseStartMs = millis();
  LOGLN("[WiFiManager] ready (not yet connecting)");
}

bool WiFiManagerNB::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManagerNB::update(DeviceState &state) {
  uint32_t now = millis();

  switch (_phase) {
    case Phase::IDLE:
      LOGF("[WiFiManager] connecting to \"%s\"\n", WIFI_SSID);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      _phase = Phase::CONNECTING;
      _phaseStartMs = now;
      state.wifiState = WifiState::CONNECTING;
      break;

    case Phase::CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        LOGF("[WiFiManager] connected, IP=%s\n", WiFi.localIP().toString().c_str());
        _phase = Phase::CONNECTED;
        state.wifiState = WifiState::CONNECTED;
      } else if (now - _phaseStartMs >= WIFI_CONNECT_TIMEOUT_MS) {
        LOGLN("[WiFiManager] connect timed out, will retry later");
        WiFi.disconnect();
        _phase = Phase::WAITING_RETRY;
        _phaseStartMs = now;
        state.wifiState = WifiState::FAILED;
      }
      break;

    case Phase::CONNECTED:
      if (WiFi.status() != WL_CONNECTED) {
        LOGLN("[WiFiManager] connection lost");
        _phase = Phase::WAITING_RETRY;
        _phaseStartMs = now;
        state.wifiState = WifiState::DISCONNECTED;
      }
      break;

    case Phase::WAITING_RETRY:
      if (now - _phaseStartMs >= WIFI_RETRY_INTERVAL_MS) {
        _phase = Phase::IDLE; // triggers a fresh WiFi.begin() next tick
      }
      break;
  }
}
