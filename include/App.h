
#ifndef APP_H
#define APP_H

#include <stdint.h>

struct App {
  virtual void setup() = 0;
  virtual void on_touch_down(uint16_t x, uint16_t y) = 0;
  virtual void on_touch_up(uint16_t x, uint16_t y) = 0;
  virtual void on_button_up() = 0;
  virtual void on_button_long_press() = 0;
  /**
   * called per second
   * */
  virtual void update() = 0;
};

#endif