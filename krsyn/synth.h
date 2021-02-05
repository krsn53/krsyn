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
#include "./math.h"
#include <ksio/io.h>

#define KS_PHASE_COARSE_BITS            1u
#define KS_PHASE_FINE_BITS              (KS_FREQUENCY_BITS)

#define KS_NUM_OPERATORS                4u

#define KS_ENVELOPE_BITS                30u
#define KS_ENVELOPE_NUM_POINTS          4u
#define KS_ENVELOPE_RELEASE_INDEX       3u

#define KS_VELOCITY_SENS_BITS           7u

#define KS_KEYSCALE_CURVE_TABLE_BITS    7u
#define KS_KEYSCALE_CURVE_BITS          16u
#define KS_KEYSCALE_CURVE_MAX_BITS      (KS_KEYSCALE_CURVE_BITS + KS_KEYSCALE_CURVE_TABLE_BITS)
#define KS_KEYSCALE_DEPTH_BITS          16u

#define KS_LFO_DEPTH_BITS               16u
#define KS_FEEDBACK_LEVEL_BITS          16u

#define KS_LEVEL_BITS                   KS_KEYSCALE_CURVE_BITS

#define KS_SAMPLE_PER_FRAMES_BITS       5u

#define KS_PANPOT_BITS          7u
#define KS_VOLUME_BITS          13u

#define KS_TIME_BITS            16u
#define KS_DELTA_TIME_BITS      30u

/**
 * @enum ks_synth_keyscale_t
 * @brief Key scale factor curve
 * keyscale_mid_point as center, volume is changed by key by key.
 * In LD and LU, volume changes linearly with frequency.
 * In ED and EU, volume changes exponentially.
*/
enum ks_keyscale_curve_t
{
    KS_KEYSCALE_CURVE_LD,
    KS_KEYSCALE_CURVE_ED,
    KS_KEYSCALE_CURVE_LU,
    KS_KEYSCALE_CURVE_EU,

    KS_KEYSCALE_CURVE_NUM_TYPES,
};

/**
 * @enum ks_synth_envelope_state
 * @brief Current envelope state
*/
typedef enum ks_envelope_state
{
    KS_ENVELOPE_OFF = 0,
    KS_ENVELOPE_ON,
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
    KS_MOD_ADD,
    KS_MOD_SUB,
    KS_MOD_MUL,
    KS_MOD_AM,
    KS_MOD_LPF, // low path filter
    KS_MOD_HPF, // high path filter
    KS_MOD_NONE,
    KS_MOD_PASS,

    KS_NUM_MODS,
}ks_mod_t;

typedef enum ks_wave_t
{
    KS_WAVE_SIN,
    KS_WAVE_TRIANGLE,
    KS_WAVE_SAW_UP,
    KS_WAVE_SAW_DOWN,
    KS_WAVE_SQUARE,
    KS_WAVE_NOISE,

    KS_NUM_WAVES,
}ks_synth_wave_t;


typedef struct ks_synth_envelope_data{
    u8 time : 5;
    u8 amp: 3;
} ks_synth_envelope_data;


typedef struct ks_synth_operator_data{
    u8                      mod_type                : 4;
    u8                      mod_src                 : 2;
    u8                      mod_sync                : 1;
    u8                      wave_type               : 3;
    u8                      velocity_sens           : 5;
    u8                      fixed_frequency         : 1;
    u8                      phase_coarse            : 6;
    u8                      phase_offset            : 4;
    u8                      phase_fine              : 4;
    u8                      semitones               : 6;
    u8                      level                   : 7;
    ks_synth_envelope_data  envelopes               [KS_ENVELOPE_NUM_POINTS];
    u8                      ratescale               : 3;
    u8                      keyscale_mid_point      : 5;
    u8                      keyscale_left           : 2;
    u8                      keyscale_right          : 2;
    u8                      keyscale_low_depth      : 4;
    u8                      keyscale_high_depth     : 4;
    u8                      lfo_ams_depth           : 4;
}ks_synth_operator_data;

typedef struct ks_synth_common_data{
    u8                     feedback_level   : 4;
    u8                     panpot           : 4;
    u8                     output           : 2;
    u8                     lfo_freq         : 5;
    u8                     lfo_fms_depth    : 5;
    u8                     lfo_wave_type    : 3;
    u8                     lfo_offset       : 4;

}ks_synth_common_data;


/**
 * @struct ks_synth_data
 * @brief Binary data of the synthesizer.
*/
typedef struct ks_synth_data{
        ks_synth_operator_data    operators[KS_NUM_OPERATORS];
        ks_synth_common_data  common;
}ks_synth_data;

typedef struct ks_synth_note ks_synth_note;
typedef struct ks_synth     ks_synth;

typedef i32 (* ks_wave_func)(u8, ks_synth_note*, u32);
typedef i32 (* ks_lfo_wave_func)(ks_synth_note*);
typedef i32 (* ks_mod_func)(u8, ks_synth_note*, i32);

/**
 * @struct ks_synth_data
 * @brief Synthesizer data read for ease of calculation.
*/
typedef struct ks_synth
{
    u32             phase_offsets               [KS_NUM_OPERATORS];
    u32             phase_coarses               [KS_NUM_OPERATORS];
    i32             phase_fines                 [KS_NUM_OPERATORS];

    u32             levels                      [KS_NUM_OPERATORS];

    i32             envelope_points             [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    u32             envelope_samples            [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];

    i8              velocity_base               [KS_NUM_OPERATORS];
    i16             velocity_sens               [KS_NUM_OPERATORS];
    u32             lfo_ams_depths              [KS_NUM_OPERATORS];

    u32             ratescales                  [KS_NUM_OPERATORS];
    u32             keyscale_low_depths         [KS_NUM_OPERATORS];
    u32             keyscale_high_depths        [KS_NUM_OPERATORS];
    u8              keyscale_mid_points         [KS_NUM_OPERATORS];
    u8              keyscale_curve_types        [2][KS_NUM_OPERATORS];

    u8              output;
    u32             feedback_level;

    i16             panpot_left,     panpot_right;

    u32             lfo_offset;
    u32             lfo_freq;
    u32             lfo_fms_depth;

    u8              semitones                   [KS_NUM_OPERATORS];

    bool            fixed_frequency             [KS_NUM_OPERATORS];

    u8              wave_types                  [KS_NUM_OPERATORS];
    ks_mod_func     mod_funcs                   [KS_NUM_OPERATORS];
    u8              mod_srcs                    [KS_NUM_OPERATORS];
    bool            mod_syncs                   [KS_NUM_OPERATORS];

    u8              lfo_wave_type;

    struct {
        bool            ams      :1;
        bool            fms      :1;
        bool            filter   :1;
    } lfo_enabled;
}
ks_synth;

/**
 * @struct ks_synth_data
 * @brief Note state.
*/
typedef  struct ks_synth_note
{
    const ks_synth* synth;

    i32                 output_logs                 [KS_NUM_OPERATORS];

    const i16*          wave_tables                 [KS_NUM_OPERATORS];
    i32                 mod_func_logs               [KS_NUM_OPERATORS][2];

    u32                 phases                      [KS_NUM_OPERATORS];
    u32                 phase_deltas                [KS_NUM_OPERATORS];

    i32                 envelope_points             [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    u32                 envelope_samples            [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    i32                 envelope_deltas             [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    i32                 envelope_diffs              [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];

    u32                 envelope_now_deltas         [KS_NUM_OPERATORS];
    i32                 envelope_now_times          [KS_NUM_OPERATORS];
    i32                 envelope_now_amps           [KS_NUM_OPERATORS];
    i32                 envelope_now_remains        [KS_NUM_OPERATORS];
    i32                 envelope_now_diffs          [KS_NUM_OPERATORS];
    i32                 envelope_now_point_amps     [KS_NUM_OPERATORS];
    u8                  envelope_states             [KS_NUM_OPERATORS];
    u8                  envelope_now_points         [KS_NUM_OPERATORS];

    u32                 lfo_phase;
    u32                 lfo_delta;
    i32                 lfo_log;
    const i16*          lfo_wave_table;
    ks_lfo_wave_func    lfo_func;

    u32                 now_frame;

}
ks_synth_note;

ks_io_decl_custom_func(ks_synth_data);
ks_io_decl_custom_func(ks_synth_common_data);
ks_io_decl_custom_func(ks_synth_envelope_data);
ks_io_decl_custom_func(ks_synth_operator_data);

ks_synth*                   ks_synth_new                    (ks_synth_data* data, u32 sampling_rate);
ks_synth*                   ks_synth_array_new              (u32 length, ks_synth_data data[], u32 sampling_rate);
void                        ks_synth_free                   (ks_synth* synth);
void                        ks_synth_data_set_default       (ks_synth_data* data);
void                        ks_synth_set                    (ks_synth* synth, u32 sampling_rate, const ks_synth_data* data);
void                        ks_synth_render                 (ks_synth_note* note, u32 volume, u32 pitchbend, i32 *buf, u32 len);
void                        ks_synth_note_on                (ks_synth_note* note, const ks_synth *synth, u32 sampling_rate,  u8 notenum, u8 velocity);
void                        ks_synth_note_off               (ks_synth_note* note);
bool                        ks_synth_note_is_enabled        (const ks_synth_note* note);
bool                        ks_synth_note_is_on             (const ks_synth_note* note);

u64                         ks_u2f                          (u32 val, int num_v);
u32                         ks_exp_u                        (u32 val, u32 base, int num_v_bit);
u32                         ks_calc_envelope_times          (u32 val);
u32                         ks_calc_envelope_samples        (u32 smp_freq, u8 val);
// min <= val < max
i64                         ks_linear                       (u8 val, i32 MIN, i32 MAX);

u32                         ks_fms_depth                    (i32 depth);

void                        ks_calc_panpot                  (i16* left, i16* right, u8 val);
i32                         ks_apply_panpot                 (i32 in, i16 pan);

#define ks_linear_i         (i32)ks_linear
#define ks_linear_u         (u32)ks_linear
#define ks_linear_u16       (u32)ks_linear16

#define calc_fixed_frequency(value)                     (value)
#define calc_frequency_fixed(value)                     ks_exp_u(value << 2, 40, 5)
#define calc_phase_coarses(value)                       (value)
#define calc_phase_offsets(value)                       ks_linear_u(ks_v(value, (8-4)), 0, ks_1(KS_PHASE_MAX_BITS))
#define calc_semitones(value)                           (value)
#define calc_phase_fines(value)                         ks_linear_i(ks_v(value,(8-4)), -ks_v(8,KS_PHASE_FINE_BITS - 12), ks_v(8,KS_PHASE_FINE_BITS - 12))
#define calc_levels(value)                              ks_linear_u(ks_v(value,(8-7)), 0, ks_1(KS_LEVEL_BITS)+ks_1(KS_LEVEL_BITS-7))
#define calc_envelope_points(value)                     ks_linear_i(ks_v(value, (8-3)), 0, ks_1(KS_ENVELOPE_BITS)+ks_1(KS_ENVELOPE_BITS - 3))
#define calc_envelope_samples(smp_freq, value)          ks_calc_envelope_samples(smp_freq, ks_v(value, (8-5)))
#define calc_envelope_times(value)                      ks_calc_envelope_times( ks_v(value, (8-5)))
#define calc_velocity_sens(value)                       ks_linear_i(ks_v(value, 8-5), -ks_1(KS_VELOCITY_SENS_BITS), ks_1(KS_VELOCITY_SENS_BITS)+9)
#define calc_ratescales(value)                          ks_exp_u(ks_v(value, (8-3)), ks_v(341, KS_RATESCALE_BITS -10) , 6)
#define calc_keyscale_low_depths(value)                 ks_linear_u(ks_v(value, (8-4)), 0, ks_1(KS_KEYSCALE_DEPTH_BITS)+ks_1(KS_KEYSCALE_DEPTH_BITS - 4))
#define calc_keyscale_high_depths(value)                ks_linear_u(ks_v(value, (8-4)), 0, ks_1(KS_KEYSCALE_DEPTH_BITS)+ks_1(KS_KEYSCALE_DEPTH_BITS - 4))
#define calc_keyscale_mid_points(value)                 ks_v(value, (8-6))
#define calc_keyscale_curve_types_left(value)           (value)
#define calc_keyscale_curve_types_right(value)          (value)
#define calc_lfo_ams_depths(value)                      ks_linear_u(ks_v(value, (8-4)), 0, ks_1(KS_LFO_DEPTH_BITS)+ ks_1(KS_LFO_DEPTH_BITS-4))
#define calc_output(value)                              (value)
#define calc_feedback_level(value)                      ks_linear_u(ks_v(value, (8-4)), 0, ks_1(KS_FEEDBACK_LEVEL_BITS))
#define calc_panpot(value)                              ks_linear_u(ks_v(value, (7-4)), 0, ks_1(8))
#define calc_lfo_wave_type(value)                      (value)
#define calc_lfo_fms_depth(value)                       ks_exp_u(ks_v(value, (8-5)), (ks_1(KS_LFO_DEPTH_BITS-11)), 4)
#define calc_lfo_freq(value)                            ks_exp_u(ks_v(value+4, (8-4)), ks_1(KS_FREQUENCY_BITS), 6)
#define calc_lfo_offset(value)                          ks_linear_u(ks_v(value, (8-4)), 0, ks_1(KS_PHASE_MAX_BITS))


#ifdef __cplusplus
}
#endif
