#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../firmware/button.h"
#include "../firmware/pomo.h"

#define MIN (60u * 1000u)

static void test_full_cycle(void)
{
  pomo_state s;
  pomo_init(&s, pomo_default_config());
  assert(s.mode == POMO_IDLE);

  for (int i = 1; i <= 4; i++) {
    pomo_press(&s);
    assert(s.mode == POMO_FOCUS);
    assert(pomo_tick(&s, 25 * MIN));
    assert(pomo_take_vibe(&s) == POMO_VIBE_FOCUS_DONE);
    assert(s.completed == (uint16_t)i);
    assert(s.mode == POMO_IDLE);
    pomo_press(&s);
    /* every 4th break is a long one */
    assert(s.mode == (i == 4 ? POMO_LONG_BREAK : POMO_SHORT_BREAK));
    assert(pomo_tick(&s, i == 4 ? 15 * MIN : 5 * MIN));
    assert(pomo_take_vibe(&s) == POMO_VIBE_BREAK_DONE);
  }
}

static void test_pause_and_format(void)
{
  pomo_state s;
  char buf[6];
  pomo_init(&s, pomo_default_config());
  pomo_press(&s);
  pomo_tick(&s, 90 * 1000 + 1);
  pomo_format_time(&s, buf);
  assert(strcmp(buf, "23:30") == 0); /* 23:29.999 rounds up */
  pomo_press(&s);                    /* pause */
  pomo_tick(&s, 10 * MIN);
  pomo_format_time(&s, buf);
  assert(strcmp(buf, "23:30") == 0);
  pomo_press(&s); /* resume */
  pomo_tick(&s, 30 * 1000 - 1);
  pomo_format_time(&s, buf);
  assert(strcmp(buf, "23:00") == 0);
  assert(pomo_progress_permille(&s) == 80);
}

static void test_skip_and_reset(void)
{
  pomo_state s;
  pomo_init(&s, pomo_default_config());
  pomo_press(&s);
  pomo_long_press(&s); /* skip focus */
  assert(s.completed == 0);
  assert(s.mode == POMO_IDLE && s.next_mode == POMO_SHORT_BREAK);
  pomo_long_press(&s); /* reset */
  assert(s.next_mode == POMO_FOCUS);
}

static void test_auto_start_carries_time(void)
{
  pomo_config c = pomo_default_config();
  c.auto_start_next = true;
  pomo_state s;
  pomo_init(&s, c);
  pomo_press(&s);
  pomo_tick(&s, 26 * MIN); /* 1 minute into the break */
  assert(s.mode == POMO_SHORT_BREAK);
  assert(s.remaining_ms == 4 * MIN);
}

static void test_button(void)
{
  button b;
  button_init(&b, 20, 800);
  /* bouncing contact: no event until stable for 20 ms */
  assert(button_update(&b, true, 0) == BUTTON_NONE);
  assert(button_update(&b, false, 5) == BUTTON_NONE);
  assert(button_update(&b, true, 8) == BUTTON_NONE);
  assert(button_update(&b, true, 30) == BUTTON_NONE); /* now pressed */
  assert(button_update(&b, false, 200) == BUTTON_NONE);
  assert(button_update(&b, false, 225) == BUTTON_SHORT);

  /* long press fires while held, release afterwards is silent */
  assert(button_update(&b, true, 1000) == BUTTON_NONE);
  assert(button_update(&b, true, 1025) == BUTTON_NONE);
  assert(button_update(&b, true, 1830) == BUTTON_LONG);
  assert(button_update(&b, true, 2500) == BUTTON_NONE);
  assert(button_update(&b, false, 2600) == BUTTON_NONE);
  assert(button_update(&b, false, 2625) == BUTTON_NONE);
}

int main(void)
{
  test_full_cycle();
  test_pause_and_format();
  test_skip_and_reset();
  test_auto_start_carries_time();
  test_button();
  puts("all tests passed");
  return 0;
}
