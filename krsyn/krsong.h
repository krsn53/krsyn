/**
 * @file krsong.h
 * @brief Song data
*/
#pragma once

#include "./krsynth.h"

#define KRSYN_CHANNEL_BITs         4u
#define KRSYN_NUM_CHANNELS         16u
#define KRSYN_MAX_POLYPHONY_BITS   4u

#define KRSYN_PANPOT_BITS          7u
#define KRSYN_PITCH_BEND_BITS      14u

typedef struct krtones krtones;
typedef struct krtones_bank krtones_bank;

typedef struct krsong_channel{
    krtones_bank        *bank;
    uint8_t             program_number;
    krsynth*            program;

    int16_t             panpot_left;
    int16_t             panpot_right;
    int16_t             pitchbend;
}krsong_channel;

typedef struct krsong_note_info{
    uint16_t            note_number: 7;
    uint16_t            channel : 4;
}krsong_note_info;

typedef struct krsong_note{
    krsong_note_info     info;
    krsynth             *synth;
    krsynth_note        note;
}krsong_note;

typedef struct krsong_state{
    uint16_t            frames_per_event;
    uint16_t            current_frame;
    uint32_t            current_tick;
    uint32_t            tempo;
    krsong_channel      channels        [KRSYN_NUM_CHANNELS];
    krsong_note         notes           [ks_1(KRSYN_MAX_POLYPHONY_BITS)];
}krsong_state;

typedef struct krsong_message{
    uint8_t             status;
    uint8_t             datas           [3];
}krsong_message;

typedef struct krsong_event{
    uint32_t            num_messages;
    krsong_message      *messages;
}krsong_event;

typedef struct krsong{
    uint16_t            resolution;
    uint32_t            num_events;
    const krsong_event  *events;
}krsong;


krsong* krsong_new(uint32_t resolution, uint32_t num_events, const krsong_event *events);
void krsong_free(krsong* song);

bool krsong_state_note_on(krsong_state* state, uint32_t sampling_rate, uint8_t channel_number, krsong_channel* channel, uint8_t note_number, uint8_t velocity);
bool krsong_state_note_off(krsong_state* state, uint8_t channel_number, krsong_channel* channel, uint8_t note_number);
bool krsong_state_program_change(krsong_state* state, const krtones*tones, krsong_channel* channel, uint8_t program);
bool krsong_vcontrol_change(krsong_state* state, krsong_channel* channel, uint8_t type, uint8_t value);
bool krsong_channel_set_panpot(krsong_channel* ch, uint8_t value);
bool krsong_channel_set_picthbend(krsong_channel* ch, uint8_t msb, uint8_t lsb);
bool krsong_state_bank_select(krsong_state* state, const krtones* tones,  krsong_channel* channel, uint8_t msb, uint8_t lsb);
bool krsong_state_bank_select_msb(krsong_state* state, const krtones* tones, krsong_channel* channel, uint8_t msb);
bool krsong_state_bank_select_lsb(krsong_state* state, const krtones* tones, krsong_channel* channel, uint8_t lsb);


void krsong_render(krsong* song, uint32_t sampling_rate, krsong_state *state, const krtones *tones, int16_t *buf, unsigned len);

void krsong_event_run(const krsong_event *event, uint32_t sampling_rate, krsong_state* state, const krtones* tones);

void krsong_state_set_default(krsong_state *state, const krtones *tones, uint32_t sampling_rate, uint32_t resolution);

krsong_event* krsong_events_new(uint32_t num_events, krsong_event events[num_events]);
void krsong_events_free(uint32_t num_events, const krsong_event *events);

krsong_message* krsong_messages_new(uint32_t num_messages, krsong_message messages[num_messages]);
void krsong_messages_free(krsong_message* messages);




static inline bool krsong_note_is_enabled(const krsong_note* note){
    return krsynth_note_is_enabled(&note->note);
}

static inline bool krsong_note_info_equals(krsong_note_info i1, krsong_note_info i2){
    return (((i1.channel)<<7) + i1.note_number) == (((i2.channel)<<7) + i2.note_number);
}

static inline int16_t krsong_note_info_hash(krsong_note_info id){
    return id.note_number +  id.channel;
}

static inline krsong_note_info krsong_note_info_of(uint8_t note_number, uint8_t channel){
    return (krsong_note_info){
                .note_number = note_number,
                .channel = channel,
            };
}
