#include "TimeManager.h"
#include "Logger.h"
#include <time.h>

void TimeManager::begin() {
  // configTime() itself is non-blocking; it kicks off the SNTP client and
  // time() will simply keep returning the (bogus, epoch-ish) time until a
  // sync completes — we track _everSynced ourselves rather than trusting
  // that.
  configTime(TZ_GMT_OFFSET_SEC, TZ_DST_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2);
  _lastSyncAttemptMs = 0;
  _everSynced = false;
  LOGLN("[TimeManager] ready (NTP will sync once WiFi is up)");
}

void TimeManager::update(DeviceState &state, bool wifiConnected) {
  uint32_t now = millis();

  if (!wifiConnected) {
    // Nothing to do — keep whatever time() currently reports (RTC/millis
    // keeps advancing on ESP32 regardless of WiFi state once first synced).
    if (_everSynced) state.timeSyncState = TimeSyncState::STALE;
    return;
  }

  bool needSync = !_everSynced ||
                  (now - _lastSyncAttemptMs >= TIME_RESYNC_INTERVAL_MS);
  if (!needSync) {
    state.timeSyncState = TimeSyncState::SYNCED;
    return;
  }

  time_t nowSec;
  time(&nowSec);
  // Heuristic: a real synced time will be well past year 2020's epoch value.
  const time_t YEAR_2020_EPOCH = 1577836800;
  if (nowSec > YEAR_2020_EPOCH) {
    if (!_everSynced) LOGLN("[TimeManager] NTP sync achieved");
    _everSynced = true;
    _lastSyncAttemptMs = now;
    state.timeSyncState = TimeSyncState::SYNCED;
  } else {
    // Sync still pending; try again shortly rather than spinning.
    if (now - _lastSyncAttemptMs > 5000) {
      _lastSyncAttemptMs = now;
      LOGLN("[TimeManager] waiting for NTP sync...");
    }
    state.timeSyncState = _everSynced ? TimeSyncState::STALE
                                       : TimeSyncState::NOT_SYNCED;
  }
}

void TimeManager::getTimeString(char* buf, size_t bufLen, bool use24h) const {
  if (!_everSynced) {
    snprintf(buf, bufLen, "--:--");
    return;
  }
  time_t nowSec;
  time(&nowSec);
  struct tm timeinfo;
  localtime_r(&nowSec, &timeinfo);
  if (use24h) {
    strftime(buf, bufLen, "%H:%M", &timeinfo);
  } else {
    strftime(buf, bufLen, "%I:%M %p", &timeinfo);
  }
}

void TimeManager::getDateString(char* buf, size_t bufLen) const {
  if (!_everSynced) {
    snprintf(buf, bufLen, "-- ---");
    return;
  }
  time_t nowSec;
  time(&nowSec);
  struct tm timeinfo;
  localtime_r(&nowSec, &timeinfo);
  strftime(buf, bufLen, "%d %b", &timeinfo);
}
