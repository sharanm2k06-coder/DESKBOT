#pragma once
#include <Arduino.h>
#include "config.h"
#include "DeviceState.h"

// Wraps WiFi.h with a non-blocking connect/retry state machine. DeskBot
// must remain fully usable (clock/sensor/OLED/BLE/touch) whether or not
// this ever succeeds.
class WiFiManagerNB {
 public:
  void begin();
  void update(DeviceState &state);  // call every loop()
  bool isConnected() const;

 private:
  enum class Phase : uint8_t { IDLE, CONNECTING, CONNECTED, WAITING_RETRY };
  Phase _phase = Phase::IDLE;
  uint32_t _phaseStartMs = 0;
};
