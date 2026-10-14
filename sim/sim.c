/*
 * Terminal simulator: runs the real firmware core at 60x speed.
 *   Enter       short press (start / pause / resume)
 *   l + Enter   long press (skip / reset)
 *   q + Enter   quit
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#include "../firmware/pomo.h"

#define SPEEDUP 60u

static uint32_t now_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

static const char *vibe_text(pomo_vibe v)
{
  switch (v) {
  case POMO_VIBE_TAP: return "bzz";
  case POMO_VIBE_FOCUS_DONE: return "BZZ BZZ BZZ  (focus done, take a break)";
  case POMO_VIBE_BREAK_DONE: return "BZZZZ BZZZZ  (break over)";
  default: return "";
  }
}

int main(void)
{
  pomo_state s;
  pomo_init(&s, pomo_default_config());
  uint32_t last = now_ms();
  char line[16], clock_text[6], bar[21];
  puts("PomoWatch simulator (60x speed). Enter = press, l = long press, q = quit");

  for (;;) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = {0, 100000};
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
      if (!fgets(line, sizeof line, stdin) || line[0] == 'q')
        break;
      if (line[0] == 'l')
        pomo_long_press(&s);
      else
        pomo_press(&s);
    }
    uint32_t t = now_ms();
    pomo_tick(&s, (t - last) * SPEEDUP);
    last = t;

    pomo_format_time(&s, clock_text);
    uint16_t p = pomo_progress_permille(&s);
    for (int i = 0; i < 20; i++)
      bar[i] = i < p / 50 ? '#' : '.';
    bar[20] = '\0';
    pomo_vibe v = pomo_take_vibe(&s);
    printf("\r%-10s %s [%s] done:%u %s%-40s", pomo_mode_name(s.mode),
           clock_text, bar, s.completed, s.paused ? "(paused) " : "", vibe_text(v));
    fflush(stdout);
  }
  putchar('\n');
  return 0;
}
