/**
 * @file ks_score.h
 * @brief Song data
*/
#pragma once

#include "./synth.h"

#define KS_CHANNEL_BITs         4u
#define KS_NUM_CHANNELS         16u
#define KS_MAX_POLYPHONY_BITS   4u

#define KS_PANPOT_BITS          7u
#define KS_PITCH_BEND_BITS      14u

typedef struct ks_tones ks_tones;
typedef struct ks_tones_bank ks_tones_bank;

typedef struct ks_score_channel{
    ks_tones_bank        *bank;
    uint8_t             program_number;
    ks_synth*            program;

    int16_t             panpot_left;
    int16_t             panpot_right;
    int16_t             pitchbend;
}ks_score_channel;

typedef struct ks_score_note_info{
    uint16_t            note_number: 7;
    uint16_t            channel : 4;
}ks_score_note_info;

typedef struct ks_score_note{
    ks_score_note_info     info;
    ks_synth             *synth;
    ks_synth_note        note;
}ks_score_note;

typedef struct ks_score_state{
    uint16_t            frames_per_event;
    uint16_t            current_frame;
    uint32_t            current_tick;
    uint32_t            tempo;
    ks_score_channel      channels        [KS_NUM_CHANNELS];
    ks_score_note         notes           [ks_1(KS_MAX_POLYPHONY_BITS)];
}ks_score_state;

typedef struct ks_score_message{
    uint8_t             status;
    uint8_t             datas           [3];
}ks_score_message;

typedef struct ks_score_event{
    uint32_t            num_messages;
    ks_score_message      *messages;
}ks_score_event;

typedef struct ks_score{
    uint16_t            resolution;
    uint32_t            num_events;
    const ks_score_event  *events;
}ks_score;


ks_score* ks_score_new(uint32_t resolution, uint32_t num_events, const ks_score_event *events);
void ks_score_free(ks_score* song);

bool ks_score_state_note_on(ks_score_state* state, uint32_t sampling_rate, uint8_t channel_number, ks_score_channel* channel, uint8_t note_number, uint8_t velocity);
bool ks_score_state_note_off(ks_score_state* state, uint8_t channel_number, ks_score_channel* channel, uint8_t note_number);
bool ks_score_state_program_change(ks_score_state* state, const ks_tones*tones, ks_score_channel* channel, uint8_t program);
bool ks_score_vcontrol_change(ks_score_state* state, ks_score_channel* channel, uint8_t type, uint8_t value);
bool ks_score_channel_set_panpot(ks_score_channel* ch, uint8_t value);
bool ks_score_channel_set_picthbend(ks_score_channel* ch, uint8_t msb, uint8_t lsb);
bool ks_score_state_bank_select(ks_score_state* state, const ks_tones* tones,  ks_score_channel* channel, uint8_t msb, uint8_t lsb);
bool ks_score_state_bank_select_msb(ks_score_state* state, const ks_tones* tones, ks_score_channel* channel, uint8_t msb);
bool ks_score_state_bank_select_lsb(ks_score_state* state, const ks_tones* tones, ks_score_channel* channel, uint8_t lsb);


void ks_score_render(ks_score* song, uint32_t sampling_rate, ks_score_state *state, const ks_tones *tones, int16_t *buf, unsigned len);

void ks_score_event_run(const ks_score_event *event, uint32_t sampling_rate, ks_score_state* state, const ks_tones* tones);

void ks_score_state_set_default(ks_score_state *state, const ks_tones *tones, uint32_t sampling_rate, uint32_t resolution);

ks_score_event* ks_score_events_new(uint32_t num_events, ks_score_event events[num_events]);
void ks_score_events_free(uint32_t num_events, const ks_score_event *events);

ks_score_message* ks_score_messages_new(uint32_t num_messages, ks_score_message messages[num_messages]);
void ks_score_messages_free(ks_score_message* messages);




static inline bool ks_score_note_is_enabled(const ks_score_note* note){
    return ks_synth_note_is_enabled(&note->note);
}

static inline bool ks_score_note_info_equals(ks_score_note_info i1, ks_score_note_info i2){
    return (((i1.channel)<<7) + i1.note_number) == (((i2.channel)<<7) + i2.note_number);
}

static inline int16_t ks_score_note_info_hash(ks_score_note_info id){
    return id.note_number +  id.channel;
}

static inline ks_score_note_info ks_score_note_info_of(uint8_t note_number, uint8_t channel){
    return (ks_score_note_info){
                .note_number = note_number,
                .channel = channel,
            };
}
