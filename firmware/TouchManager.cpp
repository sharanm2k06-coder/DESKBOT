#include "TouchManager.h"
#include "Logger.h"

void TouchManager::begin() {
  pinMode(TTP223_PIN, INPUT);
  _debouncedState = false;
  _rawLast = false;
  _lastChangeMs = millis();
  _longPressFired = false;
  LOGLN("[TouchManager] ready");
}

TouchEvent TouchManager::update() {
  bool raw = digitalRead(TTP223_PIN) == HIGH; // HIGH = touched (TTP223 default)
  uint32_t now = millis();

  if (raw != _rawLast) {
    _rawLast = raw;
    _lastChangeMs = now;
  }

  TouchEvent event = TouchEvent::NONE;

  if ((now - _lastChangeMs) >= TOUCH_DEBOUNCE_MS && raw != _debouncedState) {
    // Debounced state transition confirmed.
    _debouncedState = raw;

    if (_debouncedState) {
      // Touch started.
      _pressStartMs = now;
      _longPressFired = false;
    } else {
      // Touch released.
      if (!_longPressFired) {
        // Only fire SHORT_PRESS if we didn't already fire LONG_PRESS while
        // held, so a long press never generates both events.
        event = TouchEvent::SHORT_PRESS;
      }
    }
  }

  // Fire LONG_PRESS as soon as the threshold is crossed WHILE still held,
  // rather than waiting for release — feels far more responsive and keeps
  // this loop delay()-free.
  if (_debouncedState && !_longPressFired &&
      (now - _pressStartMs) >= TOUCH_LONG_PRESS_MS) {
    _longPressFired = true;
    event = TouchEvent::LONG_PRESS;
  }

  return event;
}
