#include "param_editor.h"


void uint8_range_widget_set(GtkWidget* widget, void*param)
{
    gtk_range_set_value(GTK_RANGE(widget), *(u8*)param);
}

void uint8_combo_widget_set(GtkWidget* widget, void*param)
{
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget), *(u8*)param);
}

void uint8_spin_widget_set(GtkWidget* widget, void*param)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), *(u8*)param);
}

void uint8_value_changed(GtkRange* range, gpointer user)
{
    u8* data = (u8*) user;
    *data = gtk_range_get_value(range);
}

void uint8_spin_value_changed(GtkSpinButton* spin, gpointer user)
{
    u8* data = (u8*)user;
    *data = gtk_spin_button_get_value(spin);
}



GtkWidget* check_param_new(u8* param_ptr,
                           void(*widget_set)(GtkWidget*, void*), void(*setter)(GtkToggleButton*, gpointer))
{
    GtkWidget* check = gtk_check_button_new();
     widget_set(check, param_ptr);
    g_signal_connect(check, "toggled", G_CALLBACK(setter), param_ptr);

    return check;
}

GtkWidget* spin_param_new(u8* param_ptr,
                          void(*widget_set)(GtkWidget*, void*), void(*setter)(GtkSpinButton*, gpointer),
                          int min, int max)
{
    GtkWidget* spin= gtk_spin_button_new_with_range(min, max, 1);
    widget_set(spin, param_ptr);
    g_signal_connect(spin, "value-changed", G_CALLBACK(setter), param_ptr);

    return spin;
}

GtkWidget* param_new(u8* param_ptr, gchar*(* format)(GtkScale*, gdouble, gpointer),
                            void(*widget_set)(GtkWidget*, void*), void(*setter)(GtkRange*, gpointer),
                            int min, int max)
{
    GtkWidget* scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, 1);
    widget_set(scale, param_ptr);
    g_signal_connect(scale, "value-changed", G_CALLBACK(setter), param_ptr);

    if(format != NULL)
    {
        g_signal_connect(scale, "format-value", G_CALLBACK(format), param_ptr);
    }


    return scale;
}

void toggle_button_checked(GtkToggleButton *togglebutton, gpointer user)
{
    u8* data = (u8*) user;
    *data = (u8)gtk_toggle_button_get_active(togglebutton);
}



GtkWidget* enum_param_new(u8* param_ptr,
                                 void(*widget_set)(GtkWidget*,void*), void(*setter)(GtkComboBox*, gpointer),
                                 GtkCellRenderer *colmun,
                                 GType list_type,
                                 void*enum_data[], int list_num)
{
    GtkListStore* list = gtk_list_store_newv(1, &list_type);
    GtkWidget* combo;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list));

    for(int j=0; j < list_num; j++)
    {
            gtk_list_store_insert_with_values(list, NULL, -1, 0, enum_data[j], -1);
    }

    // listはcomboに所有されているため初期リファレンスを解除してやる必要があるらしい。
    // 参考　https://stackoverflow.com/questions/16630528/trying-to-populate-a-gtkcombobox-with-model-in-c
    g_object_unref(list);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), colmun, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), colmun, list_type ==G_TYPE_STRING ? "text": "pixbuf",0, NULL);

    widget_set(combo, param_ptr);
    g_signal_connect(combo, "changed", G_CALLBACK(setter), param_ptr);

    return combo;
}


GtkWidget* box_with_label(const char* str)
{
    GtkWidget* box;
    GtkWidget* label;

    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    label = gtk_label_new(str);
    gtk_container_add(GTK_CONTAINER(box), label);

    return box;
}

GtkWidget* param_new_with_label(u8* param_ptr, const char* str, gchar*(* format)(GtkScale*, gdouble, gpointer),
                                void(*widget_set)(GtkWidget *, void *), void(*setter)(GtkRange*, gpointer),
                                int min, int max)
{
    GtkWidget* box;
    GtkWidget* param;

    box = box_with_label(str);
    param = param_new(param_ptr, format, widget_set, setter, min, max);
    gtk_container_add(GTK_CONTAINER(box), param);

    return box;
}


GtkWidget* spin_param_new_with_label(u8* param_ptr, const char* str,
                                     void(*widget_set)(GtkWidget*, void*), void(*setter)(GtkSpinButton *, gpointer),
                                     int min, int max)
{
    GtkWidget* box;
    GtkWidget* param;

    box = box_with_label(str);
    param = spin_param_new(param_ptr, widget_set, setter, min, max);
    gtk_container_add(GTK_CONTAINER(box), param);

    return box;
}

GtkWidget* enum_param_new_with_label(u8* param_ptr, const char* str,
                                     void(*widget_set)(GtkWidget*, void*), void(*setter)(GtkComboBox*, gpointer),
                                     GtkCellRenderer *colmun, GType list_type,
                                     void*enum_data[], int list_num)
{
    GtkWidget* box;
    GtkWidget* param;

    box = box_with_label(str);
    param = enum_param_new(param_ptr, widget_set, setter,
                           colmun, list_type,
                           enum_data, list_num);
    gtk_container_add(GTK_CONTAINER(box), param);

    return box;
}
