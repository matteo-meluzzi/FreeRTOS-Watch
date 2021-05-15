

#include "matteo_watch.h"

#include <LilyGoWatch.h>
#include <TTGO.h>
#include <freertos/semphr.h>

#include "config.h"

#include "draw_watch.h"
#include "button_state.h"

TTGOClass *watch;

SemaphoreHandle_t button_semaphore = NULL;
StaticSemaphore_t button_semaphore_buffer;

TickType_t last_woke_up_ticks = 0;

struct WatchState {
  button_statefp button = not_pressed;
};
WatchState watch_state;
typedef voidfp (* watch_statefp)(WatchState state);

void read_button_task(void *args)
{
  for (;;) {
    Serial.println("reading button")
  }
}

void setup()
{
  button_semaphore = xSemaphoreCreateBinaryStatic(&button_semaphore_buffer);
  assert(button_semaphore);

  Serial.begin(115200);
  Serial.println("Hi!");
  // Get TTGOClass instance
  watch = TTGOClass::getWatch();

  // Initialize the hardware, the BMA423 sensor has been initialized internally
  watch->begin();
  watch->bl->adjust(255);

  pinMode(AXP202_INT, INPUT_PULLUP);
  attachInterrupt(AXP202_INT, [] {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(button_semaphore, &xHigherPriorityTaskWoken);

    Serial.println("change");
  }, FALLING);

  // set touch panel in interrupt polling mode
  // see https://www.buydisplay.com/download/ic/FT6236-FT6336-FT6436L-FT6436_Datasheet.pdf
  // paragraph 1.2  
  watch->touch->disableINT(); 
  pinMode(TOUCH_INT, INPUT);
  attachInterrupt(TOUCH_INT, [] {
    Serial.print("touched: ");
    Serial.println(digitalRead(TOUCH_INT));
  }, CHANGE);

  watch->power->enableIRQ(AXP202_PEK_FALLING_EDGE_IRQ, true);
  watch->power->enableIRQ(AXP202_PEK_RISING_EDGE_IRQ, true);
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

void update_button_pressed() {
  watch->power->readIRQ();

  bool is_long = watch->power->isPEKLongPressIRQ();
  bool is_short = watch->power->isPEKShortPressIRQ();
  
  watch_state.button = (button_statefp) watch_state.button(is_long, is_short);
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
  assert(button_semaphore);
  for (;;) {
    if (xSemaphoreTake(button_semaphore, portMAX_DELAY) == pdTRUE) { 
      update_button_pressed();

      Serial.print("Button state: ");
      if (watch_state.button == not_pressed) {
        Serial.println("not pressed");
      } else if (watch_state.button == pressed) {
        Serial.println("pressed");
      } else {
        Serial.println("long pressed");
      }

      watch->power->clearIRQ();
    }
  }

  Serial.println("go to sleep");
  sleep_until_display_or_button_is_pressed();

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