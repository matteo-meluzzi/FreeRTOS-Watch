#include "TimerApp.h"

#include "matteo-watch.h"
#include <LilyGoWatch.h>
#include <TTGO.h>

#include <HTTPClient.h> // fix compiler errors
#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorWav.h"
#include "AudioOutputI2S.h"

#include "timer-sound.h"

extern TTGOClass *watch;
extern pthread_mutex_t watch_mutex;

void loop_audio(void *timer_app_void) {    
    TimerApp *timer_app = static_cast<TimerApp *>(timer_app_void);
    
    pthread_mutex_lock(&timer_app->generator_mutex);
    AudioGenerator *generator = timer_app->generator;
    if (generator->isRunning()) {
        if (!generator->loop()) generator->stop();
    }
    pthread_mutex_unlock(&timer_app->generator_mutex);
}

void TimerApp::setup() {
    pthread_mutex_lock(&watch_mutex);
    watch->tft->fillScreen(TFT_BLACK);
    watch->tft->setCursor(0, 0);
    watch->tft->fillScreen(TFT_BLACK);
    watch->tft->println("Sample MP3 playback begins");

    //!Turn on the audio power
    watch->enableLDO3();
    pthread_mutex_unlock(&watch_mutex);

    file = new AudioFileSourcePROGMEM(timer_sound, sizeof(timer_sound));
    out = new AudioOutputI2S();
    //External DAC decoding
    out->SetPinout(TWATCH_DAC_IIS_BCK, TWATCH_DAC_IIS_WS, TWATCH_DAC_IIS_DOUT);
    generator = new AudioGeneratorWAV();
    generator->begin(file, out);

    esp_timer_create_args_t args = {loop_audio, this, ESP_TIMER_TASK, "play sound timer"};
    ESP_ERROR_CHECK(esp_timer_create(&args, &loop_audio_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(loop_audio_timer, 90)); // 90 microseconds ~= 11111 Hz
}

void TimerApp::on_touch_down(uint32_t x, uint32_t y) {
}

void TimerApp::on_touch_up(uint32_t x, uint32_t y) {

}

void TimerApp::on_button_up() {
    pthread_mutex_lock(&generator_mutex);
    esp_timer_stop(loop_audio_timer);
    esp_timer_delete(loop_audio_timer);
    generator->stop();
    delete out;
    delete generator;
    delete file;
    pthread_mutex_unlock(&generator_mutex);

    App::on_button_up();
}

void TimerApp::on_button_long_press() {

}

void TimerApp::on_step_counter_counted(uint32_t steps) {

}

void TimerApp::update() {

}