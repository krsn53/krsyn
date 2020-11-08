#include "../krsyn.h"
#include <stdio.h>


int main ( void ){
    ks_string* str = ks_string_new(100);

    ks_io io={
        .str = str,
        .seek = 0,
        .indent =0,
    };

    ks_synth_binary bin = {
#include "./EPiano.ksyt"
    };


    printf("Binary serialize test\n");
    ks_io_custom_func(ks_synth_binary)(&io, &binary_io, &bin, 0, true);

    for(unsigned i = 0; i< str->length; i++){
        printf("%d, ", (uint8_t)str->ptr[i]);
    }
    printf("\n\n");
    for(unsigned i = 4; i< str->length; i++){
       str->ptr[i] = 0;
    }
    printf("Binary deserialize test\n");
    ks_io_custom_func(ks_synth_binary)(&io, &binary_io, &bin, 0, false);


    for(unsigned i = 0; i< sizeof(ks_synth_binary); i++){
        printf("%d, ", ((uint8_t*)&bin)[i]);
    }
    printf("\n\n");

    io.seek = 0;
    ks_string_clear(str);

    printf("Default (c language) serialize test\n");
    ks_io_custom_func(ks_synth_binary)(&io, &default_io, &bin, 0, true);

    printf("%s\n", str->ptr);

    ks_string_free(str);

    return 0;
}
