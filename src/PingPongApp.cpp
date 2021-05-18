
#include "PingPongApp.h"
#include "WatchApp.h"
#include "matteo-face.h"

#include <pthread.h>

#include "matteo-watch.h"
#include <LilyGoWatch.h>
#include <TTGO.h>

extern TTGOClass *watch;
extern pthread_mutex_t watch_mutex;
extern void set_current_app(App *);
extern WatchApp watch_app;

void PingPongApp::setup() {
  pthread_mutex_lock(&watch_mutex);

  watch->tft->pushImage(0, 0, 240, 240, ((uint16_t *) matteo_face.pixel_data));

  pthread_mutex_unlock(&watch_mutex);
}

void PingPongApp::on_touch_down(uint16_t x, uint16_t y) {
  //Serial.println("ping pong touch down");
}

void PingPongApp::on_touch_up(uint16_t x, uint16_t y) {
  //Serial.println("ping pong touch up");
}

void PingPongApp::on_button_up() {
  //Serial.println("ping pong Button up");
  pthread_mutex_lock(&watch_mutex);
  watch->tft->invertDisplay(true);
  pthread_mutex_unlock(&watch_mutex);

  set_current_app(&watch_app);
}

void PingPongApp::on_button_long_press() {
  //Serial.println("ping pong Button long");
}

void PingPongApp::update() {}