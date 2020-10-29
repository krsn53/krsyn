#include "logger.h"
#include "krsong.h"
#include "krtones.h"
#include "math.h"

#include <stdlib.h>



krsong* krsong_new(uint32_t resolution, uint32_t num_events, const krsong_event* events){
    krsong* ret = malloc(sizeof(krsong));
    ret->events = events;
    ret->num_events = num_events;
    ret->resolution = resolution; // 96 is resolution which can be expressed to 128th note (to 256th triplet note).

    return ret;
}

void krsong_free(krsong* song){
    krsong_events_free(song->num_events, song->events);
    free(song);
}

static inline uint32_t calc_frames_per_event(uint32_t sampling_rate, uint16_t tempo, uint16_t resolution){
    uint32_t ret = sampling_rate;
    ret /= tempo / 60;
    ret /= resolution;
    return  ret;
}

bool krsong_state_note_on(krsong_state* state, uint32_t sampling_rate, uint8_t channel_number, krsong_channel* channel, uint8_t note_number, uint8_t velocity){

     krsynth* synth = channel->program;
     if(synth == NULL) {
         krsyn_error("Failed to run note_on for not set program of channel %d at tick %d", channel_number, state->current_tick);
         return false;
     }

    krsong_note_info id =krsong_note_info_of(note_number, channel_number);
    uint16_t id_v = krsong_note_info_hash(id);
    uint16_t begin = ks_mask(id_v, KRSYN_MAX_POLYPHONY_BITS);
    uint16_t index = begin;
    while(krsong_note_is_enabled(&state->notes[index])){
        index++;
        index = ks_mask(index, KRSYN_MAX_POLYPHONY_BITS);
        if(index == begin) {
            krsyn_warning("Failed to run note_on for exceeded maximum of polyphony at tick %d", channel_number, state->current_tick);
            return false;
        }
    }

    state->notes[index].synth = synth;
    state->notes[index].info = id;
    krsynth_note_on(&state->notes[index].note, synth, sampling_rate, note_number, velocity);

    return true;
}

bool krsong_state_note_off(krsong_state* state, uint8_t channel_number, krsong_channel* channel, uint8_t note_number){
    krtones_bank* bank = channel->bank;
    if(bank == NULL) {
        krsyn_error("Failed to run note_off for not set bank of channel %d at tick %d", channel_number, state->current_tick);
        return false;
    }

    if(krtones_bank_is_empty(bank)) {
        krsyn_error("Failed to run note_off for not set bank of channel %d at tick %d", channel_number, state->current_tick);
        return false;
    }
    krsong_note_info info =krsong_note_info_of(note_number, channel_number);
    uint16_t hash = krsong_note_info_hash(info);
    uint16_t begin = ks_mask(hash, KRSYN_MAX_POLYPHONY_BITS);
    uint16_t index = begin;
    while(!krsong_note_info_equals(state->notes[index].info, info)) {
        index++;
        index = ks_mask(index, KRSYN_MAX_POLYPHONY_BITS);
        if(index == begin) {
            krsyn_warning("Failed to run note_off for not found note with note number %d and channel %d at tick %d", note_number, channel_number, state->current_tick);
            return false;
        }
    }

    krsynth_note_off(&state->notes[index].note);

    return true;
}

bool krsong_state_program_change(krsong_state* state, const krtones* tones, krsong_channel* channel, uint8_t program){

    if(channel->bank == NULL) {
        krsyn_error("Failed to run program_change of channel %d for not set bank at tick %d", (channel - state->channels) / sizeof(krsong_channel),state->current_tick);
        return false;
    }

    krsynth* p = channel->bank->programs[program];
    if(p == NULL){
        krtones_bank* bank = krtones_find_bank(tones, krtones_bank_number_of(0,0));
        if(bank == NULL){
            bank = tones->banks;
        }
        p = channel->bank->programs[program];

        krtones_bank* begin = tones->banks;
        unsigned it=0;
        while(begin[it].programs[program] == NULL){
            it++;
            it %= tones->num_banks;
            if(it == 0){
                krsyn_error("Failed to run program_change of channel %d for not found program %d at tick %d",(channel - state->channels) / sizeof(krsong_channel), program,state->current_tick);
                return  false;
            }
        }
    }
    channel->program_number = program;
    channel->program = p;
    return true;
}

bool krsong_state_bank_select(krsong_state* state, const krtones* tones, krsong_channel* channel, uint8_t msb, uint8_t lsb){
    krtones_bank *bank = krtones_find_bank(tones, krtones_bank_number_of(msb, lsb));
    if(bank == NULL) {
        krsyn_error("Failed to run bank_select of channel %d for not found bank at tick %d", (channel - state->channels) / sizeof(krsong_channel),state->current_tick);
        return false;
    }

    channel->bank = bank;

    return krsong_state_program_change(state, tones, channel, channel->program_number);
}

bool krsong_state_bank_select_msb(krsong_state* state, const krtones* tones, krsong_channel* channel, uint8_t msb){
    uint8_t lsb = 0;
    if(channel->bank != NULL){
        lsb = channel->bank->bank_number.lsb;
    }
    return krsong_state_bank_select(state, tones, channel, msb, lsb);
}

bool krsong_state_bank_select_lsb(krsong_state* state, const krtones* tones, krsong_channel* channel, uint8_t lsb){
    uint8_t msb = 0;
    if(channel->bank != NULL){
        msb = channel->bank->bank_number.msb;
    }
    return krsong_state_bank_select(state, tones, channel, msb, lsb);
}

bool krsong_channel_set_panpot(krsong_channel* channel, uint8_t value){
    channel->panpot_left =  sin_table[ks_v(value + ks_1(KRSYN_PANPOT_BITS - 1), KRSYN_TABLE_BITS - KRSYN_PANPOT_BITS - 2)];
    channel->panpot_right = sin_table[ks_v(value + ks_v(3, KRSYN_PANPOT_BITS - 1), KRSYN_TABLE_BITS - KRSYN_PANPOT_BITS - 2)];

    return true;
}

bool krsong_channel_set_picthbend(krsong_channel* channel, uint8_t msb, uint8_t lsb){
    channel->pitchbend = ks_v(msb, 7) + lsb;

    return true;
}

bool krsong_state_control_change(krsong_state* state, const krtones* tones, krsong_channel* channel, uint8_t type, uint8_t value){
    switch (type) {
    case 0x00:
        return krsong_state_bank_select_msb(state, tones, channel, value);
    case 0x20:
        return krsong_state_bank_select_lsb(state, tones, channel, value);
    case 0x10:
        return krsong_channel_set_panpot(channel, value);
    }
    return true;
}


void krsong_render(krsong* song, uint32_t sampling_rate, krsong_state* state, const krtones*tones, int16_t* buf, unsigned len){
    unsigned i=0;
    do{
        unsigned frame = MIN((len>>1)-i, state->current_frame);
        int16_t tmpbuf[frame];

        for(unsigned p=0; p<ks_1(KRSYN_MAX_POLYPHONY_BITS); p++){
            if(!krsong_note_is_enabled(&state->notes[p])) {
                continue;
            }

            krsong_channel* channel = &state->channels[state->notes[p].info.channel];

            // when note on, already checked,
            //if(channel->bank == NULL) continue;
            //if(channel->bank->programs[channel->program_number] == NULL) continue;
            krsynth* synth = channel->program;
            krsynth_note* note = &state->notes[p].note;
            int32_t pitchbend = krsyn_fms_depth(channel->pitchbend << (KRSYN_LFO_DEPTH_BITS - KRSYN_PITCH_BEND_BITS));
            krsynth_render(synth, note, pitchbend, tmpbuf, frame);

            for(unsigned b =0; b< frame; b++){
                int32_t out = tmpbuf[b];
                out *= channel->panpot_left;
                out >>= KRSYN_OUTPUT_BITS;
                buf[2*(i + b)] += out;

                out = tmpbuf[b];
                out *= channel->panpot_right;
                out >>= KRSYN_OUTPUT_BITS;
                buf[2*(i + b) + 1] += out;
            }
        }

        state->current_frame -=frame;

        if(state->current_frame == 0){
            state->current_frame += state->frames_per_event;
            krsong_event_run(&song->events[state->current_tick], sampling_rate, state, tones);
            state->current_tick ++;

            if(state->current_tick == song->num_events){
                state->current_frame = -1;
            }
        }

        i+= frame;
    }while(2*i<len);
}

krsong_event* krsong_events_new(uint32_t num_events, krsong_event events[num_events]){
    krsong_event* ret = malloc(sizeof(krsong_event) * num_events);
    for(unsigned i=0; i<num_events; i++){
        ret[i] = events[i];
    }
    return ret;
}

void krsong_events_free(uint32_t num_events, const krsong_event* events){
    for(unsigned i=0; i<num_events; i++){
        if(events[i].num_messages != 0){
            krsong_messages_free(events[i].messages);
        }
    }
    free((krsong_event*)events);
}


krsong_message* krsong_messages_new(uint32_t num_messages, krsong_message messages[num_messages]){
    krsong_message* ret = malloc(sizeof(krsong_message) * num_messages);
    for(unsigned i=0; i< num_messages; i++){
        ret[i] = messages[i];
    }
    return ret;
}

void krsong_messages_free(krsong_message* messages){
    free(messages);
}


void krsong_event_run(const krsong_event* event, uint32_t sampling_rate, krsong_state *state, const krtones *tones){
    for(unsigned i=0, e = event->num_messages; i<e; i++){
        const krsong_message* msg = &event->messages[i];
        if(msg->status >= 0xf0){
            // System common message and system real time message
        }
        else {
            // channel message
            uint8_t channel_num = msg->status & 0x0f;
            krsong_channel* channel = &state->channels[channel_num];
            switch (msg->status >> 4) {
            // note off
            case 0x8:
                krsong_state_note_off(state, channel_num, channel, msg->datas[0]);
                break;
            // note on
            case 0x9:
                krsong_state_note_on(state, sampling_rate, channel_num, channel, msg->datas[0], msg->datas[1]);
                break;
            // control change
            case 0xb:
                krsong_state_control_change(state, tones, channel, msg->datas[0], msg->datas[1]);
                break;
            // program change
            case 0xc:
                krsong_state_program_change(state, tones, channel, msg->datas[0]);
                break;
            // pich wheel change
            case 0xe:
                krsong_channel_set_picthbend(channel, msg->datas[1], msg->datas[2]);
                break;
            }
        }
    }
}


void krsong_state_set_default(krsong_state* state, const krtones* tones, uint32_t sampling_rate, uint32_t resolution){
    state->tempo = 120;
    state->current_frame = 0;
    state->frames_per_event = calc_frames_per_event(sampling_rate, state->tempo, resolution);
    state->current_tick = 0;
    for(unsigned i=0; i<ks_1(KRSYN_MAX_POLYPHONY_BITS); i++) {
        state->notes[i] = (krsong_note){ 0 };
    }
    for(unsigned i =0; i<KRSYN_NUM_CHANNELS; i++){
        state->channels[i] = (krsong_channel){ 0 };
        krsong_channel_set_panpot(&state->channels[i], 0);
        krsong_state_bank_select(state, tones, &state->channels[i], 0, 0);
    }
}
