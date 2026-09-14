#pragma once
#include <Arduino.h>
#include "config.h"
#include "DeviceState.h"

// Non-blocking DHT22 sampling. DHT22 itself has a blocking read() call
// internally (~2ms typical, occasionally longer) — we bound how often we
// call it via a timer so it never delays the main loop noticeably, and we
// isolate failures so they never propagate.
class SensorManager {
 public:
  void begin();
  void update(DeviceState &state);   // call every loop()

 private:
  uint32_t _lastReadMs = 0;
  void* _dht = nullptr; // opaque pointer to avoid forcing DHT.h into this header
};
