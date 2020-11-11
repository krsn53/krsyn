#pragma once

#include <stdint.h>

typedef struct ks_midi_file{
    // chunk type MThd
    uint32_t header_length;
    uint16_t format;
    uint16_t num_tracks;
    uint16_t resolution;
    //chunk type MTrk
    uint32_t track_length;
}ks_midi_file;
