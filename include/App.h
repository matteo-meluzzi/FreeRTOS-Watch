
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
  virtual void on_touch_down(uint16_t x, uint16_t y) = 0;
  virtual void on_touch_up(uint16_t x, uint16_t y) = 0;
  virtual void on_button_up() {
    set_current_app(next_app);
  };
  virtual void on_button_long_press() = 0;
  /**
   * called per second
   * */
  virtual void update() = 0;
};

#endif