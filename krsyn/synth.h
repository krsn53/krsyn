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
#define KS_OUTPUT_BITS                  15u
#define KS_POWER_OF_2_BITS              11u

#define KS_FREQUENCY_BITS               16u
#define KS_RATESCALE_BITS               16u

#define KS_PHASE_BITS                   (30u - KS_TABLE_BITS)
#define KS_PHASE_MAX_BITS               (KS_PHASE_BITS + KS_TABLE_BITS)
#define KS_NOISE_PHASE_BITS             (KS_PHASE_MAX_BITS - 5)


#define KS_SAMPLING_RATE_INV_BITS       30u

#define KS_PHASE_COARSE_BITS            1u
#define KS_PHASE_FINE_BITS              (KS_FREQUENCY_BITS)

#define KS_NUM_OPERATORS                4u
#define KS_NUM_ENVELOPES                2u

#define KS_ENVELOPE_BITS                30u
#define KS_ENVELOPE_NUM_POINTS          4u
#define KS_ENVELOPE_RELEASE_INDEX       3u

#define KS_VELOCITY_SENS_BITS           7u

#define KS_LFO_DEPTH_BITS               16u
#define KS_PITCH_BEND_BITS              KS_LFO_DEPTH_BITS;
#define KS_FEEDBACK_LEVEL_BITS          16u

#define KS_LEVEL_BITS                   KS_KEYSCALE_CURVE_BITS

#define KS_PANPOT_BITS          7u
#define KS_VOLUME_BITS          13u

#define KS_TIME_BITS            16u
#define KS_DELTA_TIME_BITS      30u

/*
 * @enum ks_synth_envelope_state
 * @brief Current envelope state
*/
typedef enum ks_envelope_state
{
    KS_ENVELOPE_OFF = 0,
    KS_ENVELOPE_ON = 1,
    KS_ENVELOPE_SUSTAINED,
    KS_ENVELOPE_RELEASED = 0x80,
}ks_synth_envelope_state;

/**
  * @enum ks_mod_t
  * @brief types of modulation or modifier
*/
typedef enum ks_mod_t{
    KS_MOD_FM,
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
    u32         ratescales[128];
    u16         powerof2[128];
    i16         *(wave_tables[KS_MAX_WAVES]);
}ks_synth_context;

typedef struct ks_synth_envelope_data{
    u8 time : 5;
    u8 amp: 3;
} ks_synth_envelope_data;


typedef struct ks_synth_operator_data{
    u8                      use_custom_wave         : 1;
    u8                      wave_type               : 7;
    u8                      velocity_sens           : 5;
    u8                      fixed_frequency         : 1;
    u8                      phase_coarse            : 6;
    u8                      phase_offset            : 4;
    u8                      phase_fine              : 4;
    u8                      semitones               : 6;
    u8                      lfo_ams_depth           : 4;
}ks_synth_operator_data;

typedef struct ks_synth_mod_data{
    u8                      type                    : 4;
    u8                      level                   : 4;
    u8                      sync                    : 1;
    u8                      mix_rate                : 7;
} ks_synth_mod_data;

typedef struct ks_synth_common_data{
    ks_synth_envelope_data  envelopes            [2][KS_ENVELOPE_NUM_POINTS];
    u8                      amp_ratescale        : 3;
    u8                      filter_ratescale     : 3;
    u8                      panpot               : 4;
    u8                      lfo_freq             : 5;
    u8                      lfo_fms_depth        : 5;
    u8                      lfo_use_custom_wave  : 1;
    u8                      lfo_wave_type        : 7;
    u8                      lfo_offset           : 4;

}ks_synth_common_data;


/**
 * @struct ks_synth_data
 * @brief Binary data of the synthesizer.
*/
typedef struct ks_synth_data{
        ks_synth_operator_data      operators   [KS_NUM_OPERATORS];
        ks_synth_mod_data           mods        [KS_NUM_OPERATORS-1];
        ks_synth_common_data        common;
}ks_synth_data;

typedef struct ks_synth_note ks_synth_note;
typedef struct ks_synth     ks_synth;


/**
 * @struct ks_synth_data
 * @brief Synthesizer data read for ease of calculation.
*/
typedef struct ks_synth
{
    u32             output_level                [KS_NUM_OPERATORS-1];
    u32             output_mod_level            [KS_NUM_OPERATORS-1];
    u32             mod_input                   [KS_NUM_OPERATORS-1];


    u32             phase_offsets               [KS_NUM_OPERATORS];
    u32             phase_coarses               [KS_NUM_OPERATORS];
    i32             phase_fines                 [KS_NUM_OPERATORS];

    i32             envelope_points             [KS_ENVELOPE_NUM_POINTS][2];
    u32             envelope_samples            [KS_ENVELOPE_NUM_POINTS][2];

    i8              velocity_base               [KS_NUM_OPERATORS];
    i16             velocity_sens               [KS_NUM_OPERATORS];
    u32             lfo_ams_depths              [KS_NUM_OPERATORS];

    u32             ratescales                  [KS_NUM_ENVELOPES];

    i16             panpot_left,     panpot_right;

    u32             lfo_offset;
    u32             lfo_delta;
    u32             lfo_fms_depth;

    u8              semitones                   [KS_NUM_OPERATORS];
    u8              mod_types                   [KS_NUM_OPERATORS-1];
    bool            mod_syncs                   [KS_NUM_OPERATORS-1];
    bool            fixed_frequency             [KS_NUM_OPERATORS];

    const i16*      wave_tables                 [KS_NUM_OPERATORS];

    const i16*          lfo_wave_table;

    struct {
        bool            ams      :1;
        bool            fms      :1;
    }               lfo_enabled;

    bool            enabled;
}
ks_synth;

/**
 * @struct ks_synth_data
 * @brief Note state.
*/
typedef  struct ks_synth_note
{
    const ks_synth* synth;

    u32                 phases                      [KS_NUM_OPERATORS];
    u32                 phase_deltas                [KS_NUM_OPERATORS];

    i32                 envelope_points             [KS_ENVELOPE_NUM_POINTS][KS_NUM_ENVELOPES];
    u32                 envelope_samples            [KS_ENVELOPE_NUM_POINTS][KS_NUM_ENVELOPES];
    i32                 envelope_deltas             [KS_ENVELOPE_NUM_POINTS][KS_NUM_ENVELOPES];
    i32                 envelope_diffs              [KS_ENVELOPE_NUM_POINTS][KS_NUM_ENVELOPES];

    u32                 envelope_now_deltas         [KS_NUM_ENVELOPES];
    i32                 envelope_now_times          [KS_NUM_ENVELOPES];
    i32                 envelope_now_amps           [KS_NUM_ENVELOPES];
    i32                 envelope_now_remains        [KS_NUM_ENVELOPES];
    i32                 envelope_now_diffs          [KS_NUM_ENVELOPES];
    i32                 envelope_now_point_amps     [KS_NUM_ENVELOPES];
    u8                  envelope_states             [KS_NUM_ENVELOPES];
    u8                  envelope_now_points         [KS_NUM_ENVELOPES];

    u32                 lfo_phase;
    u32                 lfo_delta;
    i32                 lfo_log;

    u32                 lfo_func_log;
}
ks_synth_note;

ks_io_decl_custom_func(ks_synth_data);
ks_io_decl_custom_func(ks_synth_common_data);
ks_io_decl_custom_func(ks_synth_envelope_data);
ks_io_decl_custom_func(ks_synth_operator_data);
ks_io_decl_custom_func(ks_synth_mod_data);

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

u64                         ks_u2f                          (const ks_synth_context*ctx, u32 val, int num_v);
u32                         ks_exp_u                        (u32 val, u32 base, int num_v_bit);
u32                         ks_calc_envelope_times          (u32 val);
u32                         ks_calc_envelope_samples        (u32 smp_freq, u8 val);
// min <= val < max
i64                         ks_linear                       (u8 val, i32 MIN, i32 MAX);

u32                         ks_fms_depth                    (i32 depth);

void                        ks_calc_panpot                  (const ks_synth_context *ctx, i16* left, i16* right, u8 val);
i32                         ks_apply_panpot                 (i32 in, i16 pan);

#define ks_linear_i         (i32)ks_linear
#define ks_linear_u         (u32)ks_linear
#define ks_linear_u16       (u32)ks_linear16

#define ks_wave_index(use_custom, wave_type)            (wave_type | ks_v(use_custom, KS_CUSTOM_WAVE_BITS))

#define calc_fixed_frequency(value)                     (value)
#define calc_frequency_fixed(value)                     ks_exp_u(value << 2, 40, 5)
#define calc_phase_coarses(value)                       (value)
#define calc_phase_offsets(value)                       ks_linear_u(ks_v(value, (8-4)), 0, ks_1(KS_PHASE_MAX_BITS))
#define calc_semitones(value)                           (value)
#define calc_phase_fines(value)                         ks_linear_i(ks_v(value,(8-4)), -ks_v(8,KS_PHASE_FINE_BITS - 12), ks_v(8,KS_PHASE_FINE_BITS - 12))
#define calc_levels(value)                              ks_linear_u(ks_v(value,(8-7)), 0, ks_1(KS_LEVEL_BITS)+ks_1(KS_LEVEL_BITS-7))
#define calc_envelope_points(value)                     ks_linear_i(ks_v(value, (8-3)), 0, ks_1(KS_ENVELOPE_BITS)+ks_v(1170, KS_ENVELOPE_BITS - 13))
#define calc_envelope_samples(smp_freq, value)          ks_calc_envelope_samples(smp_freq, ks_v(value, (8-5)))
#define calc_envelope_times(value)                      ks_calc_envelope_times( ks_v(value, (8-5)))
#define calc_velocity_sens(value)                       ks_linear_i(ks_v(value, 8-5), -ks_1(KS_VELOCITY_SENS_BITS), ks_1(KS_VELOCITY_SENS_BITS)+9)
#define calc_ratescales(value)                          ks_exp_u(ks_v(value, (8-3)), ks_v(341, KS_RATESCALE_BITS -10) , 6)
#define calc_lfo_ams_depths(value)                      ks_linear_u(ks_v(value, (8-4)), 0, ks_1(KS_LFO_DEPTH_BITS)+ ks_v(1090, KS_LFO_DEPTH_BITS-14))
#define calc_output(value)                              (value)
#define calc_panpot(value)                              ks_linear_u(ks_v(value, (7-4)), 0, 293)
#define calc_lfo_wave_type(value)                      (value)
#define calc_lfo_fms_depth(value)                       (((i64)(ks_exp_u(ks_v(value, (8-5)), (ks_1(KS_LFO_DEPTH_BITS-11)), 4))* 43691) >> 16)
#define calc_lfo_freq(value)                            ks_exp_u(ks_v(value+4, (8-4)), ks_1(KS_FREQUENCY_BITS), 6)
#define calc_lfo_offset(value)                          ks_linear_u(ks_v(value, (8-4)), 0, ks_1(KS_PHASE_MAX_BITS))


#ifdef __cplusplus
}
#endif
