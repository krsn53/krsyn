#include "fm_data_list.h"


static uint8_t get_bits_mask(uint8_t start, uint8_t num_bits)
{
    uint8_t mask = 0xff;
    mask <<= start;
    if(start + num_bits < sizeof(uint8_t)*8)
    {
        mask &= 0xff >> (sizeof(uint8_t)- (start + num_bits));
    }
    return mask;
}

static uint8_t get_bits_value(uint8_t value, uint8_t start, uint8_t num_bits)
{
    uint8_t mask = get_bits_mask(start, num_bits);
    return (value & mask) >> start;
}

static void set_bits_value(uint8_t* value_ptr, uint8_t start, uint8_t num_bits, uint8_t value)
{
    uint8_t mask = get_bits_mask(start, num_bits);
    *value_ptr &= ~mask;
    *value_ptr |= ( ( value << start ) & mask );
}

static void is_percussion_toggled(GtkToggleButton* tog, gpointer user_data)
{
    set_bits_value((uint8_t*)user_data, 0, 7, gtk_toggle_button_get_active(tog));
}

static uint8_t is_percussion_getter(void* value)
{
    return get_bits_value(*(uint8_t*)value, 0, 7);
}

static void name_entry_changed(GtkEntry *entry, gchar  *string, gpointer  user_data)
{
    char* name = (char*)user_data;
    const char* src = gtk_entry_get_text(entry);
    unsigned length = MIN(gtk_entry_get_text_length(entry), 64);
    memcpy(name, src, length);
}


static void change_show_note_num(GtkToggleButton* tog, gpointer user_data)
{
    GtkWidget *other = (GtkWidget*) user_data;

    if(gtk_toggle_button_get_active(tog))
    {
        gtk_widget_show_all(other);
    }else
    {
        gtk_widget_hide(other);
    }
}

static void note_num_spin_changed(GtkSpinButton* spin, gpointer user_data)
{
    set_bits_value((uint8_t*)user_data, 7, 1, gtk_spin_button_get_value(spin));
}

static uint8_t note_num_spin_getter(void* value)
{
    return get_bits_value(*(uint8_t*)value, 7, 1);
}

int data_meta_dialog(GtkWidget* parent, krsyn_data_meta* meta)
{
    GtkWidget* dialog;
    GtkWidget* name_label;
    GtkWidget* name_box;
    GtkWidget* name_entry;
    GtkWidget* program;
    GtkWidget* is_percussion;
    GtkWidget* note_num;
    GtkContainer* contents_area;
    int response;

    dialog= gtk_dialog_new_with_buttons("Edit Tone Info", GTK_WINDOW(parent), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_OK", GTK_RESPONSE_ACCEPT,
                                         "_Cancel", GTK_RESPONSE_REJECT,
                                         NULL);
    contents_area = GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

    name_label = gtk_label_new("Tone Name");
    name_entry = gtk_entry_new();
    name_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_container_add(GTK_CONTAINER(name_box), name_label);
    gtk_container_add(GTK_CONTAINER(name_box), name_entry);

    g_signal_connect(name_entry, "insert-at-cursor", G_CALLBACK(name_entry_changed), meta->name);

    program = spin_param_new_with_label(meta->program.byte_v + PROGRAM_INDEX, "Program Number",
                                              uint8_getter, uint8_spin_value_changed,
                                              0, 255);
    note_num = spin_param_new_with_label(meta->program.byte_v + NOTE_NUMBER_INDEX, "NoteNumber",
                                         note_num_spin_getter, note_num_spin_changed,
                                         0, 127);
    is_percussion = check_param_new(meta->program.byte_v + NOTE_NUMBER_INDEX, is_percussion_getter, is_percussion_toggled);
    g_signal_connect(is_percussion, "toggled", G_CALLBACK(change_show_note_num), note_num);

    gtk_container_add(contents_area, name_box);
    gtk_container_add(contents_area, program);
    gtk_container_add(contents_area, is_percussion);
    gtk_container_add(contents_area, note_num);

    memset(meta, 0, sizeof(krsyn_data_meta));

    gtk_widget_show_all(dialog);
    gtk_widget_hide(note_num);

    response = gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_destroy(dialog);

    return response;
}
