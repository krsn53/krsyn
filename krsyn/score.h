/**
 * @file ks_score.h
 * @brief Song data
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "./synth.h"

#define KS_CHANNEL_BITs         4u
#define KS_NUM_CHANNELS         16u

#define KS_PITCH_BEND_EVENT_BITS      13u
#define KS_QUARTER_TIME_BITS    8u

#define     KS_DEFAULT_QUARTER_TIME     ks_1(KS_QUARTER_TIME_BITS - 1)

typedef         struct ks_tone_list         ks_tone_list;
typedef         struct ks_tone_list_bank    ks_tone_list_bank;
typedef         struct ks_midi_file         ks_midi_file;

/**
  * @struct ks_score_channel
  * @brief
*/
typedef struct ks_score_channel{
    ks_tone_list_bank   *bank;
    u8                  program_number;
    ks_synth*           program;

    i16                 panpot_left;
    i16                 panpot_right;
    i32                 pitchbend;

    u8                  volume;
    u8                  expression;
    u16                 volume_cache;

    i32                *output_log;
}ks_score_channel;

/**
  * @struct ks_score_note_info
  * @brief
*/
typedef struct ks_score_note_info{
    u8            note_number;
    u8            channel;
}ks_score_note_info;

/**
  * @struct ks_score_note
  * @brief
*/
typedef struct ks_score_note{
    ks_synth_note           note;
    ks_score_note_info      info;
}ks_score_note;


typedef enum ks_effect_type{
    KS_EFFECT_VOLUME_ANALIZER,
}ks_effect_type;


typedef struct ks_volume_analizer{
    u32         length;
    u32         seek;
    i32         *log            [KS_NUM_CHANNELS + 1];
    u32         volume          [KS_NUM_CHANNELS];
    u32         volume_l,        volume_r;
}ks_volume_analizer;

/**
  * @struct ks_effect
  * @brief
*/
typedef struct ks_effect{
    ks_effect_type type;
    union{
        ks_volume_analizer volume_analizer;
    }data;
}ks_effect;

/**
  * @struct ks_effect_list
  * @brief
*/
typedef struct ks_effect_list{
    u32                     length;
    u32                     capacity;
    ks_effect               *data;
}ks_effect_list;

/**
  * @struct ks_score_state
  * @brief
*/
typedef struct ks_score_state{
    u16                 quarter_time;
    u16                 frames_per_event;
    u16                 remaining_frame;
    u16                 polyphony_bits;

    u32                 current_event;
    i32                 passed_tick;
    u32                 current_tick;

    ks_effect_list      effects;

    ks_score_channel    channels        [KS_NUM_CHANNELS];
    ks_score_note       notes           [];
}ks_score_state;

/**
  * @struct ks_score_event
  * @brief
*/
typedef struct ks_score_event{
    u32             delta;
    u8              status;
    u8              data           [3];
}ks_score_event;

/**
  * @struct ks_score_data
  * @brief
*/
typedef struct ks_score_data{
    char                title           [64];
    char                author          [64];
    char                license         [64];
    u16                 resolution;
    u32                 length;
    ks_score_event      *data;
    float               score_length;
}ks_score_data;

ks_io_decl_custom_func(ks_score_event);
ks_io_decl_custom_func(ks_score_data);

ks_score_data*      ks_score_data_new               (u32 resolution, u32 num_events, ks_score_event *events);
ks_score_data*      ks_score_data_from_midi         (ks_midi_file *file);
void                ks_score_data_free              (ks_score_data* song);

float               ks_score_data_calc_score_length (ks_score_data* data, const ks_synth_context* ctx);


ks_score_state*     ks_score_state_new              (u32 polyphony_bits);
void                ks_score_state_free             (ks_score_state* state);

bool                ks_score_state_note_on          (ks_score_state* state, const ks_synth_context* ctx, u8 channel_number, u8 note_number, u8 velocity);
bool                ks_score_state_note_off         (ks_score_state* state, u8 channel_number,  u8 note_number);
bool                ks_score_state_program_change   (ks_score_state* state, const ks_tone_list*tones,int ch_number,  u8 program);
bool                ks_score_state_tempo_change     (ks_score_state* state, const ks_synth_context*ctx, const ks_score_data* score, const u8* data);
bool                ks_score_state_control_change   (ks_score_state* state, const ks_tone_list* tones, const ks_synth_context*ctx, int ch_number,  u8 type, u8 value);
bool                ks_score_channel_set_panpot     (ks_score_channel* ch, const ks_synth_context*ctx, u8 value);
bool                ks_score_channel_set_picthbend  (ks_score_channel* ch, u8 msb, u8 lsb);
bool                ks_score_channel_set_volume     (ks_score_channel* ch, u8 value);
bool                ks_score_channel_set_expression (ks_score_channel* ch, u8 value);
bool                ks_score_state_bank_select      (ks_score_state* state, const ks_tone_list* tones, int ch_number,  u8 msb, u8 lsb);
bool                ks_score_state_bank_select_msb  (ks_score_state* state, const ks_tone_list* tones, int ch_number,  u8 msb);
bool                ks_score_state_bank_select_lsb  (ks_score_state* state, const ks_tone_list* tones, int ch_number,  u8 lsb);

void                ks_score_data_render            (const ks_score_data* score, const ks_synth_context*ctx, ks_score_state *state, const ks_tone_list *tones, i32 *buf, u32 len);
bool                ks_score_data_event_run         (const ks_score_data* score, const ks_synth_context*ctx , ks_score_state* state,  const ks_tone_list* tones);

void                ks_score_state_set_default      (ks_score_state *state, const ks_tone_list *tones, const ks_synth_context *ctx, u32 resolution);

ks_score_event*     ks_score_events_new             (u32 num_events, ks_score_event events[]);
void                ks_score_events_free            (const ks_score_event *events);

bool                ks_score_note_is_enabled        (const ks_score_note* note);
bool                ks_score_note_is_on             (const ks_score_note* note);

bool                ks_score_note_info_equals       (ks_score_note_info i1, ks_score_note_info i2);
i16                 ks_score_note_info_hash         (ks_score_note_info id);
ks_score_note_info  ks_score_note_info_of           (u8 note_number, u8 channel);

void                ks_score_state_add_volume_analizer      (ks_score_state* state, const ks_synth_context* ctx, u32 duration);

void                ks_effect_volume_analize                (ks_effect* effect, ks_score_state* state, i32 *buf, u32 len, bool channels_enabled[]);
void                ks_effect_list_data_free                (u32 length, ks_effect* data);
void                ks_effect_list_data_free                (u32 length, ks_effect* data);
void                ks_effect_list_data_free                (u32 length, ks_effect* data);
void                ks_effect_volume_analizer_clear         (ks_effect* effect);
const u32 *         ks_effect_calc_volume                   (ks_effect* effect);

u32                 ks_calc_quarter_time                    (const u8* data);
u32                 ks_calc_frames_per_event                (const ks_synth_context* ctx, u16 quarter_time, u16 resolution);


#ifdef __cplusplus
}
#endif
