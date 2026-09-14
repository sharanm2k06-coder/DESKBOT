// ============================================================================
// DESKBOT — Phase 1 firmware
// ESP32-S3 Super Mini + SH1106 128x64 OLED + DHT22 + TTP223 + Buzzer + BLE
//
// Architecture: DeviceState is the single source of truth, owned here and
// passed by reference into every manager's update(). No manager calls
// delay() anywhere — the whole loop() is a cooperative, non-blocking
// scheduler driven by millis().
//
// Libraries required (install via Arduino Library Manager):
//   - U8g2 (oliver/olikraus)              -> OLED
//   - DHT sensor library (Adafruit)        -> DHT22
//   - Adafruit Unified Sensor              -> DHT dependency
//   - ArduinoJson (Benoit Blanchon)        -> BLE + backend JSON (de)serialize
//   - ESP32 BLE Arduino (bundled with the ESP32 board package)
//   - HTTPClient / WiFiClientSecure (bundled with the ESP32 board package)
//                                           -> BackendClient HTTPS calls
//
// Phase 5 update: BLEManager now implements the full docs/BLE_PROTOCOL.md
// packet format (fragmentation + CRC16 + ACK), matching the Android
// companion's BleProtocol.kt. The previous Phase-1 "sender|title|body"
// plain-text single-packet parser has been replaced.
//
// Phase 5 update 2: BackendClient closes the other integration link from
// the root README's architecture diagram -- ESP32 <-> FastAPI/Render over
// HTTPS (poll pending messages, ack them, send heartbeats). Set
// BACKEND_BASE_URL/BACKEND_DEVICE_ID/BACKEND_DEVICE_KEY in config.h before
// flashing, or this link simply stays disabled (BLE/OLED/sensors are
// unaffected either way).
//
// Board package: esp32 by Espressif Systems, board "ESP32S3 Dev Module"
// (or your Super Mini variant if listed), USB CDC on boot = Enabled if you
// want Serial over the native USB port.
// ============================================================================

#include "config.h"
#include "Logger.h"
#include "DeviceState.h"

#include "DisplayManager.h"
#include "SensorManager.h"
#include "TouchManager.h"
#include "WiFiManager.h"
#include "TimeManager.h"
#include "BLEManager.h"
#include "MessageManager.h"
#include "NotificationManager.h"
#include "BackendClient.h"

DeviceState        g_state;
DisplayManager      g_display;
SensorManager       g_sensors;
TouchManager        g_touch;
WiFiManagerNB       g_wifi;
TimeManager         g_time;
BLEManager          g_ble;
MessageManager      g_messages;
NotificationManager g_notify;
BackendClient       g_backend;

// ---------------------------------------------------------------------------
// Boot sequence — the only place we deliberately take a little wall-clock
// time up front, since nothing else is running yet. Even here we avoid long
// delay() calls; each step is short and cosmetic.
// ---------------------------------------------------------------------------
static void runBootSequence() {
  g_display.showBootStep("BOOTING...", "");
  delay(300);

  g_display.showBootStep("OLED OK", "");
  delay(200);

  g_sensors.begin();
  g_display.showBootStep("OLED OK", "DHT22 OK");
  delay(200);

  g_ble.begin(&g_messages, &g_notify);
  g_display.showBootStep("BLE STARTING", "");
  delay(300);

  g_display.showBootStep("WIFI CONNECTING", "");
  // Actual WiFi connect is kicked off in the main loop (non-blocking) —
  // this label is cosmetic; loop() will update state.wifiState shortly.
  delay(300);

  g_display.showBootStep("DESKBOT READY", "");
  delay(500);

  g_state.bootSequenceDone = true;
  g_state.currentScreen = ScreenId::HOME;
}

// ---------------------------------------------------------------------------
// Touch -> navigation. Kept here (not inside TouchManager) because it needs
// to know about screen/message context, which is app-level policy, not
// input-driver logic.
// ---------------------------------------------------------------------------
static void handleTouchEvent(TouchEvent event) {
  if (event == TouchEvent::NONE) return;

  if (event == TouchEvent::LONG_PRESS) {
    // Long touch always returns Home, from anywhere.
    g_state.currentScreen = ScreenId::HOME;
    g_state.messageTextPage = 0;
    LOGLN("[Touch] long press -> HOME");
    return;
  }

  // SHORT_PRESS behavior depends on context.
  if (g_state.currentScreen == ScreenId::MESSAGES && !g_messages.empty()) {
    // Advance to the next message; wrap around. (Full "page within a long
    // message first" behavior hooks in once DisplayManager's paging output
    // is threaded back — see messageTextPage in DeviceState.)
    g_messages.markReadByDisplayIndex(g_state.messageCursor);
    g_state.messageCursor++;
    g_state.messageTextPage = 0;
    if (g_state.messageCursor >= g_messages.count()) {
      g_state.messageCursor = 0;
      g_state.currentScreen = ScreenId::ENVIRONMENT; // cycle onward
    }
    return;
  }

  // Default: cycle HOME -> MESSAGES -> ENVIRONMENT -> DEVICE_STATUS -> HOME
  switch (g_state.currentScreen) {
    case ScreenId::HOME:          g_state.currentScreen = ScreenId::MESSAGES; break;
    case ScreenId::MESSAGES:      g_state.currentScreen = ScreenId::ENVIRONMENT; break;
    case ScreenId::ENVIRONMENT:   g_state.currentScreen = ScreenId::DEVICE_STATUS; break;
    case ScreenId::DEVICE_STATUS: g_state.currentScreen = ScreenId::HOME; break;
    default:                      g_state.currentScreen = ScreenId::HOME; break;
  }
  g_state.messageCursor = 0;
  g_state.messageTextPage = 0;
}

void setup() {
  LOG_BEGIN();
  delay(100); // let USB-CDC serial settle; harmless, boot-only
  LOGLN("\n[DeskBot] booting Phase 1 firmware " FIRMWARE_VERSION);

  g_display.begin();
  g_touch.begin();
  g_messages.begin();
  g_notify.begin();
  g_wifi.begin();
  g_time.begin();
  g_backend.begin(&g_messages);

  runBootSequence(); // brings up sensors + BLE too, see above
}

void loop() {
  // --- Input --------------------------------------------------------------
  TouchEvent touchEvent = g_touch.update();
  handleTouchEvent(touchEvent);

  // --- Connectivity ---------------------------------------------------------
  g_wifi.update(g_state);
  g_time.update(g_state, g_wifi.isConnected());
  g_ble.update(g_state);
  g_backend.update(g_state, g_wifi.isConnected());

  // --- Sensing --------------------------------------------------------------
  g_sensors.update(g_state);

  // --- Message lifecycle ------------------------------------------------
  g_messages.update();

  // --- Face state auto-recovery: once the user has acknowledged/paged past
  // an alert (i.e. left the MESSAGES screen), fall back to NORMAL/OFFLINE
  // rather than staying stuck on ALERT/MESSAGE forever.
  if (g_state.currentScreen != ScreenId::MESSAGES &&
      (g_state.faceState == FaceState::ALERT || g_state.faceState == FaceState::MESSAGE)) {
    g_state.faceState = (g_state.wifiState == WifiState::CONNECTED ||
                          g_state.bleState == BleState::CONNECTED)
                             ? FaceState::NORMAL
                             : FaceState::OFFLINE;
  }

  // --- Feedback (buzzer FSM) ------------------------------------------------
  g_notify.update(g_state);

  // --- Rendering --------------------------------------------------------------
  g_display.update(g_state, g_messages, g_time);
}
