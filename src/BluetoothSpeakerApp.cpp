
#include "BluetoothSpeakerApp.h"
#include <btAudio.h>

#include "bluetooth-logo.h"

#include <pthread.h>
#include "matteo-watch.h"
#include <LilyGoWatch.h>
#include <TTGO.h>

extern TTGOClass *watch;
extern pthread_mutex_t watch_mutex;

extern btAudio audio;
extern pthread_mutex_t audio_mutex;

void BluetoothSpeakerApp::setup() {    
    pthread_mutex_lock(&watch_mutex);

    watch->tft->pushImage(0, 0, 240, 240, ((uint16_t *) bluetooth_logo.pixel_data));

    //! Open aduio power
    watch->enableLDO3();
    pthread_mutex_unlock(&watch_mutex);

    pthread_mutex_lock(&audio_mutex);
    // start bluetooth audio
    audio.begin();

    //  attach to pins
    int bck = TWATCH_DAC_IIS_BCK;
    int ws = TWATCH_DAC_IIS_WS;
    int dout = TWATCH_DAC_IIS_DOUT;
    audio.I2S(bck, dout, ws);

    pthread_mutex_unlock(&audio_mutex);
}
void BluetoothSpeakerApp::on_touch_down(uint32_t x, uint32_t y) {

}
void BluetoothSpeakerApp::on_touch_up(uint32_t x, uint32_t y) {

}
void BluetoothSpeakerApp::on_button_up() {
    pthread_mutex_lock(&watch_mutex);
    watch->enableLDO3(false);
    pthread_mutex_unlock(&watch_mutex);

    pthread_mutex_lock(&audio_mutex);
    audio.end();
    pthread_mutex_unlock(&audio_mutex);

    App::on_button_up();
}
void BluetoothSpeakerApp::on_button_long_press() {

}
void BluetoothSpeakerApp::update() {
    
}
