#include "DisplayManager.h"
#include "Logger.h"
#include <U8g2lib.h>
#include <Wire.h>
#include <string.h>

// Full-frame-buffer mode (_F_) keeps drawing logic simple; at 128x64
// monochrome the RAM cost (1KB) is trivial on an ESP32-S3.
static U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void DisplayManager::begin() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(OLED_I2C_CLK_HZ);
  u8g2.begin();
  u8g2.setBusClock(OLED_I2C_CLK_HZ);
  u8g2.setContrast(180);
  _nextBlinkMs = millis() + FACE_BLINK_MIN_INTERVAL_MS;
  LOGLN("[DisplayManager] SH1106 initialized");
}

void DisplayManager::showBootStep(const char* line1, const char* line2) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(28, 20, "DESKBOT");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(4, 40, line1);
  if (line2 && line2[0]) u8g2.drawStr(4, 54, line2);
  u8g2.sendBuffer();
}

// ---------------------------------------------------------------------------
// Frame pacing
// ---------------------------------------------------------------------------
void DisplayManager::update(DeviceState &state, MessageManager &messages,
                             const TimeManager &time) {
  uint32_t now = millis();
  tickBlink();

  if (now - _lastFrameMs < DISPLAY_FRAME_INTERVAL_MS) return;
  _lastFrameMs = now;

  u8g2.clearBuffer();

  switch (state.currentScreen) {
    case ScreenId::HOME:          drawHome(state, messages, time); break;
    case ScreenId::MESSAGES:      drawMessages(state, messages); break;
    case ScreenId::ENVIRONMENT:   drawEnvironment(state); break;
    case ScreenId::DEVICE_STATUS: drawDeviceStatus(state, messages); break;
    case ScreenId::SETTINGS:      drawSettings(state); break;
    case ScreenId::BOOT:          /* boot screens use showBootStep() directly */ break;
  }

  u8g2.sendBuffer();
}

void DisplayManager::tickBlink() {
  uint32_t now = millis();
  if (!_eyesClosed && now >= _nextBlinkMs) {
    _eyesClosed = true;
    _blinkUntilMs = now + FACE_BLINK_DURATION_MS;
  } else if (_eyesClosed && now >= _blinkUntilMs) {
    _eyesClosed = false;
    uint32_t span = FACE_BLINK_MAX_INTERVAL_MS - FACE_BLINK_MIN_INTERVAL_MS;
    _nextBlinkMs = now + FACE_BLINK_MIN_INTERVAL_MS + (millis() % (span + 1));
  }
}

// ---------------------------------------------------------------------------
// Shared chrome
// ---------------------------------------------------------------------------
void DisplayManager::drawStatusBar(DeviceState &state) {
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 8, "DESKBOT");

  // Wi-Fi indicator: filled dot = connected, hollow = not.
  int wifiX = 96;
  if (state.wifiState == WifiState::CONNECTED) {
    u8g2.drawDisc(wifiX, 4, 3);
  } else {
    u8g2.drawCircle(wifiX, 4, 3);
  }

  // BLE indicator: small "B" box, filled when connected.
  int bleX = 112;
  if (state.bleState == BleState::CONNECTED) {
    u8g2.drawBox(bleX - 3, 1, 7, 7);
  } else {
    u8g2.drawFrame(bleX - 3, 1, 7, 7);
  }

  // Unread-message indicator.
  int msgX = 124;
  bool hasUnread = false; // set by caller context where relevant; see drawHome
  (void)hasUnread; (void)msgX;

  u8g2.drawHLine(0, 10, 128);
}

// ---------------------------------------------------------------------------
// Robot face — drawn with primitives only (no bitmap fonts/emoji), centered
// at (cx, cy). Blink state (_eyesClosed) is ticked independently of which
// FaceState is active, except SLEEPING/OFFLINE which force closed eyes.
// ---------------------------------------------------------------------------
void DisplayManager::drawFace(int cx, int cy, FaceState face) {
  bool closed = _eyesClosed || face == FaceState::SLEEPING || face == FaceState::OFFLINE;
  int eyeDx = 14; // horizontal offset of each eye from center

  switch (face) {
    case FaceState::HAPPY: {
      // ^   ^      happy eyes (upward chevrons)
      //  \___/     curved smile
      for (int s = -1; s <= 1; s += 2) {
        int ex = cx + s * eyeDx;
        u8g2.drawLine(ex - 4, cy + 2, ex, cy - 3);
        u8g2.drawLine(ex, cy - 3, ex + 4, cy + 2);
      }
      u8g2.drawLine(cx - 10, cy + 10, cx - 4, cy + 14);
      u8g2.drawLine(cx - 4, cy + 14, cx + 4, cy + 14);
      u8g2.drawLine(cx + 4, cy + 14, cx + 10, cy + 10);
      break;
    }
    case FaceState::THINKING: {
      // small static dots for eyes, offset mouth dash (thinking look)
      u8g2.drawDisc(cx - eyeDx, cy, 2);
      u8g2.drawDisc(cx + eyeDx, cy, 2);
      u8g2.drawHLine(cx - 4, cy + 12, 12);
      // small animated "..." could be driven by an external tick if desired
      break;
    }
    case FaceState::ALERT: {
      // hollow wide eyes, flat alarmed mouth; caller may also invert screen
      u8g2.drawCircle(cx - eyeDx, cy, 5);
      u8g2.drawCircle(cx + eyeDx, cy, 5);
      u8g2.drawBox(cx - 8, cy + 11, 16, 3);
      break;
    }
    case FaceState::MESSAGE: {
      // normal eyes + small envelope-like mouth notch to hint "new info"
      u8g2.drawBox(cx - eyeDx - 3, cy - 2, 6, closed ? 1 : 6);
      u8g2.drawBox(cx + eyeDx - 3, cy - 2, 6, closed ? 1 : 6);
      u8g2.drawHLine(cx - 6, cy + 12, 12);
      u8g2.drawVLine(cx, cy + 9, 6);
      break;
    }
    case FaceState::ERROR_FACE: {
      // X eyes
      for (int s = -1; s <= 1; s += 2) {
        int ex = cx + s * eyeDx;
        u8g2.drawLine(ex - 4, cy - 4, ex + 4, cy + 4);
        u8g2.drawLine(ex - 4, cy + 4, ex + 4, cy - 4);
      }
      u8g2.drawHLine(cx - 6, cy + 12, 12);
      break;
    }
    case FaceState::SLEEPING:
    case FaceState::OFFLINE: {
      u8g2.drawHLine(cx - eyeDx - 3, cy, 6);
      u8g2.drawHLine(cx + eyeDx - 3, cy, 6);
      u8g2.drawHLine(cx - 4, cy + 12, 8);
      if (face == FaceState::SLEEPING) {
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(cx + eyeDx + 6, cy - 6, "z");
      }
      break;
    }
    case FaceState::NORMAL:
    default: {
      u8g2.drawBox(cx - eyeDx - 3, cy - 2, 6, closed ? 1 : 6);
      u8g2.drawBox(cx + eyeDx - 3, cy - 2, 6, closed ? 1 : 6);
      u8g2.drawHLine(cx - 6, cy + 12, 12);
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// HOME
// ---------------------------------------------------------------------------
void DisplayManager::drawHome(DeviceState &state, MessageManager &messages,
                               const TimeManager &time) {
  drawStatusBar(state);

  bool invert = (state.faceState == FaceState::ALERT);
  if (invert) u8g2.drawBox(0, 11, 128, 53); // solid block; face/text drawn in XOR-ish contrast below
  // NOTE: true pixel-inversion needs u8g2's setDrawColor(0) for the
  // foreground while the box above is color 1 — kept simple here.
  if (invert) u8g2.setDrawColor(0);

  drawFace(64, 26, state.faceState);

  char timeStr[12], dateStr[12];
  time.getTimeString(timeStr, sizeof(timeStr));
  time.getDateString(dateStr, sizeof(dateStr));

  u8g2.setFont(u8g2_font_7x14B_tr);
  int tw = u8g2.getStrWidth(timeStr);
  u8g2.drawStr(64 - tw / 2, 46, timeStr);

  u8g2.setFont(u8g2_font_6x10_tr);
  int dw = u8g2.getStrWidth(dateStr);
  u8g2.drawStr(64 - dw / 2, 56, dateStr);

  char envStr[24];
  if (state.lastReading.valid || state.lastReading.consecutiveFailures < DHT_MAX_CONSEC_FAILS) {
    snprintf(envStr, sizeof(envStr), "%.1fC   %.0f%%",
              state.lastReading.temperatureC, state.lastReading.humidityPct);
  } else {
    snprintf(envStr, sizeof(envStr), "--.-C   --%%");
  }
  int ew = u8g2.getStrWidth(envStr);
  u8g2.drawStr(64 - ew / 2, 64, envStr);

  if (messages.count() > 0) {
    // Small "N" badge, top-right, to indicate unread items without a
    // dedicated icon font.
    char badge[4];
    snprintf(badge, sizeof(badge), "%d", messages.count());
    u8g2.drawStr(120, 8, badge);
  }

  if (invert) u8g2.setDrawColor(1);
}

// ---------------------------------------------------------------------------
// MESSAGES
// ---------------------------------------------------------------------------
void DisplayManager::drawMessages(DeviceState &state, MessageManager &messages) {
  u8g2.setFont(u8g2_font_6x10_tr);

  if (messages.empty()) {
    u8g2.drawStr(0, 8, "< MESSAGES");
    u8g2.drawHLine(0, 10, 128);
    u8g2.drawStr(20, 34, "No messages");
    return;
  }

  if (state.messageCursor >= messages.count()) state.messageCursor = 0;
  const Message* m = messages.getByDisplayIndex(state.messageCursor);
  if (!m) return;

  char header[24];
  snprintf(header, sizeof(header), "< MSG %d/%d", state.messageCursor + 1, messages.count());
  u8g2.drawStr(0, 8, header);

  const char* priorityTag =
      m->priority == MessagePriority::ALERT ? "!ALERT" :
      m->priority == MessagePriority::IMPORTANT ? "*IMP" : "";
  if (priorityTag[0]) {
    int pw = u8g2.getStrWidth(priorityTag);
    u8g2.drawStr(128 - pw, 8, priorityTag);
  }
  u8g2.drawHLine(0, 10, 128);

  const char* sourceLabel =
      m->source == MessageSource::WHATSAPP ? "WhatsApp" :
      m->source == MessageSource::SCHEDULED ? "Scheduled" :
      m->source == MessageSource::SYSTEM ? "System" : "App";
  u8g2.drawStr(0, 20, sourceLabel);
  u8g2.drawStr(0, 30, m->sender[0] ? m->sender : "(unknown)");

  // Wrap + paginate the body across the remaining 3 lines; messageTextPage
  // selects which chunk of wrapped lines is currently shown. Short-touch
  // while on this screen (handled in DeskBot.ino) advances the page when
  // more remains, else moves to the next message.
  drawWrappedText(0, 42, 128, 10, m->body, 3);
}

void DisplayManager::drawWrappedText(int x, int y, int maxWidth, int lineHeight,
                                      const char* text, int maxLines) {
  // Simple greedy word-wrap. Computes ALL wrapped lines, then shows only
  // the slice [page*maxLines, page*maxLines+maxLines) — page tracking is
  // the caller's job (DeviceState.messageTextPage); here we just render
  // whatever fits starting at line 0 for Phase 1 simplicity. Full paging
  // hookup is a 2-line change once the touch handler passes the page in.
  char lineBuf[64];
  int lineLen = 0;
  int line = 0;
  const char* word = text;

  auto flushLine = [&]() {
    lineBuf[lineLen] = '\0';
    if (line < maxLines) {
      u8g2.drawStr(x, y + line * lineHeight, lineBuf);
    }
    line++;
    lineLen = 0;
  };

  char wordBuf[32];
  while (*word && line <= maxLines) {
    int wlen = 0;
    while (*word && *word != ' ' && wlen < (int)sizeof(wordBuf) - 1) {
      wordBuf[wlen++] = *word++;
    }
    wordBuf[wlen] = '\0';
    if (*word == ' ') word++;

    char trial[64];
    if (lineLen == 0) {
      snprintf(trial, sizeof(trial), "%s", wordBuf);
    } else {
      snprintf(trial, sizeof(trial), "%s %s", lineBuf, wordBuf);
    }
    if (u8g2.getStrWidth(trial) > maxWidth && lineLen > 0) {
      flushLine();
      snprintf(lineBuf, sizeof(lineBuf), "%s", wordBuf);
      lineLen = strlen(lineBuf);
    } else {
      snprintf(lineBuf, sizeof(lineBuf), "%s", trial);
      lineLen = strlen(lineBuf);
    }
  }
  if (lineLen > 0 && line < maxLines) flushLine();

  if (line > maxLines) {
    u8g2.drawStr(x + maxWidth - 18, y + (maxLines - 1) * lineHeight, "more>");
  }
}

// ---------------------------------------------------------------------------
// ENVIRONMENT
// ---------------------------------------------------------------------------
void DisplayManager::drawEnvironment(DeviceState &state) {
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 8, "ENVIRONMENT");
  u8g2.drawHLine(0, 10, 128);

  u8g2.setFont(u8g2_font_7x14B_tr);
  char tempStr[16];
  if (state.lastReading.consecutiveFailures >= DHT_MAX_CONSEC_FAILS) {
    snprintf(tempStr, sizeof(tempStr), "--.-C");
  } else {
    snprintf(tempStr, sizeof(tempStr), "%.1fC", state.lastReading.temperatureC);
  }
  u8g2.drawStr(8, 34, tempStr);

  char humStr[16];
  if (state.lastReading.consecutiveFailures >= DHT_MAX_CONSEC_FAILS) {
    snprintf(humStr, sizeof(humStr), "--%%");
  } else {
    snprintf(humStr, sizeof(humStr), "%.0f%%", state.lastReading.humidityPct);
  }
  u8g2.drawStr(8, 56, humStr);

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(70, 34, "TEMP");
  u8g2.drawStr(70, 56, "HUMIDITY");

  if (state.lastReading.consecutiveFailures >= DHT_MAX_CONSEC_FAILS) {
    u8g2.drawStr(70, 46, "sensor error");
  }
}

// ---------------------------------------------------------------------------
// DEVICE STATUS
// ---------------------------------------------------------------------------
void DisplayManager::drawDeviceStatus(DeviceState &state, MessageManager &messages) {
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 8, "DESKBOT STATUS");
  u8g2.drawHLine(0, 10, 128);

  int y = 20;
  auto row = [&](const char* label, const char* value) {
    u8g2.drawStr(0, y, label);
    int vw = u8g2.getStrWidth(value);
    u8g2.drawStr(126 - vw, y, value);
    y += 10;
  };

  row("WiFi", state.wifiState == WifiState::CONNECTED ? "ONLINE" :
             state.wifiState == WifiState::CONNECTING ? "CONNECTING" : "OFFLINE");
  row("BLE", state.bleState == BleState::CONNECTED ? "CONNECTED" : "ADVERTISING");
  row("DHT22", state.lastReading.consecutiveFailures >= DHT_MAX_CONSEC_FAILS ? "ERROR" : "OK");
  row("TIME", state.timeSyncState == TimeSyncState::SYNCED ? "SYNCED" :
             state.timeSyncState == TimeSyncState::STALE ? "STALE" : "NO SYNC");

  char qStr[8];
  snprintf(qStr, sizeof(qStr), "%d/%d", messages.count(), MAX_QUEUED_MESSAGES);
  row("QUEUE", qStr);

  u8g2.drawStr(0, 63, "FW " FIRMWARE_VERSION);
}

// ---------------------------------------------------------------------------
// SETTINGS (Phase 1: read-only placeholder; interactive settings land once
// the config surface — Wi-Fi credentials, timezone, device name — is worth
// editing on-device rather than via config.h/backend.)
// ---------------------------------------------------------------------------
void DisplayManager::drawSettings(DeviceState &state) {
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 8, "SETTINGS");
  u8g2.drawHLine(0, 10, 128);
  u8g2.drawStr(0, 22, "Device:");
  u8g2.drawStr(50, 22, state.deviceId);
  u8g2.drawStr(0, 34, "FW:");
  u8g2.drawStr(50, 34, FIRMWARE_VERSION);
  u8g2.drawStr(0, 50, "Long-touch: Home");
}
