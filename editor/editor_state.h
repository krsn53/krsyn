#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include <gtk/gtk.h>
#include "../krsyn.h"

typedef struct AudioState AudioState;

typedef struct EditorState
{
    AudioState* state;
    GtkWidget* param_editor;
    KrsynFMData data;
    KrsynFMData* data_editing;
}EditorState;
#endif // EDITOR_STATE_H
