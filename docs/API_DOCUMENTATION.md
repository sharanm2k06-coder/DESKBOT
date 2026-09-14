# DeskBot API Documentation (Phase 2)

Base URL (production): `https://<your-render-service>.onrender.com`
Base URL (local): `http://localhost:8000`

Interactive Swagger docs are auto-generated at `/docs` on any running instance.

All request/response bodies are JSON. All timestamps are ISO-8601 UTC.

## Authentication

Two separate credential types — see `backend/README.md#two-credential-types`
for the rationale.

### User JWT

```
Authorization: Bearer <token>
```

Obtained from `POST /api/v1/auth/login`. Expires after
`ACCESS_TOKEN_EXPIRE_MINUTES` (default 24h).

### Device key

```
X-Device-Key: <raw key, shown once at registration>
```

Some endpoints also need the device to identify itself:

```
X-Device-Id: <device id>       # only for endpoints with no {device_id} in the URL
```

---

## Auth

### `POST /api/v1/auth/register`
```json
{ "email": "you@example.com", "password": "at-least-8-chars" }
```
→ `201` `{ "id": "...", "email": "..." }`. `409` if email already registered.

### `POST /api/v1/auth/login`
```json
{ "email": "you@example.com", "password": "..." }
```
→ `200` `{ "access_token": "...", "token_type": "bearer" }`. `401` on bad credentials.

---

## Devices

### `POST /api/v1/devices/register` — requires user JWT
```json
{ "device_uid": "DESKBOT-01", "name": "Desk", "firmware_version": "1.0.0" }
```
→ `201`
```json
{ "id": "uuid", "device_uid": "DESKBOT-01", "name": "Desk", "device_key": "..." }
```
`device_key` is shown **only in this response**. `409` if `device_uid` is
already taken.

### `GET /api/v1/devices` — requires user JWT
Lists the caller's own devices, each with a computed `status`
(`ONLINE`/`OFFLINE`/`UNKNOWN` — see below).

### `GET /api/v1/devices/{device_id}` — requires user JWT
Single device, `404` if not found or not owned by caller.

### `POST /api/v1/devices/{device_id}/heartbeat` — requires `X-Device-Key`
Called by the ESP32 periodically (independent of message polling).
```json
{
  "firmware_version": "1.0.0",
  "wifi_connected": true,
  "ble_connected": false,
  "temperature_c": 28.4,
  "humidity_pct": 62.0
}
```
All fields optional — send whatever the firmware currently knows.
→ `200` `{ "status": "ok", "server_time": "..." }`.

**Status computation**: a device is `ONLINE` immediately after a heartbeat;
if no heartbeat has arrived in `HEARTBEAT_STALE_SECONDS` (90s, in
`device_service.py`) it's reported as `OFFLINE` on the next read — this is
computed at read time, not via a background sweep, to keep Phase 2 simple.

---

## Messages

### `POST /api/v1/messages` — requires user JWT
```json
{
  "device_id": "uuid",
  "body": "Team meeting at 11:00",
  "source": "APP",
  "sender": "You",
  "title": "Reminder",
  "priority": "NORMAL",
  "ttl_seconds": 600
}
```
`source` ∈ `APP | WHATSAPP | SYSTEM | SCHEDULED` (in practice, only `APP`
messages are created via this endpoint — `SCHEDULED` messages are created
internally by the scheduler; WhatsApp messages never touch this backend at
all, see below). `priority` ∈ `NORMAL | IMPORTANT | ALERT`.
→ `201`, the created message (`status: "PENDING"`). `404` if the device
isn't owned by the caller.

### `GET /api/v1/messages?device_id=...` — requires user JWT
Message history for the caller (optionally filtered to one device),
newest first, capped at 50.

### `GET /api/v1/messages/{message_id}` — requires user JWT

### `POST /api/v1/messages/{message_id}/read` — requires user JWT
Marks a message read (e.g. from the web dashboard).

### `GET /api/v1/devices/{device_id}/messages/pending` — requires `X-Device-Key`
**Polled by the ESP32**, suggested interval 5s (`DEVICE_POLL_INTERVAL_MS`,
configurable in firmware). Returns the single oldest `PENDING` message for
that device, or `null` if there's nothing — **always `200`, never `404`**,
so "nothing pending" is a normal response, not an error the firmware needs
to branch on specially. Returning it flips its status to `DELIVERED`.
Expired messages (`expires_at` in the past) are silently marked `EXPIRED`
and skipped before this check runs.

### `POST /api/v1/messages/{message_id}/ack` — requires `X-Device-Id` + `X-Device-Key`
Called by the ESP32 once it has displayed (or the phone has forwarded) the
message. Moves it from `DELIVERED`/`PENDING` → `ACKED`. `409` if already
in a terminal state, `404` if it doesn't belong to that device.

---

## Schedules

### `POST /api/v1/schedules` — requires user JWT
```json
{
  "device_id": "uuid",
  "message": "Good morning!",
  "scheduled_at": "2026-09-13T02:30:00Z",
  "repeat_rule": "DAILY"
}
```
`repeat_rule` ∈ `ONCE | DAILY | WEEKDAYS | WEEKLY`.

### `GET /api/v1/schedules?device_id=...` — requires user JWT

### `DELETE /api/v1/schedules/{schedule_id}` — requires user JWT
→ `204`.

**How firing works**: a background asyncio loop inside the API process
(`app/services/scheduler_service.py`, started from `main.py`'s lifespan)
wakes up every `SCHEDULER_POLL_INTERVAL_SECONDS` (default 30s), finds
enabled schedules whose `scheduled_at` has passed, creates a `SCHEDULED`
message (`PENDING`, ready for the device to poll) for each, and either
disables it (`ONCE`) or advances `scheduled_at` to the next occurrence
(`DAILY`/`WEEKLY`/`WEEKDAYS`, skipping weekends for the latter).

---

## Error format

FastAPI's default: `{"detail": "human-readable message"}`, with the
appropriate HTTP status code (400/401/404/409/422/429).

## Rate limiting

A simple in-memory sliding-window limiter (`app/rate_limit.py`) caps each
client IP at `RATE_LIMIT_PER_MINUTE` (default 120) requests/minute across
the whole API, returning `429` past that. Single-instance only — see the
note in that file about swapping in Redis if DeskBot ever scales beyond one
Render instance.

## Privacy & WhatsApp

**WhatsApp message content never reaches this backend.** Per the
architecture rule in the master spec (§11, §43), WhatsApp notifications
flow Android → BLE → ESP32 directly; the `Message` model and every
`/messages` endpoint here exist only for `APP`/`SYSTEM`/`SCHEDULED`
messages sent via the web app or API. If a future version ever wants to
mirror WhatsApp messages into this backend for cross-device history, that
must be an explicit, off-by-default, user-controlled opt-in — not a default
behavior.

## Real-time architecture

Phase 2 uses plain HTTPS polling from the ESP32 (see spec §30–31 — this was
an explicit design decision, not a shortcut). If push-based delivery is
wanted later:

- The device-facing contract (`GET .../messages/pending`, `POST
  .../ack`) doesn't need to change — a WebSocket or MQTT channel could be
  added *alongside* polling, with polling remaining as the fallback for
  flaky Wi-Fi, per the offline-behavior requirements in the spec.
- The `message_service` functions are already decoupled from HTTP, so a
  push transport can call `get_pending_for_device` from a different
  handler without touching route code.
