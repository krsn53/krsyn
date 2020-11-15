#include "midi.h"

ks_io_begin_custom_func(ks_midi_track)
    ks_magic_number("MTrk");
    ks_fp_u32(length);
    ks_fp_arr_len_u8(data, ks_elem_access(length));
ks_io_end_custom_func(ks_midi_track)

ks_io_begin_custom_func(ks_midi_file)
    ks_magic_number("MThd");
    ks_fp_u32(length);
    ks_fp_u16(format);
    ks_fp_u16(num_tracks);
    ks_fp_u16(resolution);
    ks_fp_arr_len_obj(tracks, ks_midi_track, ks_elem_access(num_tracks));
ks_io_end_custom_func(ks_midi_file)
