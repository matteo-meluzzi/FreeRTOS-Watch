#include "matteo-watch.h"

#include <LilyGoWatch.h>
#include <TTGO.h>
#include <freertos/semphr.h>
#include <WiFi.h>
#include <pthread.h>
#include <btAudio.h>

#include "WatchApp.h"
#include "PingPongApp.h"
#include "BluetoothSpeakerApp.h"

void set_current_app(App *new_app);
void sleep_until_display_or_button_is_pressed();
void wake_up();

void read_button_task(void *args);
void read_touch_task(void *args);
void read_step_counter_task(void *args);

typedef enum {
  TOUCH_DOWN_EVENT,
  TOUCH_UP_EVENT,
  BUTTON_UP_EVENT,
  BUTTON_LONG_PRESS_EVENT,
  STEP_COUNTER_EVENT
} EventType;

struct Event {
  EventType type;
  uint32_t param1;
  uint32_t param2;
};

TTGOClass *watch;
pthread_mutex_t watch_mutex;

btAudio audio = btAudio("Matteo's watch");
pthread_mutex_t audio_mutex;

TickType_t last_woke_up_ticks = 0;

SemaphoreHandle_t button_semaphore = NULL;
SemaphoreHandle_t touch_semaphore = NULL;
SemaphoreHandle_t step_counter_semaphore = NULL;

QueueHandle_t event_queue = NULL;

App *current_app;

WatchApp watch_app = NULL;
PingPongApp ping_pong_app = NULL;
BluetoothSpeakerApp speaker_app = NULL;

void set_current_app(App *new_app) {
  current_app = new_app;
  current_app->setup();
}

esp_timer_handle_t one_second_timer;
void every_second(void *arg) {
  current_app->update();
}

const char *ssid            = "room 12.4 rules";
const char *password        = "cocongo72";

const char *ntpServer       = "pool.ntp.org";
const long  gmtOffset_sec   = 3600;
const int   daylightOffset_sec = 3600;

void update_rtc_from_wifi() {
  watch->tft->print("connecting to the wifi");
  WiFi.begin(ssid, password);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED && count++ < 20) {
      delay(500);
      Serial.print(".");
  }
  Serial.print("\nConnected: ");
  Serial.println(WiFi.status() == WL_CONNECTED);
  watch->tft->setCursor(0, 0);
  watch->tft->println("                      ");
  watch->tft->setCursor(0, 0);

  if (WiFi.status() == WL_CONNECTED) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    watch->tft->println("obtaining time from the internet");

    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        delay(100);
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

    watch->rtc->syncToRtc();
  }
  WiFi.disconnect(true, false);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Hi!");

  ESP_ERROR_CHECK(pthread_mutex_init(&watch_mutex, NULL));
  ESP_ERROR_CHECK(pthread_mutex_init(&audio_mutex, NULL));

  button_semaphore = xSemaphoreCreateCounting(1024, 0);
  assert(button_semaphore);
  touch_semaphore = xSemaphoreCreateCounting(1024, 0);
  assert(touch_semaphore);
  step_counter_semaphore = xSemaphoreCreateCounting(1024, 0);
  assert(step_counter_semaphore);
  event_queue = xQueueCreate(25, sizeof(Event));
  assert(event_queue);

  watch_app = WatchApp(&ping_pong_app);
  ping_pong_app = PingPongApp(&speaker_app);
  speaker_app = BluetoothSpeakerApp(&watch_app);

  current_app = &watch_app;
  assert(current_app);

  watch = TTGOClass::getWatch();

  // Initialize the hardware, the BMA423 sensor has been initialized internally
  watch->begin();
  watch->bl->adjust(32);
  watch->tft->setSwapBytes(true); // Swap the 2 bytes of an image which form a pixel

  update_rtc_from_wifi();

  pinMode(AXP202_INT, INPUT_PULLUP);
  attachInterrupt(AXP202_INT, [] {
    xSemaphoreGiveFromISR(button_semaphore, nullptr);
  }, FALLING);
  watch->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
  watch->power->clearIRQ();

  // set touch panel in interrupt polling mode
  // see https://www.buydisplay.com/download/ic/FT6236-FT6336-FT6436L-FT6436_Datasheet.pdf
  // paragraph 1.2  
  watch->touch->disableINT(); 
  pinMode(TOUCH_INT, INPUT);
  attachInterrupt(TOUCH_INT, [] {
    xSemaphoreGiveFromISR(touch_semaphore, nullptr);
  }, CHANGE);

  watch->power->adc1Enable(AXP202_VBUS_VOL_ADC1 |
    AXP202_VBUS_CUR_ADC1 |
    AXP202_BATT_CUR_ADC1 |
    AXP202_BATT_VOL_ADC1,
    true);
  
  Acfg cfg = {BMA4_OUTPUT_DATA_RATE_25HZ, BMA4_ACCEL_RANGE_2G, BMA4_ACCEL_NORMAL_AVG4, BMA4_CONTINUOUS_MODE};
  watch->bma->accelConfig(cfg);
  watch->bma->enableAccel();
  pinMode(BMA423_INT1, INPUT);
  attachInterrupt(BMA423_INT1, [] {
    xSemaphoreGiveFromISR(step_counter_semaphore, nullptr);
  }, RISING);
  watch->bma->enableFeature(BMA423_STEP_CNTR, true);
  watch->bma->resetStepCounter();
  watch->bma->enableStepCountInterrupt();

  esp_timer_init();
  esp_timer_create_args_t args = {every_second, nullptr, ESP_TIMER_TASK, "one second timer"};
  ESP_ERROR_CHECK(esp_timer_create(&args, &one_second_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(one_second_timer, 1000000));

  xTaskCreate(read_button_task, "read_button_task", 2048, NULL, 1, NULL);
  xTaskCreate(read_touch_task, "read_touch_task", 2048, NULL, 1, NULL);
  xTaskCreate(read_step_counter_task, "read_step_counter_task", 2048, NULL, 1, NULL);
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

      if (is_short) {
        Event e = {BUTTON_UP_EVENT, 0, 0};
        xQueueSendToBack(event_queue, (const void *) &e, (TickType_t) 0);
      }
      else if (is_long) {
        Event e = {BUTTON_LONG_PRESS_EVENT, 0, 0};
        xQueueSendToBack(event_queue, (const void *) &e, (TickType_t) 0);
      }
      watch->power->clearIRQ();
      pthread_mutex_unlock(&watch_mutex);
    }
  }
}

void read_step_counter_task(void *args) 
{
  for (;;) {
    if (xSemaphoreTake(step_counter_semaphore, portMAX_DELAY) == pdTRUE) {
      pthread_mutex_lock(&watch_mutex);

      if (!watch->bma->readInterrupt()) continue;

      // Check if it is a step interrupt
      if (watch->bma->isStepCounter()) {
        Serial.println("Step count ");
        Serial.println(watch->bma->getCounter());
        // Get step data from register
        uint32_t steps = watch->bma->getCounter();
        Event e = {STEP_COUNTER_EVENT, steps, 0};
        xQueueSendToBack(event_queue, (const void *) &e, (TickType_t) 0);
      }

      pthread_mutex_unlock(&watch_mutex);
    }
  }
}

void loop()
{
  // Serial.println("loop");
  wake_up();

  for (;;) {
    Event event;

    if (xQueueReceive(event_queue, &event, portMAX_DELAY) == pdTRUE && xTaskGetTickCount() - last_woke_up_ticks > 200) {
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
      case STEP_COUNTER_EVENT:
        current_app->on_step_counter_counted(event.param1);
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

  esp_timer_stop(one_second_timer);

  esp_sleep_enable_ext0_wakeup((gpio_num_t) AXP202_INT, LOW);
  esp_sleep_enable_ext1_wakeup(GPIO_SEL_38, ESP_EXT1_WAKEUP_ALL_LOW);

  esp_light_sleep_start();

  watch->power->clearIRQ();

  ESP_ERROR_CHECK(esp_timer_start_periodic(one_second_timer, 1000000));

  pthread_mutex_unlock(&watch_mutex);

  wake_up();
}