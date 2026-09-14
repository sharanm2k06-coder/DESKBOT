# DeskBot BLE Protocol

Version: **1** (the `version` byte in every packet — bump this and branch on
it if the format ever changes, rather than assuming both ends upgrade
simultaneously).

This document is the single source of truth for both the ESP32 firmware
(`firmware/BLEManager.cpp`, extended in Phase 5) and the Android companion
(`android/.../ble/BleProtocol.kt`). If the two ever disagree, this file
wins — fix whichever side drifted.

## Service & characteristics

Matches `firmware/config.h`:

| Name | UUID | Properties | Direction |
|---|---|---|---|
| DeskBot Service | `7d4a0001-8c4b-4f5b-9e11-123456789abc` | — | — |
| Device Info | `7d4a0002-8c4b-4f5b-9e11-123456789abc` | Read | ESP32 → phone |
| Command | `7d4a0003-8c4b-4f5b-9e11-123456789abc` | Write | Phone → ESP32 |
| Message | `7d4a0004-8c4b-4f5b-9e11-123456789abc` | Write | Phone → ESP32 |
| Status | `7d4a0005-8c4b-4f5b-9e11-123456789abc` | Notify | ESP32 → phone |

The Client Characteristic Configuration Descriptor (CCCD,
`00002902-0000-1000-8000-00805f9b34fb`) must be written on **Status** to
enable notifications — this is how the phone receives message-received
acks and device state changes.

## Why one channel handles both WhatsApp forwarding and app commands

The **Message** characteristic carries every phone→device payload
(WhatsApp-sourced notifications *and* short app-originated test messages);
the **type** byte in the packet header (below) tells the ESP32 which kind
it is so `MessageManager`/`NotificationManager` can route it correctly.
This keeps the GATT table small and matches the Phase-1 skeleton already
exposed in firmware.

## Packet format

Every write to **Message** or **Command**, and every notification on
**Status**, is one *fragment* of this format:

```
Offset  Size  Field
0       1     version            (always 1 for this spec revision)
1       1     type               (see Message Types below)
2       4     message_id         uint32, little-endian — same value on every fragment of one message
6       1     sequence           uint8, 0-based index of this fragment
7       1     total              uint8, total number of fragments for this message_id
8       2     payload_length     uint16, little-endian — length of `payload` in THIS fragment
10      N     payload            UTF-8 bytes (message text) or binary (commands/status)
10+N    2     crc16              uint16, little-endian, CRC-16/CCITT-FALSE over bytes [0 .. 10+N)
```

Fixed overhead per fragment: **12 bytes** (10-byte header + 2-byte CRC).

### Maximum packet size & fragmentation

`BLE_MAX_SINGLE_PACKET_LEN` in `config.h` is **180 bytes** — chosen to sit
safely under the negotiated MTU on most phones/stacks without relying on a
successful MTU-negotiation handshake. Maximum payload per fragment is
therefore `180 - 12 = 168` bytes.

A message longer than 168 bytes (e.g. a long WhatsApp message plus sender
name) is split into multiple fragments, all sharing the same
`message_id`, numbered `sequence = 0..total-1`. The receiver buffers
fragments by `message_id` and reassembles once it has seen `total` of
them. `BleProtocol.kt`'s `fragment()`/`Reassembler` implement exactly this;
firmware's Phase-5 `BLEManager` reassembly must mirror it.

### CRC16

CRC-16/CCITT-FALSE: poly `0x1021`, init `0xFFFF`, no reflection, no final
XOR. Reference implementation in `BleProtocol.kt` (`crc16`). A fragment
that fails CRC is dropped silently by the receiver (not NAKed at the
fragment level — see retry/timeout below).

## Message types (the `type` byte)

| Value | Name | Payload | Sent on |
|---|---|---|---|
| 1 | `MESSAGE` | UTF-8 JSON: `{"source","sender","title","body","timestamp"}` | Message char (phone→ESP32) |
| 2 | `ACK` | UTF-8: the `message_id` (as decimal string) being acknowledged | Status char (ESP32→phone) |
| 3 | `COMMAND` | UTF-8 JSON: `{"cmd": "..."}` — e.g. `{"cmd":"clear_queue"}` | Command char (phone→ESP32) |
| 4 | `PING` | empty | either direction, connection liveness check |
| 5 | `STATUS_UPDATE` | UTF-8 JSON: `{"wifi","ble","battery"}` | Status char (ESP32→phone) |

A `MESSAGE` payload's JSON mirrors the `NotificationMessage` object Android
builds in `notifications/DeskBotNotificationListenerService.kt` and the
`Message` object in the master spec (§10):

```json
{
  "source": "WHATSAPP",
  "sender": "Rahul",
  "title": "WhatsApp",
  "body": "Are you coming to the meeting?",
  "timestamp": 1732400000000
}
```

## Acknowledgement & retry

1. Phone sends all fragments of a `MESSAGE` (or `COMMAND`), back-to-back,
   with no fragment-level ACK (BLE writes are already delivery-confirmed
   at the link layer when using **Write Request**, not **Write Without
   Response** — this protocol assumes Write Request for exactly that
   reason).
2. Once the ESP32 has reassembled all fragments for a `message_id`, it
   enqueues the message and sends an `ACK` notification on **Status**
   carrying that `message_id`.
3. If the phone doesn't see an `ACK` within **3 seconds**, it retries the
   full fragment set once. If still no `ACK` after a second timeout, the
   send is reported as failed to the caller (`BleManager` surfaces this as
   a `SendResult.Failed`) — there's no infinite retry loop, so a genuinely
   unreachable device fails fast rather than hanging the UI.
4. **Duplicate detection**: if the ESP32 receives a complete, valid
   message with a `message_id` it already has in its recently-seen ring
   (`MessageManager::wasRecentlySeen`, Phase 1), it re-sends the `ACK`
   without re-enqueuing — this is what makes retry #3 safe.

## Reconnection behavior

The Android app's `BleManager` is the side responsible for reconnecting
(the ESP32 simply advertises and accepts). Backoff schedule (matches
master spec §47):

```
1s → 2s → 5s → 10s → 30s (then stays at 30s)
```

Backoff resets to 1s on any successful connection. Retrying stops when the
user disables the device connection in the app (Home screen toggle) — it
does **not** stop just because a few attempts failed, since a phone
walking out of range and back is the common case, not an error.

## Security considerations

- **No pairing/bonding is required for Phase 4** — GATT traffic on most
  Android/ESP32 BLE stacks is not encrypted by default without bonding,
  so treat this as an unauthenticated local link, appropriate for a
  same-room desk accessory, not for sensitive payloads beyond what's
  already visible as a phone notification anyway.
- WhatsApp message bodies are **never written to disk** (Android side: no
  logging, no Room/SQLite persistence of `body`; only kept in memory long
  enough to fragment and send — see `NotificationRepository`). ESP32 side:
  `MessageManager` is RAM-only, per Phase 1.
- The Command channel accepts only a small fixed set of JSON `cmd` values
  (allow-list, not free-form) — this is enforced on the ESP32 side in
  Phase 5's `BLEManager`, and the app never needs to send anything else.
- No API keys, Wi-Fi credentials, or backend tokens are ever sent over
  BLE — the ESP32's HTTPS credentials to the Render backend are configured
  separately (`config.h` / a future provisioning flow), not via this
  protocol.

## Versioning

If a future revision changes the header layout, bump `version` and have
both ends refuse (not crash on) packets whose `version` they don't
recognize — `BleProtocol.kt`'s parser already does this
(`ParseResult.UnsupportedVersion`).
