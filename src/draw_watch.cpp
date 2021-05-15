

#include "matteo_watch.h"
#include <LilyGoWatch.h>
#include <TTGO.h>

#include "draw_watch.h"

uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
  return ((red & 0b00011111000) << 8) | ((green & 0b00011111100) << 3) | (blue >> 3);
}

void draw_hour_ticks(TFT_eSPI *tft, uint8_t cx, uint8_t cy, uint8_t r1, uint8_t r2, uint16_t color) {
  for (uint16_t h = 0; h < 12; h++) {
    tft->drawTick(cx, cy, r1, r2, h * 30.0, color);
  }
}

void draw_minute_ticks(TFT_eSPI *tft, uint8_t cx, uint8_t cy, uint8_t r1, uint8_t r2, uint16_t color) {
  for (uint16_t m = 0; m < 60; m++) {
    tft->drawTick(cx, cy, r1, r2, m * 6.0, color);
  }
}

void draw_watch(TFT_eSPI *tft) {
  draw_minute_ticks(tft, 120, 120, 117, 107, rgb565(128, 128, 128));
  draw_hour_ticks(tft, 120, 120, 117, 90, rgb565(128, 128, 128));

  double second = 45.0;
  double minute = 15.0;
  double hour = 11.0;

  // hours
  tft->drawThickTick(120, 120, 0, 16, 360.0 / 12.0 * (1.0 * hour + minute / 60.0), 1, rgb565(255, 255, 255));
  tft->drawThickTick(120, 120, 16, 60, 360.0 / 12.0 * (1.0 * hour + minute / 60.0), 4, rgb565(255, 255, 255));

  // minutes
  tft->drawThickTick(120, 120, 0, 16, 360.0 / 60.0 * (1.0 * minute + second / 60.0), 1, rgb565(255, 255, 255));
  tft->drawThickTick(120, 120, 16, 105, 360.0 / 60.0 * (1.0 * minute + second / 60.0), 4, rgb565(255, 255, 255));
  
  // seconds
  tft->fillCircle(120, 120, 3, rgb565(255, 0, 0));
  tft->drawThickTick(120, 120, 0, 16, 360.0 / 60.0 * second, 1, rgb565(255, 0, 0));
  tft->drawThickTick(120, 120, 0, 110, 360.0 / 60.0 * second, 1, rgb565(255, 0, 0));
}