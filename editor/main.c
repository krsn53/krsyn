#include "fm_data_params.h"
#include "audio_manager.h"
#include "editor_menu.h"


static void activate (GtkApplication *app,  gpointer user_data)
{
    GtkWidget *window;
    GtkWidget* left_box;
    GtkWidget* right_box;
    GtkWidget* up_box;
    GtkWidget* out_box;
    GtkWidget* common_params;
    GtkWidget* operator_params;
    editor_state *state;
    GtkWidget* wave_viewer;
    GtkWidget* keyboard;

    GtkCssProvider* provider;
    GError* error;

    state = (editor_state*)user_data;
    state->state = audio_state_new();

    ks_synth_binary_set_default(&state->data);

    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "Krsyn Tone Editor");

    provider = gtk_css_provider_new();
    error = NULL;

    // CSSファイルの読み込み.
    gtk_css_provider_load_from_path(provider, "resource/css/style.css", &error);

    if( error != NULL ){

        // 読み込み失敗.
        return;
    }
    gtk_style_context_add_provider_for_screen(gtk_widget_get_screen(window), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    up_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    out_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    state->param_editor = out_box;

    add_editor_menu(app, out_box, state);

    g_signal_new("update-params",
                 G_TYPE_OBJECT,
                 G_SIGNAL_RUN_FIRST,
                 0, NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);

    gtk_box_pack_start(GTK_BOX(out_box), up_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(up_box), left_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(up_box), right_box, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(left_box), common_params = common_params_new(state));
    gtk_container_add(GTK_CONTAINER(left_box), wave_viewer = wave_viewer_new(state->state));
    gtk_container_add(GTK_CONTAINER(right_box), operator_params = operator_params_new(state));

    gtk_container_add(GTK_CONTAINER(out_box), keyboard = keyboard_new(state));
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(out_box));

    gtk_window_set_default_size (GTK_WINDOW (window), 960, -1);

    if(strcmp(state->save_file, "") != 0)
    {
        editor_open_tone(state->save_file, state);
    }

    gtk_widget_show_all (window);
}

static void shutdown(GtkApplication* widget, gpointer user_data)
{
    editor_state * state = (editor_state*)user_data;
    audio_state_free(state->state);
}

int main (int  argc, char **argv)
{

    GtkApplication *app;
    int status;
    editor_state state;
    memset(&state, 0, sizeof(editor_state));

    if(argc == 2)
    {
        strcpy(state.save_file, argv[1]);
    }

    app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);

    g_signal_connect (app, "activate", G_CALLBACK (activate), &state);
    g_signal_connect (app, "shutdown", G_CALLBACK (shutdown), &state);

    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}
