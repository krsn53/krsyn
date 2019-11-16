#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include <gtk/gtk.h>
#include "../krsyn.h"

typedef struct AudioState AudioState;

typedef struct EditorState
{
    AudioState* state;
    GtkWidget* param_editor;
    char save_file[256];
    KrsynFMData data;
}EditorState;
#endif // EDITOR_STATE_H
