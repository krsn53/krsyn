#include "fm_data_params.h"




static void activate (GtkApplication *app,  gpointer user_data)
{
    GtkWidget *window;
    GtkWidget* box;
    krsyn_fm_data data;

    krsyn_fm_set_data_default(&data);

    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "Window");
    gtk_window_set_default_size (GTK_WINDOW (window), 640, 480);

    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

    add_common_params(box, &data);
    add_operator_params(box, &data);

    gtk_container_add(GTK_CONTAINER(window), box);

    gtk_widget_show_all (window);
}

int main (int  argc, char **argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}
