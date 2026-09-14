# DeskBot Firmware — Phase 1

Implements the first milestone from the master spec (Section 69):
ESP32-S3 + SH1106 OLED + DHT22 + TTP223 + Buzzer, with a futuristic
monochrome UI, animated robot face, clock, environment readout, touch
navigation, a local RAM message queue, and a BLE skeleton.

**Not yet in this phase:** the Android companion, the FastAPI backend, the
Next.js frontend, and the full fragmented BLE protocol (`BLE_PROTOCOL.md`).
Those land in later phases once this base is confirmed working on real
hardware — see the root `README.md` for the phase plan.

## Directory structure

```
firmware/
├── DeskBot.ino              — setup()/loop(), boot sequence, touch->nav wiring
├── config.h                 — ALL pins and tunables (edit this first)
├── DeviceState.h            — shared enums/structs (screens, face states, Message)
├── Logger.h                 — Serial logging macros (compiled out if DEBUG_ENABLED=0)
├── DisplayManager.h/.cpp    — U8g2 SH1106 driver, all screens, robot face
├── SensorManager.h/.cpp     — non-blocking DHT22 reads
├── TouchManager.h/.cpp      — TTP223 debounce + short/long press detection
├── WiFiManager.h/.cpp       — non-blocking WiFi connect/retry
├── TimeManager.h/.cpp       — NTP sync + graceful degrade when offline
├── BLEManager.h/.cpp        — BLE advertising + skeleton characteristics
├── MessageManager.h/.cpp    — fixed-size RAM message queue
├── NotificationManager.h/.cpp — buzzer patterns + face/screen reaction
├── HARDWARE_WIRING.md
└── README.md                — this file
```

## Libraries (Arduino Library Manager)

| Library | Author | Purpose |
|---|---|---|
| U8g2 | olikraus | SH1106 OLED driver |
| DHT sensor library | Adafruit | DHT22 reads |
| Adafruit Unified Sensor | Adafruit | dependency of the above |

BLE support comes from the **ESP32 board package's bundled BLE library**
(`BLEDevice.h` etc.) — no separate install needed once the `esp32` board
package (by Espressif Systems) is installed via Boards Manager.

## Board settings (Arduino IDE)

- Board: **ESP32S3 Dev Module** (or your Super Mini's specific entry, if the
  board package lists it by name)
- USB CDC On Boot: **Enabled** (so Serial Monitor works over the native USB port)
- Flash Size / Partition Scheme: defaults are fine for this phase (no OTA yet)
- Upload Speed: 921600 (or lower if you see upload errors)

## Build & flash

1. Install the `esp32` board package (Espressif Systems) via **Boards
   Manager** if you haven't already.
2. Install the two libraries above via **Library Manager**.
3. Open `DeskBot.ino` (all other files in this folder must sit alongside it
   — the Arduino IDE treats the whole folder as one sketch).
4. Edit `config.h`: confirm/adjust GPIO pins per `HARDWARE_WIRING.md`, set
   `WIFI_SSID`/`WIFI_PASSWORD`.
5. Select your board + port, click Upload.
6. Open Serial Monitor at 115200 baud to watch boot logs.

## What you should see on first boot

1. `BOOTING...` → `OLED OK` → `OLED OK / DHT22 OK` → `BLE STARTING` →
   `WIFI CONNECTING` → `DESKBOT READY`, then the Home screen.
2. Home screen: robot face (blinking), Wi-Fi/BLE dots in the status bar,
   clock + date once NTP syncs, temperature/humidity once the first DHT22
   read succeeds.
3. Short touch cycles Home → Messages → Environment → Device Status → Home.
4. Long touch returns to Home from anywhere.
5. DeskBot is BLE-advertising as `DeskBot-01` — connect with a generic BLE
   tool (e.g. nRF Connect) and write a UTF-8 string like
   `Rahul|WhatsApp|Are you coming to the meeting?` to the message
   characteristic (`...789abc` ending `0004`) to see the message land in
   the queue, trigger the buzzer, and show up on the Messages screen.

## Known Phase-1 simplifications (intentional, not bugs)

- BLE message intake is single-packet plain text (`sender|title|body`),
  capped at `BLE_MAX_SINGLE_PACKET_LEN`. The fragmented/ack'd binary
  protocol described in the master spec is a Phase 2/4 deliverable, once
  there's an Android app on the other end to test against.
- Long message bodies wrap across 3 lines and show `more>` if they overflow
  — full multi-page paging (tracked via `DeviceState.messageTextPage`) is
  stubbed and ready to wire up but not yet driven by the touch handler.
- Settings screen is read-only for now.
