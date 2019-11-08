#include "fm_data_params.h"
#include "fm_data_list.h"
#include "audio_manager.h"

static void sample_note_on(GtkButton* button, gpointer user_data)
{
    EditorState *state = (EditorState*)user_data;
    krsyn_fm_set(state->state->core, &state->state->fm, &state->data);
    krsyn_fm_note_on(state->state->core, &state->state->fm, &state->state->note, 60, 100);
}

static void activate (GtkApplication *app,  gpointer user_data)
{
    GtkWidget *window;
    GtkPaned* vertical_box;
    GtkPaned* horizontal_box;
    GtkWidget* left_box;
    GtkWidget* right_box;
    GtkWidget* bottom_box;
    GtkWidget* common_params;
    GtkWidget* operator_params;
    EditorState *state = (EditorState*)user_data;
    GtkWidget* sample_note_on_button;
    GtkWidget* wave_viewer;
    GtkWidget* keyboard;

    state->state = audio_state_new();
    Pa_StartStream(state->state->stream);

    krsyn_fm_set_data_default(&state->data);

    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "Window");

    vertical_box = GTK_PANED(gtk_paned_new(GTK_ORIENTATION_VERTICAL));
    horizontal_box = GTK_PANED(gtk_paned_new(GTK_ORIENTATION_HORIZONTAL));
    left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    bottom_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

    gtk_paned_pack1(vertical_box, GTK_WIDGET(horizontal_box), TRUE, FALSE);
    gtk_paned_pack2(vertical_box, GTK_WIDGET(bottom_box), TRUE, FALSE);
    gtk_paned_pack1(horizontal_box, left_box, TRUE, FALSE);
    gtk_paned_pack2(horizontal_box, right_box, TRUE, FALSE);
    gtk_container_add(GTK_CONTAINER(left_box), common_params = common_params_new(&state->data));
    gtk_container_add(GTK_CONTAINER(right_box), operator_params = operator_params_new(&state->data));

    sample_note_on_button = gtk_button_new_with_label("noteon");
    gtk_container_add(GTK_CONTAINER(left_box), wave_viewer = wave_viewer_new(state->state));
    gtk_container_add(GTK_CONTAINER(left_box), sample_note_on_button);

    gtk_container_add(GTK_CONTAINER(bottom_box), keyboard = keyboard_new(state));

    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vertical_box));

    gtk_window_set_default_size (GTK_WINDOW (window), 960, -1);
    gtk_paned_set_position(horizontal_box, 0);

    gtk_widget_show_all (window);
}

static void shutdown(GtkApplication* widget, gpointer user_data)
{
    EditorState * state = (EditorState*)user_data;
    Pa_StopStream(state->state->stream);
    audio_state_free(state->state);
}

int main (int  argc, char **argv)
{
    GtkApplication *app;
    int status;
    EditorState state;

    app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), &state);
    g_signal_connect (app, "shutdown", G_CALLBACK (shutdown), &state);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}
