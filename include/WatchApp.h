#ifndef WATCH_APP_H
#define WATCH_APP_H

#include "App.h"
#include <pthread.h>

struct WatchApp : public App 
{
  WatchApp(App *next_app);

  void setup();
  void on_touch_down(uint32_t x, uint32_t y);
  void on_touch_up(uint32_t x, uint32_t y);
  void on_button_long_press();
  void on_step_counter_counted(uint32_t steps);
  void update();

private:
  uint32_t steps = 0;
  pthread_mutex_t steps_mutex;
};

#endif