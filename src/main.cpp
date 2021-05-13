
#define LILYGO_WATCH_2020_V1

#include <LilyGoWatch.h>
#include <TTGO.h>

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

void draw_watch(TFT_eSPI *tft) {
  tft->drawCircle(120, 120, 119, TFT_RED);
  //tft->drawLine(0, 0, 240, 240, TFT_BLUE);
  tft->drawThickLine(0, 0, 240, 240, 5, TFT_BLUE);
}

void loop()
{
  Serial.println("loop");

  watch->tft->begin();
  watch->tft->fillScreen(TFT_BLACK);

  watch->openBL();

  watch->tft->setCursor(0, 0);
  watch->tft->setTextSize(3);
  watch->tft->println("Matteo genius");

  draw_watch(watch->tft);

  for (;;) {
    if (irq_axp) {
      irq_axp = false;
      watch->power->readIRQ();
      
      if (watch->power->isPEKLongPressIRQ()) {
        watch->power->clearIRQ();
        break;
      }
      watch->power->clearIRQ();
    }
    // uncomment to have the watch sleep after 5 seconds
    // if (xTaskGetTickCount() - last_woke_up_ticks >= 5000) { 
    //   break;
    // }
  }

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