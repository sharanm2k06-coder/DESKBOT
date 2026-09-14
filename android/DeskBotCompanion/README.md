# DeskBot Companion (Android)

Kotlin + Jetpack Compose + Material 3 app that connects a phone to a
DeskBot over BLE and forwards WhatsApp notifications to it. Package:
`com.deskbot.companion`. Min SDK 26.

## What it does (and deliberately doesn't)

- **Does**: scan for and connect to a DeskBot over BLE (`ble/BleManager.kt`),
  detect WhatsApp notifications via `NotificationListenerService`
  (`notifications/`), forward them over BLE while the app is backgrounded
  (`service/DeskBotForegroundService.kt`), and let you sign in / register a
  device against the Phase-2 backend (`network/ApiClient.kt`).
- **Doesn't**: send scheduled or ad-hoc messages to the backend on the
  user's behalf — that's the web console's job (see `ui/screens/MessagesScreen.kt`'s
  and `SchedulesScreen.kt`'s doc comments for why). This app's `MessagesScreen`
  only offers a direct BLE test-send, useful for confirming the connection
  works without the web app open.
- **Never**: logs into WhatsApp, reads its database, or stores message
  bodies on disk. See `docs/BLE_PROTOCOL.md` → "Security considerations".

## Project layout

```
app/src/main/java/com/deskbot/companion/
  MainActivity.kt              # permission requests, Compose host
  DeskBotApp.kt
  data/
    model/                     # NotificationMessage, DeskBotDevice, etc.
    local/AppPreferences.kt    # DataStore: forwarding toggle, keep-connected, last device
  network/
    ApiClient.kt               # OkHttp client for the Phase-2 backend
    ApiService.kt
  ble/
    BleManager.kt              # scan/connect/reconnect/send+ack
    BleDevice.kt                # GATT UUIDs
    BleProtocol.kt             # packet format, CRC16, fragmentation, reassembly
  notifications/
    DeskBotNotificationListenerService.kt
    NotificationRepository.kt
  service/
    DeskBotForegroundService.kt
    BleManagerHolder.kt
  security/
    SecureStorage.kt           # EncryptedSharedPreferences for the backend JWT
  ui/
    AppNavHost.kt
    screens/                   # Splash, Home, Devices, Messages, WhatsApp, Schedules, Settings, About
    components/                # Panel, StatusPill, ScreenScaffold
    theme/                     # Color.kt, Theme.kt, Type.kt
```

## Design plan

Same visual language as the web frontend (see `../../frontend/README.md`) —
monochrome dark surfaces, hairline borders, JetBrains-Mono-style monospace
for technical/status text (the platform's built-in `FontFamily.Monospace`
is used rather than bundling font files, to keep this buildable without
extra binary assets — swap in a real JetBrains Mono `.ttf` under `res/font/`
if you want pixel-parity with the web app), color reserved for status only.

## Permissions

Requested at runtime, only what's needed for the running OS version
(`MainActivity.requestRuntimePermissions()`):

| Permission | Why | OS version |
|---|---|---|
| `BLUETOOTH_SCAN` / `BLUETOOTH_CONNECT` | BLE scan & connect | API 31+ |
| `ACCESS_FINE_LOCATION` | BLE scan requires it on older stacks | API < 31 |
| `POST_NOTIFICATIONS` | foreground-service + "forwarded" notifications | API 33+ |

**Not** requestable via the runtime dialog — these need an explicit trip to
system Settings, handled with in-app buttons:
- Notification listener access (`ui/screens/WhatsAppScreen.kt` → opens
  `Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS`)

## Building

This was written in a sandbox with no network access, so **the Gradle
wrapper JAR is not included** (it's a binary normally fetched via
`gradle wrapper`) and `./gradlew` cannot be run here. To build:

```bash
cd android/DeskBotCompanion
gradle wrapper --gradle-version 8.7   # generates gradlew + the wrapper jar, one time
./gradlew assembleDebug
```

Or open the `android/DeskBotCompanion` folder directly in Android Studio
(Iguana+) — it will offer to generate the wrapper for you and sync Gradle
automatically. Either way, an internet connection is required for Gradle
to resolve the dependencies listed in `app/build.gradle.kts`.

**This code has not been compiled.** Every `.kt` file was reviewed by hand
against the exact Kotlin/AndroidX/Compose APIs it uses, and all 31 files
pass a bracket-balance sanity check, but please run `./gradlew assembleDebug`
(or open in Android Studio and let it build) as your first step, and fix
whatever the compiler flags before relying on this for real hardware.

## Running against the backend

1. Start/deploy the Phase-2 backend (`../../backend`).
2. In the app's Settings screen, set **API URL** — for the Android
   emulator talking to a backend running on your host machine, use
   `http://10.0.2.2:8000` (the default already in `SettingsScreen.kt`); for
   a physical phone, use your machine's LAN IP or the deployed Render URL.
3. Register/sign in from the same screen.
4. Register a device from the web console (`../../frontend`) or extend
   `SettingsScreen` to call `ApiClient.registerDevice()` — the client
   method already exists, it's just not wired to a button yet in this
   pass.

## Running the BLE side without real ESP32 hardware yet

`ui/screens/DevicesScreen.kt` scans for any BLE peripheral advertising the
DeskBot service UUID (`docs/BLE_PROTOCOL.md`). Until Phase 5 wires the
ESP32's `BLEManager.cpp` up to actually advertise + implement this GATT
server, there's nothing for it to find — that integration is exactly what
Phase 5 covers.

## Tests

```bash
./gradlew testDebugUnitTest
```

- `ble/BleProtocolTest.kt` — fragmentation, out-of-order reassembly, CRC
  corruption detection, unsupported-version rejection, single-fragment
  short-circuit.
- `notifications/NotificationMessageTest.kt` — JSON round-trip, and that
  missing/malformed fields degrade to `null`/sane defaults rather than
  throwing (per master spec §45's "do not crash if fields are missing").

Both test classes are plain JVM unit tests (no emulator needed) — the
`org.json:json` test dependency in `app/build.gradle.kts` exists solely so
`NotificationMessageTest` runs against a real JSON implementation instead
of Android's unit-test stub, which throws on every call by default.

## Known limitations / left for later phases

- **OTA updates**: not in scope for the phone app either — see master spec
  §62 and `docs/DEPLOYMENT.md`.
- **Battery level**: the Home screen has a slot for it in the design intent
  (master spec §14) but no battery data is currently reported over BLE —
  add a field to the `STATUS_UPDATE` payload in `docs/BLE_PROTOCOL.md` and
  firmware first.
- **Multiple notification sources** (Telegram/Gmail/Instagram/SMS/Teams):
  the architecture point for this is `DeskBotNotificationListenerService.SUPPORTED_PACKAGES`
  — add a package name + `MessageSource`, no other code needs to change.
- **Device registration button**: `ApiClient.registerDevice()` exists but
  isn't yet wired to a Settings-screen button (see "Running against the
  backend" above) — the fastest path today is registering from the web
  console and pasting the resulting device key into `config.h` on the
  ESP32 directly.
