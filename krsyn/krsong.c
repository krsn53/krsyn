#include "logger.h"
#include "krsong.h"
#include "krtones.h"
#include "math.h"

#include <stdlib.h>



krsong* krsong_new(uint32_t sampling_rate, uint32_t resolution, const krtones* tones, uint32_t num_events, const krsong_event* events){
    krsong* ret = malloc(sizeof(krsong));
    ret->events = events;
    ret->num_events = num_events;
    ret->sampling_rate = sampling_rate;
    ret->tones = tones;

    ret->resolution = resolution; // 96 is resolution which can be expressed to 128th note (to 256th triplet note).
    krsong_state_set_default(ret, sampling_rate, resolution);

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

bool krsong_note_on(krsong* song, uint8_t channel_number, krsong_channel* channel, uint8_t note_number, uint8_t velocity){

     krsynth* synth = channel->program;
     if(synth == NULL) {
         krsyn_error("Failed to run note_on for not set program of channel %d at tick %d", channel_number, song->state.current_tick);
         return false;
     }

    krsong_note_info id =krsong_note_info_of(note_number, channel_number);
    uint16_t id_v = krsong_note_info_hash(id);
    uint16_t begin = ks_mask(id_v, KRSYN_MAX_POLYPHONY_BITS);
    uint16_t index = begin;
    while(krsong_note_is_enabled(&song->state.notes[index])){
        index++;
        index = ks_mask(index, KRSYN_MAX_POLYPHONY_BITS);
        if(index == begin) {
            krsyn_warning("Failed to run note_on for exceeded maximum of polyphony at tick %d", channel_number, song->state.current_tick);
            return false;
        }
    }

    song->state.notes[index].synth = synth;
    song->state.notes[index].info = id;
    krsynth_note_on(&song->state.notes[index].note, synth, song->sampling_rate, note_number, velocity);

    return true;
}

bool krsong_note_off(krsong* song, uint8_t channel_number, krsong_channel* channel, uint8_t note_number){
    krtones_bank* bank = channel->bank;
    if(bank == NULL) {
        krsyn_error("Failed to run note_off for not set bank of channel %d at tick %d", channel_number, song->state.current_tick);
        return false;
    }

    if(krtones_bank_is_empty(bank)) {
        krsyn_error("Failed to run note_off for not set bank of channel %d at tick %d", channel_number, song->state.current_tick);
        return false;
    }
    krsong_note_info info =krsong_note_info_of(note_number, channel_number);
    uint16_t hash = krsong_note_info_hash(info);
    uint16_t begin = ks_mask(hash, KRSYN_MAX_POLYPHONY_BITS);
    uint16_t index = begin;
    while(!krsong_note_info_equals(song->state.notes[index].info, info)) {
        index++;
        index = ks_mask(index, KRSYN_MAX_POLYPHONY_BITS);
        if(index == begin) {
            krsyn_warning("Failed to run note_off for not found note with note number %d and channel %d at tick %d", note_number, channel_number, song->state.current_tick);
            return false;
        }
    }

    krsynth_note_off(&song->state.notes[index].note);

    return true;
}

bool krsong_program_change(krsong* song, krsong_channel* channel, uint8_t program){
    if(song->tones == NULL){
        krsyn_error("Failed to run program_change of channel %d for not set tone at tick %d", (channel - song->state.channels) / sizeof(krsong_channel), song->state.current_tick);
        return false;
    }

    if(channel->bank == NULL) {
        krsyn_error("Failed to run program_change of channel %d for not set bank at tick %d", (channel - song->state.channels) / sizeof(krsong_channel), song->state.current_tick);
        return false;
    }

    krsynth* p = channel->bank->programs[program];
    if(p == NULL){
        krtones_bank* bank = krtones_find_bank(song->tones, krtones_bank_number_of(0,0));
        if(bank == NULL){
            bank = song->tones->banks;
        }
        p = channel->bank->programs[program];

        krtones_bank* begin = song->tones->banks;
        unsigned it=0;
        while(begin[it].programs[program] == NULL){
            it++;
            it %= song->tones->num_banks;
            if(it == 0){
                krsyn_error("Failed to run program_change of channel %d for not found program %d at tick %d",(channel - song->state.channels) / sizeof(krsong_channel), program, song->state.current_tick);
                return  false;
            }
        }
    }
    channel->program_number = program;
    channel->program = p;
    return true;
}

bool krsong_bank_select(krsong* song, krsong_channel* channel, uint8_t msb, uint8_t lsb){
    if(song->tones == NULL){
        krsyn_error("Failed to run bank_select of channel %d for not set tone at tick %d", (channel - song->state.channels) / sizeof(krsong_channel), song->state.current_tick);
        return false;
    }
    krtones_bank *bank = krtones_find_bank(song->tones, krtones_bank_number_of(msb, lsb));
    if(bank == NULL) {
        krsyn_error("Failed to run bank_select of channel %d for not found bank at tick %d", (channel - song->state.channels) / sizeof(krsong_channel), song->state.current_tick);
        return false;
    }

    channel->bank = bank;

    return krsong_program_change(song, channel, channel->program_number);
}

bool krsong_bank_select_msb(krsong* song, krsong_channel* channel, uint8_t msb){
    uint8_t lsb = 0;
    if(channel->bank != NULL){
        lsb = channel->bank->bank_number.lsb;
    }
    return krsong_bank_select(song, channel, msb, lsb);
}

bool krsong_bank_select_lsb(krsong* song, krsong_channel* channel, uint8_t lsb){
    uint8_t msb = 0;
    if(channel->bank != NULL){
        msb = channel->bank->bank_number.msb;
    }
    return krsong_bank_select(song, channel, msb, lsb);
}

bool krsong_channel_set_panpot(krsong_channel* channel, uint8_t value){
    channel->panpot_left =  sin_table[ks_v(value + ks_1(KRSYN_PANPOT_BITS - 1), KRSYN_TABLE_BITS - KRSYN_PANPOT_BITS - 2)];
    channel->panpot_right = sin_table[ks_v(value + ks_v(3, KRSYN_PANPOT_BITS - 1), KRSYN_TABLE_BITS - KRSYN_PANPOT_BITS - 2)];

    return true;
}

bool krsong_channel_set_picthbend(krsong_channel* channel, uint8_t msb, uint8_t lsb){
    channel->pitchbend;

    return true;
}

bool krsong_control_change(krsong* song, krsong_channel* channel, uint8_t type, uint8_t value){
    switch (type) {
    case 0x00:
        return krsong_bank_select_msb(song, channel, value);
    case 0x20:
        return krsong_bank_select_lsb(song, channel, value);
    case 0x10:
        return krsong_channel_set_panpot(channel, value);
    }
    return true;
}


void krsong_render(krsong* song, int16_t* buf, unsigned len){
    unsigned i=0;
    do{
        unsigned frame = MIN((len>>1)-i, song->state.current_frame);
        int16_t tmpbuf[frame];

        for(unsigned p=0; p<ks_1(KRSYN_MAX_POLYPHONY_BITS); p++){
            if(!krsong_note_is_enabled(&song->state.notes[p])) {
                continue;
            }

            krsong_channel* channel = &song->state.channels[song->state.notes[p].info.channel];

            // when note on, already checked,
            //if(channel->bank == NULL) continue;
            //if(channel->bank->programs[channel->program_number] == NULL) continue;
            krsynth* synth = channel->program;
            krsynth_note* note = &song->state.notes[p].note;
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

        song->state.current_frame -=frame;

        if(song->state.current_frame == 0){
            song->state.current_frame += song->state.frames_per_event;
            krsong_event_run(&song->events[song->state.current_tick], song);
            song->state.current_tick ++;

            if(song->state.current_tick == song->num_events){
                song->state.current_frame = -1;
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


void krsong_event_run(const krsong_event* event, krsong* song){
    for(unsigned i=0, e = event->num_messages; i<e; i++){
        const krsong_message* msg = &event->messages[i];
        if(msg->status >= 0xf0){
            // System common message and system real time message
        }
        else {
            // channel message
            uint8_t channel_num = msg->status & 0x0f;
            krsong_channel* channel = &song->state.channels[channel_num];
            switch (msg->status >> 4) {
            // note off
            case 0x8:
                krsong_note_off(song, channel_num, channel, msg->datas[0]);
                break;
            // note on
            case 0x9:
                krsong_note_on(song, channel_num, channel, msg->datas[0], msg->datas[1]);
                break;
            // control change
            case 0xb:
                krsong_control_change(song, channel, msg->datas[0], msg->datas[1]);
                break;
            // program change
            case 0xc:
                krsong_program_change(song, channel, msg->datas[0]);
                break;
            // pich wheel change
            case 0xe:
                krsong_channel_set_picthbend(channel, msg->datas[1], msg->datas[2]);
                break;
            }
        }
    }
}


void krsong_state_set_default(krsong* song, uint32_t sampling_rate, uint32_t resolution){
    song->state.tempo = 120;
    song->state.current_frame = 0;
    song->state.frames_per_event = calc_frames_per_event(sampling_rate, song->state.tempo, resolution);
    song->state.current_tick = 0;
    for(unsigned i=0; i<ks_1(KRSYN_MAX_POLYPHONY_BITS); i++) {
        song->state.notes[i] = (krsong_note){ 0 };
    }
    for(unsigned i =0; i<KRSYN_NUM_CHANNELS; i++){
        song->state.channels[i] = (krsong_channel){ 0 };
        krsong_channel_set_panpot(&song->state.channels[i], 0);
        krsong_bank_select(song, &song->state.channels[i], 0, 0);
    }
}
