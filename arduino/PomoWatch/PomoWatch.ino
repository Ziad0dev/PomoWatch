// PomoWatch firmware for a Seeed XIAO ESP32-C3 with a 128x64 SSD1306 OLED,
// a coin vibration motor and one button. The timer logic lives in
// src/pomo.c (shared with the host tests and simulator).
//
// Libraries: Adafruit SSD1306, Adafruit GFX.
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

extern "C" {
#include "src/button.h"
#include "src/pomo.h"
}

constexpr int PIN_BUTTON = D1;  // to GND, internal pull-up
constexpr int PIN_MOTOR = D2;   // drives the motor through an N-MOSFET
constexpr uint32_t SCREEN_TIMEOUT_MS = 8000;

Adafruit_SSD1306 display(128, 64, &Wire, -1);
pomo_state timer;
button btn;
uint32_t last_ms;
uint32_t last_activity_ms;

// Non-blocking vibration patterns: alternating on/off durations in ms
const uint16_t TAP[] = {40, 0};
const uint16_t FOCUS_DONE[] = {200, 150, 200, 150, 200, 0};
const uint16_t BREAK_DONE[] = {600, 300, 600, 0};
const uint16_t *pattern = nullptr;
uint8_t pattern_step;
uint32_t step_started;

void start_vibe(pomo_vibe v) {
  pattern = v == POMO_VIBE_TAP ? TAP : v == POMO_VIBE_FOCUS_DONE ? FOCUS_DONE : v == POMO_VIBE_BREAK_DONE ? BREAK_DONE : nullptr;
  pattern_step = 0;
  step_started = millis();
  digitalWrite(PIN_MOTOR, pattern ? HIGH : LOW);
}

void update_vibe(uint32_t now) {
  if (!pattern) return;
  if (now - step_started < pattern[pattern_step]) return;
  pattern_step++;
  step_started = now;
  if (pattern[pattern_step] == 0) {
    pattern = nullptr;
    digitalWrite(PIN_MOTOR, LOW);
  } else {
    digitalWrite(PIN_MOTOR, pattern_step % 2 == 0 ? HIGH : LOW);
  }
}

void draw(uint32_t now) {
  bool awake = now - last_activity_ms < SCREEN_TIMEOUT_MS || timer.mode == POMO_IDLE;
  display.ssd1306_command(awake ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
  if (!awake) return;

  char clock_text[6];
  pomo_format_time(&timer, clock_text);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(pomo_mode_name(timer.mode == POMO_IDLE ? timer.next_mode : timer.mode));
  if (timer.mode == POMO_IDLE) display.print(" (press)");
  if (timer.paused) display.print(" PAUSED");
  display.setTextSize(3);
  display.setCursor(18, 20);
  display.print(clock_text);
  // progress bar and session dots
  display.drawRect(0, 54, 100, 8, SSD1306_WHITE);
  display.fillRect(0, 54, pomo_progress_permille(&timer) / 10, 8, SSD1306_WHITE);
  for (uint8_t i = 0; i < timer.cfg.sessions_per_long_break; i++) {
    uint8_t done = timer.completed % timer.cfg.sessions_per_long_break;
    int x = 106 + i * 6;
    if (i < done) display.fillCircle(x, 58, 2, SSD1306_WHITE);
    else display.drawCircle(x, 58, 2, SSD1306_WHITE);
  }
  display.display();
}

void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_MOTOR, OUTPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pomo_init(&timer, pomo_default_config());
  button_init(&btn, 25, 800);
  last_ms = last_activity_ms = millis();
}

void loop() {
  uint32_t now = millis();
  pomo_tick(&timer, now - last_ms);
  last_ms = now;

  switch (button_update(&btn, digitalRead(PIN_BUTTON) == LOW, now)) {
    case BUTTON_SHORT:
      // first press only wakes the screen
      if (now - last_activity_ms >= SCREEN_TIMEOUT_MS && timer.mode != POMO_IDLE) break;
      pomo_press(&timer);
      break;
    case BUTTON_LONG: pomo_long_press(&timer); break;
    default: break;
  }
  if (digitalRead(PIN_BUTTON) == LOW) last_activity_ms = now;

  pomo_vibe v = pomo_take_vibe(&timer);
  if (v != POMO_VIBE_NONE) {
    start_vibe(v);
    last_activity_ms = now;
  }
  update_vibe(now);
  draw(now);
  delay(10);  // light sleep would go here for battery life
}
