
#include "PingPongApp.h"
#include "BluetoothSpeakerApp.h"
// #include "matteo-face.h"

#include <pthread.h>
#include "matteo-watch.h"
#include <LilyGoWatch.h>
#include <TTGO.h>

extern TTGOClass *watch;
extern pthread_mutex_t watch_mutex;

constexpr uint32_t buttons_radius = 50;

void PingPongApp::setup() {
  pthread_mutex_lock(&watch_mutex);

  // watch->tft->pushImage(0, 0, 240, 240, ((uint16_t *) matteo_face.pixel_data));

  watch->tft->fillScreen(TFT_BLACK);
  watch->tft->setTextColor(TFT_WHITE, TFT_BLACK); // white text with black background
  watch->tft->setTextDatum(MC_DATUM); // the string should be in the middle
  watch->tft->setTextSize(5);

  pthread_mutex_unlock(&watch_mutex);

  this->draw_button1();
  this->draw_button2();
  this->draw_score1();
  this->draw_score2();
}

void PingPongApp::draw_button1() {
  pthread_mutex_lock(&watch_mutex);
  watch->tft->fillCircle(180, 60, buttons_radius, TFT_RED);
  pthread_mutex_unlock(&watch_mutex);
}

void PingPongApp::draw_button1_pressed() {
  pthread_mutex_lock(&watch_mutex);
  watch->tft->fillCircle(180, 60, buttons_radius, TFT_MAROON);
  pthread_mutex_unlock(&watch_mutex);
}

void PingPongApp::draw_button2() {
  pthread_mutex_lock(&watch_mutex);
  watch->tft->fillCircle(180, 180, buttons_radius, TFT_GREEN);
  pthread_mutex_unlock(&watch_mutex);
}

void PingPongApp::draw_button2_pressed() {
  pthread_mutex_lock(&watch_mutex);
  watch->tft->fillCircle(180, 180, buttons_radius, TFT_DARKGREEN);
  pthread_mutex_unlock(&watch_mutex);
}

void PingPongApp::draw_score1() {
  pthread_mutex_lock(&watch_mutex);
  watch->tft->drawString("  ", 60, 60);
  watch->tft->drawNumber(this->score1, 60, 60);
  pthread_mutex_unlock(&watch_mutex);
}

void PingPongApp::draw_score2() {
  pthread_mutex_lock(&watch_mutex);
  watch->tft->drawString("  ", 60, 180);
  watch->tft->drawNumber(this->score2, 60, 180);
  pthread_mutex_unlock(&watch_mutex);
}

void PingPongApp::set_score1(long score1) {
  this->score1 = max(0l, min(99l, score1));
  draw_score1();
}

void PingPongApp::set_score2(long score2) {
  this->score2 = max(0l, min(99l, score2));
  draw_score2();
}

void PingPongApp::on_touch_down(uint16_t x, uint16_t y) {
  if (x < 120 && y < 120) set_score1(this->score1 - 1);
  else if (x < 120 && y >= 120) set_score2(this->score2 - 1);
  else if (x >= 120 && y < 120) {
    set_score1(this->score1 + 1);
    this->draw_button1_pressed();
  } 
  else {
    set_score2(this->score2 + 1);
    this->draw_button2_pressed();
  }
}

void PingPongApp::on_touch_up(uint16_t x, uint16_t y) {
  // pthread_mutex_lock(&watch_mutex);
  // watch->tft->fillScreen(TFT_BLACK);
  // pthread_mutex_unlock(&watch_mutex);

  this->draw_button1();
  this->draw_button2();
}

void PingPongApp::on_button_up() {
  //Serial.println("ping pong Button up");
  // pthread_mutex_lock(&watch_mutex);
  // watch->tft->invertDisplay(true);
  // pthread_mutex_unlock(&watch_mutex);

  App::on_button_up();
}

void PingPongApp::on_button_long_press() {
  set_score1(0);
  set_score2(0);
}

void PingPongApp::update() {}