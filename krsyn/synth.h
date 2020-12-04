/**
 * @file ks_synth.h
 * @brief Synthesizer core
*/
#pragma once

//#include <xmmintrin.h> perhaps, my implementation is so bad, so I have compiler vectorize automatically. It is faster.
#include <stdint.h>
#include <stdbool.h>
#include "./math.h"
#include "./io.h"

#define KS_FREQUENCY_BITS               16u

#define KS_PHASE_COARSE_BITS            1u
#define KS_PHASE_FINE_BITS              (KS_FREQUENCY_BITS - 1u)

#define KS_NUM_OPERATORS                 4u

#define KS_ENVELOPE_BITS                 30u
#define KS_ENVELOPE_NUM_POINTS            4u

#define KS_RATESCALE_BITS                      16u
#define KS_VELOCITY_SENS_BITS           16u

#define KS_KEYSCALE_CURVE_TABLE_BITS    7u
#define KS_KEYSCALE_CURVE_BITS          16u
#define KS_KEYSCALE_CURVE_MAX_BITS      (KS_KEYSCALE_CURVE_BITS + KS_KEYSCALE_CURVE_TABLE_BITS)
#define KS_KEYSCALE_DEPTH_BITS          16u

#define KS_LFO_DEPTH_BITS               16u
#define KS_FEEDBACK_LEVEL_BITS          16u

#define KS_SAMPLE_PER_FRAMES_BITS       5u

#define KS_PANPOT_BITS          7u


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
    KS_ENVELOPE_RELEASED,
}ks_synth_envelope_state;

/**
 * @enum ks_synthLFOWaveType
 * @brief LFO wave type
*/
typedef enum ks_lfo_wave_t
{
    KS_LFO_WAVE_TRIANGLE,
    KS_LFO_WAVE_SAW_UP,
    KS_LFO_WAVE_SAW_DOWN,
    KS_LFO_WAVE_SQUARE,
    KS_LFO_WAVE_SIN,

    KS_LFO_NUM_WAVES,
}ks_synth_lfo_wave_t;


typedef struct ks_phase_coarse_t
{
    u8 fixed_frequency: 1;
    u8 value :7;
}ks_phase_coarse_t;

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
    //! Frequency magnification of output.
    union {
        ks_phase_coarse_t       st                   [KS_NUM_OPERATORS];
        u8                      b                    [KS_NUM_OPERATORS];
    }phase_coarses;

    //! Fine of frequency. Max value is half octave.
    u8                     phase_fines               [KS_NUM_OPERATORS];

    //! Initial phase of wave. Every time note on event arises, phase is initialized.
    u8                     phase_dets                [KS_NUM_OPERATORS];

    //! Amplitude at envelope_times. i.e. Total Level and Sustain Level.
    u8                     envelope_points          [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];

    //! Time until becoming envelop_points. i.e. Attack Time and Sustain Time.
    u8                     envelope_times           [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];

    //! Release time. Time until amplitude becoming zero from note off.
    u8                     envelope_release_times   [KS_NUM_OPERATORS];


    //! Volume change degree by velocity.
    u8                     velocity_sens           [KS_NUM_OPERATORS];

    //! Envelope speed change degree by note number.
    u8                     ratescales              [KS_NUM_OPERATORS];

    //! Volume change degree by note number when note number is smaller than keyscale_mid_points.
    u8                     keyscale_low_depths     [KS_NUM_OPERATORS];

    //! Volume change degree by note number when note number is bigger than keyscale_mid_points.
    u8                     keyscale_high_depths    [KS_NUM_OPERATORS];

    //! Note number which is center of keyscale.
    u8                     keyscale_mid_points     [KS_NUM_OPERATORS];

    //! Kind of keyscale curve. Upper 4 bits are value for the notes which bellow keyscale_mid_points, and lower 4 bits are value for the notes which above keyscale_mid_points.
    union {
         ks_keyscale_curve_t    st                  [KS_NUM_OPERATORS];
         u8                     b                   [KS_NUM_OPERATORS];
    }keyscale_curve_types;

    //! Amplitude modulation sensitivity of LFO.
    u8                     lfo_ams_depths          [KS_NUM_OPERATORS]; // zero : disabled


    //! algorithm number, 0~7:FM 8~10:PCM
    u8                     algorithm;

    //! FM feed back degree.
    u8                     feedback_level;

    //! instrumental panpot
    u8                     panpot;

    //! Wave type of LFO.
    u8                     lfo_wave_type;

    //! Frequency of LFO.
    u8                     lfo_freq;

    //! Initial phase of LFO.
    u8                     lfo_det;
    
    //! Frequency modulation sensitivity of LFO.
    u8                     lfo_fms_depth;                                 // zero : disabled

}
ks_synth_data;


/**
 * @struct ks_synth_data
 * @brief Synthesizer data read for ease of calculation.
*/
typedef struct ks_synth
{
    u32         phase_coarses           [KS_NUM_OPERATORS];
    u32         phase_fines             [KS_NUM_OPERATORS];
    u32         phase_dets              [KS_NUM_OPERATORS];

    i32         envelope_points          [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    u32         envelope_samples         [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    u32         envelope_release_samples [KS_NUM_OPERATORS];

    u32         velocity_sens           [KS_NUM_OPERATORS];
    u32         lfo_ams_depths          [KS_NUM_OPERATORS];

    u32         ratescales              [KS_NUM_OPERATORS];
    u32         keyscale_low_depths     [KS_NUM_OPERATORS];
    u32         keyscale_high_depths    [KS_NUM_OPERATORS];
    u8          keyscale_mid_points     [KS_NUM_OPERATORS];
    u8          keyscale_curve_types    [2][KS_NUM_OPERATORS];

    u8          algorithm;
    u32         feedback_level;

    i16         panpot_left,            panpot_right;

    u32         lfo_det;
    u32         lfo_freq;
    u32         lfo_fms_depth;

    bool        fixed_frequency         [KS_NUM_OPERATORS];

    u8          lfo_wave_type;
    bool        lfo_ams_enabled;
    bool        lfo_fms_enabled;
}
ks_synth;

/**
 * @struct ks_synth_data
 * @brief Note state.
*/
typedef  struct ks_synth_note
{
    i32     output_log              [KS_NUM_OPERATORS];

    u32    phases                  [KS_NUM_OPERATORS];
    u32    phase_deltas            [KS_NUM_OPERATORS];

    i32     envelope_points          [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    u32    envelope_samples         [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    i32     envelope_deltas          [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    u32    envelope_release_samples [KS_NUM_OPERATORS];

    i32     envelope_now_deltas      [KS_NUM_OPERATORS];
    i32     envelope_now_times       [KS_NUM_OPERATORS];
    i32     envelope_now_amps        [KS_NUM_OPERATORS];
    u8     envelope_states          [KS_NUM_OPERATORS];
    u8     envelope_now_points      [KS_NUM_OPERATORS];

    u32    lfo_phase;
    u32    lfo_delta;
    i32     lfo_log;
    i32     feedback_log;

    u32    now_frame;
}
ks_synth_note;



/**
 * @brief ks_io_synth_data
 * @param io
 * @param funcs
 * @param data
 * @param offset
 * @param serialize
 * @return If success, true otherwise false
 */
ks_io_decl_custom_func(ks_synth_data);

/**
 * @brief ks_synth_new Allocate synth data from data
 * @param data
 * @param sampling_rate Sampling rate
 * @return Allocated synth data
 */
ks_synth*                    ks_synth_new                     (ks_synth_data* data, u32 sampling_rate);

/**
 * @brief ks_synth_array_new Allocate synth data array from data array
 * @param length Length of data
 * @param data Binary data of array
 * @param sampling_rate Sampling rate
 * @return Allocated synth data array
 */
ks_synth*                    ks_synth_array_new               (u32 length, ks_synth_data data[], u32 sampling_rate);

/**
 * @brief ks_synth_free
 * @param synth Synth to free
 */
void                        ks_synth_free                    (ks_synth* synth);

/**
 * @fn ks_synth_data_set_default
 * @brief Data initialize to default
 * @param data Initializing data
*/
void                        ks_synth_data_set_default      (ks_synth_data* data);

/**
 * @fn ks_synth_set
 * @brief Read data from data.
 * @param synth Dist data
 * @param sampling_rate Sampling rate
 * @param data Source data
*/
void                        ks_synth_set                    (ks_synth* synth, u32 sampling_rate, const ks_synth_data* data);

/**
 * @fn ks_synth_render
 * @brief Synthesize note of synth to buf.
 * @param synth Tone data of synthesizer
 * @param note Current note state
 * @param buf Write buffer
 * @param len Length of buffer
*/
void                        ks_synth_render                 (const ks_synth *synth, ks_synth_note* note, u32 pitchbend, i16 *buf, u32 len);

/**
 * @fn ks_synth_note_on
 * @brief Note on tone with note number and velocity.
 * @param note A note noteon
 * @param synth Tone data of synthesizer
 * @param sampling_rate Sampling rate
 * @param notenum Note number of note
 * @param velocity Velociry of note
*/
void                        ks_synth_note_on                 (ks_synth_note* note, const ks_synth *synth, u32 sampling_rate,  u8 notenum, u8 velocity);

/**
 * @fn ks_synth_note_off
 * @brief Note off tone
 * @param note A note noteoff
*/
void                        ks_synth_note_off                (ks_synth_note* note);

/**
 * @brief ks_synth_note_is_enabled
 * @param note Checked note which is enabled or not
 * @return
 */
bool                        ks_synth_note_is_enabled        (const ks_synth_note* note);

u32 ks_exp_u(u8 val, u32 base, int num_v_bit);

u32 ks_calc_envelope_times(u32 val);

u32 ks_calc_envelope_samples(u32 smp_freq, u8 val);
// min <= val < max
i64 ks_linear(u8 val, i32 MIN, i32 MAX);
// min <= val <= max
i64 ks_linear2(u8 val, i32 MIN, i32 MAX);
u32 ks_fms_depth(i32 depth);

void ks_calc_panpot(i16* left, i16* right, u8 val);
i16 ks_apply_panpot(i16 in, i16 pan);

#define ks_linear_i (i32)ks_linear
#define ks_linear_u (u32)ks_linear
#define ks_linear2_i (i32)ks_linear2
#define ks_linear2_u (u32)ks_linear2

#define calc_fixed_frequency(value)                     (value)
#define calc_phase_coarses(value)                       (value)
#define calc_phase_fines(value)                         ks_linear_u(value, 0, 1 << (KS_PHASE_FINE_BITS))
#define calc_phase_dets(value)                          ks_linear_u(value, 0, ks_1(KS_PHASE_MAX_BITS))
#define calc_envelope_points(value)                      ks_linear2_i(value, 0, 1 << KS_ENVELOPE_BITS)
#define calc_envelope_samples(smp_freq, value)           ks_calc_envelope_samples(smp_freq, value)
#define calc_envelope_release_samples(smp_freq, value)   calc_envelope_samples(smp_ferq,value)
#define calc_velocity_sens(value)                       ks_linear2_u(value, 0, 1 << KS_VELOCITY_SENS_BITS)
#define calc_ratescales(value)                          ks_linear2_u(value, 0, 1 << KS_RATESCALE_BITS)
#define calc_keyscale_low_depths(value)                 ks_linear2_u(value, 0, 1 << KS_KEYSCALE_DEPTH_BITS)
#define calc_keyscale_high_depths(value)                ks_linear2_u(value, 0, 1 << KS_KEYSCALE_DEPTH_BITS)
#define calc_keyscale_mid_points(value)                 (value & 0x7f)
#define calc_keyscale_curve_types_left(value)           (value)
#define calc_keyscale_curve_types_right(value)          (value)
#define calc_lfo_ams_depths(value)                      ks_linear2_u(value, 0, 1 << KS_LFO_DEPTH_BITS)
#define calc_algorithm(value)                           (value)
#define calc_feedback_level(value)                      ks_linear_u(value, 0, 2<<KS_FEEDBACK_LEVEL_BITS)
#define calc_panpot(value)                              (value)
#define calc_lfo_wave_type(value)                      (value)
#define calc_lfo_fms_depth(value)                       ks_exp_u(value, 1 << (KS_LFO_DEPTH_BITS-12), 4)
#define calc_lfo_freq(value)                            ks_exp_u(value, 1<<(KS_FREQUENCY_BITS-5), 4)
#define calc_lfo_det(value)                             ks_linear_u(value, 0, ks_1(KS_PHASE_MAX_BITS))

