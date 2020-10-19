#include "editor_menu.h"
#include <stdlib.h>

static const char file_check[5] = {'F', 'M', 'T', 'N', 0};

static void emit_update_params_signal(GtkWidget* current)
{
    if(current == NULL) return;
    if(GTK_IS_BOX(current) || GTK_IS_GRID(current)){
        for(GList *l=gtk_container_get_children(GTK_CONTAINER(current)); l!= NULL; l= l->next){
            emit_update_params_signal(GTK_WIDGET(l->data));
        }
    }
    else {
        g_signal_emit_by_name(current, "update-params");
    }
}

static GtkFileFilter* tone_file_filter_new(){
    GtkFileFilter *ret = gtk_file_filter_new();
    gtk_file_filter_add_pattern(ret, "*.fmt");

    return ret;
}

static void set_title(GtkWindow* window, const char* file){
    if(strcmp(file, "") == 0){
        gtk_window_set_title(window, "Krsyn Tone Editor");
        return;
    }
    else {
        gchar* title;
        unsigned long file_name_pos;

        file_name_pos = strlen(file);
        for(;file_name_pos > 0; file_name_pos--){
            if(file[file_name_pos] == '/' || file[file_name_pos] == '\\')
            {
                file_name_pos++;
                break;
            }
        }

        title = g_strdup_printf("Krsyn Tone Editor - %s", file + file_name_pos);
        gtk_window_set_title(window, title);

        g_free(title);
    }
}

static void new_menu_activate(GtkMenuItem *item, gpointer user_data){
    editor_state* state;

    state = user_data;
    krsyn_binary_set_default(&state->data);
    emit_update_params_signal(state->param_editor);
    set_title(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(state->param_editor))), "");
    memset(state->save_file, 0, sizeof(state->save_file));
}

gboolean editor_open_tone(const char* file, editor_state* state)
{
    FILE* fp;

    if((fp = fopen(file, "rb")) != NULL)
    {
        char fmtn[5];

        fread( fmtn, 1, 4, fp );
        fmtn[4] = 0;
        if(strcmp(fmtn, file_check) != 0)
        {
            GtkWidget *error_dialog;

            error_dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(state->param_editor))),
                                                  GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK, "Failed to load tone");

            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);

            fclose(fp);
            return FALSE;
        }
        fread( &state->data, 1,sizeof(state->data), fp );
        fclose(fp);

        memset(state->save_file, 0, sizeof(state->save_file));
        strcpy(state->save_file, file);

        set_title(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(state->param_editor))), state->save_file);
        return TRUE;
    }
    return FALSE;
}

gboolean editor_save_tone(const char* file, editor_state* state){
    char * file_path;
    FILE* fp;
    unsigned long len;

    len = strlen(file);

    if(strcmp(file + len - 4, ".fmt") != 0){
        file_path = g_strdup_printf("%s.fmt", file);
    }
    else {
        file_path = g_strdup(file);
    }

    if((fp = fopen(file_path, "wb")) != NULL){
        fwrite(file_check, 1, 4, fp);
        fwrite(&state->data, 1, sizeof(state->data), fp);
        fclose(fp);

        memset(state->save_file, 0, sizeof(state->save_file));
        strcpy(state->save_file, file_path);

        set_title(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(state->param_editor))), state->save_file);
        g_free(file_path);
        return TRUE;
    }
    g_free(file_path);
    return FALSE;
}

static void open_menu_activate(GtkMenuItem *item, gpointer user_data){
    editor_state* state;
    GtkWidget* file_dialog;
    GtkFileFilter *file_filter;
    int response;

    state = user_data;
    file_dialog = gtk_file_chooser_dialog_new("Open Tone", GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(item))),
                                              GTK_FILE_CHOOSER_ACTION_OPEN,
                                              "_Open", GTK_RESPONSE_OK,
                                              "_Cancel", GTK_RESPONSE_CANCEL,
                                              NULL);
    file_filter = tone_file_filter_new();
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(file_dialog), file_filter);
    response = gtk_dialog_run(GTK_DIALOG(file_dialog));
    if(response == GTK_RESPONSE_OK){
        GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(file_dialog));
        char* file_path = g_file_get_path(file);

        if(editor_open_tone(file_path, state))
        {
            emit_update_params_signal(state->param_editor);
        }

        g_free(file_path);
    }
    gtk_widget_destroy(file_dialog);
}

static void save_as_menu_activate(GtkMenuItem *item, gpointer user_data)
{
    editor_state* state;
    GtkWidget* file_dialog;
    GtkFileFilter *file_filter;
    int response;

    state = user_data;
    file_dialog = gtk_file_chooser_dialog_new("Open Tone", GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(item))),
                                              GTK_FILE_CHOOSER_ACTION_SAVE,
                                              "_Save", GTK_RESPONSE_OK,
                                              "_Cancel", GTK_RESPONSE_CANCEL,
                                              NULL);
    file_filter = tone_file_filter_new();
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(file_dialog), file_filter);
    response = gtk_dialog_run(GTK_DIALOG(file_dialog));
    if(response == GTK_RESPONSE_OK){
        GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(file_dialog));
        char* file_path = g_file_get_path(file);
        editor_save_tone(file_path, state);
        g_free(file_path);
    }
    gtk_widget_destroy(file_dialog);
}

static void save_menu_activate(GtkMenuItem *item, gpointer user_data)
{
    editor_state* state;

    state = user_data;

    if(strcmp(state->save_file, "") == 0)
    {
        save_as_menu_activate(item, user_data);
        return ;
    }

    if(editor_save_tone(state->save_file, state))
    {
        memset(state->save_file, 0, sizeof(state->save_file));
        strcpy(state->save_file, state->save_file);
    }
}


static void quit_menu_activate(GtkMenuItem *item, gpointer user_data)
{
    g_application_quit(user_data);
}

void add_editor_menu(GtkApplication* app, GtkWidget* vbox, editor_state* editor_state){
    GtkWidget *menu_bar;
    GtkWidget *file;
    GtkWidget *file_items;
    GtkWidget *new_tone;
    GtkWidget *open;
    GtkWidget *save;
    GtkWidget *save_as;
    GtkWidget *quit;

    menu_bar = gtk_menu_bar_new();
    file = gtk_menu_item_new_with_label("File");
    file_items = gtk_menu_new();
    new_tone = gtk_menu_item_new_with_label("New");
    open = gtk_menu_item_new_with_label("Open");
    save = gtk_menu_item_new_with_label("Save");
    save_as = gtk_menu_item_new_with_label("Save As");
    quit = gtk_menu_item_new_with_label("Quit");

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), file_items);

    gtk_menu_shell_append(GTK_MENU_SHELL(file_items), new_tone);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_items), open);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_items), save);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_items), save_as);

    gtk_menu_shell_append(GTK_MENU_SHELL(file_items), gtk_separator_menu_item_new());

    gtk_menu_shell_append(GTK_MENU_SHELL(file_items), quit);

    g_signal_connect(new_tone, "activate", G_CALLBACK(new_menu_activate), editor_state);
    g_signal_connect(open, "activate", G_CALLBACK(open_menu_activate), editor_state);
    g_signal_connect(save, "activate", G_CALLBACK(save_menu_activate), editor_state);
    g_signal_connect(save_as, "activate", G_CALLBACK(save_as_menu_activate), editor_state);
    g_signal_connect(quit, "activate", G_CALLBACK(quit_menu_activate), app);

    gtk_container_add(GTK_CONTAINER(vbox), menu_bar);
}