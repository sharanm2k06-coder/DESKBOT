#pragma once
#include <Arduino.h>
#include "config.h"
#include "DeviceState.h"

// TTP223 wired in its default mode: output is HIGH while touched, LOW when
// idle. This manager debounces the raw signal and classifies each release
// as either a SHORT_PRESS or LONG_PRESS, delivered via update()'s return
// value — never via delay().
class TouchManager {
 public:
  void begin();
  TouchEvent update(); // call every loop(); returns NONE most of the time

 private:
  bool _debouncedState = false;   // true = currently considered "touched"
  bool _rawLast = false;
  uint32_t _lastChangeMs = 0;
  uint32_t _pressStartMs = 0;
  bool _longPressFired = false;
};
