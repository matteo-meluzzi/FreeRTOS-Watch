#ifndef BLUETOOTH_SPEAKER_APP_H
#define BLUETOOTH_SPEAKER_APP_H

#include "App.h"

struct BluetoothSpeakerApp: public App
{
  BluetoothSpeakerApp(App *next_app): App(next_app) {}

  void setup();
  void on_touch_down(uint32_t x, uint32_t y);
  void on_touch_up(uint32_t x, uint32_t y);
  void on_button_up();
  void on_button_long_press();
  void update();
};

#endif