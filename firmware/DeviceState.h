#pragma once
#include <Arduino.h>
#include "config.h"

// ============================================================================
// Shared state definitions. Every manager reads/writes into a single
// DeviceState instance owned by DeskBot.ino and passed around by reference,
// so no manager needs to know about another manager's internals.
// ============================================================================

enum class ScreenId : uint8_t {
  BOOT,
  HOME,
  MESSAGES,
  ENVIRONMENT,
  DEVICE_STATUS,
  SETTINGS
};

enum class FaceState : uint8_t {
  NORMAL,
  HAPPY,
  THINKING,
  SLEEPING,
  ALERT,
  MESSAGE,
  ERROR_FACE,   // avoid clashing with ERROR macro on some platforms
  OFFLINE
};

enum class WifiState : uint8_t {
  DISCONNECTED,
  CONNECTING,
  CONNECTED,
  FAILED
};

enum class BleState : uint8_t {
  IDLE,
  ADVERTISING,
  CONNECTED
};

enum class TimeSyncState : uint8_t {
  NOT_SYNCED,
  SYNCED,
  STALE          // was synced, wifi has since dropped for a long time
};

enum class TouchEvent : uint8_t {
  NONE,
  SHORT_PRESS,
  LONG_PRESS
};

enum class MessageSource : uint8_t {
  APP,
  WHATSAPP,
  SYSTEM,
  SCHEDULED
};

enum class MessagePriority : uint8_t {
  NORMAL,
  IMPORTANT,
  ALERT
};

struct Message {
  uint32_t id = 0;
  MessageSource source = MessageSource::SYSTEM;
  MessagePriority priority = MessagePriority::NORMAL;
  char sender[MESSAGE_SENDER_MAX_LEN] = {0};
  char title[MESSAGE_TITLE_MAX_LEN] = {0};
  char body[MESSAGE_BODY_MAX_LEN] = {0};
  uint32_t createdAtMs = 0;   // millis() at enqueue time (device-local clock)
  uint32_t ttlMs = MESSAGE_DEFAULT_TTL_MS;
  bool read = false;
  bool valid = false;         // false = empty slot
};

struct SensorReading {
  float temperatureC = NAN;
  float humidityPct = NAN;
  bool valid = false;
  uint8_t consecutiveFailures = 0;
};

struct DeviceState {
  ScreenId currentScreen = ScreenId::BOOT;
  FaceState faceState = FaceState::NORMAL;

  WifiState wifiState = WifiState::DISCONNECTED;
  BleState bleState = BleState::IDLE;
  TimeSyncState timeSyncState = TimeSyncState::NOT_SYNCED;

  SensorReading lastReading;

  int messageCursor = 0;      // which queued message is on screen
  int messageTextPage = 0;    // which page of a long message body is shown
  bool bootSequenceDone = false;

  char deviceId[32] = DEVICE_ID_DEFAULT;
};
