#ifndef TIMER_APP_H
#define TIMER_APP_H

#include "App.h"

struct TimerApp: public App {
  TimerApp(App *next_app): App(next_app) {};

  void setup();
  void on_touch_down(uint32_t x, uint32_t y);
  void on_touch_up(uint32_t x, uint32_t y);
  void on_button_up();
  void on_button_long_press();
  void on_step_counter_counted(uint32_t steps);
  void update();
};

#endif