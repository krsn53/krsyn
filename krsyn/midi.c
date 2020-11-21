#include "midi.h"
#include <string.h>

bool ks_io_variable_length_number(ks_io* io, const ks_io_funcs*funcs, ks_property prop,  bool serialize){
    if(funcs != &binary_big_endian_serializer && funcs != & binary_big_endian_deserializer) {
        return ks_io_fixed_property(io, funcs, prop, serialize);
    }

    if(serialize){
        uint8_t out[4] = { 0 };
        uint32_t in = *(uint32_t*)prop.value.data;

        ks_property p = prop;
        p.value.type = KS_VALUE_U8;

        out[0] = in & 0x7f;
        in >>= 7;

        int32_t i = 0;

        while(in != 0){
            i++;
            out[i] = in & 0x7f;
            out[i] |= 0x80;
            in >>= 7;
        }
        for(; i>=0; i--){
            p.value.data =  &out[i];
            if(!ks_io_value(io, funcs, p, 0, serialize)) return false;
        }

        return true;
    } else {
        uint8_t in=0;

        ks_property p = prop;
        p.value.type = KS_VALUE_U8;
        p.value.data = & in;
        uint32_t out=0;

        do{
            if(!ks_io_value(io, funcs, p, 0, serialize)) return false;
            out <<= 7;
            out |= in & 0x7f;
        }while(in >= 0x80);

        *(uint32_t*)prop.value.data = out;

        return true;
    }
}

ks_io_begin_custom_func(ks_midi_event)
    ks_func_prop(ks_io_variable_length_number, ks_prop_u32(delta));
    ks_fp_u8(status);
    switch (ks_access(status)) {
        //SysEx
        case 0xf0:
        case 0xf7:
            ks_func_prop(ks_io_variable_length_number, ks_prop_u32(data.sys_ex.length));
            ks_fp_arr_u8_len(data.sys_ex.data, ks_access(data.sys_ex.length));
            break;
       //meta
        case 0xff:
            ks_fp_u8(data.meta.type);
            ks_func_prop(ks_io_variable_length_number, ks_prop_u32(data.meta.length));
            if(ks_access(data.meta.type) > 0x00 && ks_access(data.meta.type) < 0x08){
                ks_fp_str_len(data.meta.data, ks_access(data.meta.length));
            } else {
                 ks_fp_arr_u8_len(data.meta.data, ks_access(data.meta.length));
            }
            break;
       //midi event
        default:
            ks_fp_arr_u8(data.data);
            break;
    }
ks_io_end_custom_func(ks_midi_event)

ks_io_begin_custom_func(ks_midi_track)
    ks_magic_number("MTrk");
    ks_fp_u32(length);

    if(__SERIALIZE){
        ks_fp_arr_obj_len(events, ks_midi_event, ks_access(num_events));
    } else {
        ks_access(num_events) = 0;

        ks_array_data arr = ks_prop_arr_data_len(ks_access(length) / 4, events, ks_value_obj(events, ks_midi_event), NULL, false);

        if(!ks_io_array_begin(__IO, __FUNCS, &arr, 0, __SERIALIZE)) return false;

        for(;;){
            __FUNCS->array_elem(__IO, __FUNCS, &arr, ks_access(num_events));
            if(ks_access(events)[ks_access(num_events)].status == 0xff &&
                ks_access(events)[ks_access(num_events)].data.meta.type == 0x2f){
                ks_access(num_events) ++;
                break;
            }
            ks_access(num_events) ++;
        }

        ks_access(events) = realloc(ks_access(events), sizeof(ks_midi_event)*ks_access(num_events));

        if(!ks_io_array_end(__IO, __FUNCS, &arr, 0, __SERIALIZE)) return false;
    }
ks_io_end_custom_func(ks_midi_track)

ks_io_begin_custom_func(ks_midi_file)
    ks_magic_number("MThd");
    ks_fp_u32(length);
    ks_fp_u16(format);
    ks_fp_u16(num_tracks);
    ks_fp_u16(resolution);
    ks_fp_arr_obj_len(tracks, ks_midi_track, ks_access(num_tracks));
ks_io_end_custom_func(ks_midi_file)


ks_midi_file* ks_midi_file_new(){
    ks_midi_file* midi = calloc(1, sizeof(ks_midi_file));
    return midi;
}

void ks_midi_file_free(ks_midi_file* file){
    ks_midi_tracks_free(file->num_tracks, file->tracks);
    free(file);
}

void ks_midi_tracks_free(uint32_t num_tracks, ks_midi_track* tracks){
    for(uint32_t t=0; t < num_tracks; t++){
        ks_midi_events_free(tracks[t].num_events, tracks[t].events);
    }
    free(tracks);
}

void ks_midi_events_free(uint32_t num_events, ks_midi_event* events){
    for(uint32_t e=0; e<num_events; e++){
        switch (events[e].status) {
        case 0xf0:
        case 0xf7:
            if(events[e].data.sys_ex.data != NULL && events[e].data.sys_ex.length > 0){
                free(events[e].data.sys_ex.data);
            }
            break;
        case 0xff:
            if(events[e].data.meta.data != NULL && events[e].data.meta.length > 0){
                free(events[e].data.meta.data);
            }
            break;
        default:
            break;
        }
    }
    free(events);
}
