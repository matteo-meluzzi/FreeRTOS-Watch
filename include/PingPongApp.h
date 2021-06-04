
#ifndef PING_PONG_APP_H
#define PING_PONG_APP_H

#include "App.h"

struct PingPongApp: public App
 {
  PingPongApp(App *next_app): App(next_app) {}

  void setup();
  void on_touch_down(uint32_t x, uint32_t y);
  void on_touch_up(uint32_t x, uint32_t y);
  void on_button_up();
  void on_button_long_press();
  void update();

private:
  long score1 = 0;
  long score2 = 0;

  void set_score1(long score1);
  void set_score2(long score2);

  void draw_score1();
  void draw_score2();
  void draw_button1();
  void draw_button1_pressed();
  void draw_button2();
  void draw_button2_pressed();
};

#endif