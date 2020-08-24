#include "audio_driver_wii.h"

Error AudioDriverWii::init()
{
    return OK;
}

void AudioDriverWii::start() {

}

int AudioDriverWii::get_mix_rate() const {
    return 0;
}

AudioDriver::SpeakerMode AudioDriverWii::get_speaker_mode() const {
    return SpeakerMode::SPEAKER_MODE_STEREO;
}

void AudioDriverWii::lock() {}
void AudioDriverWii::unlock() {}
void AudioDriverWii::finish() {}