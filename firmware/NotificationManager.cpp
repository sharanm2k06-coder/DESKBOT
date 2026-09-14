#include "NotificationManager.h"
#include "Logger.h"

// Each pattern is a sequence of durations in ms, alternating ON, OFF, ON, OFF...
// starting with ON. Terminated by a 0.
static const uint16_t PATTERN_NORMAL[]      = {80, 0};
static const uint16_t PATTERN_IMPORTANT[]   = {80, 100, 80, 0};
static const uint16_t PATTERN_ALERT[]       = {120, 90, 120, 90, 120, 0};
static const uint16_t PATTERN_BLE_CONNECT[] = {60, 0};
static const uint16_t PATTERN_BLE_DISCONNECT[] = {60, 80, 60, 0};

void NotificationManager::begin() {
  pinMode(BUZZER_PIN, OUTPUT);
  setBuzzer(false);
  _pattern = BuzzPattern::NONE;
  LOGLN("[NotificationManager] ready");
}

void NotificationManager::setBuzzer(bool on) {
  _pinHigh = on;
  digitalWrite(BUZZER_PIN, (on == (BUZZER_ACTIVE_HIGH != 0)) ? HIGH : LOW);
}

void NotificationManager::startPattern(BuzzPattern p) {
  _pattern = p;
  _step = 0;
  _stepStartMs = millis();
  setBuzzer(true); // every pattern starts with an ON segment
}

void NotificationManager::driveBuzzer() {
  if (_pattern == BuzzPattern::NONE) return;

  const uint16_t* steps = nullptr;
  switch (_pattern) {
    case BuzzPattern::NORMAL_BEEP:           steps = PATTERN_NORMAL; break;
    case BuzzPattern::IMPORTANT_BEEP:        steps = PATTERN_IMPORTANT; break;
    case BuzzPattern::ALERT_BEEP:            steps = PATTERN_ALERT; break;
    case BuzzPattern::BLE_CONNECTED_TONE:    steps = PATTERN_BLE_CONNECT; break;
    case BuzzPattern::BLE_DISCONNECTED_TONE: steps = PATTERN_BLE_DISCONNECT; break;
    default: return;
  }

  uint16_t stepDuration = steps[_step];
  if (stepDuration == 0) {
    // Pattern finished.
    setBuzzer(false);
    _pattern = BuzzPattern::NONE;
    return;
  }

  if (millis() - _stepStartMs >= stepDuration) {
    _step++;
    _stepStartMs = millis();
    uint16_t next = steps[_step];
    if (next == 0) {
      setBuzzer(false);
      _pattern = BuzzPattern::NONE;
    } else {
      // odd steps are OFF segments, even steps are ON segments
      setBuzzer((_step % 2) == 0);
    }
  }
}

void NotificationManager::update(DeviceState &state) {
  driveBuzzer();
  (void)state; // reserved: future face-animation ticking could live here
}

void NotificationManager::onNewMessage(DeviceState &state, MessagePriority priority) {
  switch (priority) {
    case MessagePriority::NORMAL:
      state.faceState = FaceState::MESSAGE;
      startPattern(BuzzPattern::NORMAL_BEEP);
      break;
    case MessagePriority::IMPORTANT:
      state.faceState = FaceState::MESSAGE;
      startPattern(BuzzPattern::IMPORTANT_BEEP);
      break;
    case MessagePriority::ALERT:
      state.faceState = FaceState::ALERT;
      startPattern(BuzzPattern::ALERT_BEEP);
      break;
  }
  state.currentScreen = ScreenId::MESSAGES;
  LOGLN("[NotificationManager] new message alert triggered");
}

void NotificationManager::playBleConnected() {
  startPattern(BuzzPattern::BLE_CONNECTED_TONE);
}

void NotificationManager::playBleDisconnected() {
  startPattern(BuzzPattern::BLE_DISCONNECTED_TONE);
}
