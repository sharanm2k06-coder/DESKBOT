#pragma once
#include <Arduino.h>
#include "config.h"
#include "DeviceState.h"
#include "MessageManager.h"
#include "TimeManager.h"

// Owns the U8g2 instance and draws every screen. Redraws are paced to
// DISPLAY_FPS_TARGET via millis() — update() is cheap to call every loop()
// iteration and internally decides whether it's actually time to render.
class DisplayManager {
 public:
  void begin();
  void update(DeviceState &state, MessageManager &messages, const TimeManager &time);

  void showBootStep(const char* line1, const char* line2);

 private:
  uint32_t _lastFrameMs = 0;

  // Face blink state, ticks independently of screen redraw rate.
  bool _eyesClosed = false;
  uint32_t _nextBlinkMs = 0;
  uint32_t _blinkUntilMs = 0;

  void tickBlink();

  void drawHome(DeviceState &state, MessageManager &messages, const TimeManager &time);
  void drawMessages(DeviceState &state, MessageManager &messages);
  void drawEnvironment(DeviceState &state);
  void drawDeviceStatus(DeviceState &state, MessageManager &messages);
  void drawSettings(DeviceState &state);

  void drawStatusBar(DeviceState &state);
  void drawFace(int cx, int cy, FaceState face);
  void drawWrappedText(int x, int y, int maxWidth, int lineHeight,
                        const char* text, int maxLines);
};
