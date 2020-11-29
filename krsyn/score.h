/**
 * @file ks_score.h
 * @brief Song data
*/
#pragma once

#include "./synth.h"

#define KS_CHANNEL_BITs         4u
#define KS_NUM_CHANNELS         16u

#define KS_PITCH_BEND_BITS      14u

#define KS_QUARTER_TIME_BITS    8u

typedef struct tone_list tone_list;
typedef struct tone_list_bank tone_list_bank;
typedef struct ks_midi_file ks_midi_file;

typedef struct ks_score_channel{
    tone_list_bank        *bank;
    u8             program_number;
    ks_synth*            program;

    i16             panpot_left;
    i16             panpot_right;
    i32             pitchbend;
}ks_score_channel;

typedef struct ks_score_note_info{
    u8            note_number;
    u8            channel;
}ks_score_note_info;

typedef struct ks_score_note{
    ks_synth_note        note;
    ks_score_note_info     info;
    ks_synth             *synth;
}ks_score_note;

typedef struct ks_score_state{
    u16            quater_time;
    u16            frames_per_event;
    u16            current_frame;
    u16            polyphony_bits;

    u32            current_event;
    u32            current_tick;

    ks_score_channel    channels        [KS_NUM_CHANNELS];
    ks_score_note       notes           [];
}ks_score_state;


typedef struct ks_score_event{
    u32            delta;
    u8             status;
    u8             data           [3];
}ks_score_event;

typedef struct ks_score_data{
    char                title           [64];
    char                author          [64];
    char                license         [64];
    u16            resolution;
    u32            num_events;
    const ks_score_event  *events;
}ks_score_data;

ks_io_decl_custom_func(ks_score_event);
ks_io_decl_custom_func(ks_score_data);

ks_score_data* ks_score_data_new(u32 resolution, u32 num_events, const ks_score_event *events);
void ks_score_data_free(ks_score_data* song);

ks_score_state* ks_score_state_new(u32 polyphony_bits);
void ks_score_state_free(ks_score_state* state);

bool ks_score_state_note_on(ks_score_state* state, u32 sampling_rate, u8 channel_number, ks_score_channel* channel, u8 note_number, u8 velocity);
bool ks_score_state_note_off(ks_score_state* state, u8 channel_number, ks_score_channel* channel, u8 note_number);
bool ks_score_state_program_change(ks_score_state* state, const tone_list*tones, ks_score_channel* channel, u8 program);
bool ks_score_state_tempo_change(ks_score_state* state, u32 sampling_rate, const ks_score_data* score, const u8* data);
bool ks_score_state_control_change(ks_score_state* state, const tone_list* tones, ks_score_channel* channel, u8 type, u8 value);
bool ks_score_channel_set_panpot(ks_score_channel* ch, u8 value);
bool ks_score_channel_set_picthbend(ks_score_channel* ch, u8 msb, u8 lsb);
bool ks_score_state_bank_select(ks_score_state* state, const tone_list* tones,  ks_score_channel* channel, u8 msb, u8 lsb);
bool ks_score_state_bank_select_msb(ks_score_state* state, const tone_list* tones, ks_score_channel* channel, u8 msb);
bool ks_score_state_bank_select_lsb(ks_score_state* state, const tone_list* tones, ks_score_channel* channel, u8 lsb);


void ks_score_data_render(const ks_score_data* score, u32 sampling_rate, ks_score_state *state, const tone_list *tones, i16 *buf, u32 len);

bool ks_score_data_event_run(const ks_score_data* score, u32 sampling_rate, ks_score_state* state,  const tone_list* tones);

void ks_score_state_set_default(ks_score_state *state, const tone_list *tones, u32 sampling_rate, u32 resolution);

ks_score_event* ks_score_events_new(u32 num_events, ks_score_event events[]);
void ks_score_events_free(const ks_score_event *events);

bool ks_score_note_is_enabled(const ks_score_note* note);

bool ks_score_note_info_equals(ks_score_note_info i1, ks_score_note_info i2);
i16 ks_score_note_info_hash(ks_score_note_info id);
ks_score_note_info ks_score_note_info_of(u8 note_number, u8 channel);


ks_score_data* ks_score_data_from_midi(ks_midi_file *file);
