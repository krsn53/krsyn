#pragma once

#include "./io.h"


typedef struct ks_midi_event{
    uint64_t time;
    uint32_t delta;
    uint8_t status;
    union{
        uint8_t datas[2];

        struct{
            uint32_t length;
            uint8_t* data;
        }sys_ex;

        struct{
            uint8_t type;
            uint32_t length;
            uint8_t* data;
        }meta;

    }message;
} ks_midi_event;

typedef struct ks_midi_track{
    //chunk type MTrk
    uint32_t length;
    //uint8_t *data;
    uint32_t num_events;
    ks_midi_event* events;
} ks_midi_track;

typedef struct ks_midi_file{
    // chunk type MThd
    uint32_t length;
    uint16_t format;
    uint16_t num_tracks;
    uint16_t resolution;
    ks_midi_track* tracks;
}ks_midi_file;

bool ks_io_variable_length_number(ks_io* io, const ks_io_funcs*funcs, ks_property prop, bool serialize);

ks_io_decl_custom_func(ks_midi_event);
ks_io_decl_custom_func(ks_midi_track);
ks_io_decl_custom_func(ks_midi_file);

ks_midi_file* ks_midi_file_new();
void ks_midi_file_free(ks_midi_file* file);

void ks_midi_file_calc_time(ks_midi_file* file);

ks_midi_file *ks_midi_file_conbine_tracks(ks_midi_file *file);

ks_midi_track* ks_midi_tracks_new(uint32_t num_tracks);
void ks_midi_tracks_free(uint32_t num_tracks, ks_midi_track* tracks);
ks_midi_event* ks_midi_events_new(uint32_t num_events);
void ks_midi_events_free(uint32_t num_events, ks_midi_event* events);

