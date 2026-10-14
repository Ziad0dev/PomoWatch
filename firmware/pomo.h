/*
 * Hardware-independent pomodoro core for PomoWatch.
 *
 * The firmware feeds it elapsed time and button gestures; it returns what to
 * show and when to buzz. No dynamic memory, no floating point, no OS.
 */
#ifndef POMO_H
#define POMO_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { POMO_IDLE, POMO_FOCUS, POMO_SHORT_BREAK, POMO_LONG_BREAK } pomo_mode;

typedef enum {
  POMO_VIBE_NONE,
  POMO_VIBE_TAP,        /* short acknowledgement */
  POMO_VIBE_FOCUS_DONE, /* three pulses: take a break */
  POMO_VIBE_BREAK_DONE, /* two long pulses: back to work */
} pomo_vibe;

typedef struct {
  uint32_t focus_ms;
  uint32_t short_break_ms;
  uint32_t long_break_ms;
  uint8_t sessions_per_long_break;
  bool auto_start_next; /* start the next phase without a button press */
} pomo_config;

typedef struct {
  pomo_config cfg;
  pomo_mode mode;
  pomo_mode next_mode;     /* what IDLE will start next */
  bool paused;
  uint32_t remaining_ms;
  uint32_t phase_ms;       /* full length of the current phase */
  uint16_t completed;      /* focus sessions finished today */
  pomo_vibe pending_vibe;
} pomo_state;

pomo_config pomo_default_config(void);
void pomo_init(pomo_state *s, pomo_config cfg);

/* Advance time; returns true when a phase finished during this tick. */
bool pomo_tick(pomo_state *s, uint32_t elapsed_ms);

/* Short press: start / pause / resume. */
void pomo_press(pomo_state *s);
/* Long press: skip to the next phase (or reset the cycle when idle). */
void pomo_long_press(pomo_state *s);

/* Take (and clear) the vibration request. */
pomo_vibe pomo_take_vibe(pomo_state *s);

/* "25:00" style text into buf (at least 6 bytes). */
void pomo_format_time(const pomo_state *s, char *buf);
const char *pomo_mode_name(pomo_mode m);
/* Progress through the current phase, 0..1000. */
uint16_t pomo_progress_permille(const pomo_state *s);

#endif
