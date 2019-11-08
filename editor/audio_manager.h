#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <gtk/gtk.h>
#include <portaudio.h>
#include "../krsyn.h"

#define SAMPLE_RATE (44100)
#define NUM_CHANNELS (2)
#define NUM_SAMPLES (256)

typedef struct AudioState
{
    KrsynCore*  core;
    KrsynFM     fm;
    KrsynFMNote note;

    PaStream*   stream;
    int16_t*    log;
}AudioState;

typedef struct EditorState
{
    AudioState* state;
    KrsynFMData data;
}EditorState;

AudioState* audio_state_new();
void audio_state_free(AudioState* state);

GtkWidget* wave_viewer_new(AudioState* state);
GtkWidget* keyboard_new(EditorState* state);

#endif // AUDIO_MANAGER_H
