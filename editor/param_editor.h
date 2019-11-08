#ifndef PARAM_EDITOR_H
#define PARAM_EDITOR_H

#include <stdint.h>
#include <gtk/gtk.h>

typedef struct ComboAttribute
{
    const char* name;
    unsigned num;
}ComboAttribute;

static ComboAttribute combo_text[] ={
    {.name="text", .num=0},
    };

static ComboAttribute combo_pixbuf[]={
    {.name="pixbuf", .num=0},
    };

static GType combo_text_type_list[] ={
    G_TYPE_STRING,
    };

static GType combo_pixbuf_type_list[] ={
    G_TYPE_OBJECT,
    };



uint8_t uint8_getter(void*param);
void uint8_value_changed(GtkRange *range, gpointer user);
void uint8_spin_value_changed(GtkSpinButton* spin, gpointer user);
void toggle_button_checked(GtkToggleButton *togglebutton, gpointer user);


GtkWidget* param_new(uint8_t* param_ptr, gchar*(* format)(GtkScale*, gdouble, gpointer),
                     uint8_t(*getter)(void*), void(*setter)(GtkRange*, gpointer),
                     int min, int max);

GtkWidget* check_param_new(uint8_t* param_ptr,
                           uint8_t(*getter)(void*), void(*setter)(GtkToggleButton*, gpointer));

GtkWidget* spin_param_new(uint8_t* param_ptr,
                          uint8_t(*getter)(void*), void(*setter)(GtkSpinButton *, gpointer),
                          int min, int max);

GtkWidget* enum_param_new(uint8_t* param_ptr,
                          uint8_t(*getter)(void*), void(*setter)(GtkComboBox*, gpointer),
                          GtkCellRenderer *colmun, unsigned num_colmun,
                          GType list_types[], const ComboAttribute attributes[],
                          void*enum_data[], int list_num);

GtkWidget* param_new_with_label(uint8_t* param_ptr, const char* str, gchar*(* format)(GtkScale*, gdouble, gpointer),
                     uint8_t(*getter)(void*), void(*setter)(GtkRange*, gpointer),
                     int min, int max);

GtkWidget* spin_param_new_with_label(uint8_t* param_ptr, const char* str,
                          uint8_t(*getter)(void*), void(*setter)(GtkSpinButton *, gpointer),
                          int min, int max);

GtkWidget* enum_param_new_with_label(uint8_t* param_ptr, const char* str,
                          uint8_t(*getter)(void*), void(*setter)(GtkComboBox*, gpointer),
                          GtkCellRenderer *colmun, unsigned num_colmun,
                          GType list_types[], const ComboAttribute attributes[],
                          void*enum_data[], int list_num);



#endif // PARAM_EDITOR_H
