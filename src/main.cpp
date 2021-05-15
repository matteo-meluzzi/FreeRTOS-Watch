

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
button_statefp button_state = not_pressed;
typedef voidfp (* watch_statefp)(button_statefp button_pressed);

SemaphoreHandle_t touch_semaphore = NULL;
StaticSemaphore_t touch_semaphore_buffer;

void read_button_task(void *args);
void read_touch_task(void *args);

void setup()
{
  button_semaphore = xSemaphoreCreateBinaryStatic(&button_semaphore_buffer);
  assert(button_semaphore);
  touch_semaphore = xSemaphoreCreateBinaryStatic(&touch_semaphore_buffer);
  assert(touch_semaphore);

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

    // Serial.println("button state change");
  }, FALLING);

  // set touch panel in interrupt polling mode
  // see https://www.buydisplay.com/download/ic/FT6236-FT6336-FT6436L-FT6436_Datasheet.pdf
  // paragraph 1.2  
  watch->touch->disableINT(); 
  pinMode(TOUCH_INT, INPUT);
  attachInterrupt(TOUCH_INT, [] {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(touch_semaphore, &xHigherPriorityTaskWoken);

    // Serial.print("touched: ");
    // Serial.println(digitalRead(TOUCH_INT));
  }, CHANGE);

  watch->power->enableIRQ(AXP202_PEK_FALLING_EDGE_IRQ, true);
  watch->power->enableIRQ(AXP202_PEK_RISING_EDGE_IRQ, true);
  watch->power->clearIRQ();

  xTaskCreate(read_button_task, "read_button_task", 1024, NULL, 1, NULL);
  xTaskCreate(read_touch_task, "read_touch_task", 1024, NULL, 1, NULL);
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

void read_touch_task(void *args)
{
  for (;;) {
    if (xSemaphoreTake(touch_semaphore, portMAX_DELAY) == pdTRUE) {
      Serial.print("touched: ");
      Serial.println(watch->touch->getTouched());
    }
  }
}

void read_button_task(void *args)
{
  for (;;) {
    if (xSemaphoreTake(button_semaphore, portMAX_DELAY) == pdTRUE) { 
      watch->power->readIRQ();

      bool is_long = watch->power->isPEKLongPressIRQ();
      bool is_short = watch->power->isPEKShortPressIRQ();
      
      button_state = (button_statefp) button_state(is_long, is_short);

      Serial.print("Button state: ");
      if (button_state == not_pressed) {
        Serial.println("not pressed");
      } else if (button_state == pressed) {
        Serial.println("pressed");
      } else {
        Serial.println("long pressed");
      }

      watch->power->clearIRQ();
    }
  }
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
    delay(1000);
    // if (xSemaphoreTake(button_semaphore, portMAX_DELAY) == pdTRUE) { 
    //   update_button_pressed();

    //   Serial.print("Button state: ");
    //   if (watch_state.button == not_pressed) {
    //     Serial.println("not pressed");
    //   } else if (watch_state.button == pressed) {
    //     Serial.println("pressed");
    //   } else {
    //     Serial.println("long pressed");
    //   }

    //   watch->power->clearIRQ();
    // }
  }

  Serial.println("go to sleep");
  sleep_until_display_or_button_is_pressed();

  watch->power->clearIRQ();
}