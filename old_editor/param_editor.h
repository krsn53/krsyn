#ifndef PARAM_EDITOR_H
#define PARAM_EDITOR_H


#include <stdint.h>
#include <gtk/gtk.h>

#include "../krsyn/math.h"

void uint8_range_widget_set(GtkWidget* widget, void*param);
void uint8_combo_widget_set(GtkWidget* widget, void*param);
void uint8_spin_widget_set(GtkWidget* widget, void*param);
void uint8_value_changed(GtkRange *range, gpointer user);
void uint8_spin_value_changed(GtkSpinButton* spin, gpointer user);
void toggle_button_checked(GtkToggleButton *togglebutton, gpointer user);


GtkWidget* param_new(u8* param_ptr, gchar*(* format)(GtkScale*, gdouble, gpointer),
                     void(*widget_set)(GtkWidget *, void *), void(*setter)(GtkRange*, gpointer),
                     int min, int max);

GtkWidget* check_param_new(u8* param_ptr,
                           void(*widget_set)(GtkWidget *, void *), void(*setter)(GtkToggleButton*, gpointer));

GtkWidget* spin_param_new(u8* param_ptr,
                          void(*widget_set)(GtkWidget *, void *), void(*setter)(GtkSpinButton *, gpointer),
                          int min, int max);

GtkWidget* enum_param_new(u8* param_ptr,
                          void(*widget_set)(GtkWidget *, void *), void(*setter)(GtkComboBox*, gpointer),
                          GtkCellRenderer *colmun,  GType list_type,
                          void*enum_data[], int list_num);

GtkWidget* param_new_with_label(u8* param_ptr, const char* str, gchar*(* format)(GtkScale*, gdouble, gpointer),
                     void(*widget_set)(GtkWidget*, void*), void(*setter)(GtkRange*, gpointer),
                     int min, int max);

GtkWidget* spin_param_new_with_label(u8* param_ptr, const char* str,
                          void (*widget_set)(GtkWidget *, void *), void(*setter)(GtkSpinButton *, gpointer),
                          int min, int max);

GtkWidget* enum_param_new_with_label(u8* param_ptr, const char* str,
                          void (*widget_set)(GtkWidget*, void*), void(*setter)(GtkComboBox*, gpointer),
                          GtkCellRenderer *colmun, GType list_type,
                          void*enum_data[], int list_num);



#endif // PARAM_EDITOR_H
