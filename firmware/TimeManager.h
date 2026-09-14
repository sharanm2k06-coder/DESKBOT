#pragma once
#include <Arduino.h>
#include "config.h"
#include "DeviceState.h"

// Syncs time via NTP whenever WiFi is connected, and otherwise keeps
// advancing from the last known good sync using millis() deltas — so the
// clock never just freezes or blanks when WiFi drops.
class TimeManager {
 public:
  void begin();
  void update(DeviceState &state, bool wifiConnected); // call every loop()

  // Formats into caller-provided buffers; safe to call even if never synced
  // (returns placeholder strings in that case).
  void getTimeString(char* buf, size_t bufLen, bool use24h = false) const;
  void getDateString(char* buf, size_t bufLen) const;

 private:
  uint32_t _lastSyncAttemptMs = 0;
  bool _everSynced = false;
};
