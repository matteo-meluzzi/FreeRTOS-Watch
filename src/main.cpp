
#define LILYGO_WATCH_2020_V1

#include <LilyGoWatch.h>
#include <TTGO.h>
#include <pthread.h>

#include "config.h"

TTGOClass *watch;

bool irq_axp = false;
TickType_t last_woke_up_ticks = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("Hi!");
  // Get TTGOClass instance
  watch = TTGOClass::getWatch();

  // Initialize the hardware, the BMA423 sensor has been initialized internally
  watch->begin();
  watch->bl->adjust(255);

  pinMode(AXP202_INT, INPUT_PULLUP);
  attachInterrupt(AXP202_INT, [] {
    irq_axp = true;
    Serial.println("irq_axp = true");
  }, FALLING);
  watch->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
  watch->power->clearIRQ();

  last_woke_up_ticks = xTaskGetTickCount();
}

void sleep_until_display_or_button_is_pressed() 
{
  watch->power->clearIRQ();

  watch->displaySleep();
  watch->powerOff();
  watch->bl->off();

  esp_sleep_enable_ext0_wakeup((gpio_num_t) AXP202_INT, LOW);
  esp_sleep_enable_ext1_wakeup(GPIO_SEL_38, ESP_EXT1_WAKEUP_ALL_LOW);

  esp_light_sleep_start();
}

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

void loop()
{
  Serial.println("loop");

  watch->tft->begin();
  watch->openBL();
  watch->tft->fillScreen(TFT_BLACK);

  Serial.println("draw_watch");
  draw_watch(watch->tft);

  Serial.println("Matteo genius");
  watch->tft->setCursor(0, 0);
  watch->tft->setTextSize(3);
  watch->tft->println("Matteo genius");

  Serial.println("for (;;)");
  for (;;) {
    if (irq_axp) {
      Serial.println("irq_axp is true");
      irq_axp = false;
      watch->power->readIRQ();
      
      if (watch->power->isPEKLongPressIRQ()) {
        watch->power->clearIRQ();
        Serial.println("a long press");
        break;
      }
      Serial.println("not a long press");
      watch->power->clearIRQ();
    }
    // uncomment to have the watch sleep after 5 seconds
    if (xTaskGetTickCount() - last_woke_up_ticks >= 5000) { 
      break;
    }
    sleep(1);
  }

  Serial.println("go to sleep");
  sleep_until_display_or_button_is_pressed();
  irq_axp = false;
  watch->power->clearIRQ();
  last_woke_up_ticks = xTaskGetTickCount();
}




/*


#define LILYGO_WATCH_2020_V1

#include <LilyGoWatch.h>
#include <TTGO.h>

#include "config.h"

TTGOClass *watch;

bool irq_axp = false;

void setup()
{
  Serial.begin(115200);
  Serial.println("Hi!");
  // Get TTGOClass instance
  watch = TTGOClass::getWatch();

  // Initialize the hardware, the BMA423 sensor has been initialized internally
  watch->begin();

  // Turn on the backlight
  watch->openBL();

  pinMode(AXP202_INT, INPUT_PULLUP);
  attachInterrupt(AXP202_INT, [] {
    irq_axp = true;
    Serial.println("irq_axp = true");
  }, FALLING);
  watch->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
  watch->power->clearIRQ();
}

void sleep_until_display_is_pressed() {
  watch->power->clearIRQ();

  // Set screen and touch to sleep mode
  watch->displaySleep();
  watch->bl->off();

  esp_sleep_enable_ext0_wakeup((gpio_num_t)AXP202_INT, LOW);
  // esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);

  esp_light_sleep_start();
  irq_axp = false;
  watch->power->clearIRQ();
  Serial.println("irq_axp = false");
}

void loop()
{
  Serial.println("loop");

  watch->tft->begin();
  watch->tft->setCursor(0, 0);
  watch->tft->fillScreen(TFT_BLACK);

  watch->openBL();

  watch->tft->println("Wait for the PEKKey interrupt to come...");
  Serial.println("Wait for the PEKKey interrupt to come");

  // Wait for the power button to be pressed
  while (!irq_axp) {
    Serial.println(xTaskGetTickCount());
    sleep(1);
  }
  Serial.println("Button pressed");

  sleep_until_display_is_pressed();
}







*/