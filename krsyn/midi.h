#pragma once

#include "./io.h"

typedef struct ks_midi_event{
    uint32_t delta;
    uint8_t status;
    uint8_t* data;
} ks_midi_event;

typedef struct ks_midi_track{
    //chunk type MTrk
    uint32_t length;
    uint8_t *data;
} ks_midi_track;

typedef struct ks_midi_file{
    // chunk type MThd
    uint32_t length;
    uint16_t format;
    uint16_t num_tracks;
    uint16_t resolution;
    ks_midi_track* tracks;
}ks_midi_file;


ks_io_decl_custom_func(ks_midi_track);
ks_io_decl_custom_func(ks_midi_file);
