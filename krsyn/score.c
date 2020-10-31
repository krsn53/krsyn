#include "logger.h"
#include "score.h"
#include "tones.h"
#include "math.h"

#include <stdlib.h>



ks_score* ks_score_new(uint32_t resolution, uint32_t num_events, const ks_score_event* events){
    ks_score* ret = malloc(sizeof(ks_score));
    ret->events = events;
    ret->num_events = num_events;
    ret->resolution = resolution; // 96 is resolution which can be expressed to 128th note (to 256th triplet note).

    return ret;
}

void ks_score_free(ks_score* song){
    ks_score_events_free(song->num_events, song->events);
    free(song);
}

static inline uint32_t calc_frames_per_event(uint32_t sampling_rate, uint16_t tempo, uint16_t resolution){
    uint32_t ret = sampling_rate;
    ret /= tempo / 60;
    ret /= resolution;
    return  ret;
}

bool ks_score_state_note_on(ks_score_state* state, uint32_t sampling_rate, uint8_t channel_number, ks_score_channel* channel, uint8_t note_number, uint8_t velocity){

     ks_synth* synth = channel->program;
     if(synth == NULL) {
         krsyn_error("Failed to run note_on for not set program of channel %d at tick %d", channel_number, state->current_tick);
         return false;
     }

    ks_score_note_info id =ks_score_note_info_of(note_number, channel_number);
    uint16_t id_v = ks_score_note_info_hash(id);
    uint16_t begin = ks_mask(id_v, KS_MAX_POLYPHONY_BITS);
    uint16_t index = begin;
    while(ks_score_note_is_enabled(&state->notes[index])){
        index++;
        index = ks_mask(index, KS_MAX_POLYPHONY_BITS);
        if(index == begin) {
            krsyn_warning("Failed to run note_on for exceeded maximum of polyphony at tick %d", channel_number, state->current_tick);
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
        krsyn_error("Failed to run note_off for not set bank of channel %d at tick %d", channel_number, state->current_tick);
        return false;
    }

    if(ks_tones_bank_is_empty(bank)) {
        krsyn_error("Failed to run note_off for not set bank of channel %d at tick %d", channel_number, state->current_tick);
        return false;
    }
    ks_score_note_info info =ks_score_note_info_of(note_number, channel_number);
    uint16_t hash = ks_score_note_info_hash(info);
    uint16_t begin = ks_mask(hash, KS_MAX_POLYPHONY_BITS);
    uint16_t index = begin;
    while(!ks_score_note_info_equals(state->notes[index].info, info)) {
        index++;
        index = ks_mask(index, KS_MAX_POLYPHONY_BITS);
        if(index == begin) {
            krsyn_warning("Failed to run note_off for not found note with note number %d and channel %d at tick %d", note_number, channel_number, state->current_tick);
            return false;
        }
    }

    ks_synth_note_off(&state->notes[index].note);

    return true;
}

bool ks_score_state_program_change(ks_score_state* state, const ks_tones* tones, ks_score_channel* channel, uint8_t program){

    if(channel->bank == NULL) {
        krsyn_error("Failed to run program_change of channel %d for not set bank at tick %d", (channel - state->channels) / sizeof(ks_score_channel),state->current_tick);
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
        unsigned it=0;
        while(begin[it].programs[program] == NULL){
            it++;
            it %= tones->num_banks;
            if(it == 0){
                krsyn_error("Failed to run program_change of channel %d for not found program %d at tick %d",(channel - state->channels) / sizeof(ks_score_channel), program,state->current_tick);
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
        krsyn_error("Failed to run bank_select of channel %d for not found bank at tick %d", (channel - state->channels) / sizeof(ks_score_channel),state->current_tick);
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
    channel->panpot_left =  sin_table[ks_v(value + ks_1(KS_PANPOT_BITS - 1), KS_TABLE_BITS - KS_PANPOT_BITS - 2)];
    channel->panpot_right = sin_table[ks_v(value + ks_v(3, KS_PANPOT_BITS - 1), KS_TABLE_BITS - KS_PANPOT_BITS - 2)];

    return true;
}

bool ks_score_channel_set_picthbend(ks_score_channel* channel, uint8_t msb, uint8_t lsb){
    channel->pitchbend = ks_v(msb, 7) + lsb;
    channel->pitchbend = krsyn_fms_depth(channel->pitchbend << (KS_LFO_DEPTH_BITS - KS_PITCH_BEND_BITS));
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


void ks_score_render(ks_score* song, uint32_t sampling_rate, ks_score_state* state, const ks_tones*tones, int16_t* buf, unsigned len){
    unsigned i=0;
    do{
        unsigned frame = MIN((len>>1)-i, state->current_frame);
        int16_t tmpbuf[frame];

        for(unsigned p=0; p<ks_1(KS_MAX_POLYPHONY_BITS); p++){
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

            for(unsigned b =0; b< frame; b++){
                int32_t out = tmpbuf[b];
                out *= channel->panpot_left;
                out >>= KS_OUTPUT_BITS;
                buf[2*(i + b)] += out;

                out = tmpbuf[b];
                out *= channel->panpot_right;
                out >>= KS_OUTPUT_BITS;
                buf[2*(i + b) + 1] += out;
            }
        }

        state->current_frame -=frame;

        if(state->current_frame == 0){
            state->current_frame += state->frames_per_event;
            ks_score_event_run(&song->events[state->current_tick], sampling_rate, state, tones);
            state->current_tick ++;

            if(state->current_tick == song->num_events){
                state->current_frame = -1;
            }
        }

        i+= frame;
    }while(2*i<len);
}

ks_score_event* ks_score_events_new(uint32_t num_events, ks_score_event events[num_events]){
    ks_score_event* ret = malloc(sizeof(ks_score_event) * num_events);
    for(unsigned i=0; i<num_events; i++){
        ret[i] = events[i];
    }
    return ret;
}

void ks_score_events_free(uint32_t num_events, const ks_score_event* events){
    for(unsigned i=0; i<num_events; i++){
        if(events[i].num_messages != 0){
            ks_score_messages_free(events[i].messages);
        }
    }
    free((ks_score_event*)events);
}


ks_score_message* ks_score_messages_new(uint32_t num_messages, ks_score_message messages[num_messages]){
    ks_score_message* ret = malloc(sizeof(ks_score_message) * num_messages);
    for(unsigned i=0; i< num_messages; i++){
        ret[i] = messages[i];
    }
    return ret;
}

void ks_score_messages_free(ks_score_message* messages){
    free(messages);
}


void ks_score_event_run(const ks_score_event* event, uint32_t sampling_rate, ks_score_state *state, const ks_tones *tones){
    for(unsigned i=0, e = event->num_messages; i<e; i++){
        const ks_score_message* msg = &event->messages[i];
        if(msg->status >= 0xf0){
            // System common message and system real time message
        }
        else {
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
}


void ks_score_state_set_default(ks_score_state* state, const ks_tones* tones, uint32_t sampling_rate, uint32_t resolution){
    state->tempo = 120;
    state->current_frame = 0;
    state->frames_per_event = calc_frames_per_event(sampling_rate, state->tempo, resolution);
    state->current_tick = 0;
    for(unsigned i=0; i<ks_1(KS_MAX_POLYPHONY_BITS); i++) {
        state->notes[i] = (ks_score_note){ 0 };
    }
    for(unsigned i =0; i<KS_NUM_CHANNELS; i++){
        state->channels[i] = (ks_score_channel){ 0 };
        ks_score_channel_set_panpot(&state->channels[i], 0);
        ks_score_state_bank_select(state, tones, &state->channels[i], 0, 0);
    }
}
