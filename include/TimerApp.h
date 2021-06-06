#ifndef TIMER_APP_H
#define TIMER_APP_H

#include "App.h"

#include <pthread.h>
#include <esp_timer.h>

class AudioFileSourcePROGMEM;
class AudioGeneratorWAV;
class AudioOutputI2S;
class TTGOClass;

struct TimerApp: public App {
  TimerApp(App *next_app): App(next_app) {};

  void setup();
  void on_touch_down(uint32_t x, uint32_t y);
  void on_touch_up(uint32_t x, uint32_t y);
  void on_button_up();
  void on_button_long_press();
  void on_step_counter_counted(uint32_t steps);
  void update();

  AudioFileSourcePROGMEM *file;
  AudioGeneratorWAV *generator;
  pthread_mutex_t generator_mutex;
  AudioOutputI2S *out;
  esp_timer_handle_t loop_audio_timer;
};

#endif