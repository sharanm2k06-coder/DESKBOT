#pragma once

#include <Arduino.h>

#include "config.h"
#include "DeviceState.h"
#include "MessageManager.h"

// ============================================================================
// DESKBOT BACKEND CLIENT  (Phase 5 integration)
// ============================================================================
//
// The one link the architecture diagram in README.md calls "ESP32 <-> Backend
// (HTTPS polling)" but that Phases 1-4 never actually implemented: BLE
// (phone<->ESP32) was already complete, and the web/backend/Android stacks
// were each built and tested against their own contract, but nothing on the
// firmware side ever called the FastAPI service. This class is that call.
//
// Responsibilities, per docs/API_DOCUMENTATION.md:
//   1. Poll GET /api/v1/devices/{id}/messages/pending every
//      BACKEND_POLL_INTERVAL_MS. A 200 with a null body means "nothing
//      pending" -- not an error, nothing to branch on specially.
//   2. Enqueue whatever comes back into the local MessageManager (the same
//      RAM queue BLE-forwarded WhatsApp messages use), then POST
//      /messages/{id}/ack. Acking immediately (rather than waiting for the
//      user to view it on-device) is deliberate: the backend already flips
//      PENDING -> DELIVERED the instant it hands the message over via the
//      pending-poll GET, and only serves PENDING messages from that
//      endpoint -- so there's no backend-side retry to preserve by holding
//      the ack back, and keeping this stateless means the firmware doesn't
//      need to track a local<->backend id mapping across reboots.
//   3. POST /devices/{id}/heartbeat every BACKEND_HEARTBEAT_INTERVAL_MS with
//      whatever the firmware currently knows (wifi/ble state, last DHT22
//      reading) -- all fields are optional server-side, so a stale/NAN
//      sensor reading is simply omitted rather than sent as garbage.
//
// Entirely non-blocking against the caller: every network call is a
// synchronous HTTPClient request (the ESP32 HTTP stack has no async client),
// so calls are kept short (single small JSON bodies, BACKEND_HTTP_TIMEOUT_MS
// ceiling) and are only attempted when WiFi is actually connected --
// display/BLE/touch continue to be serviced every loop() regardless of
// whether the backend is reachable, matching the "device stays useful
// without the backend" requirement in the root README.
// ============================================================================

class BackendClient {
 public:
  void begin(MessageManager* messages);

  // Call once per loop(). Only does network work when wifiConnected is true
  // and the relevant interval has elapsed; otherwise returns immediately.
  void update(DeviceState &state, bool wifiConnected);

 private:
  MessageManager* _messages = nullptr;

  uint32_t _lastPollMs = 0;
  uint32_t _lastHeartbeatMs = 0;

  // True once BACKEND_DEVICE_ID/BACKEND_DEVICE_KEY have been edited away
  // from their placeholder values in config.h. Avoids spamming failed
  // requests (and the serial log) against an unconfigured backend.
  bool _configured = false;

  void pollPendingMessage();
  void sendHeartbeat(DeviceState &state);

  static MessageSource parseSource(const char* s);
  static MessagePriority parsePriority(const char* s);
};
