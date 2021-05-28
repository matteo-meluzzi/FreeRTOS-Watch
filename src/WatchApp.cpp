#include "PingPongApp.h"
#include "WatchApp.h"

#include <pthread.h>

#include "matteo-watch.h"
#include <LilyGoWatch.h>
#include <TTGO.h>

extern TTGOClass *watch;
extern pthread_mutex_t watch_mutex;
extern void sleep_until_display_or_button_is_pressed();

void WatchApp::setup() {
  pthread_mutex_lock(&watch_mutex);

  watch->tft->setTextColor(TFT_WHITE, TFT_BLACK);
  watch->tft->fillScreen(TFT_BLACK);

  pthread_mutex_unlock(&watch_mutex);
  this->update();
}

void WatchApp::on_touch_down(uint16_t x, uint16_t y) {
  // Serial.println("watch Touch Down");
}

void WatchApp::on_touch_up(uint16_t x, uint16_t y) {
  // Serial.println("watch Touch Up");
  
  sleep_until_display_or_button_is_pressed();
}

void WatchApp::on_button_long_press() {
  //Serial.println("watch Button long");
}

void WatchApp::update() {
  pthread_mutex_lock(&watch_mutex);

  watch->tft->setTextSize(3);
  watch->tft->setCursor(30, 20);
  watch->tft->println(watch->rtc->formatDateTime(PCF_TIMEFORMAT_DD_MM_YYYY));
  watch->tft->println("");
  watch->tft->println("");
  watch->tft->setTextSize(5);
  watch->tft->println(watch->rtc->formatDateTime(PCF_TIMEFORMAT_HMS));
  watch->tft->println("");
  watch->tft->println("");

  watch->tft->setTextSize(2);
  watch->tft->setTextDatum(MC_DATUM);
  int percent = min(99, max(0, watch->power->getBattPercentage()));
  String percent_string = String(percent);
  String consumption = String(watch->power->getBattDischargeCurrent());
  watch->tft->drawString(percent_string + "% " + consumption + "mA", 120, 200);

  pthread_mutex_unlock(&watch_mutex);
}

/* example digital
#include "WatchApp.h"
#include "PingPongApp.h"

#include <pthread.h>

#include "matteo-watch.h"
#include <LilyGoWatch.h>
#include <TTGO.h>

extern TTGOClass *watch;
extern pthread_mutex_t watch_mutex;
extern void set_current_app(App *);
extern void sleep_until_display_or_button_is_pressed();
extern PingPongApp ping_pong_app;

byte omm = 99;
boolean initial = 1;
byte xcolon = 0;
unsigned int colour = 0;

static uint8_t conv2d(const char *p)
{
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';
}

uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3), ss = conv2d(__TIME__ + 6); // Get H, M, S from compile time

void WatchApp::setup() {
    pthread_mutex_lock(&watch_mutex);
    watch->tft->fillScreen(TFT_BLACK);

    watch->tft->setTextFont(1);
    watch->tft->fillScreen(TFT_BLACK);
    watch->tft->setTextColor(TFT_YELLOW, TFT_BLACK); // Note: the new fonts do not draw the background colour

    pthread_mutex_unlock(&watch_mutex);
    this->update();
}

void WatchApp::on_touch_down(uint16_t x, uint16_t y) {
  //Serial.println("watch Touch Down");
}

void WatchApp::on_touch_up(uint16_t x, uint16_t y) {
  //Serial.println("watch Touch Up");
  sleep_until_display_or_button_is_pressed();
}

void WatchApp::on_button_up() {
  //Serial.println("watch Button up");
  set_current_app(&ping_pong_app);
}

void WatchApp::on_button_long_press() {
  //Serial.println("watch Button long");
}

void WatchApp::update() {
  pthread_mutex_lock(&watch_mutex);

  ss++;              // Advance second
  if (ss == 60) {
      ss = 0;
      omm = mm;
      mm++;            // Advance minute
      if (mm > 59) {
          mm = 0;
          hh++;          // Advance hour
          if (hh > 23) {
              hh = 0;
          }
      }
  }

  if (ss == 0 || initial) {
      initial = 0;
      watch->tft->setTextColor(TFT_GREEN, TFT_BLACK);
      watch->tft->setCursor (8, 52);
      watch->tft->print(__DATE__); // This uses the standard ADAFruit small font

      watch->tft->setTextColor(TFT_BLUE, TFT_BLACK);
      watch->tft->drawCentreString("It is windy", 120, 48, 2); // Next size up font 2

      //watch->tft->setTextColor(0xF81F, TFT_BLACK); // Pink
      //watch->tft->drawCentreString("12.34",80,100,6); // Large font 6 only contains characters [space] 0 1 2 3 4 5 6 7 8 9 . : a p m
  }

  // Update digital time
  byte xpos = 6;
  byte ypos = 0;
  if (omm != mm) { // Only redraw every minute to minimise flicker
      // Uncomment ONE of the next 2 lines, using the ghost image demonstrates text overlay as time is drawn over it
      watch->tft->setTextColor(0x39C4, TFT_BLACK);  // Leave a 7 segment ghost image, comment out next line!
      //watch->tft->setTextColor(TFT_BLACK, TFT_BLACK); // Set font colour to black to wipe image
      // Font 7 is to show a pseudo 7 segment display.
      // Font 7 only contains characters [space] 0 1 2 3 4 5 6 7 8 9 0 : .
      watch->tft->drawString("88:88", xpos, ypos, 7); // Overwrite the text to clear it
      watch->tft->setTextColor(0xFBE0, TFT_BLACK); // Orange
      omm = mm;

      if (hh < 10) xpos += watch->tft->drawChar('0', xpos, ypos, 7);
      xpos += watch->tft->drawNumber(hh, xpos, ypos, 7);
      xcolon = xpos;
      xpos += watch->tft->drawChar(':', xpos, ypos, 7);
      if (mm < 10) xpos += watch->tft->drawChar('0', xpos, ypos, 7);
      watch->tft->drawNumber(mm, xpos, ypos, 7);
  }

  if (ss % 2) { // Flash the colon
      watch->tft->setTextColor(0x39C4, TFT_BLACK);
      xpos += watch->tft->drawChar(':', xcolon, ypos, 7);
      watch->tft->setTextColor(0xFBE0, TFT_BLACK);
  } else {
      watch->tft->drawChar(':', xcolon, ypos, 7);
      colour = random(0xFFFF);
      // Erase the old text with a rectangle, the disadvantage of this method is increased display flicker
      watch->tft->fillRect (0, 64, 160, 20, TFT_BLACK);
      watch->tft->setTextColor(colour);
      watch->tft->drawRightString("Colour", 75, 64, 4); // Right justified string drawing to x position 75
      String scolour = String(colour, HEX);
      scolour.toUpperCase();
      char buffer[20];
      scolour.toCharArray(buffer, 20);
      watch->tft->drawString(buffer, 82, 64, 4);
  }

  pthread_mutex_unlock(&watch_mutex);
}
*/

/* draw analog like in the example

#include "watch.h"
#include "ping-pong.h"

#include <pthread.h>

#include "matteo-watch.h"
#include <LilyGoWatch.h>
#include <TTGO.h>

extern TTGOClass *watch;
extern pthread_mutex_t watch_mutex;
extern void set_current_app(App *);
extern void sleep_until_display_or_button_is_pressed();
extern PingPongApp ping_pong_app;

float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;    // Saved H, M, S x & y multipliers
float sdeg = 0, mdeg = 0, hdeg = 0;
uint16_t osx = 120, osy = 120, omx = 120, omy = 120, ohx = 120, ohy = 120; // Saved H, M, S x & y coords
uint16_t x0 = 0, x1 = 0, yy0 = 0, yy1 = 0;

static uint8_t conv2d(const char *p); // Forward declaration needed for IDE 1.6.x
uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3), ss = conv2d(__TIME__ + 6); // Get H, M, S from compile time

boolean initial = 1;

static uint8_t conv2d(const char *p)
{
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';
}

void WatchApp::setup() {
    pthread_mutex_lock(&watch_mutex);
    watch->tft->fillScreen(TFT_BLACK);


    // Draw clock face
    watch->tft->fillCircle(120, 120, 118, TFT_WHITE);
    watch->tft->fillCircle(120, 120, 110, TFT_BLACK);

    // Draw 12 lines
    for (int i = 0; i < 360; i += 30) {
        sx = cos((i - 90) * 0.0174532925);
        sy = sin((i - 90) * 0.0174532925);
        x0 = sx * 114 + 120;
        yy0 = sy * 114 + 120;
        x1 = sx * 100 + 120;
        yy1 = sy * 100 + 120;

        watch->tft->drawLine(x0, yy0, x1, yy1, TFT_RED);
    }

    // Draw 60 dots
    for (int i = 0; i < 360; i += 6) {
        sx = cos((i - 90) * 0.0174532925);
        sy = sin((i - 90) * 0.0174532925);
        x0 = sx * 102 + 120;
        yy0 = sy * 102 + 120;
        // Draw minute markers
        watch->tft->drawPixel(x0, yy0, TFT_WHITE);

        // Draw main quadrant dots
        if (i == 0 || i == 180) watch->tft->fillCircle(x0, yy0, 2, TFT_WHITE);
        if (i == 90 || i == 270) watch->tft->fillCircle(x0, yy0, 2, TFT_WHITE);
    }

    watch->tft->fillCircle(120, 121, 3, TFT_WHITE);

    pthread_mutex_unlock(&watch_mutex);
    this->update();
}

void WatchApp::on_touch_down(uint16_t x, uint16_t y) {
  //Serial.println("watch Touch Down");
}

void WatchApp::on_touch_up(uint16_t x, uint16_t y) {
  //Serial.println("watch Touch Up");
  sleep_until_display_or_button_is_pressed();
}

void WatchApp::on_button_up() {
  //Serial.println("watch Button up");
  set_current_app(&ping_pong_app);
}

void WatchApp::on_button_long_press() {
  //Serial.println("watch Button long");
}

void WatchApp::update() {
  pthread_mutex_lock(&watch_mutex);

//   watch->tft->setTextSize(3);
//   watch->tft->setCursor(30, 20);
//   watch->tft->println(watch->rtc->formatDateTime(PCF_TIMEFORMAT_DD_MM_YYYY));
//   watch->tft->println("");
//   watch->tft->println("");
//   watch->tft->setTextSize(5);
//   watch->tft->println(watch->rtc->formatDateTime(PCF_TIMEFORMAT_HMS));

    ss++;              // Advance second
    if (ss == 60) {
        ss = 0;
        mm++;            // Advance minute
        if (mm > 59) {
            mm = 0;
            hh++;          // Advance hour
            if (hh > 23) {
                hh = 0;
            }
        }
    }

    // Pre-compute hand degrees, x & y coords for a fast screen update
    sdeg = ss * 6;                // 0-59 -> 0-354
    mdeg = mm * 6 + sdeg * 0.01666667; // 0-59 -> 0-360 - includes seconds
    hdeg = hh * 30 + mdeg * 0.0833333; // 0-11 -> 0-360 - includes minutes and seconds
    hx = cos((hdeg - 90) * 0.0174532925);
    hy = sin((hdeg - 90) * 0.0174532925);
    mx = cos((mdeg - 90) * 0.0174532925);
    my = sin((mdeg - 90) * 0.0174532925);
    sx = cos((sdeg - 90) * 0.0174532925);
    sy = sin((sdeg - 90) * 0.0174532925);

    if (ss == 0 || initial) {
        initial = 0;
        // Erase hour and minute hand positions every minute
        watch->tft->drawLine(ohx, ohy, 120, 121, TFT_BLACK);
        ohx = hx * 62 + 121;
        ohy = hy * 62 + 121;
        watch->tft->drawLine(omx, omy, 120, 121, TFT_BLACK);
        omx = mx * 84 + 120;
        omy = my * 84 + 121;
    }

    // Redraw new hand positions, hour and minute hands not erased here to avoid flicker
    watch->tft->drawLine(osx, osy, 120, 121, TFT_BLACK);
    osx = sx * 90 + 121;
    osy = sy * 90 + 121;
    watch->tft->drawLine(osx, osy, 120, 121, TFT_RED);
    watch->tft->drawLine(ohx, ohy, 120, 121, TFT_WHITE);
    watch->tft->drawLine(omx, omy, 120, 121, TFT_WHITE);
    watch->tft->drawLine(osx, osy, 120, 121, TFT_RED);
    watch->tft->fillCircle(120, 121, 3, TFT_RED);


pthread_mutex_unlock(&watch_mutex);
}

*/