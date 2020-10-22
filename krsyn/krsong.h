/**
 * @file krsong.h
 * @brief Song data
*/
#pragma once

#include "krsynth.h"

#define KRSYN_CHANNEL_BITs         4u
#define KRSYN_NUM_CHANNELS         16u
#define KRSYN_MAX_POLYPHONY_BITS   4u

#define KRSYN_PANPOT_BITS          7u

typedef struct krtones krtones;
typedef struct krtones_bank krtones_bank;

typedef struct krsong_channel{
    krtones_bank        *bank;
    uint8_t             program_number;

    int8_t              panpot;
    int32_t             pitchbend;
}krsong_channel;

typedef struct krsong_note_id{
    uint16_t            enable : 1;
    uint16_t            note_number: 7;
    uint16_t            channel : 4;
}krsong_note_id;

typedef struct krsong_note{
    krsong_note_id      id;
    krsynth             *synth;
    krsynth_note        note;
}krsong_note;

typedef struct krsong_state{
    uint16_t            frames_per_event;
    uint16_t            now_frame;
    uint32_t            now_tick;
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
    uint32_t            sampling_rate;
    krsong_state        state;
    const krtones       *tones;
    uint32_t            num_events;
    const krsong_event  *events;
}krsong;


krsong* krsong_new(uint32_t sampling_rate, uint32_t resolution, uint32_t num_events, const krsong_event *events);
void krsong_free(krsong* song);

bool krsong_note_on(krsong* song, uint8_t channel, uint8_t note_number, uint8_t velocity);
bool krsong_note_off(krsong* song, uint8_t channel, uint8_t note_number);
void krsong_render(krsong* song, int16_t *buf, unsigned len);

void krsong_state_set_default(krsong_state* state, uint32_t sampling_rate, uint32_t resolution);

krsong_event* krsong_events_new(uint32_t num_events, krsong_event events[num_events]);
void krsong_events_free(uint32_t num_events, const krsong_event *events);

krsong_message* krsong_messages_new(uint32_t num_messages, krsong_message messages[num_messages]);
void krsong_messages_free(krsong_message* messages);

void krsong_event_run(const krsong_event *event, krsong* song);


static inline bool krsong_note_id_is_enabled(krsong_note_id id){
    return id.enable ==1;
}

static inline int16_t krsong_note_id_get_value(krsong_note_id id){
    return  ks_v(id.note_number, KRSYN_CHANNEL_BITs) +  id.channel;
}

static inline krsong_note_id krsong_note_id_of(uint8_t note_number, uint8_t channel){
    return (krsong_note_id){
                .enable = 1,
                .note_number = note_number,
                .channel = channel,
            };
}
