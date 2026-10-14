# PomoWatch
Designs and code for the pomodoro smart watch that i am building for private use. Might consider building more or putting a how to guide to building your own. 

## Layout
- `firmware/`: portable C11 core: the pomodoro state machine (`pomo.c`) and debounced short/long button presses (`button.c`). No heap, no floats, no OS
- `arduino/PomoWatch/`: sketch for a XIAO ESP32-C3 + SSD1306 OLED + vibration motor (links to the core in `firmware/`)
- `sim/`: terminal simulator running the real core at 60× speed
- `tests/`: host unit tests
- `docs/hardware.md`: parts, wiring, power budget

```sh
make test     # host tests
make sim      # Enter = press, l = long press, q = quit
```
Classic cycle: 25 min focus, 5 min break, long 15 min break after every 4th session; all configurable in `pomo_config`.
