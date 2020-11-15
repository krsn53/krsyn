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


    {
        printf("Binary serialize test\n");
        ks_io_custom_func_serializer(ks_synth_binary)(&io, &binary_little_endian_serializer, &bin, 0);

        for(unsigned i = 0; i< str->length; i++){
            printf("%d, ", (uint8_t)str->ptr[i]);
        }

        bin = (ks_synth_binary){ 0 };
        printf("\nBinary deserialize test\n");
        ks_io_custom_func_deserializer(ks_synth_binary)(&io, &binary_little_endian_deserializer, &bin, 0);


        for(unsigned i = 0; i< sizeof(ks_synth_binary); i++){
            printf("%d, ", ((uint8_t*)&bin)[i]);
        }
        printf("\n\n");
    }

    io.seek = 0;
    ks_string_clear(str);

    {
        printf("c like serialize test\n");

        ks_io_custom_func_serializer(ks_synth_binary)(&io, &clike_serializer, &bin, 0);

        printf("%s\n", str->ptr);

        bin = (ks_synth_binary){ 0 };
        printf("\nc like deserialize test\n");
        ks_io_custom_func_deserializer(ks_synth_binary)(&io, &clike_deserializer, &bin, 0);

        for(unsigned i = 0; i< sizeof(ks_synth_binary); i++){
            printf("%d, ", ((uint8_t*)&bin)[i]);
        }

        printf("\n\n");
    }

    ks_string_free(str);

    return 0;
}
