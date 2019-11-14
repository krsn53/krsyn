#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "editor_state.h"
#include <AL/al.h>
#include <AL/alc.h>

#define SAMPLE_RATE (44100)
#define NUM_CHANNELS (2)
#define NUM_SAMPLES (4410)

typedef struct AudioState
{
    KrsynCore*  core;
    KrsynFM     fm;
    KrsynFMNote note;
    int8_t noteon;

    ALCdevice *device;
    ALCcontext *context ;
    ALuint buffers[4];
    ALuint source;
    gboolean is_playing;
    int16_t    buf[NUM_SAMPLES*NUM_CHANNELS];
}AudioState;


AudioState* audio_state_new();
void audio_state_free(AudioState* state);

GtkWidget* wave_viewer_new(AudioState* state);
GtkWidget* keyboard_new(EditorState* state);

#endif // AUDIO_MANAGER_H
