#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "editor_state.h"
#include <AL/al.h>
#include <AL/alc.h>

#define SAMPLE_RATE (44100)
#define NUM_CHANNELS (1)
#define NUM_SAMPLES (4410)

typedef struct audio_state
{
    u32            sampling_rate;
    ks_synth             fm;
    ks_synth_note        note;
    i8              noteon;

    ALCdevice           *device;
    ALCcontext          *context ;
    ALuint              buffers[4];
    ALuint              source;
    gboolean            is_playing;
    i16             buf[NUM_SAMPLES*NUM_CHANNELS];
}audio_state;


audio_state* audio_state_new();
void audio_state_free(audio_state* state);

GtkWidget* wave_viewer_new(audio_state* state);
GtkWidget* keyboard_new(editor_state* state);

#endif // AUDIO_MANAGER_H
