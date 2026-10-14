/* Debounced single-button input with short / long press detection. */
#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { BUTTON_NONE, BUTTON_SHORT, BUTTON_LONG } button_event;

typedef struct {
  uint16_t debounce_ms;
  uint16_t long_press_ms;
  bool stable;          /* debounced level, true = pressed */
  bool raw;
  uint32_t raw_since;   /* when raw last changed */
  uint32_t pressed_at;
  bool long_fired;
} button;

void button_init(button *b, uint16_t debounce_ms, uint16_t long_press_ms);
/* Call regularly with the raw pin level and a millisecond clock. */
button_event button_update(button *b, bool raw_pressed, uint32_t now_ms);

#endif
