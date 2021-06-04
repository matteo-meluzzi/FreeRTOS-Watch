
#ifndef APP_H
#define APP_H

#include <stdint.h>

struct App;

extern void set_current_app(App *);

struct App {
  App *next_app;
  App(App *next_app) {
    this->next_app = next_app;
  }

  virtual void setup() = 0;
  virtual void on_touch_down(uint32_t x, uint32_t y) = 0;
  virtual void on_touch_up(uint32_t x, uint32_t y) = 0;
  virtual void on_button_up() {
    set_current_app(next_app);
  };
  virtual void on_button_long_press() = 0;
  virtual void on_step_counter_counted(uint32_t steps) {};
  /**
   * called per second
   * */
  virtual void update() = 0;
};

#endif