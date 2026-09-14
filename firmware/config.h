#pragma once
// ============================================================================
// DESKBOT — config.h
// ALL board-specific pins and tunables live here. Nothing else in the
// firmware should hard-code a GPIO number or a magic constant.
//
// IMPORTANT: ESP32-S3 "Super Mini" boards from different vendors do NOT
// always label pins identically. Verify against your specific board's
// silkscreen/pinout diagram before wiring. The values below are sane
// defaults for the common ESP32-S3 Super Mini (WeAct-style) board, but
// treat them as a starting point, not gospel.
// ============================================================================

// ---------------------------------------------------------------------------
// DEVICE IDENTITY
// ---------------------------------------------------------------------------
#define DEVICE_ID_DEFAULT      "DESKBOT-01"   // shown to user; NOT the MAC
#define FIRMWARE_VERSION       "1.0.0"

// ---------------------------------------------------------------------------
// OLED — SH1106, 128x64, I2C
// ---------------------------------------------------------------------------
#define OLED_SDA_PIN           8
#define OLED_SCL_PIN           9
#define OLED_RESET_PIN         -1      // -1 = no dedicated reset pin wired
#define OLED_I2C_ADDR           0x3C
#define OLED_I2C_CLK_HZ         400000

// ---------------------------------------------------------------------------
// DHT22 — temperature / humidity
// ---------------------------------------------------------------------------
#define DHT_PIN                 4
#define DHT_TYPE_DHT22           22     // matches DHT.h's DHT22 constant
#define DHT_READ_INTERVAL_MS     5000   // DHT22 min sample interval is ~2s
#define DHT_MAX_CONSEC_FAILS     5      // after this many, show "--.-"

// ---------------------------------------------------------------------------
// TTP223 — touch sensor (HIGH = touched, per datasheet default config)
// ---------------------------------------------------------------------------
#define TTP223_PIN               5
#define TOUCH_DEBOUNCE_MS        40
#define TOUCH_LONG_PRESS_MS      700

// ---------------------------------------------------------------------------
// BUZZER — passive/active, driven digitally with non-blocking tone patterns
// ---------------------------------------------------------------------------
#define BUZZER_PIN               6
#define BUZZER_ACTIVE_HIGH       1     // set 0 if your buzzer module is active-low

// ---------------------------------------------------------------------------
// WI-FI
// ---------------------------------------------------------------------------
#define WIFI_SSID                "Sharan"
#define WIFI_PASSWORD            "12345678"
#define WIFI_CONNECT_TIMEOUT_MS  15000
#define WIFI_RETRY_INTERVAL_MS   30000

// ---------------------------------------------------------------------------
// BACKEND (Phase 5 integration — ESP32 <-> FastAPI/Render, HTTPS polling)
//
// Fill these in after running `POST /api/v1/devices/register` once (e.g.
// via curl or the web console) with your user JWT. That call returns
// device_key exactly once — paste it below; it is never shown again.
// See docs/API_DOCUMENTATION.md for the full contract this client follows.
// ---------------------------------------------------------------------------
#define BACKEND_BASE_URL          "https://your-service.onrender.com"
#define BACKEND_DEVICE_ID         "REPLACE_WITH_DEVICE_UUID"     // "id" from /devices/register
#define BACKEND_DEVICE_KEY        "REPLACE_WITH_DEVICE_KEY"      // "device_key" from /devices/register (shown once)
#define BACKEND_POLL_INTERVAL_MS       5000UL    // GET .../messages/pending cadence
#define BACKEND_HEARTBEAT_INTERVAL_MS  60000UL   // POST .../heartbeat cadence
#define BACKEND_HTTP_TIMEOUT_MS        8000UL    // per-request timeout
// TLS: Render's certs chain to a well-known public CA, but pinning/bundling
// that root on the ESP32 is brittle across CA rotations for a hobby build.
// setInsecure() (in BackendClient.cpp) skips certificate validation --
// traffic is still encrypted, but a MITM with a fake cert wouldn't be
// detected. Fine for a desk gadget on a home LAN; swap in a pinned root CA
// via WiFiClientSecure::setCACert() before relying on this over hostile
// networks.

// ---------------------------------------------------------------------------
// TIME / NTP
// ---------------------------------------------------------------------------
#define NTP_SERVER_1             "pool.ntp.org"
#define NTP_SERVER_2             "time.google.com"
#define TZ_GMT_OFFSET_SEC        (5 * 3600 + 1800)   // Asia/Kolkata, UTC+5:30
#define TZ_DST_OFFSET_SEC        0
#define TIME_RESYNC_INTERVAL_MS  (6UL * 60UL * 60UL * 1000UL) // resync every 6h

// ---------------------------------------------------------------------------
// BLE
// ---------------------------------------------------------------------------
#define BLE_DEVICE_NAME          "DeskBot-01"
#define BLE_SERVICE_UUID              "7d4a0001-8c4b-4f5b-9e11-123456789abc"
#define BLE_CHAR_DEVICEINFO_UUID      "7d4a0002-8c4b-4f5b-9e11-123456789abc" // read
#define BLE_CHAR_COMMAND_UUID         "7d4a0003-8c4b-4f5b-9e11-123456789abc" // write
#define BLE_CHAR_MESSAGE_UUID         "7d4a0004-8c4b-4f5b-9e11-123456789abc" // write (phone -> ESP32)
#define BLE_CHAR_STATUS_UUID          "7d4a0005-8c4b-4f5b-9e11-123456789abc" // notify (ESP32 -> phone)
// Full fragmentation/ack protocol documented separately in BLE_PROTOCOL.md
// (added in Phase 2 once the Android companion exists). Phase 1 exposes the
// skeleton service/characteristics so the board is BLE-visible and can
// accept short, single-packet test messages.
#define BLE_MAX_SINGLE_PACKET_LEN     180   // conservative, under typical MTU

// ---------------------------------------------------------------------------
// MESSAGE QUEUE (RAM-resident, not persisted)
// ---------------------------------------------------------------------------
#define MAX_QUEUED_MESSAGES      10
#define MESSAGE_SENDER_MAX_LEN   32
#define MESSAGE_TITLE_MAX_LEN    32
#define MESSAGE_BODY_MAX_LEN     200
#define MESSAGE_DEFAULT_TTL_MS   (10UL * 60UL * 1000UL) // 10 min default expiry

// ---------------------------------------------------------------------------
// DISPLAY / UI TIMING
// ---------------------------------------------------------------------------
#define DISPLAY_FPS_TARGET        20
#define DISPLAY_FRAME_INTERVAL_MS (1000 / DISPLAY_FPS_TARGET)
#define FACE_BLINK_MIN_INTERVAL_MS 2500
#define FACE_BLINK_MAX_INTERVAL_MS 6000
#define FACE_BLINK_DURATION_MS     120

// ---------------------------------------------------------------------------
// DEBUG
// ---------------------------------------------------------------------------
#define DEBUG_SERIAL_BAUD        115200
#define DEBUG_ENABLED            1
