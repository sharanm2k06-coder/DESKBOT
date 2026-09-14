# DESKBOT

A futuristic desktop companion device: ESP32-S3 + OLED robot face, fed by a
web console, a FastAPI/PostgreSQL backend, and an Android companion that
forwards WhatsApp notifications over BLE.

This is the **integrated** project tree — firmware, backend, frontend, and
Android app living together in one repo, wired to each other end-to-end,
rather than four components each built and tested only against their own
spec.

## Architecture

```
                    USER
                     |
          ┌──────────┴──────────┐
          |                     |
       Web App              WhatsApp
       (frontend/)              |
          |                     |
          v                     v
       Backend             Android App
       (backend/)              |
          |                    BLE
          |                     |
          └──────────┬──────────┘
                     v
                ESP32-S3
              (firmware/)
                     |
              ┌──────┼──────┐
              |      |      |
             OLED   DHT22  TTP223
              |
          Buzzer/Face
```

| Link | Transport | Status |
|---|---|---|
| Web app → Backend | HTTPS/REST | ✅ `frontend/lib/api.ts` → `backend/app/routes/` |
| Android app → Backend | HTTPS/REST | ✅ `android/.../network/ApiClient.kt` → same backend |
| **ESP32 → Backend** | **HTTPS polling** | ✅ `firmware/BackendClient.{h,cpp}` — new in this integration pass |
| Phone ↔ ESP32 | BLE (fragmented, CRC16, ACK) | ✅ `android/.../ble/BleProtocol.kt` ↔ `firmware/BLEManager.cpp`, spec in `docs/BLE_PROTOCOL.md` |
| WhatsApp → Android | `NotificationListenerService` | ✅ `android/.../notifications/` |

**ESP32 never touches WhatsApp directly.** WhatsApp → Android
`NotificationListenerService` → BLE → ESP32. No credentials, scraping, or
WhatsApp API access anywhere in this system.

**Phone ↔ ESP32 works even if the backend is down** — that's a hard
architectural requirement, not an optimization. Losing WiFi/backend only
takes out the web-originated message path; BLE-forwarded WhatsApp messages
and the on-device clock/sensor/UI keep working regardless.

## What "integration" means here

Each of the four components (`firmware/`, `backend/`, `frontend/`,
`android/`) was previously built and verified against its own contract in
isolation. Checking them against each other found:

- **Backend ↔ Frontend ↔ Android**: already correctly wired. The frontend's
  `lib/api.ts` and the Android app's `network/ApiClient.kt` both call the
  exact endpoints/fields in `docs/API_DOCUMENTATION.md`; no changes needed.
- **Phone ↔ ESP32 (BLE)**: already fully implemented on both ends per
  `docs/BLE_PROTOCOL.md` (fragmentation, CRC16, ACK/retry, reconnection
  backoff) — firmware's `BLEManager.cpp` and Android's `ble/BleProtocol.kt`
  agree on the packet format. No changes needed.
- **ESP32 ↔ Backend (HTTPS polling)**: this was the missing link — the
  firmware had no HTTP client at all, despite the backend already exposing
  `GET /devices/{id}/messages/pending`, `POST /devices/{id}/heartbeat`, and
  `POST /messages/{id}/ack` for exactly this purpose. `firmware/BackendClient.{h,cpp}`
  is new: it polls for pending messages, feeds them into the same RAM queue
  `MessageManager` already uses for BLE-forwarded messages, acks them, and
  sends periodic heartbeats with WiFi/BLE state and the latest DHT22
  reading.

## Setup, in dependency order

1. **Backend** — see `backend/README.md`. Deploy (Render, or run locally
   with `uvicorn app.main:app --reload`), then hit `/docs` to confirm it's up.
2. **Register a user + device** against the backend (via `/docs`, curl, or
   step 3's web app once it's running):
   - `POST /api/v1/auth/register`, then `/auth/login` for a JWT.
   - `POST /api/v1/devices/register` with that JWT → note the returned
     `id` and `device_key` (the key is shown **once**).
3. **Firmware** — open `firmware/DeskBot.ino` in the Arduino IDE. In
   `firmware/config.h`, set `WIFI_SSID`/`WIFI_PASSWORD` and
   `BACKEND_BASE_URL`/`BACKEND_DEVICE_ID`/`BACKEND_DEVICE_KEY` from step 2.
   Flash to the ESP32-S3. See `firmware/HARDWARE_WIRING.md` for pinout.
4. **Frontend** — see `frontend/README.md`. Set `NEXT_PUBLIC_API_URL` to
   the backend URL, `npm install && npm run dev` (or deploy to Vercel).
   Log in, register the device from here too if you skipped step 2's manual
   call, and send it a test message.
5. **Android** — see `android/DeskBotCompanion/README.md`. Build in
   Android Studio, enter the backend URL in Settings, sign in, grant
   notification-listener access for WhatsApp, and pair over BLE with the
   ESP32 (advertises as `DeskBot-01`, per `firmware/config.h`).

## Directory structure

```
DESKBOT/
├── firmware/    — ESP32-S3 Arduino sketch (OLED/DHT22/TTP223/BLE/backend HTTPS)
├── backend/     — FastAPI + PostgreSQL (auth, devices, messages, schedules)
├── frontend/    — Next.js web console
├── android/     — Kotlin/Compose companion app (BLE + WhatsApp forwarding)
├── docs/        — BLE_PROTOCOL.md, API_DOCUMENTATION.md, DEPLOYMENT.md
└── README.md    — this file
```

## Docs

- [`docs/API_DOCUMENTATION.md`](docs/API_DOCUMENTATION.md) — full backend REST contract
- [`docs/BLE_PROTOCOL.md`](docs/BLE_PROTOCOL.md) — phone↔ESP32 packet format
- [`docs/DEPLOYMENT.md`](docs/DEPLOYMENT.md) — backend/frontend deployment notes
- [`firmware/HARDWARE_WIRING.md`](firmware/HARDWARE_WIRING.md) — pinout/wiring
