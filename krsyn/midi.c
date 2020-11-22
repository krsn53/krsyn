#include "midi.h"
#include "math.h"
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

    if(!__SERIALIZE){
        ks_access(time) = (__INDEX == 0 ? 0 : ks_access_before(time, 1)) + ks_access(delta);
    }
    ks_fp_u8(status);
    switch (ks_access(status)) {
        //SysEx
        case 0xf0:
        case 0xf7:
            ks_func_prop(ks_io_variable_length_number, ks_prop_u32(message.sys_ex.length));
            ks_fp_arr_u8_len(message.sys_ex.data, ks_access(message.sys_ex.length));
            break;
       //meta
        case 0xff:
            ks_fp_u8(message.meta.type);
            ks_func_prop(ks_io_variable_length_number, ks_prop_u32(message.meta.length));
            if(ks_access(message.meta.type) > 0x00 && ks_access(message.meta.type) < 0x08){
                ks_fp_str_len(message.meta.data, ks_access(message.meta.length));
            } else {
                 ks_fp_arr_u8_len(message.meta.data, ks_access(message.meta.length));
            }
            break;
       //midi event
        case 0xf1:
        case 0xf3:
            ks_fp_u8(message.data[0]);
            break;
        case 0xf6:
        case 0xf8:
        case 0xfa:
        case 0xfb:
        case 0xfc:
        case 0xfe:
            ks_fp_arr_u8(message.data);
            break;
        default:
            switch (ks_access(status) >> 4) {
            case 0xc:
            case 0xd:
                ks_fp_u8(message.data[0]);
                break;
            default:
                 ks_fp_arr_u8(message.data);
                break;
            }
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
                ks_access(events)[ks_access(num_events)].message.meta.type == 0x2f){
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

int compare_midi_event_time (const void *a, const void *b){
    const ks_midi_event* a1 = a, *b1 =b;
    return (int64_t)a1->time - (int64_t)b1->time;
}

void ks_midi_file_conbine_tracks(ks_midi_file* file){
    ks_midi_file ret;

    ret.format = 0;
    ret.length = file->length;
    ret.num_tracks = 1;
    ret.resolution = file->resolution;
    ret.tracks = ks_midi_tracks_new(1);

    uint32_t num_events = 0;
    uint32_t length =0;
    for(uint32_t i=0; i<file->num_tracks; i++){
        length += file->tracks[i].length;
        num_events += file->tracks[i].num_events;
    }
    // end of track * (num_tracks-1)
    num_events -= file->num_tracks -1;

    ret.tracks[0].length = length;
    ret.tracks[0].num_events = num_events;
    ret.tracks[0].events = ks_midi_events_new(num_events);

    uint32_t e=0;
    uint32_t end_time=0;

    for(uint32_t i=0; i<file->num_tracks; i++){
        for(uint32_t j=0; j<file->tracks[i].num_events; j++){
            if(file->tracks[i].events[j].status == 0xff &&
            file->tracks[i].events[j].message.meta.type == 0x2f){
                end_time = MAX(end_time, file->tracks[i].events[j].time);
                uint32_t delta = file->tracks[i].events[j].delta;
                int32_t delta_bits=0;
                do{
                    delta_bits++;
                    delta >>= 7;
                }while(delta != 0);
                ret.tracks[0].length -= 3 + delta_bits;
                continue;
            }
            ret.tracks[0].events[e] = file->tracks[i].events[j];
            e++;
        }
    }
    ret.tracks[0].events[e] = (ks_midi_event){
        .status = 0xff,
        .message.meta.type = 0x2f,
        .message.meta.length = 0,
        .time = end_time,
        .delta = 0,
    };
    ret.tracks[0].length += 1 + 3; // 1 delta bit + 3 event bits

    qsort(ret.tracks->events, num_events, sizeof(ks_midi_event), compare_midi_event_time);

    {
        uint32_t old = ret.tracks[0].events[0].delta;
        int32_t old_bits = 0;
        uint32_t new = ret.tracks[0].events[0].time;
        int32_t new_bits = 0;

        do{
            old_bits++;
            old >>= 7;
        }while(old != 0);
        do{
            new_bits++;
            new >>= 7;
        }while(new != 0);

        ret.tracks[0].length -= old_bits - new_bits;
    }

    for(uint32_t i=1; i<ret.tracks[0].num_events; i++){
        uint32_t old = ret.tracks[0].events[i].delta;
        uint32_t new;
        int32_t old_bits = 0;
        int32_t new_bits = 0;

        ret.tracks[0].events[i].delta = new = ret.tracks[0].events[i].time - ret.tracks[0].events[i-1].time;
        do{
            old_bits++;
            old >>= 7;
        }while(old != 0);
        do{
            new_bits++;
            new >>= 7;
        }while(new != 0);
        ret.tracks[0].length -= old_bits - new_bits;
    }

    for(uint32_t i=0; i<file->num_tracks; i++){
        free(file->tracks[i].events);
    }
    free(file->tracks);

    *file = ret;
}

ks_midi_track* ks_midi_tracks_new(uint32_t num_tracks){
    return calloc(num_tracks, sizeof(ks_midi_track));
}

ks_midi_event* ks_midi_events_new(uint32_t num_events){
    return calloc(num_events, sizeof(ks_midi_event));
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
            if(events[e].message.sys_ex.data != NULL && events[e].message.sys_ex.length > 0){
                free(events[e].message.sys_ex.data);
            }
            break;
        case 0xff:
            if(events[e].message.meta.data != NULL && events[e].message.meta.length > 0){
                free(events[e].message.meta.data);
            }
            break;
        default:
            break;
        }
    }
    free(events);
}
