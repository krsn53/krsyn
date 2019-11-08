#ifndef FM_DATA_LIST_H
#define FM_DATA_LIST_H

#include <gtk/gtk.h>
#include "param_editor.h"
#include "../krsyn.h"

#define PROGRAM_INDEX (0)
#define NOTE_NUMBER_INDEX (1)

typedef union ProgramNumber
{
    uint16_t uint_v;
    uint8_t  byte_v[sizeof(uint16_t)];
}ProgramNumber;

typedef struct krsyn_data_meta
{
    char            name[64];
    ProgramNumber   program;
}
krsyn_data_meta;

GtkWidget* data_list_new();
int data_meta_dialog(GtkWidget *parent, krsyn_data_meta* meta);

#endif // FM_DATA_LIST_H
