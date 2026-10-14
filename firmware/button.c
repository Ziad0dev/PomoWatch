#include "button.h"

void button_init(button *b, uint16_t debounce_ms, uint16_t long_press_ms)
{
  b->debounce_ms = debounce_ms;
  b->long_press_ms = long_press_ms;
  b->stable = false;
  b->raw = false;
  b->raw_since = 0;
  b->pressed_at = 0;
  b->long_fired = false;
}

button_event button_update(button *b, bool raw_pressed, uint32_t now_ms)
{
  if (raw_pressed != b->raw) {
    b->raw = raw_pressed;
    b->raw_since = now_ms;
  }

  /* Long press fires while still held, so the user feels the buzz before letting go */
  if (b->stable && !b->long_fired && now_ms - b->pressed_at >= b->long_press_ms) {
    b->long_fired = true;
    return BUTTON_LONG;
  }

  if (b->raw == b->stable || now_ms - b->raw_since < b->debounce_ms)
    return BUTTON_NONE;

  b->stable = b->raw;
  if (b->stable) {
    b->pressed_at = now_ms;
    b->long_fired = false;
    return BUTTON_NONE;
  }
  /* released */
  return b->long_fired ? BUTTON_NONE : BUTTON_SHORT;
}
