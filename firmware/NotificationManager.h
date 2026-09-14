#pragma once
#include <Arduino.h>
#include "config.h"
#include "DeviceState.h"

// Drives the buzzer with short, non-blocking beep patterns and drives the
// face/screen reaction when a new message arrives. All timing uses millis()
// state machines — nothing here ever calls delay().
//
// PRIVACY NOTE (relevant once WhatsApp forwarding lands in Phase 4):
// this manager only ever touches Message structs already in RAM via
// MessageManager. It must never log message bodies/senders — see Logger.h.
class NotificationManager {
 public:
  void begin();
  void update(DeviceState &state);   // call every loop(); advances buzzer FSM

  // Call when a new message is enqueued, to kick off the alert sequence
  // (face change + buzzer pattern) appropriate to its priority.
  void onNewMessage(DeviceState &state, MessagePriority priority);

 private:
  enum class BuzzPattern : uint8_t { NONE, NORMAL_BEEP, IMPORTANT_BEEP,
                                      ALERT_BEEP, BLE_CONNECTED_TONE,
                                      BLE_DISCONNECTED_TONE };
  BuzzPattern _pattern = BuzzPattern::NONE;
  uint8_t _step = 0;
  uint32_t _stepStartMs = 0;
  bool _pinHigh = false;

  void startPattern(BuzzPattern p);
  void driveBuzzer();
  void setBuzzer(bool on);

 public:
  // Exposed so WiFiManager/BLEManager can request their connect/disconnect
  // tones without needing to know about message priorities.
  void playBleConnected();
  void playBleDisconnected();
};
