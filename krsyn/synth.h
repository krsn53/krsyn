/**
 * @file ks_synth.h
 * @brief Synthesizer core
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//#include <xmmintrin.h> perhaps, my implementation is so bad, so I have compiler vectorize automatically. It is faster.
#include <stdint.h>
#include <stdbool.h>
#include <ksio/io.h>

// macro utils
// a * 2^x
#define ks_v(a, x)                      ((a) << (x))
// 2^x
#define ks_1(x)                         ks_v(1, x)
// 2^x -1
#define ks_m(x)                         (ks_1(x) - 1)
// a & (2^x - 1)
#define ks_mask(a, x)                   ((a) & ks_m(x))


// fixed point macros
#define KS_TABLE_BITS                   10u
#define KS_OUTPUT_BITS                  14u
#define KS_POWER_OF_2_BITS              11u

#define KS_FREQUENCY_BITS               16u
#define KS_RATESCALE_BITS               KS_POWER_OF_2_BITS

#define KS_PHASE_BITS                   (30u - KS_TABLE_BITS)
#define KS_PHASE_MAX_BITS               (KS_PHASE_BITS + KS_TABLE_BITS)
#define KS_NOISE_PHASE_BITS             (KS_PHASE_MAX_BITS - 5)



#define KS_SAMPLING_RATE_INV_BITS       30u
#define KS_RATESCALE_INV_BITS           30u
#define KS_KEYSENS_INV_BITS             30u

#define KS_PHASE_COARSE_BITS            1u
#define KS_PHASE_FINE_BITS              (KS_FREQUENCY_BITS)

#define KS_NUM_OPERATORS                4u
#define KS_NUM_ENVELOPES                2u
#define KS_FILTER_LOG_BITS              1u
#define KS_FILTER_NUM_LOGS              ks_1(KS_FILTER_LOG_BITS)

#define KS_ENVELOPE_BITS                30u
#define KS_ENVELOPE_NUM_POINTS          4u
#define KS_ENVELOPE_RELEASE_INDEX       3u

#define KS_VELOCITY_SENS_BITS           7u
#define KS_FILTER_Q_BITS                7u
#define KS_MIX_BITS                     7u

#define KS_LFO_DEPTH_BITS               16u
#define KS_NUM_LFOS                     2u
#define KS_PITCH_BEND_BITS              KS_LFO_DEPTH_BITS;

#define KS_LEVEL_BITS                   16u

#define KS_PANPOT_BITS                  KS_OUTPUT_BITS
#define KS_VOLUME_BITS                  13u

#define KS_TIME_BITS                    16u
#define KS_DELTA_TIME_BITS              30u

#define KS_UPDATE_PER_FRAMES_BITS       3u
#define KS_UPDATE_PER_FRAMES            ks_1(KS_UPDATE_PER_FRAMES_BITS)

/*
 * @enum ks_envelope_state
 * @brief Current envelope state
*/
typedef enum ks_envelope_state
{
    KS_ENVELOPE_OFF = 0,
    KS_ENVELOPE_ON = 1,
    KS_ENVELOPE_SUSTAINED,
    KS_ENVELOPE_RELEASED = 0x80,
}ks_envelope_state;

typedef enum ks_filter_t{
    KS_LOW_PASS_FILTER,
    KS_HIGH_PASS_FILTER,
    KS_BAND_PASS_FILTER,

    KS_NUM_FILTER_TYPES,
} ks_filter_t;

/**
  * @enum ks_mod_t
  * @brief types of modulation or modifier
*/
typedef enum ks_mod_t{
    KS_MOD_MIX,
    KS_MOD_MUL,
    KS_MOD_AM,
    KS_MOD_PASS,

    KS_NUM_MODS,
}ks_mod_t;

typedef enum ks_wave_t
{
    KS_WAVE_SIN,
    KS_WAVE_TRIANGLE,
    KS_WAVE_FAKE_TRIANGLE,
    KS_WAVE_SAW_UP,
    KS_WAVE_SAW_DOWN,
    KS_WAVE_SQUARE,
    KS_WAVE_NOISE,

    KS_NUM_WAVES,
}ks_synth_wave_t;


#define KS_MAX_WAVES                128u
#define KS_CUSTOM_WAVE_BITS         3u

typedef struct ks_synth_context{
    u32         sampling_rate;
    u32         sampling_rate_inv;
    u32         note_deltas[128];
    u16         powerof2[ks_1(KS_TABLE_BITS)]; // 1 ~ 2^4
    i16         *(wave_tables[KS_MAX_WAVES]);
}ks_synth_context;


typedef struct ks_envelope_point_data{
    u8 time : 5;
    u8 amp: 3;
} ks_envelope_point_data;

typedef struct ks_envelope_data{
    ks_envelope_point_data          points              [KS_ENVELOPE_NUM_POINTS];
    u8                              level               : 8;
    u8                              ratescale           : 4;
    u8                              velocity_sens       : 4;
} ks_envelope_data;


typedef struct ks_operator_data{
    u8                      use_custom_wave         : 1;
    u8                      wave_type               : 7;
    u8                      fixed_frequency         : 1;
    u8                      phase_coarse            : 6;
    u8                      phase_offset            : 4;
    u8                      phase_fine              : 4;
    u8                      semitones               : 6;
}ks_operator_data;

typedef struct ks_mod_data{
    u8                      type                    : 2;
    u8                      fm_level                : 6;
    u8                      sync                    : 1;
    u8                      mix                     : 7;
} ks_mod_data;

typedef struct ks_lfo_data{
    u8                      op_enabled       : 4;
    u8                      offset           : 4;
    u8                      filter_enabled   : 1;
    u8                      panpot_enabled   : 1;
    u8                      freq             : 5;
    u8                      level            : 5;
    u8                      use_custom_wave  : 1;
    u8                      wave             : 7;
}ks_lfo_data;


/**
 * @struct ks_synth_data
 * @brief Binary data of the synthesizer.
*/
typedef struct ks_synth_data{
        ks_operator_data    operators           [KS_NUM_OPERATORS];
        ks_mod_data         mods                [KS_NUM_OPERATORS-1];
        ks_envelope_data    envelopes           [KS_NUM_ENVELOPES];
        ks_lfo_data         lfos                [KS_NUM_LFOS];
        u8                  filter_type         : 3;
        u8                  filter_cutoff       : 5;
        u8                  filter_q            : 4;
        u8                  filter_key_sens     : 4;
        u8                  panpot              : 4;
}ks_synth_data;

typedef struct ks_synth_note ks_synth_note;
typedef struct ks_synth     ks_synth;


typedef struct ks_synth_operator{
    u32             phase_offset;
    u32             phase_coarse;
    i32             phase_fine;

    u8              lfo_op_enable               [KS_NUM_LFOS];

    u8              semitone;
    bool            fixed_frequency;

    u8              filter_type;
    const i16*      wave_table;
} ks_synth_operator;

typedef struct ks_synth_mod {
    u32             output_level;
    u32             mod_level;
    u32             fm_level;
    u8              type;
    bool            sync;
} ks_synth_mod;

typedef struct ks_synth_envelope{
    i32             points             [KS_ENVELOPE_NUM_POINTS];
    u32             samples            [KS_ENVELOPE_NUM_POINTS];

    i16             velocity_sens;
    u32             ratescale;
    i32             level;
}ks_synth_envelope;

typedef struct ks_synth_lfo {
    u32             level                  [KS_NUM_LFOS];
    u32             offset                 [KS_NUM_LFOS];
    u32             delta                  [KS_NUM_LFOS];

    const i16*      wave_table             [KS_NUM_LFOS];

}ks_synth_lfo;

/**
 * @struct ks_synth_data
 * @brief Synthesizer data read for ease of calculation.
*/
typedef struct ks_synth
{
    ks_synth_mod        mods                    [KS_NUM_OPERATORS-1];
    ks_synth_operator   operators               [KS_NUM_OPERATORS];
    ks_synth_envelope   envelopes               [KS_NUM_ENVELOPES];

    u32             panpot;

    u32             lfo_levels                  [KS_NUM_LFOS];
    u32             lfo_offsets                 [KS_NUM_LFOS];
    u32             lfo_deltas                  [KS_NUM_LFOS];

    u32             filter_cutoff;
    u32             filter_q;
    u32             filter_key_sens;
    u16             filter_envelope_base;

    u8              lfo_filter_enabled;
    u8              lfo_panpot_enabled;

    u8              filter_type;


    const i16*      lfo_wave_tables             [KS_NUM_LFOS];

    bool            enabled;
}
ks_synth;

typedef struct ks_synth_note_operator{
    u32                 phase;
    u32                 phase_delta;
}ks_synth_note_operator;

typedef struct ks_synth_note_envelope{
    i32                 level;
    i32                 points             [KS_ENVELOPE_NUM_POINTS];
    u32                 samples            [KS_ENVELOPE_NUM_POINTS];
    i32                 deltas             [KS_ENVELOPE_NUM_POINTS];
    i32                 diffs              [KS_ENVELOPE_NUM_POINTS];

    u32                 now_delta;
    i32                 now_time;
    i32                 now_amp;
    i32                 now_remain;
    i32                 now_diff;
    i32                 now_point_amp;
    u32                 update_clock;
    u8                  state;
    u8                  now_point;

}ks_synth_note_envelope;

/**
 * @struct ks_synth_data
 * @brief Note state.
*/
typedef  struct ks_synth_note
{
    const ks_synth* synth;

    ks_synth_note_operator  operators                   [KS_NUM_OPERATORS];
    ks_synth_note_envelope  envelopes                   [KS_NUM_ENVELOPES];

    i32                     filter_in_logs              [4];
    i32                     filter_out_logs             [4];
    u32                     filter_seek;
    u32                     filter_cutoff;

    i32 a1a0;
    i32 a2a0;

    i32 b0a0;
    i32 b1a0;
    i32 b2a0;

    u32                     lfo_phases                  [KS_NUM_LFOS];
    u32                     noise_table_offset;
}
ks_synth_note;

ks_io_decl_custom_func(ks_synth_data);
ks_io_decl_custom_func(ks_envelope_data);
ks_io_decl_custom_func(ks_envelope_point_data);
ks_io_decl_custom_func(ks_operator_data);
ks_io_decl_custom_func(ks_mod_data);
ks_io_decl_custom_func(ks_lfo_data);

ks_synth_context*           ks_synth_context_new            (u32 sampling_rate);
void                        ks_synth_context_free           (ks_synth_context* ctx);

ks_synth*                   ks_synth_new                    (ks_synth_data* data, const ks_synth_context *ctx);
ks_synth*                   ks_synth_array_new              (u32 length, ks_synth_data data[], const ks_synth_context *ctx);
void                        ks_synth_free                   (ks_synth* synth);
void                        ks_synth_data_set_default       (ks_synth_data* data);
void                        ks_synth_set                    (ks_synth* synth, const ks_synth_context *ctx, const ks_synth_data* data);
void                        ks_synth_render                 (const ks_synth_context*ctx, ks_synth_note* note, u32 volume, u32 pitchbend, i32 *buf, u32 len);
void                        ks_synth_note_on                (ks_synth_note* note, const ks_synth *synth, const ks_synth_context* ctx,  u8 notenum, u8 velocity);
void                        ks_synth_note_off               (ks_synth_note* note);
bool                        ks_synth_note_is_enabled        (const ks_synth_note* note);
bool                        ks_synth_note_is_on             (const ks_synth_note* note);

// ((1<<v_bits) + v) << (e-1)
// max : (1<<v_bits) << (1<<e_bits)
// example, v_bits = 4, e_bits = 4:
// max = ((1<<4)+15)<<(15-1) < 1<<5<<14 = 1<<19 = 1<<(e_maｘ+v_bits)
u64                         ks_exp_u                        (u32 val, int num_v_bit);
u32                         ks_calc_envelope_time           (u32 val);
u32                         ks_calc_envelope_samples        (u32 smp_freq, u8 val);
// min <= val < max
i64                         ks_linear                       (u8 val, i32 MIN, i32 MAX);

u32                         ks_fms_depth                    (i32 depth);

void                        ks_calc_panpot                  (const ks_synth_context *ctx, i16* left, i16* right, u32 val);
i32                         ks_apply_panpot                 (i32 in, i16 pan);

#define ks_linear_i         (i32)ks_linear
#define ks_linear_u         (u32)ks_linear
#define ks_linear_u16       (u32)ks_linear16

#define ks_wave_index(use_custom, wave_type)            (wave_type | ks_v(use_custom, KS_CUSTOM_WAVE_BITS))

#define calc_fixed_frequency(value)                     (value)
#define calc_frequency_fixed(value)                     ((ks_exp_u(value << 2, 5)*40) >> 7)
#define calc_phase_coarses(value)                       (value)
#define calc_phase_offsets(value)                       ks_linear_u(ks_v(value, (8-4)), 0, ks_1(KS_PHASE_MAX_BITS))
#define calc_semitones(value)                           (value)
#define calc_phase_fines(value)                         ks_linear_i(ks_v(value,(8-4)), -ks_v(8,KS_PHASE_FINE_BITS - 12), ks_v(8,KS_PHASE_FINE_BITS - 12))
#define calc_fm_levels(value)                           ks_linear_u(ks_v(value,(8-6)), 0, ks_1(KS_LEVEL_BITS))
#define calc_mix_levels(value)                          ks_linear_u(ks_v(value,(8-7)), 0, ks_1(KS_LEVEL_BITS))
#define calc_levels(value)                              ks_linear_i(value, - ks_1(KS_LEVEL_BITS), ks_1(KS_LEVEL_BITS)+ks_v(66, KS_LEVEL_BITS-13))
#define calc_envelope_points(value)                     ks_linear_i(ks_v(value, (8-3)), 0, ks_1(KS_ENVELOPE_BITS)+ks_v(1170, KS_ENVELOPE_BITS - 13))
#define calc_envelope_samples(smp_freq, value)          ks_calc_envelope_samples(smp_freq, ks_v(value, (8-5)))
#define calc_envelope_time(value)                       ks_calc_envelope_time( ks_v(value, (8-5)))
#define calc_velocity_sens(value)                       ks_linear_i(ks_v(value, 8-4), 0, ks_1(KS_VELOCITY_SENS_BITS)+9)
#define calc_filter_q(value)                            ks_linear_i(ks_v(value, 8-4), ks_1(KS_FILTER_Q_BITS-1), ks_v(9, KS_FILTER_Q_BITS-1))
#define calc_filter_cutoff(ctx, value)                  (ks_exp_u(value, 1) * ctx->note_deltas[0] >> 3)
#define calc_filter_key_sens(value)                     (value == 0 ? 0 :ks_v(1ll, KS_KEYSENS_INV_BITS) /(ks_exp_u((19-value), 2)*3))
#define calc_ratescales(value)                          (value == 0 ? 0 :ks_v(1ll, KS_KEYSENS_INV_BITS) /(ks_exp_u((19-value), 2)*3))
#define calc_output(value)                              (value)
#define calc_panpot(value)                              ks_linear_u(ks_v(value, (7-4)), 0, 293)
#define calc_lfo_wave_type(value)                       (value)
#define calc_lfo_depth(value)                           (((i64)(ks_exp_u(ks_v(value, (8-5)), 4))* 43691) >> 18)
#define calc_lfo_freq(value)                            ks_exp_u(ks_v(value+4, (8-4)), 6) * ks_1(KS_FREQUENCY_BITS-7)
#define calc_lfo_offset(value)                          ks_linear_u(ks_v(value, (8-4)), 0, ks_1(KS_PHASE_MAX_BITS))


#ifdef __cplusplus
}
#endif
