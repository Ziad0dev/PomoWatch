#include "pomo.h"

pomo_config pomo_default_config(void)
{
  pomo_config c = {
    .focus_ms = 25u * 60u * 1000u,
    .short_break_ms = 5u * 60u * 1000u,
    .long_break_ms = 15u * 60u * 1000u,
    .sessions_per_long_break = 4,
    .auto_start_next = false,
  };
  return c;
}

static uint32_t phase_length(const pomo_config *c, pomo_mode m)
{
  switch (m) {
  case POMO_FOCUS: return c->focus_ms;
  case POMO_SHORT_BREAK: return c->short_break_ms;
  case POMO_LONG_BREAK: return c->long_break_ms;
  default: return 0;
  }
}

static void enter(pomo_state *s, pomo_mode m)
{
  s->mode = m;
  s->paused = false;
  s->phase_ms = phase_length(&s->cfg, m);
  s->remaining_ms = s->phase_ms;
}

/* Which phase follows the one that just ended. */
static pomo_mode after(const pomo_state *s, pomo_mode finished)
{
  if (finished != POMO_FOCUS)
    return POMO_FOCUS;
  uint8_t n = s->cfg.sessions_per_long_break ? s->cfg.sessions_per_long_break : 4;
  return (s->completed % n == 0) ? POMO_LONG_BREAK : POMO_SHORT_BREAK;
}

void pomo_init(pomo_state *s, pomo_config cfg)
{
  s->cfg = cfg;
  s->mode = POMO_IDLE;
  s->next_mode = POMO_FOCUS;
  s->paused = false;
  s->remaining_ms = cfg.focus_ms;
  s->phase_ms = cfg.focus_ms;
  s->completed = 0;
  s->pending_vibe = POMO_VIBE_NONE;
}

static void finish_phase(pomo_state *s)
{
  pomo_mode finished = s->mode;
  if (finished == POMO_FOCUS)
    s->completed++;
  s->pending_vibe = finished == POMO_FOCUS ? POMO_VIBE_FOCUS_DONE : POMO_VIBE_BREAK_DONE;
  s->next_mode = after(s, finished);
  if (s->cfg.auto_start_next) {
    enter(s, s->next_mode);
  } else {
    s->mode = POMO_IDLE;
    s->paused = false;
    s->phase_ms = phase_length(&s->cfg, s->next_mode);
    s->remaining_ms = s->phase_ms;
  }
}

bool pomo_tick(pomo_state *s, uint32_t elapsed_ms)
{
  if (s->mode == POMO_IDLE || s->paused)
    return false;
  if (elapsed_ms < s->remaining_ms) {
    s->remaining_ms -= elapsed_ms;
    return false;
  }
  /* Carry leftover time into the next phase only when it starts automatically */
  uint32_t leftover = elapsed_ms - s->remaining_ms;
  finish_phase(s);
  if (s->mode != POMO_IDLE && leftover)
    pomo_tick(s, leftover);
  return true;
}

void pomo_press(pomo_state *s)
{
  if (s->mode == POMO_IDLE)
    enter(s, s->next_mode);
  else
    s->paused = !s->paused;
  s->pending_vibe = POMO_VIBE_TAP;
}

void pomo_long_press(pomo_state *s)
{
  if (s->mode == POMO_IDLE) {
    /* reset the whole cycle */
    pomo_init(s, s->cfg);
  } else {
    /* skip: a skipped focus session doesn't count as completed */
    pomo_mode skipped = s->mode;
    s->next_mode = skipped == POMO_FOCUS ? POMO_SHORT_BREAK : POMO_FOCUS;
    s->mode = POMO_IDLE;
    s->paused = false;
    s->phase_ms = phase_length(&s->cfg, s->next_mode);
    s->remaining_ms = s->phase_ms;
  }
  s->pending_vibe = POMO_VIBE_TAP;
}

pomo_vibe pomo_take_vibe(pomo_state *s)
{
  pomo_vibe v = s->pending_vibe;
  s->pending_vibe = POMO_VIBE_NONE;
  return v;
}

void pomo_format_time(const pomo_state *s, char *buf)
{
  /* round up so the display never shows 00:00 while time is left */
  uint32_t secs = (s->remaining_ms + 999u) / 1000u;
  uint32_t m = secs / 60u, sec = secs % 60u;
  if (m > 99u)
    m = 99u;
  buf[0] = (char)('0' + m / 10u);
  buf[1] = (char)('0' + m % 10u);
  buf[2] = ':';
  buf[3] = (char)('0' + sec / 10u);
  buf[4] = (char)('0' + sec % 10u);
  buf[5] = '\0';
}

const char *pomo_mode_name(pomo_mode m)
{
  switch (m) {
  case POMO_FOCUS: return "FOCUS";
  case POMO_SHORT_BREAK: return "BREAK";
  case POMO_LONG_BREAK: return "LONG BREAK";
  default: return "READY";
  }
}

uint16_t pomo_progress_permille(const pomo_state *s)
{
  if (s->phase_ms == 0 || s->mode == POMO_IDLE)
    return 0;
  return (uint16_t)(((uint64_t)(s->phase_ms - s->remaining_ms) * 1000u) / s->phase_ms);
}
