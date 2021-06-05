#include "TimerApp.h"

#include <pthread.h>

#include "matteo-watch.h"
// Except T-Watch2020, other versions need to be selected according to the actual situation
#if  !defined(LILYGO_WATCH_2020_V1) && !defined(LILYGO_WATCH_2020_V3)

// T-Watch comes with the default backplane, it uses internal DAC
#define STANDARD_BACKPLANE

// Such as MAX98357A, PCM5102 external DAC backplane
// #define EXTERNAL_DAC_BACKPLANE

#else
// T-Watch2020 uses external DAC
#undef STANDARD_BACKPLANE
#define EXTERNAL_DAC_BACKPLANE

#endif
#include <LilyGoWatch.h>
#include <TTGO.h>

#include <WiFi.h>
#include <HTTPClient.h>         //Remove Audio Lib error
#include "AudioFileSourcePROGMEM.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorWav.h"
#include "AudioOutputI2S.h"
#include "timer-sound.h"
#include "esp_task_wdt.h"

TTGOClass *ttgo;

AudioFileSourcePROGMEM *file;
// AudioFileSourceID3 *id3;
AudioGeneratorWAV *mp3;
AudioOutputI2S *out;

esp_timer_handle_t loop_audio_timer;
void loop_audio(void *args) {
    if (mp3->isRunning()) {
        if (!mp3->loop()) mp3->stop();
    }
}

void TimerApp::setup() {
    ttgo = TTGOClass::getWatch();

    ttgo->tft->fillScreen(TFT_BLACK);
    ttgo->tft->setCursor(0, 0);
    ttgo->tft->fillScreen(TFT_BLACK);
    ttgo->tft->println("Sample MP3 playback begins");

    //!Turn on the audio power
    ttgo->enableLDO3();

    file = new AudioFileSourcePROGMEM(timer_sound, sizeof(timer_sound));
    // id3 = new AudioFileSourceID3(file);
    out = new AudioOutputI2S();
    //External DAC decoding
    out->SetPinout(TWATCH_DAC_IIS_BCK, TWATCH_DAC_IIS_WS, TWATCH_DAC_IIS_DOUT);
    mp3 = new AudioGeneratorWAV();
    mp3->begin(file, out);

    esp_timer_create_args_t args = {loop_audio, nullptr, ESP_TIMER_TASK, "play sound timer"};
    ESP_ERROR_CHECK(esp_timer_create(&args, &loop_audio_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(loop_audio_timer, 90)); // 90 microseconds ~= 11111 Hz
}

void TimerApp::on_touch_down(uint32_t x, uint32_t y) {
}

void TimerApp::on_touch_up(uint32_t x, uint32_t y) {

}

void TimerApp::on_button_up() {
    esp_timer_stop(loop_audio_timer);
    esp_timer_delete(loop_audio_timer);
    mp3->stop();
    delete out;
    delete mp3;
    // delete id3;
    delete file;

    App::on_button_up();
}

void TimerApp::on_button_long_press() {

}

void TimerApp::on_step_counter_counted(uint32_t steps) {

}

void TimerApp::update() {

}