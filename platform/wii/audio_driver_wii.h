#ifndef AUDIO_DRIVER_WII_H
#define AUDIO_DRIVER_WII_H

#include "servers/audio_server.h"

class AudioDriverWii : public AudioDriver {

public:
    virtual const char *get_name() const {return "Wii";}

    virtual Error init();
    virtual void start();
    virtual int get_mix_rate() const;
    virtual SpeakerMode get_speaker_mode() const;
    virtual void lock();
    virtual void unlock();
    virtual void finish();
};

#endif