#include <ksio/logger.h>
#include "score.h"
#include "tone_list.h"
#include "math.h"
#include "midi.h"

#include <stdlib.h>

ks_io_begin_custom_func(ks_score_event)
    ks_func_prop(ks_io_variable_length_number, ks_prop_u32(delta));
    ks_u8(status);
    if(ks_access(status) == 0xff ) {
        ks_u8(data[0]);
        if(ks_access(data[0]) == 0x51){
            ks_u8(data[2]);
            ks_u8(data[3]);
        }
    }
     else {
        switch (ks_access(status) >> 4) {
        case 0xc:
        case 0xd:
            ks_u8(data[0]);
            break;
        default:
            ks_u8(data[0]);
            ks_u8(data[1]);
            break;
        }
    }
ks_io_end_custom_func(ks_score_event)

ks_io_begin_custom_func(ks_score_data)
    ks_magic_number("KSCR");
    ks_str(title);
    ks_str(author);
    ks_str(license);
    ks_u16(resolution);
    ks_u32(length);
    ks_arr_obj_len(data, ks_score_event, ks_access(length));
ks_io_end_custom_func(ks_score_data)

inline bool ks_score_note_is_enabled(const ks_score_note* note){
    return ks_synth_note_is_enabled(&note->note);
}

inline bool ks_score_note_is_on(const ks_score_note* note){
    return ks_synth_note_is_on(&note->note);
}

inline bool ks_score_note_info_equals(ks_score_note_info i1, ks_score_note_info i2){
    return (((i1.channel)<<8) + i1.note_number) == (((i2.channel)<<8) + i2.note_number);
}

inline i16 ks_score_note_info_hash(ks_score_note_info id){
    return id.note_number +  id.channel;
}

inline ks_score_note_info ks_score_note_info_of(u8 note_number, u8 channel){
    return (ks_score_note_info){
                .note_number = note_number,
                .channel = channel,
            };
}


ks_score_data* ks_score_data_new(u32 resolution, u32 num_events, ks_score_event* events){
    ks_score_data* ret = malloc(sizeof(ks_score_data));
    ret->data = events;
    ret->length = num_events;
    ret->resolution = resolution;

    return ret;
}

void ks_score_data_free(ks_score_data* song){
    ks_score_events_free(song->data);
    free(song);
}


static inline u32 calc_frames_per_event(u32 sampling_rate, u16 quater_time, u16 resolution){
    u64 ret = sampling_rate;
    ret *= quater_time;
    ret /= resolution;
    ret >>= KS_QUARTER_TIME_BITS;
    return (u32)ret;
}

ks_score_state* ks_score_state_new(u32 polyphony_bits){
    ks_score_state* ret = malloc(sizeof(ks_score_state) + ks_1(polyphony_bits)* sizeof(ks_score_note));
    ret->polyphony_bits = polyphony_bits;

    return ret;
}

void ks_score_state_free(ks_score_state* state){
    free(state);
}

bool ks_score_state_note_on(ks_score_state* state, u32 sampling_rate, u8 channel_number, ks_score_channel* channel, u8 note_number, u8 velocity){

     ks_synth* synth = channel->program;
     if(synth == NULL) {
         ks_error("Failed to run note_on for not set program of channel %d at tick %d", channel_number, state->current_tick);
         return false;
     }

    ks_score_note_info id =ks_score_note_info_of(note_number, channel_number);
    u16 id_v = ks_score_note_info_hash(id);
    u16 begin = ks_mask(id_v, state->polyphony_bits);
    u16 index = begin;
    while(ks_score_note_is_enabled(&state->notes[index])){
        // if found same channel and notenumber note, note off
        if(ks_score_note_info_equals(id, state->notes[index].info)){
            ks_synth_note_off(&state->notes[index].note);
        }

        index++;
        index = ks_mask(index, state->polyphony_bits);

        if(index == begin) {
            ks_warning("Failed to run note_on for exceeded maximum of polyphony at tick %d", channel_number, state->current_tick);
            return false;
        }
    }

    state->notes[index].synth = synth;
    state->notes[index].info = id;
    ks_synth_note_on(&state->notes[index].note, synth, sampling_rate, note_number, velocity);

    return true;
}

bool ks_score_state_note_off(ks_score_state* state, u8 channel_number, ks_score_channel* channel, u8 note_number){
    ks_tone_list_bank* bank = channel->bank;
    if(bank == NULL) {
        ks_error("Failed to run note_off for not set bank of channel %d at tick %d", channel_number, state->current_tick);
        return false;
    }

    if(ks_tone_list_bank_is_empty(bank)) {
        ks_error("Failed to run note_off for not set bank of channel %d at tick %d", channel_number, state->current_tick);
        return false;
    }
    ks_score_note_info info =ks_score_note_info_of(note_number, channel_number);
    u16 hash = ks_score_note_info_hash(info);
    u16 begin = ks_mask(hash, state->polyphony_bits);
    u16 index = begin;
    while(!ks_score_note_info_equals(state->notes[index].info, info) || !ks_score_note_is_on(&state->notes[index])) {
        index++;
        index = ks_mask(index, state->polyphony_bits);
        if(index == begin) {
            ks_warning("Failed to run note_off for not found note with note number %d and channel %d at tick %d", note_number, channel_number, state->current_tick);
            return false;
        }
    }

    ks_synth_note_off(&state->notes[index].note);

    return true;
}

bool ks_score_state_program_change(ks_score_state* state, const ks_tone_list* tones, ks_score_channel* channel, u8 program){

    if(channel->bank == NULL) {
        ks_error("Failed to run program_change of channel %d for not set bank at tick %d", (channel - state->channels) / sizeof(ks_score_channel),state->current_tick);
        return false;
    }

    ks_synth* p = channel->bank->programs[program];
    if(p == NULL){
        ks_tone_list_bank* bank = ks_tone_list_find_bank(tones, ks_tone_list_bank_number_of(0,0));
        if(bank == NULL){
            bank = tones->data;
        }
        p = channel->bank->programs[program];

        ks_tone_list_bank* begin = tones->data;
        u32 it=0;
        while(begin[it].programs[program] == NULL){
            it++;
            it %= tones->length;
            if(it == 0){
                ks_error("Failed to run program_change of channel %d for not found program %d at tick %d",(channel - state->channels) / sizeof(ks_score_channel), program,state->current_tick);
                return  false;
            }
        }
    }
    channel->program_number = program;
    channel->program = p;
    return true;
}

bool ks_score_state_bank_select(ks_score_state* state, const ks_tone_list* tones, ks_score_channel* channel, u8 msb, u8 lsb){
    ks_tone_list_bank *bank = ks_tone_list_find_bank(tones, ks_tone_list_bank_number_of(msb, lsb));
    if(bank == NULL) {
        ks_error("Failed to run bank_select of channel %d for not found bank at tick %d", (channel - state->channels) / sizeof(ks_score_channel),state->current_tick);
        return false;
    }

    channel->bank = bank;

    return ks_score_state_program_change(state, tones, channel, channel->program_number);
}

bool ks_score_state_bank_select_msb(ks_score_state* state, const ks_tone_list* tones, ks_score_channel* channel, u8 msb){
    u8 lsb = 0;
    if(channel->bank != NULL){
        lsb = channel->bank->bank_number.lsb;
    }
    return ks_score_state_bank_select(state, tones, channel, msb, lsb);
}

bool ks_score_state_bank_select_lsb(ks_score_state* state, const ks_tone_list* tones, ks_score_channel* channel, u8 lsb){
    u8 msb = 0;
    if(channel->bank != NULL){
        msb = channel->bank->bank_number.msb;
    }
    return ks_score_state_bank_select(state, tones, channel, msb, lsb);
}

bool ks_score_channel_set_panpot(ks_score_channel* channel, u8 value){
    ks_calc_panpot(&channel->panpot_left, &channel->panpot_right, value);
    return true;
}

bool ks_score_channel_set_picthbend(ks_score_channel* channel, u8 msb, u8 lsb){
    channel->pitchbend = ks_v(msb, 7) + lsb - ks_1(KS_PITCH_BEND_BITS);
    channel->pitchbend = ks_fms_depth(channel->pitchbend << (KS_LFO_DEPTH_BITS - KS_PITCH_BEND_BITS));
    return true;
}

bool ks_score_state_tempo_change(ks_score_state* state, u32 sampling_rate, const ks_score_data* score, const u8* data){
    const u32 quarter = data[1] + ks_v(data[2], KS_QUARTER_TIME_BITS);
    state->quater_time = quarter;
    state->frames_per_event= calc_frames_per_event(sampling_rate, state->quater_time, score->resolution);
    return true;
}

bool ks_score_state_control_change(ks_score_state* state, const ks_tone_list* tones, ks_score_channel* channel, u8 type, u8 value){
    switch (type) {
    case 0x00:
        return ks_score_state_bank_select_msb(state, tones, channel, value);
    case 0x20:
        return ks_score_state_bank_select_lsb(state, tones, channel, value);
    case 0x10:
        return ks_score_channel_set_panpot(channel, value);
    }
    return true;
}


void ks_score_data_render(const ks_score_data *score, u32 sampling_rate, ks_score_state* state, const ks_tone_list*tones, i16* buf, u32 len){
    u32 i=0;
    memset(buf, 0, sizeof(i16)*len);
    do{
        u32 frame = MIN(len-i, state->remaining_frame*2);
        i16 tmpbuf[frame];

        for(u32 p=0; p<ks_1(state->polyphony_bits); p++){
            if(!ks_score_note_is_enabled(&state->notes[p])) {
                continue;
            }

            ks_score_channel* channel = &state->channels[state->notes[p].info.channel];

            // when note on, already checked,
            //if(channel->bank == NULL) continue;
            //if(channel->bank->programs[channel->program_number] == NULL) continue;
            ks_synth* synth = channel->program;
            ks_synth_note* note = &state->notes[p].note;
            ks_synth_render(synth, note, channel->pitchbend, tmpbuf, frame);

            for(u32 b =0; b< frame; b+=2){
                buf[(i + b)] += ks_apply_panpot(tmpbuf[b], channel->panpot_left);
                buf[(i + b) + 1] += ks_apply_panpot(tmpbuf[b+1], channel->panpot_right);
            }
        }

        state->remaining_frame -= frame >> 1;

        if(state->remaining_frame == 0){
            if(state->passed_tick >= score->data[state->current_event].delta){
                state->passed_tick -= score->data[state->current_event].delta;
                if(ks_score_data_event_run(score, sampling_rate, state, tones)){
                    state->passed_tick = -1;
                }
            } else {
                state->passed_tick ++;
            }
            state->current_tick ++;

            state->remaining_frame += state->frames_per_event;
        }

        i+= frame;
    }while(i<len);
}

ks_score_event* ks_score_events_new(u32 num_events, ks_score_event events[]){
    ks_score_event* ret = malloc(sizeof(ks_score_event) * num_events);
    for(u32 i=0; i<num_events; i++){
        ret[i] = events[i];
    }
    return ret;
}

void ks_score_events_free(const ks_score_event* events){
    free((ks_score_event*)events);
}



bool ks_score_data_event_run(const ks_score_data* score, u32 sampling_rate, ks_score_state *state,  const ks_tone_list *tones){
    do{
        const ks_score_event* msg = &score->data[state->current_event];
        switch (msg->status) {
        case 0xff:
            // tempo
            if(msg->data[0] == 0x51){
                ks_score_state_tempo_change(state, sampling_rate, score, msg->data);
            }
            // end of track
            else if(msg->data[0] == 0x2f){
                return true;
            }


        default:{
            // channel message
            u8 channel_num = msg->status & 0x0f;
            ks_score_channel* channel = &state->channels[channel_num];
            switch (msg->status >> 4) {
            // note off
            case 0x8:
                ks_score_state_note_off(state, channel_num, channel, msg->data[0]);
                break;
            // note on
            case 0x9:
                ks_score_state_note_on(state, sampling_rate, channel_num, channel, msg->data[0], msg->data[1]);
                break;
            // control change
            case 0xb:
                ks_score_state_control_change(state, tones, channel, msg->data[0], msg->data[1]);
                break;
            // program change
            case 0xc:
                ks_score_state_program_change(state, tones, channel, msg->data[0]);
                break;
            // pich wheel change
            case 0xe:
                ks_score_channel_set_picthbend(channel, msg->data[1], msg->data[2]);
                break;
            }
        }
        }
        state->current_event++;
    }while(score->data[state->current_event].delta == 0); // state->current_event < score->length

    return false;
}


void ks_score_state_set_default(ks_score_state* state, const ks_tone_list *tones, u32 sampling_rate, u32 resolution){
    state->quater_time = ks_1(KS_QUARTER_TIME_BITS - 1); // 0.5
    state->remaining_frame = 0;
    state->current_event = 0;
    state->frames_per_event = calc_frames_per_event(sampling_rate, state->quater_time, resolution);
    state->passed_tick = 0;
    for(u32 i=0; i<ks_1(state->polyphony_bits); i++) {
        memset(&state->notes[i], 0 , sizeof(ks_score_note));
    }
    for(u32 i =0; i<KS_NUM_CHANNELS; i++){
        memset(&state->channels[i], 0 , sizeof(ks_score_channel));
        ks_score_channel_set_panpot(&state->channels[i], 64);
        ks_score_channel_set_picthbend(&state->channels[i], 0, 0);
        ks_score_state_bank_select(state, tones, &state->channels[i], 0, 0);
    }
}

ks_score_data* ks_score_data_from_midi(ks_midi_file* file){
    const ks_midi_file* conbined = file->format == 0 ? file : ks_midi_file_conbine_tracks(file);

    ks_score_data* ret = ks_score_data_new(file->resolution, 0, NULL);
    ks_score_event* events = calloc(file->tracks[0].num_events, sizeof(ks_score_event));


    u64 time=0;
    for(u32 i=0; i< file->tracks[0].num_events; i++){
        ks_midi_event* msg = &file->tracks[0].events[i];
        switch (msg->status) {
        case 0xff:
            // tempo
            if(msg->message.meta.type == 0x51){
                events[ret->length].status = 0xff;
                events[ret->length].data[0] = 0x51;
                //msg->message.meta.length == 0x03;
                const u32 quarter_micro = ks_v((u32)msg->message.meta.data[0], 16) +
                        ks_v((u32)msg->message.meta.data[1], 8) +
                       (u32)msg->message.meta.data[2];
                const u32 quarter_mili = quarter_micro / 1000;
                const u32 quarter_mili_fp8 = quarter_mili * ks_1(KS_QUARTER_TIME_BITS);
                const u32 quarter_fp8 = quarter_mili_fp8 / 1000;
                // little endian
                events[ret->length].data[1] = ks_mask(quarter_fp8, KS_QUARTER_TIME_BITS);
                events[ret->length].data[2] = quarter_fp8 >> KS_QUARTER_TIME_BITS;

                ret->length++;
                events[ret->length].delta = msg->time - time;
                time = msg->time;
            }
            // end of track
            else if(msg->message.meta.type == 0x2f){
                events[ret->length].status = 0xff;
                events[ret->length].data[0] = 0x2f;
                events[ret->length].data[1] = 0x00;

                ret->length++;
                events[ret->length].delta = msg->time - time;
                time = msg->time;
            }
            break;
        default:
            if(msg->status >= 0x80 &&
                    msg->status < 0xe0){
                events[ret->length].status = msg->status;
                events[ret->length].data[0] = msg->message.data[0];
                events[ret->length].data[1] = msg->message.data[1];

                ret->length++;
                events[ret->length].delta = msg->time - time;
                time = msg->time;
            }
            break;
        }
    }
    if(file->format != 0) ks_midi_file_free((ks_midi_file*)conbined);

    ret->data = events;

    return ret;
}
