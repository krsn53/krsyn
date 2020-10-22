#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include <gtk/gtk.h>
#include "../krsyn.h"

typedef struct audio_state audio_state;

typedef struct editor_state
{
    audio_state* state;
    GtkWidget* param_editor;
    char save_file[256];
    krsynth_binary data;
}editor_state;
#endif // EDITOR_STATE_H
