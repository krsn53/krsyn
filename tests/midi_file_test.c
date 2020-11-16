#include "../krsyn/midi.h"

#include <stdio.h>

int main(int argc, char** argv) {
    ks_string *str = ks_string_new();

    FILE *f = fopen("test.mid", "rb");
    if(!f) return -1;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    ks_string_resize(str, fsize+1);

    fread(str->data, 1, fsize, f);
    fclose(f);

    ks_io io = {
        .indent =0,
        .seek = 0,
        .str = str,
    };

    ks_midi_file midi = { 0 };

    ks_io_custom_func_deserializer(ks_midi_file)(&io, &binary_big_endian_deserializer, &midi, 0);

    ks_string_clear(str);

    ks_io_custom_func_serializer(ks_midi_file)(&io, &clike_serializer, &midi, 0);

    printf("%s\n", str->data);

    ks_string_free(str);

    return 0;
}
