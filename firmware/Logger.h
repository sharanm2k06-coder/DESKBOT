#pragma once
#include <Arduino.h>
#include "config.h"

// Centralized logging so we never scatter Serial.print calls, and so we can
// globally disable logging (e.g. before shipping) from one place.
// NEVER log message bodies, BLE payloads, or anything WhatsApp-derived here
// once Phase 2/4 land — see PRIVACY notes in NotificationManager.

#if DEBUG_ENABLED
  #define LOG_BEGIN()        Serial.begin(DEBUG_SERIAL_BAUD)
  #define LOG(...)           Serial.print(__VA_ARGS__)
  #define LOGLN(...)         Serial.println(__VA_ARGS__)
  #define LOGF(fmt, ...)     Serial.printf(fmt, ##__VA_ARGS__)
#else
  #define LOG_BEGIN()
  #define LOG(...)
  #define LOGLN(...)
  #define LOGF(...)
#endif
