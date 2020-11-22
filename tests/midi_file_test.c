#include "../krsyn/midi.h"

#include <stdio.h>

int main(int argc, char** argv) {

    ks_io *io = ks_io_new();
    ks_midi_file midi;

    if(!ks_io_read_file(io, "test.mid")) return -1;

    ks_io_begin_deserialize(io, binary_big_endian, ks_prop_root(midi, ks_midi_file));
    ks_io_begin_serialize(io, clike, ks_prop_root(midi, ks_midi_file));

    printf("%s", io->str->data);

    ks_midi_file_conbine_tracks(&midi);

    ks_io_begin_serialize(io, binary_big_endian, ks_prop_root(midi, ks_midi_file));

    FILE* fp = fopen("test0.mid", "wb");
    fwrite(io->str->data, 1, io->str->length, fp);
    fclose(fp);

    ks_midi_tracks_free(midi.num_tracks, midi.tracks);

    ks_io_free(io);
    return 0;
}
