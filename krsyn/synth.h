/**
 * @file ks_synth.h
 * @brief Synthesizer core
*/
#pragma once

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

#define KS_VELOCITY_SENS_BITS           16u

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

/**
 * @enum ks_synth_keyscale_t
 * @brief Key scale factor curve
 * keyscale_mid_point as center, volume is changed by key by key.
 * In LD and LU, volume changes linearly with frequency.
 * In ED and EU, volume changes exponentially.
*/
enum ks_keyscale_t
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
 * @enum ks_synthWaveType
 * @brief Wave type
*/
typedef enum ks_lfo_wave_t
{
    KS_WAVE_SIN,
    KS_WAVE_TRIANGLE,
    KS_WAVE_SAW_UP,
    KS_WAVE_SAW_DOWN,
    KS_WAVE_SQUARE,
    KS_WAVE_NOISE,

    KS_NUM_WAVES,
}ks_synth_lfo_wave_t;

/**
 * @enum ks_synthLFOWaveType
 * @brief LFO wave type
*/
typedef enum ks_wave_t
{
    KS_LFO_WAVE_SIN,
    KS_LFO_WAVE_TRIANGLE,
    KS_LFO_WAVE_SAW_UP,
    KS_LFO_WAVE_SAW_DOWN,
    KS_LFO_WAVE_SQUARE,
    KS_LFO_WAVE_NOISE,

    KS_LFO_NUM_WAVES,
}ks_synth_wave_t;

/**
  * @struct ks_phase_coarse_t
  * @brief Bit field of phase coarse data
  */
typedef struct ks_phase_coarse_t
{
    u8 fixed_frequency: 1;
    u8 value :7;
}ks_phase_coarse_t;

/**
  * @struct ks_keyscale_curve_t
  * @brief Bit field of keyscale curve data
 */
typedef struct ks_keyscale_curve_t
{
    u8 left : 4;
    u8 right : 4;
}ks_keyscale_curve_t;

/**
 * @struct ks_synth_data
 * @brief Binary data of the synthesizer.
*/
typedef struct ks_synth_data
{
    u8                      wave_types              [KS_NUM_OPERATORS];
    union {
        ks_phase_coarse_t       st                  [KS_NUM_OPERATORS];
        u8                      b                   [KS_NUM_OPERATORS];
    }phase_coarses;
    u8                      phase_offsets           [KS_NUM_OPERATORS];
    u8                      phase_fines             [KS_NUM_OPERATORS];
    u8                      phase_tunes             [KS_NUM_OPERATORS];
    u8                      levels                  [KS_NUM_OPERATORS];
    u8                      envelope_points         [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    u8                      envelope_times          [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    u8                      velocity_sens           [KS_NUM_OPERATORS];
    u8                      ratescales              [KS_NUM_OPERATORS];
    u8                      keyscale_low_depths     [KS_NUM_OPERATORS];
    u8                      keyscale_high_depths    [KS_NUM_OPERATORS];
    u8                      keyscale_mid_points     [KS_NUM_OPERATORS];
    union {
         ks_keyscale_curve_t    st                  [KS_NUM_OPERATORS];
         u8                     b                   [KS_NUM_OPERATORS];
    }keyscale_curve_types;
    u8                     lfo_ams_depths           [KS_NUM_OPERATORS];
    u8                     algorithm;
    u8                     feedback_level;
    u8                     panpot;
    u8                     lfo_wave_type;
    u8                     lfo_freq;
    u8                     lfo_det;
    u8                     lfo_fms_depth;

}
ks_synth_data;

typedef struct ks_synth_note ks_synth_note;
typedef struct ks_synth     ks_synth;

typedef i16 (* ks_wave_func)(u32);
typedef i32 (* ks_mod_func)(u8, ks_synth_note*, i32);

/**
 * @struct ks_synth_data
 * @brief Synthesizer data read for ease of calculation.
*/
typedef struct ks_synth
{
    u32             phase_offsets               [KS_NUM_OPERATORS];
    u32             phase_coarses               [KS_NUM_OPERATORS];
    u32             phase_fines                 [KS_NUM_OPERATORS];
    u32             phase_tunes                 [KS_NUM_OPERATORS];
    u32             levels                      [KS_NUM_OPERATORS];

    i32             envelope_points             [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    u32             envelope_samples            [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];

    u32             velocity_sens               [KS_NUM_OPERATORS];
    u32             lfo_ams_depths              [KS_NUM_OPERATORS];

    u32             ratescales                  [KS_NUM_OPERATORS];
    u32             keyscale_low_depths         [KS_NUM_OPERATORS];
    u32             keyscale_high_depths        [KS_NUM_OPERATORS];
    u8              keyscale_mid_points         [KS_NUM_OPERATORS];
    u8              keyscale_curve_types        [2][KS_NUM_OPERATORS];

    u8              algorithm;
    u32             feedback_level;

    i16             panpot_left,     panpot_right;

    u32             lfo_det;
    u32             lfo_freq;
    u32             lfo_fms_depth;

    bool            fixed_frequency             [KS_NUM_OPERATORS];

    u8              wave_types                  [KS_NUM_OPERATORS];

    u8              lfo_wave_type;
    bool            lfo_ams_enabled;
    bool            lfo_fms_enabled;
}
ks_synth;

/**
 * @struct ks_synth_data
 * @brief Note state.
*/
typedef  struct ks_synth_note
{
    i32             output_log                  [KS_NUM_OPERATORS];

    ks_mod_func     mod_funcs                   [KS_NUM_OPERATORS];
    i32             mod_func_log                [KS_NUM_OPERATORS];

    u32             phases                      [KS_NUM_OPERATORS];
    u32             phase_deltas                [KS_NUM_OPERATORS];

    i32             envelope_points             [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    u32             envelope_samples            [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    i32             envelope_deltas             [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];

    i32             envelope_now_deltas         [KS_NUM_OPERATORS];
    i32             envelope_now_times          [KS_NUM_OPERATORS];
    i32             envelope_now_amps           [KS_NUM_OPERATORS];
    u8              envelope_states             [KS_NUM_OPERATORS];
    u8              envelope_now_points         [KS_NUM_OPERATORS];

    u32             lfo_phase;
    u32             lfo_delta;
    i32             lfo_log;
    i32             lfo_func_log;
    i32             feedback_log;

    u32             now_frame;
}
ks_synth_note;

ks_io_decl_custom_func(ks_synth_data);

ks_synth*                   ks_synth_new                    (ks_synth_data* data, u32 sampling_rate);
ks_synth*                   ks_synth_array_new              (u32 length, ks_synth_data data[], u32 sampling_rate);
void                        ks_synth_free                   (ks_synth* synth);
void                        ks_synth_data_set_default       (ks_synth_data* data);
void                        ks_synth_set                    (ks_synth* synth, u32 sampling_rate, const ks_synth_data* data);
void                        ks_synth_render                 (const ks_synth *synth, ks_synth_note* note, u32 volume, u32 pitchbend, i32 *buf, u32 len);
void                        ks_synth_note_on                (ks_synth_note* note, const ks_synth *synth, u32 sampling_rate,  u8 notenum, u8 velocity);
void                        ks_synth_note_off               (ks_synth_note* note);
bool                        ks_synth_note_is_enabled        (const ks_synth_note* note);
bool                        ks_synth_note_is_on             (const ks_synth_note* note);

u32                         ks_exp_u                        (u8 val, u32 base, int num_v_bit);
u32                         ks_calc_envelope_times          (u32 val);
u32                         ks_calc_envelope_samples        (u32 smp_freq, u8 val);
// min <= val < max
i64                         ks_linear                       (u8 val, i32 MIN, i32 MAX);
// min <= val <= max
i64                         ks_linear2                      (u8 val, i32 MIN, i32 MAX);
u32                         ks_fms_depth                    (i32 depth);

void                        ks_calc_panpot                  (i16* left, i16* right, u8 val);
i32                         ks_apply_panpot                 (i32 in, i16 pan);

#define ks_linear_i         (i32)ks_linear
#define ks_linear_u         (u32)ks_linear
#define ks_linear_u16       (u32)ks_linear16
#define ks_linear2_i        (i32)ks_linear2
#define ks_linear2_u        (u32)ks_linear2

#define calc_fixed_frequency(value)                     (value)
#define calc_phase_coarses(value)                       (value)
#define calc_phase_offsets(value)                       ks_linear_u(value, 0, 1 << (KS_PHASE_MAX_BITS))
#define calc_phase_fines(value)                         ks_linear_u(value, 0, 1 << (KS_PHASE_FINE_BITS))
#define calc_phase_tunes(value)                         ks_linear_u(value-127, 0, 1<< (KS_PHASE_FINE_BITS - 8))
#define calc_levels(value)                              ks_linear_u(value, 0, ks_1(KS_LEVEL_BITS))
#define calc_envelope_points(value)                     ks_linear2_i(value, 0, 1 << KS_ENVELOPE_BITS)
#define calc_envelope_samples(smp_freq, value)          ks_calc_envelope_samples(smp_freq, value)
#define calc_velocity_sens(value)                       ks_linear2_u(value, 0, 1 << KS_VELOCITY_SENS_BITS)
#define calc_ratescales(value)                          ks_linear2_u(value, 0, 1 << KS_RATESCALE_BITS)
#define calc_keyscale_low_depths(value)                 ks_linear2_u(value, 0, 1 << KS_KEYSCALE_DEPTH_BITS)
#define calc_keyscale_high_depths(value)                ks_linear2_u(value, 0, 1 << KS_KEYSCALE_DEPTH_BITS)
#define calc_keyscale_mid_points(value)                 (value & 0x7f)
#define calc_keyscale_curve_types_left(value)           (value)
#define calc_keyscale_curve_types_right(value)          (value)
#define calc_lfo_ams_depths(value)                      ks_linear2_u(value, 0, 1 << KS_LFO_DEPTH_BITS)
#define calc_algorithm(value)                           (value)
#define calc_feedback_level(value)                      ks_linear_u(value, 0, 1<<KS_FEEDBACK_LEVEL_BITS)
#define calc_panpot(value)                              (value)
#define calc_lfo_wave_type(value)                      (value)
#define calc_lfo_fms_depth(value)                       ks_exp_u(value, 1 << (KS_LFO_DEPTH_BITS-12), 4)
#define calc_lfo_freq(value)                            ks_exp_u(value, 1<<(KS_FREQUENCY_BITS-5), 4)
#define calc_lfo_det(value)                             ks_linear_u(value, 0, ks_1(KS_PHASE_MAX_BITS))

