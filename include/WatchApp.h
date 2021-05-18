#ifndef WATCH_APP_H
#define WATCH_APP_H

#include "App.h"

struct WatchApp : App {
  void setup();
  void on_touch_down(uint16_t x, uint16_t y);
  void on_touch_up(uint16_t x, uint16_t y);
  void on_button_up();
  void on_button_long_press();
  void update();
};

#endif