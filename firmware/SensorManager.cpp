#include "SensorManager.h"
#include "Logger.h"
#include <DHT.h>

// We keep a static instance rather than heap-allocating via the opaque
// pointer, since DHT's constructor needs the pin/type at construction time
// and this keeps the header clean of the library include.
static DHT dhtSensor(DHT_PIN, DHT_TYPE_DHT22);

void SensorManager::begin() {
  dhtSensor.begin();
  _lastReadMs = 0; // force an immediate first read
  LOGLN("[SensorManager] DHT22 ready");
}

void SensorManager::update(DeviceState &state) {
  uint32_t now = millis();
  if (now - _lastReadMs < DHT_READ_INTERVAL_MS) return;
  _lastReadMs = now;

  float h = dhtSensor.readHumidity();
  float t = dhtSensor.readTemperature();

  if (isnan(h) || isnan(t)) {
    state.lastReading.valid = false;
    if (state.lastReading.consecutiveFailures < 255) {
      state.lastReading.consecutiveFailures++;
    }
    LOGF("[SensorManager] read failed (consecutive=%d)\n",
         state.lastReading.consecutiveFailures);
    // Deliberately do NOT touch temperatureC/humidityPct here — the UI
    // decides whether to keep showing the last good value or fall back to
    // "--.-" based on consecutiveFailures vs DHT_MAX_CONSEC_FAILS.
    return;
  }

  state.lastReading.temperatureC = t;
  state.lastReading.humidityPct = h;
  state.lastReading.valid = true;
  state.lastReading.consecutiveFailures = 0;
}
