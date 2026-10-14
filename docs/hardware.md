# PomoWatch hardware (prototype v0)

## Bill of materials
| Part | Notes | ~Price |
| --- | --- | --- |
| Seeed Studio XIAO ESP32-C3 | 21×17.5 mm, built-in LiPo charger, BLE for a future phone sync | $5 |
| 0.96" SSD1306 OLED, 128×64, I²C | address 0x3C | $3 |
| 10 mm coin vibration motor (3 V) | | $1 |
| AO3400 N-MOSFET + 1N4148 flyback diode + 1 kΩ gate resistor | motor driver; the GPIO can't drive the motor directly | $0.5 |
| 6×6 mm tactile button | wired to GND, internal pull-up | $0.1 |
| 3.7 V LiPo, ~200 mAh (e.g. 402030) | | $4 |
| 22 mm watch strap + 3D-printed case | | $5 |

## Wiring
```
XIAO D1 ── button ── GND
XIAO D2 ── 1kΩ ── AO3400 gate
                  AO3400 drain ── motor (−)      motor (+) ── 3V3
                  AO3400 source ── GND           1N4148 across the motor (cathode to 3V3)
XIAO D4 (SDA), D5 (SCL), 3V3, GND ── SSD1306
LiPo ── XIAO BAT+ / BAT− pads
```

## Power budget (rough)
- ESP32-C3 active with Wi-Fi/BLE off: ~20 mA; with light sleep between 10 ms ticks: ~1–2 mA
- OLED on: ~10 mA, so it turns off after 8 s and a button press wakes it
- motor: ~80 mA for well under a second per phase change

With light sleep and the screen mostly off, a 200 mAh cell should last several days of use; the next firmware step is light sleep + wake on button GPIO.

## Gestures
- short press: start / pause / resume (when the screen is off, it only wakes it)
- long press (0.8 s): skip the current phase; when idle, reset the cycle
- vibration: tap = acknowledged, 3 short = focus done, 2 long = break over
