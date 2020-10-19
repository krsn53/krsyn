#ifndef EDITOR_MENU_H
#define EDITOR_MENU_H

#include<gtk/gtk.h>
#include "editor_state.h"

gboolean editor_open_tone(const char* file, editor_state* state);
gboolean editor_save_tone(const char* file, editor_state* state);
void add_editor_menu(GtkApplication* app, GtkWidget *vbox, editor_state* editor_state);

#endif // EDITOR_MENU_H
