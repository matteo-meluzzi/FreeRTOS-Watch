

#include "matteo_watch.h"

#include <LilyGoWatch.h>
#include <TTGO.h>
#include <freertos/semphr.h>

#include "config.h"

#include "draw_watch.h"
#include "button_state.h"

#include <pthread.h>

TTGOClass *watch;
pthread_mutex_t watch_mutex;

void sleep_until_display_or_button_is_pressed();
void wake_up();
TickType_t last_woke_up_ticks = 0;

SemaphoreHandle_t button_semaphore = NULL;
button_statefp button_state = not_pressed;

SemaphoreHandle_t touch_semaphore = NULL;

QueueHandle_t event_queue = NULL;
// typedef void (* eventfp)(uint16_t, uint16_t);
typedef enum {
  TOUCH_DOWN_EVENT,
  TOUCH_UP_EVENT,
  BUTTON_UP_EVENT,
  BUTTON_LONG_PRESS_EVENT
} EventType;

struct Event {
  EventType type;
  uint16_t param1;
  uint16_t param2;
};

struct App {
  virtual void setup() = 0;
  virtual void on_touch_down(uint16_t x, uint16_t y) = 0;
  virtual void on_touch_up(uint16_t x, uint16_t y) = 0;
  virtual void on_button_up() = 0;
  virtual void on_button_long_press() = 0;
  /**
   * called per second
   * */
  virtual void update() = 0;
};
App *current_app;

void set_current_app(App *new_app) {
  current_app = new_app;
  current_app->setup();
}

struct WatchApp : App {
  void setup();
  void on_touch_down(uint16_t x, uint16_t y);
  void on_touch_up(uint16_t x, uint16_t y);
  void on_button_up();
  void on_button_long_press();
  void update();
};
WatchApp watch_app = {};

struct PingPongApp: App {
  void setup();
  void on_touch_down(uint16_t x, uint16_t y);
  void on_touch_up(uint16_t x, uint16_t y);
  void on_button_up();
  void on_button_long_press();
  void update();
};
PingPongApp ping_pong_app = {};

void WatchApp::setup() {
  this->update();
}

void WatchApp::on_touch_down(uint16_t x, uint16_t y) {
  Serial.print("Touch Down: ");
  Serial.print(x);
  Serial.print(" ");
  Serial.println(y);
}

void WatchApp::on_touch_up(uint16_t x, uint16_t y) {
  Serial.print("Touch Up: ");
  Serial.print(x);
  Serial.print(" ");
  Serial.println(y);
  if (xTaskGetTickCount() - last_woke_up_ticks > 250) {
    Serial.println("Going to sleep. Bye!");
    sleep_until_display_or_button_is_pressed();
  }
}

void WatchApp::on_button_up() {
  Serial.println("Button up");
  set_current_app(&ping_pong_app);
}

void WatchApp::on_button_long_press() {
  Serial.println("Button long");
}

void WatchApp::update() {
  pthread_mutex_lock(&watch_mutex);

  // RTC_Date date = watch->rtc->getDateTime();
  // double hour = (double) (date.hour % 12);
  // double minute = (double) date.minute;
  // double second = (double) date.second;

  // Serial.print("hr: ");
  // Serial.print(hour);
  // Serial.print("min: ");
  // Serial.print(minute);
  // Serial.print("sec: ");
  // Serial.println(second);
  
  //draw_watch(watch->tft, hour, minute, second);

  watch->tft->setTextSize(3);
  watch->tft->setCursor(30, 20);
  watch->tft->println(watch->rtc->formatDateTime(PCF_TIMEFORMAT_DD_MM_YYYY));
  watch->tft->println("");
  watch->tft->println("");
  // watch->tft->println("");
  watch->tft->setTextSize(5);
  // watch->tft->setCursor(20, watch->tft->getCursorY());
  watch->tft->println(watch->rtc->formatDateTime(PCF_TIMEFORMAT_HMS));

  pthread_mutex_unlock(&watch_mutex);
}

void PingPongApp::setup() {
  pthread_mutex_lock(&watch_mutex);

  watch->tft->fillScreen(TFT_RED);

  watch->tft->setCursor(0, 0);
  watch->tft->setTextSize(3);
  watch->tft->println("Ping Pong Counter");

  pthread_mutex_unlock(&watch_mutex);
}
void PingPongApp::on_touch_down(uint16_t x, uint16_t y) {
}

void PingPongApp::on_touch_up(uint16_t x, uint16_t y) {
}

void PingPongApp::on_button_up() {
  set_current_app(&watch_app);
}

void PingPongApp::on_button_long_press() {
}

void PingPongApp::update() {}

void read_button_task(void *args);
void read_touch_task(void *args);

esp_timer_handle_t one_second_timer;
void every_second(void *arg) {
  current_app->update();
}

void setup()
{
  ESP_ERROR_CHECK(pthread_mutex_init(&watch_mutex, NULL));
  button_semaphore = xSemaphoreCreateBinary();
  assert(button_semaphore);
  touch_semaphore = xSemaphoreCreateBinary();
  assert(touch_semaphore);
  event_queue = xQueueCreate(25, sizeof(Event));
  assert(event_queue);

  current_app = &watch_app;
  assert(current_app);

  Serial.begin(115200);
  Serial.println("Hi!");
  // Serial.println(sizeof(voidfp));
  // Serial.println(sizeof(std::function<void(void)>));

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

  esp_timer_init();
  esp_timer_create_args_t args = {every_second, nullptr, ESP_TIMER_TASK, "one second timer"};
  ESP_ERROR_CHECK(esp_timer_create(&args, &one_second_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(one_second_timer, 1000000));

  xTaskCreate(read_button_task, "read_button_task", 2048, NULL, 1, NULL);
  xTaskCreate(read_touch_task, "read_touch_task", 2048, NULL, 1, NULL);
}

void read_touch_task(void *args)
{
  for (;;) {
    if (xSemaphoreTake(touch_semaphore, portMAX_DELAY) == pdTRUE) {
      pthread_mutex_lock(&watch_mutex);

      bool touched = watch->touch->getTouched();
      uint16_t x, y;
      watch->touch->getPoint(x, y);

      pthread_mutex_unlock(&watch_mutex);

      EventType t = touched ? TOUCH_DOWN_EVENT : TOUCH_UP_EVENT;
      Event e = {t, x, y};
      xQueueSendToBack(event_queue, (const void *) &e, (TickType_t) 0);
    }
  }
}

void read_button_task(void *args)
{
  for (;;) {
    if (xSemaphoreTake(button_semaphore, portMAX_DELAY) == pdTRUE) {
      pthread_mutex_lock(&watch_mutex);
      watch->power->readIRQ();

      bool is_long = watch->power->isPEKLongPressIRQ();
      bool is_short = watch->power->isPEKShortPressIRQ();

      watch->power->clearIRQ();
      pthread_mutex_unlock(&watch_mutex);

      button_statefp old_state = button_state;
      button_state = (button_statefp) button_state(is_long, is_short);

      if (button_state == not_pressed && old_state == pressed) {
        Event e = {BUTTON_UP_EVENT, 0, 0};
        xQueueSendToBack(event_queue, (const void *) &e, (TickType_t) 0);
      }
      else if (button_state == pressed_long && old_state == pressed) {
        Event e = {BUTTON_LONG_PRESS_EVENT, 0, 0};
        xQueueSendToBack(event_queue, (const void *) &e, (TickType_t) 0);
      } 
    }
  }
}

void loop()
{
  Serial.println("loop");
  wake_up();

  for (;;) {
    Event event;
    // Serial.println("waiting for event");
    if (xQueueReceive(event_queue, &event, portMAX_DELAY) == pdTRUE) {
      //event.action(event.param1, event.param2);
      switch (event.type)
      {
      case TOUCH_DOWN_EVENT:
        current_app->on_touch_down(event.param1, event.param2);
        break;
      case TOUCH_UP_EVENT:
        current_app->on_touch_up(event.param1, event.param2);
        break;
      case BUTTON_UP_EVENT:
        current_app->on_button_up();
        break;
      case BUTTON_LONG_PRESS_EVENT:
        current_app->on_button_long_press();
        break;
      default:
        break;
      }
    }
  }
}

void wake_up() {
  pthread_mutex_lock(&watch_mutex);
  watch->tft->begin();
  watch->openBL();
  pthread_mutex_unlock(&watch_mutex);

  current_app->setup();

  last_woke_up_ticks = xTaskGetTickCount();
}

void sleep_until_display_or_button_is_pressed() 
{
  pthread_mutex_lock(&watch_mutex);

  watch->power->clearIRQ();

  watch->displaySleep();
  watch->powerOff();
  watch->bl->off();

  esp_sleep_enable_ext0_wakeup((gpio_num_t) AXP202_INT, LOW);
  esp_sleep_enable_ext1_wakeup(GPIO_SEL_38, ESP_EXT1_WAKEUP_ALL_LOW);

  esp_light_sleep_start();

  watch->power->clearIRQ();

  pthread_mutex_unlock(&watch_mutex);

  wake_up();
}