#include "param_editor.h"


uint8_t uint8_getter(void*param)
{
    return *((uint8_t*)param);
}

void uint8_value_changed(GtkRange* range, gpointer user)
{
    uint8_t* data = (uint8_t*) user;
    *data = gtk_range_get_value(range);
}

void uint8_spin_value_changed(GtkSpinButton* spin, gpointer user)
{
    uint8_t* data = (uint8_t*)user;
    *data = gtk_spin_button_get_value(spin);
}

void uint8_toggled(GtkToggleButton* tog, gpointer user)
{
    uint8_t*data= (uint8_t*) tog;
    *data = gtk_toggle_button_get_active(tog);
}


GtkWidget* check_param_new(uint8_t* param_ptr,
                           uint8_t(*getter)(void*), void(*setter)(GtkToggleButton*, gpointer))
{
    GtkWidget* check = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), getter(param_ptr));
    g_signal_connect(check, "toggled", G_CALLBACK(getter), param_ptr);

    return check;
}

GtkWidget* spin_param_new(uint8_t* param_ptr,
                          uint8_t(*getter)(void*), void(*setter)(GtkSpinButton*, gpointer),
                          int min, int max)
{
    GtkWidget* spin= gtk_spin_button_new_with_range(min, max, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), getter(param_ptr));
    g_signal_connect(spin, "value-changed", G_CALLBACK(setter), param_ptr);

    return spin;
}

GtkWidget* param_new(uint8_t* param_ptr, gchar*(* format)(GtkScale*, gdouble, gpointer),
                            uint8_t(*getter)(void*), void(*setter)(GtkRange*, gpointer),
                            int min, int max)
{
    GtkWidget* scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, 1);
    gtk_range_set_value(GTK_RANGE(scale), getter(param_ptr));
    g_signal_connect(scale, "value-changed", G_CALLBACK(setter), param_ptr);

    if(format != NULL)
    {
        g_signal_connect(scale, "format-value", G_CALLBACK(format), param_ptr);
    }


    return scale;
}

void toggle_button_checked(GtkToggleButton *togglebutton, gpointer user)
{
    uint8_t* data = (uint8_t*) user;
    *data = gtk_toggle_button_get_active(togglebutton);
}



GtkWidget* enum_param_new(uint8_t* param_ptr,
                                 uint8_t(*getter)(void*), void(*setter)(GtkComboBox*, gpointer),
                                 GtkCellRenderer *colmun, unsigned num_colmun,
                                 GType list_types[], const ComboAttribute attributes[],
                                 void*enum_data[], int list_num)
{
    GtkListStore* list = gtk_list_store_newv(num_colmun, list_types);
    GtkWidget* combo;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list));

    for(int j=0; j < list_num; j++)
    {
        for(int k=0; k < num_colmun; k++)
        {
            gtk_list_store_insert_with_values(list, NULL, -1, 0, enum_data[j*num_colmun + k], -1);
        }
    }

    // listはcomboに所有されているため初期リファレンスを解除してやる必要があるらしい。
    // 参考　https://stackoverflow.com/questions/16630528/trying-to-populate-a-gtkcombobox-with-model-in-c
    g_object_unref(list);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), colmun, TRUE);

    for(int j=0; j < num_colmun; j++)
    {
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), colmun, attributes[j].name, attributes[j].num, NULL);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), getter(param_ptr));
    g_signal_connect(combo, "changed", G_CALLBACK(setter), param_ptr);

    return combo;
}


GtkWidget* box_with_label(const char* str)
{
    GtkWidget* box;
    GtkWidget* label;

    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_container_add(GTK_CONTAINER(box), label);

    return box;
}

GtkWidget* param_new_with_label(uint8_t* param_ptr, const char* str, gchar*(* format)(GtkScale*, gdouble, gpointer),
                                uint8_t(*getter)(void*), void(*setter)(GtkRange*, gpointer),
                                int min, int max)
{
    GtkWidget* box;
    GtkWidget* param;

    box = box_with_label(str);
    param = param_new(param_ptr, format, getter, setter, min, max);
    gtk_container_add(GTK_CONTAINER(box), param);

    return box;
}


GtkWidget* spin_param_new_with_label(uint8_t* param_ptr, const char* str,
                                     uint8_t(*getter)(void*), void(*setter)(GtkSpinButton *, gpointer),
                                     int min, int max)
{
    GtkWidget* box;
    GtkWidget* param;

    box = box_with_label(str);
    param = spin_param_new(param_ptr, getter, setter, min, max);
    gtk_container_add(GTK_CONTAINER(box), param);

    return box;
}

GtkWidget* enum_param_new_with_label(uint8_t* param_ptr, const char* str,
                                     uint8_t(*getter)(void*), void(*setter)(GtkComboBox*, gpointer),
                                     GtkCellRenderer *colmun, unsigned num_colmun,
                                     GType list_types[], const ComboAttribute attributes[],
                                     void*enum_data[], int list_num)
{
    GtkWidget* box;
    GtkWidget* param;

    box = box_with_label(str);
    param = enum_param_new(param_ptr, getter, setter,
                           colmun, num_colmun,
                           list_types, attributes,
                           enum_data, list_num);
    gtk_container_add(GTK_CONTAINER(box), param);

    return box;
}
