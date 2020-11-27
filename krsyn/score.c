#include "logger.h"
#include "score.h"
#include "tones.h"
#include "math.h"
#include "midi.h"

#include <stdlib.h>

ks_io_begin_custom_func(ks_score_event)
    ks_func_prop(ks_io_variable_length_number, ks_prop_u32(delta));
    ks_fp_u8(status);
    if(ks_access(status) == 0xff ) {
        ks_fp_u8(datas[0]);
        if(ks_access(datas[0]) == 0x51){
            ks_fp_u8(datas[2]);
            ks_fp_u8(datas[3]);
        }
    }
     else {
        switch (ks_access(status) >> 4) {
        case 0xc:
        case 0xd:
            ks_fp_u8(datas[0]);
            break;
        default:
            ks_fp_u8(datas[0]);
            ks_fp_u8(datas[1]);
            break;
        }
    }
ks_io_end_custom_func(ks_score_event)

ks_io_begin_custom_func(ks_score_data)
    ks_magic_number("KSCR");
    ks_fp_str(title);
    ks_fp_str(author);
    ks_fp_str(license);
    ks_fp_u16(resolution);
    ks_fp_u32(num_events);
    ks_fp_arr_obj_len(events, ks_score_event, ks_access(num_events));
ks_io_end_custom_func(ks_score_data)

inline bool ks_score_note_is_enabled(const ks_score_note* note){
    return ks_synth_note_is_enabled(&note->note);
}
inline bool ks_score_note_info_equals(ks_score_note_info i1, ks_score_note_info i2){
    return (((i1.channel)<<8) + i1.note_number) == (((i2.channel)<<8) + i2.note_number);
}

inline int16_t ks_score_note_info_hash(ks_score_note_info id){
    return id.note_number +  id.channel;
}

inline ks_score_note_info ks_score_note_info_of(uint8_t note_number, uint8_t channel){
    return (ks_score_note_info){
                .note_number = note_number,
                .channel = channel,
            };
}


ks_score_data* ks_score_data_new(uint32_t resolution, uint32_t num_events, const ks_score_event* events){
    ks_score_data* ret = malloc(sizeof(ks_score_data));
    ret->events = events;
    ret->num_events = num_events;
    ret->resolution = resolution;

    return ret;
}

void ks_score_data_free(ks_score_data* song){
    ks_score_events_free(song->events);
    free(song);
}


static inline uint32_t calc_frames_per_event(uint32_t sampling_rate, uint16_t quater_time, uint16_t resolution){
    uint64_t ret = sampling_rate;
    ret *= quater_time;
    ret /= resolution;
    ret >>= KS_QUARTER_TIME_BITS;
    return (uint32_t)ret;
}

ks_score_state* ks_score_state_new(uint32_t polyphony_bits){
    ks_score_state* ret = malloc(sizeof(ks_score_state) + ks_1(polyphony_bits)* sizeof(ks_score_note));
    ret->polyphony_bits = polyphony_bits;

    return ret;
}

void ks_score_state_free(ks_score_state* state){
    free(state);
}

bool ks_score_state_note_on(ks_score_state* state, uint32_t sampling_rate, uint8_t channel_number, ks_score_channel* channel, uint8_t note_number, uint8_t velocity){

     ks_synth* synth = channel->program;
     if(synth == NULL) {
         ks_error("Failed to run note_on for not set program of channel %d at tick %d", channel_number, state->current_tick);
         return false;
     }

    ks_score_note_info id =ks_score_note_info_of(note_number, channel_number);
    uint16_t id_v = ks_score_note_info_hash(id);
    uint16_t begin = ks_mask(id_v, state->polyphony_bits);
    uint16_t index = begin;
    while(ks_score_note_is_enabled(&state->notes[index])){
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

bool ks_score_state_note_off(ks_score_state* state, uint8_t channel_number, ks_score_channel* channel, uint8_t note_number){
    ks_tones_bank* bank = channel->bank;
    if(bank == NULL) {
        ks_error("Failed to run note_off for not set bank of channel %d at tick %d", channel_number, state->current_tick);
        return false;
    }

    if(ks_tones_bank_is_empty(bank)) {
        ks_error("Failed to run note_off for not set bank of channel %d at tick %d", channel_number, state->current_tick);
        return false;
    }
    ks_score_note_info info =ks_score_note_info_of(note_number, channel_number);
    uint16_t hash = ks_score_note_info_hash(info);
    uint16_t begin = ks_mask(hash, state->polyphony_bits);
    uint16_t index = begin;
    while(!ks_score_note_info_equals(state->notes[index].info, info)) {
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

bool ks_score_state_program_change(ks_score_state* state, const ks_tones* tones, ks_score_channel* channel, uint8_t program){

    if(channel->bank == NULL) {
        ks_error("Failed to run program_change of channel %d for not set bank at tick %d", (channel - state->channels) / sizeof(ks_score_channel),state->current_tick);
        return false;
    }

    ks_synth* p = channel->bank->programs[program];
    if(p == NULL){
        ks_tones_bank* bank = ks_tones_find_bank(tones, ks_tones_bank_number_of(0,0));
        if(bank == NULL){
            bank = tones->banks;
        }
        p = channel->bank->programs[program];

        ks_tones_bank* begin = tones->banks;
        uint32_t it=0;
        while(begin[it].programs[program] == NULL){
            it++;
            it %= tones->num_banks;
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

bool ks_score_state_bank_select(ks_score_state* state, const ks_tones* tones, ks_score_channel* channel, uint8_t msb, uint8_t lsb){
    ks_tones_bank *bank = ks_tones_find_bank(tones, ks_tones_bank_number_of(msb, lsb));
    if(bank == NULL) {
        ks_error("Failed to run bank_select of channel %d for not found bank at tick %d", (channel - state->channels) / sizeof(ks_score_channel),state->current_tick);
        return false;
    }

    channel->bank = bank;

    return ks_score_state_program_change(state, tones, channel, channel->program_number);
}

bool ks_score_state_bank_select_msb(ks_score_state* state, const ks_tones* tones, ks_score_channel* channel, uint8_t msb){
    uint8_t lsb = 0;
    if(channel->bank != NULL){
        lsb = channel->bank->bank_number.lsb;
    }
    return ks_score_state_bank_select(state, tones, channel, msb, lsb);
}

bool ks_score_state_bank_select_lsb(ks_score_state* state, const ks_tones* tones, ks_score_channel* channel, uint8_t lsb){
    uint8_t msb = 0;
    if(channel->bank != NULL){
        msb = channel->bank->bank_number.msb;
    }
    return ks_score_state_bank_select(state, tones, channel, msb, lsb);
}

bool ks_score_channel_set_panpot(ks_score_channel* channel, uint8_t value){
    ks_calc_panpot(&channel->panpot_left, &channel->panpot_right, value);
    return true;
}

bool ks_score_channel_set_picthbend(ks_score_channel* channel, uint8_t msb, uint8_t lsb){
    channel->pitchbend = ks_v(msb, 7) + lsb;
    channel->pitchbend = ks_fms_depth(channel->pitchbend << (KS_LFO_DEPTH_BITS - KS_PITCH_BEND_BITS));
    return true;
}

bool ks_score_state_tempo_change(ks_score_state* state, uint32_t sampling_rate, const ks_score_data* score, const uint8_t* datas){
    const uint32_t quarter = datas[1] + ks_v(datas[2], KS_QUARTER_TIME_BITS);
    state->quater_time = quarter;
    state->frames_per_event= calc_frames_per_event(sampling_rate, state->quater_time, score->resolution);
    return true;
}

bool ks_score_state_control_change(ks_score_state* state, const ks_tones* tones, ks_score_channel* channel, uint8_t type, uint8_t value){
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


void ks_score_data_render(const ks_score_data *score, uint32_t sampling_rate, ks_score_state* state, const ks_tones*tones, int16_t* buf, uint32_t len){
    uint32_t i=0;
    do{
        uint32_t frame = MIN(len-i, state->current_frame*2);
        int16_t tmpbuf[frame];

        for(uint32_t p=0; p<ks_1(state->polyphony_bits); p++){
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

            for(uint32_t b =0; b< frame; b+=2){
                buf[(i + b)] += ks_apply_panpot(tmpbuf[b], channel->panpot_left);
                buf[(i + b) + 1] += ks_apply_panpot(tmpbuf[b+1], channel->panpot_right);
            }
        }

        state->current_frame -= frame >> 1;

        if(state->current_frame == 0){

            if(state->current_tick == score->events[state->current_event].delta){
                state->current_tick = 0;
                if(ks_score_data_event_run(score, sampling_rate, state, tones)){
                    state->current_tick = -1;
                }
            } else {
                state->current_tick ++;
            }

            state->current_frame += state->frames_per_event;
        }

        i+= frame;
    }while(i<len);
}

ks_score_event* ks_score_events_new(uint32_t num_events, ks_score_event events[]){
    ks_score_event* ret = malloc(sizeof(ks_score_event) * num_events);
    for(uint32_t i=0; i<num_events; i++){
        ret[i] = events[i];
    }
    return ret;
}

void ks_score_events_free(const ks_score_event* events){
    free((ks_score_event*)events);
}



bool ks_score_data_event_run(const ks_score_data* score, uint32_t sampling_rate, ks_score_state *state,  const ks_tones *tones){
    do{
        const ks_score_event* msg = &score->events[state->current_event];
        switch (msg->status) {
        case 0xff:
            // tempo
            if(msg->datas[0] == 0x51){
                ks_score_state_tempo_change(state, sampling_rate, score, msg->datas);
            }
            // end of track
            else if(msg->datas[0] == 0x2f){
                return true;
            }


        default:{
            // channel message
            uint8_t channel_num = msg->status & 0x0f;
            ks_score_channel* channel = &state->channels[channel_num];
            switch (msg->status >> 4) {
            // note off
            case 0x8:
                ks_score_state_note_off(state, channel_num, channel, msg->datas[0]);
                break;
            // note on
            case 0x9:
                ks_score_state_note_on(state, sampling_rate, channel_num, channel, msg->datas[0], msg->datas[1]);
                break;
            // control change
            case 0xb:
                ks_score_state_control_change(state, tones, channel, msg->datas[0], msg->datas[1]);
                break;
            // program change
            case 0xc:
                ks_score_state_program_change(state, tones, channel, msg->datas[0]);
                break;
            // pich wheel change
            case 0xe:
                ks_score_channel_set_picthbend(channel, msg->datas[1], msg->datas[2]);
                break;
            }
        }
        }
        state->current_event++;
    }while(score->events[state->current_event].delta == 0);

    return false;
}


void ks_score_state_set_default(ks_score_state* state, const ks_tones* tones, uint32_t sampling_rate, uint32_t resolution){
    state->quater_time = ks_1(KS_QUARTER_TIME_BITS - 1); // 0.5
    state->current_frame = 0;
    state->current_event = 0;
    state->frames_per_event = calc_frames_per_event(sampling_rate, state->quater_time, resolution);
    state->current_tick = 0;
    for(uint32_t i=0; i<ks_1(state->polyphony_bits); i++) {
        state->notes[i] = (ks_score_note){ 0 };
    }
    for(uint32_t i =0; i<KS_NUM_CHANNELS; i++){
        state->channels[i] = (ks_score_channel){ 0 };
        ks_score_channel_set_panpot(&state->channels[i], 64);
        ks_score_channel_set_picthbend(&state->channels[i], 0, 0);
        ks_score_state_bank_select(state, tones, &state->channels[i], 0, 0);
    }
}

ks_score_data* ks_score_data_from_midi(ks_midi_file* file){
    const ks_midi_file* conbined = file->format == 0 ? file : ks_midi_file_conbine_tracks(file);

    ks_score_data* ret = ks_score_data_new(file->resolution, 0, NULL);
    ks_score_event* events = calloc(file->tracks[0].num_events, sizeof(ks_score_event));


    uint64_t time=0;
    for(uint32_t i=0; i< file->tracks[0].num_events; i++){
        ks_midi_event* msg = &file->tracks[0].events[i];
        switch (msg->status) {
        case 0xff:
            // tempo
            if(msg->message.meta.type == 0x51){
                events[ret->num_events].status = 0xff;
                events[ret->num_events].datas[0] = 0x51;
                //msg->message.meta.length == 0x03;
                const uint32_t quarter_micro = ks_v((uint32_t)msg->message.meta.data[0], 16) +
                        ks_v((uint32_t)msg->message.meta.data[1], 8) +
                       (uint32_t)msg->message.meta.data[2];
                const uint32_t quarter_mili = quarter_micro / 1000;
                const uint32_t quarter_mili_fp8 = quarter_mili * ks_1(KS_QUARTER_TIME_BITS);
                const uint32_t quarter_fp8 = quarter_mili_fp8 / 1000;
                // little endian
                events[ret->num_events].datas[1] = ks_mask(quarter_fp8, KS_QUARTER_TIME_BITS);
                events[ret->num_events].datas[2] = quarter_fp8 >> KS_QUARTER_TIME_BITS;

                ret->num_events++;
                events[ret->num_events].delta = msg->time - time;
                time = msg->time;
            }
            // end of track
            else if(msg->message.meta.type == 0x2f){
                events[ret->num_events].status = 0xff;
                events[ret->num_events].datas[0] = 0x2f;
                events[ret->num_events].datas[1] = 0x00;

                ret->num_events++;
                events[ret->num_events].delta = msg->time - time;
                time = msg->time;
            }
            break;
        default:
            if(msg->status >= 0x80 &&
                    msg->status < 0xe0){
                events[ret->num_events].status = msg->status;
                events[ret->num_events].datas[0] = msg->message.datas[0];
                events[ret->num_events].datas[1] = msg->message.datas[1];

                ret->num_events++;
                events[ret->num_events].delta = msg->time - time;
                time = msg->time;
            }
            break;
        }
    }
    if(file->format != 0) ks_midi_file_free((ks_midi_file*)conbined);

    ret->events = events;

    return ret;
}
