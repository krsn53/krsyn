#include "krsong.h"
#include "krtones.h"
#include "constants.h"

#include <stdlib.h>


krsong* krsong_new(uint32_t sampling_rate, uint32_t resolution, uint32_t num_events, const krsong_event* events){
    krsong* ret = malloc(sizeof(krsong));
    ret->events = events;
    ret->num_events = num_events;
    ret->sampling_rate = sampling_rate;
    ret->tones = NULL;

    ret->resolution = resolution; // 96 is resolution which can be expressed to 128th note (to 256th triplet note).
    krsong_state_set_default(&ret->state, sampling_rate, resolution);

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

bool krsong_note_on(krsong* song, uint8_t channel, uint8_t note_number, uint8_t velocity){
    if(song->state.channels[channel].bank == NULL) return false;
    if(!song->state.channels[channel].bank->bank_number.enable) return false;

     krsynth* synth = NULL;

    if(song->state.channels[channel].bank->bank_number.percussion) {
        synth = song->state.channels[channel].bank->programs[song->state.channels[channel].program_number] + note_number;
    }
    else {
        synth = song->state.channels[channel].bank->programs[song->state.channels[channel].program_number];
    }
    if(synth == NULL) return false;

    krsong_note_id id =krsong_note_id_of(note_number, channel);
    uint16_t id_v = krsong_note_id_get_value(id);
    uint16_t begin = ks_mask(id_v, KRSYN_MAX_POLYPHONY_BITS);
    uint16_t index = begin;
    while(krsong_note_id_is_enabled(song->state.notes[index].id)){
        index++;
        index = ks_mask(index, KRSYN_MAX_POLYPHONY_BITS);
        if(index == begin) return false;
    }

    song->state.notes[index].synth = synth;
    song->state.notes[index].id = id;
    krsynth_note_on(&song->state.notes[index].note, synth, song->sampling_rate, note_number, velocity);

    return true;
}

bool krsong_note_off(krsong* song, uint8_t channel, uint8_t note_number){
    if(song->state.channels[channel].bank == NULL) return false;
    if(!song->state.channels[channel].bank->bank_number.enable) return false;

    uint16_t id = krsong_note_id_get_value(krsong_note_id_of(note_number, channel));
    uint16_t begin = ks_mask(id, KRSYN_MAX_POLYPHONY_BITS);
    uint16_t index = begin;
    while(krsong_note_id_get_value(song->state.notes[index].id) != id){
        index++;
        index = ks_mask(index, KRSYN_MAX_POLYPHONY_BITS);
        if(index == begin) return false;
    }

    krsynth_note_off(&song->state.notes[index].note);

    return true;
}

void krsong_render(krsong* song, int16_t* buf, unsigned len){
    unsigned i=0;
    do{
        unsigned frame = MIN((len>>1)-i, song->state.now_frame);
        int16_t tmpbuf[frame];

        for(unsigned p=0; p<ks_1(KRSYN_MAX_POLYPHONY_BITS); p++){
            if(!krsong_note_id_is_enabled(song->state.notes[p].id)) continue;
            if(!krsynth_note_is_enabled(&song->state.notes[p].note)) {
                song->state.notes[p].id.enable = 0;
                continue;
            }

            krsong_channel* ch = &song->state.channels[song->state.notes[p].id.channel];
            int16_t panright = sin_table[ks_v(ch->panpot + ks_v(3, KRSYN_PANPOT_BITS - 1), KRSYN_TABLE_BITS - KRSYN_PANPOT_BITS - 2)];
            int16_t panleft = sin_table[ks_v(ch->panpot + ks_1(KRSYN_PANPOT_BITS - 1), KRSYN_TABLE_BITS - KRSYN_PANPOT_BITS - 2)];

            // when note on, already checked,
            //if(ch->bank == NULL) continue;
            //if(ch->bank->programs[ch->program_number] == NULL) continue;
            krsynth* synth = ch->bank->programs[ch->program_number];
            krsynth_note* note = &song->state.notes[p].note;

            krsynth_render(synth, note, tmpbuf, frame);

            for(unsigned b =0; b< frame; b++){
                int32_t out = tmpbuf[b];
                out *= panleft;
                out >>= KRSYN_OUTPUT_BITS;
                buf[2*(i + b)] += out;

                out = tmpbuf[b];
                out *= panright;
                out >>= KRSYN_OUTPUT_BITS;
                buf[2*(i + b) + 1] += out;
            }
        }

        song->state.now_frame -=frame;

        if(song->state.now_frame == 0){
            song->state.now_frame += song->state.frames_per_event;
            krsong_event_run(&song->events[song->state.now_tick], song);
            song->state.now_tick ++;

            if(song->state.now_tick == song->num_events){
                song->state.now_frame = -1;
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
            uint8_t channel = msg->status & 0x0f;
            switch (msg->status & 0xf0) {
            // note off
            case 0x80:
                krsong_note_off(song, channel, msg->datas[0]);
                break;
            case 0x90:
                krsong_note_on(song, channel, msg->datas[0], msg->datas[1]);
                break;
            }
        }
    }
}


void krsong_state_set_default(krsong_state* state, uint32_t sampling_rate, uint32_t resolution){
    state->tempo = 120;
    state->now_frame = 0;
    state->frames_per_event = calc_frames_per_event(sampling_rate, state->tempo, resolution);
    state->now_tick = 0;
    for(unsigned i=0; i<ks_1(KRSYN_MAX_POLYPHONY_BITS); i++) {
        state->notes[i] = (krsong_note){ 0 };
    }

    for(unsigned i =0; i<KRSYN_NUM_CHANNELS; i++){
        state->channels[i] = (krsong_channel){
                .panpot=0,
                .pitchbend=0,
                .bank =NULL,
                .program_number = 0,
        };
    }
}
