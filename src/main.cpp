

#include "matteo_watch.h"

#include <LilyGoWatch.h>
#include <TTGO.h>
#include <freertos/semphr.h>

#include "config.h"

#include "draw_watch.h"
#include "button_state.h"

#include <pthread.h>

TTGOClass *watch;

void sleep_until_display_or_button_is_pressed();

struct WatchInput {
  bool button_down = false;
  bool button_up_short = false;
  bool button_up_long = false;
  bool touch_press_down = false;
  bool touch_press_up = false;
};
typedef voidfp (* watch_statefp)(WatchInput *watch_input);
voidfp off(WatchInput *watch_input);
voidfp show_time(WatchInput *watch_input);
voidfp show_time_pressing(WatchInput *watch_input);

voidfp off(WatchInput *watch_input) {
  return (voidfp) show_time;
}
voidfp show_time(WatchInput *watch_input) {
  if (watch_input->touch_press_up) return (voidfp) off;
  if (watch_input->button_down) return (voidfp) show_time_pressing;

  return (voidfp) show_time;
}
voidfp show_time_pressing(WatchInput *watch_input) {
  if (watch_input->button_up_long) return (voidfp) off;
  if (watch_input->button_up_short) return (voidfp) show_time;

  return (voidfp) show_time_pressing;
}

watch_statefp watch_state = show_time;

WatchInput watch_input;
pthread_mutex_t watch_input_mutex;
pthread_cond_t watch_input_cond;

SemaphoreHandle_t button_semaphore = NULL;
StaticSemaphore_t button_semaphore_buffer;
button_statefp button_state = not_pressed;

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
  assert(!pthread_mutex_init(&watch_input_mutex, NULL));
  assert(!pthread_cond_init(&watch_input_cond, NULL));

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

  watch->power->clearIRQ();
}

void read_touch_task(void *args)
{
  for (;;) {
    if (xSemaphoreTake(touch_semaphore, portMAX_DELAY) == pdTRUE) {
      // Serial.print("touched: ");
      // Serial.println(watch->touch->getTouched());

      pthread_mutex_lock(&watch_input_mutex);
      watch_input.touch_press_down = watch->touch->getTouched();
      watch_input.touch_press_up = !watch_input.touch_press_down;
      pthread_cond_signal(&watch_input_cond);
      pthread_mutex_unlock(&watch_input_mutex);
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
      
      button_statefp old_state = button_state;
      button_state = (button_statefp) button_state(is_long, is_short);

      pthread_mutex_lock(&watch_input_mutex);
      Serial.print("Button state: ");
      if (button_state == not_pressed) {
        Serial.println("released");

        watch_input.button_down = false;
        if (old_state == pressed_long) watch_input.button_up_long = true;
        else if (old_state == pressed) watch_input.button_up_short = true;

        pthread_cond_signal(&watch_input_cond);
      } else if (button_state == pressed) {
        Serial.println("pressed");

        watch_input.button_down = true;
        watch_input.button_up_long = false;
        watch_input.button_up_short = false;

        pthread_cond_signal(&watch_input_cond);
      } else if (button_state == pressed_long) {
        Serial.println("long pressed");
      }
      pthread_mutex_unlock(&watch_input_mutex);

      watch->power->clearIRQ();
    }
  }
}

void loop()
{
loop_init:
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

  // Serial.println("entering for (;;)");
  for (;;) {
    pthread_mutex_lock(&watch_input_mutex);
    pthread_cond_wait(&watch_input_cond, &watch_input_mutex);

    Serial.println("");
    Serial.println("state changed: ");
    Serial.print("pressed: ");
    Serial.println(watch_input.button_down);
    Serial.print("short release: ");
    Serial.println(watch_input.button_up_short);
    Serial.print("long release: ");
    Serial.println(watch_input.button_up_long);
    Serial.print("touch down: ");
    Serial.println(watch_input.touch_press_down);
    Serial.print("touch up: ");
    Serial.println(watch_input.touch_press_up);

    watch_state = (watch_statefp) watch_state(&watch_input);
    if (watch_state == off) puts("off");
    else if (watch_state == show_time) puts("show_time");
    else if (watch_state == show_time_pressing) puts("show_time_pressing");
    // else if (watch_state == ping_pong) puts("ping_pong");
    // else if (watch_state == ping_pong_pressing) puts("ping_pong_pressing");
    else puts("unknown state");

    if (watch_state == off) {
      memset((void *) (&watch_input), 0, sizeof(watch_input));
      sleep_until_display_or_button_is_pressed();
      watch_state = (watch_statefp) watch_state(&watch_input);
      pthread_mutex_unlock(&watch_input_mutex);
      goto loop_init;
    }

    pthread_mutex_unlock(&watch_input_mutex);
  }

  Serial.println("go to sleep");
  sleep_until_display_or_button_is_pressed();

}