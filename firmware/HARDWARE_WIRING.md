# DeskBot — Hardware Wiring (Phase 1)

**Board:** ESP32-S3 Super Mini
**Note:** ESP32-S3 Super Mini boards from different sellers do not always
label pins identically, and some silkscreens are wrong. Always confirm
against your board's actual pinout diagram before wiring, and treat the
GPIO numbers below as the `config.h` defaults you'll adjust to match.

## Components

| Component | Interface | Notes |
|---|---|---|
| SH1106 OLED, 128x64 | I2C | 3.3V logic |
| DHT22 | 1-wire digital | needs a 4.7k–10k pull-up on DATA if your module doesn't already have one |
| TTP223 touch sensor | digital, HIGH when touched | module usually has onboard pull-down |
| Buzzer | digital output | passive buzzer recommended for the tone patterns used |

## Default pin map (see `config.h` to change)

| Signal | ESP32-S3 GPIO | Wire to |
|---|---|---|
| OLED SDA | GPIO 8 | OLED SDA |
| OLED SCL | GPIO 9 | OLED SCL |
| OLED VCC | 3V3 | OLED VCC |
| OLED GND | GND | OLED GND |
| DHT22 DATA | GPIO 4 | DHT22 DATA (pin 2) |
| DHT22 VCC | 3V3 | DHT22 VCC (pin 1) |
| DHT22 GND | GND | DHT22 GND (pin 4) |
| TTP223 SIG | GPIO 5 | TTP223 OUT/SIG |
| TTP223 VCC | 3V3 | TTP223 VCC |
| TTP223 GND | GND | TTP223 GND |
| Buzzer + | GPIO 6 | Buzzer + |
| Buzzer − | GND | Buzzer − |

## Voltage considerations

- The ESP32-S3's GPIOs are **3.3V only** — do not wire a 5V-logic OLED or
  sensor module directly. Most SH1106/DHT22/TTP223 breakout modules sold
  for hobbyist use already run fine at 3.3V; check your specific module's
  markings.
- If your DHT22 module has no onboard pull-up resistor, add a 4.7kΩ–10kΩ
  resistor between DATA and 3V3.
- Keep OLED I2C wires short (a few cm) at 400kHz; if you see garbled frames,
  drop `OLED_I2C_CLK_HZ` in `config.h` to `100000`.

## After wiring

1. Open `config.h` and update any pins that don't match your actual wiring.
2. Set `WIFI_SSID` / `WIFI_PASSWORD`.
3. Flash `DeskBot.ino` (see the root `README.md` for library list and board
   settings).
4. Watch the Serial Monitor at 115200 baud during first boot — every
   manager logs a one-line "ready" message, which is the fastest way to
   spot a wiring or library problem.
