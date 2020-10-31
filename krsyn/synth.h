/**
 * @file ks_synth.h
 * @brief Synthesizer core
*/
#pragma once

//#include <xmmintrin.h> perhaps, my implementation is so bad, so I have compiler vectorize automatically. It is faster.
#include <stdint.h>
#include <stdbool.h>
#include "./math.h"

#define KS_FREQUENCY_BITS               16u

#define KS_PHASE_COARSE_BITS            1u
#define KS_PHASE_FINE_BITS              (KS_FREQUENCY_BITS - 1u)

#define KS_NUM_OPERATORS                 4u

#define KS_ENVELOPE_BITS                 30u
#define KS_ENVELOPE_NUM_POINTS            4u

#define KS_RS_BITS                      16u
#define KS_VELOCITY_SENS_BITS           16u

#define KS_KEYSCALE_CURVE_TABLE_BITS    7u
#define KS_KEYSCALE_CURVE_BITS          16u
#define KS_KEYSCALE_CURVE_MAX_BITS      (KS_KEYSCALE_CURVE_BITS + KS_KEYSCALE_CURVE_TABLE_BITS)
#define KS_KEYSCALE_DEPTH_BITS          16u

#define KS_LFO_DEPTH_BITS               16u
#define KS_FEEDBACK_LEVEL_BITS          16u

#define KS_SAMPLE_PER_FRAMES_BITS       5u

#define KS_DATA_SIZE                     sizeof(ks_synth_binary)


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


typedef struct ks_phase_coarse
{
    uint8_t fixed_frequency: 1;
    uint8_t value :7;
}ks_synth_phase_coarse;

typedef struct ks_keyscale_curve_t
{
    uint8_t left : 4;
    uint8_t right : 4;
}ks_synth_keyscale_curve_t;

/**
 * @struct ks_synth_binary
 * @brief Binary data of the synthesizer.
*/
typedef struct ks_synth_binary
{
    //! Frequency magnification of output.
    ks_synth_phase_coarse          phase_coarses           [KS_NUM_OPERATORS];

    //! Detune of frequency. Max value is half octave.
    uint8_t                     phase_fines             [KS_NUM_OPERATORS];

    //! Initial phase of wave. Every time note on event arises, phase is initialized.
    uint8_t                     phase_dets              [KS_NUM_OPERATORS];

    //! Amplitude at envelope_times. i.e. Total Level and Sustain Level.
    uint8_t                     envelope_points          [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];

    //! Time until becoming envelop_points. i.e. Attack Time and Sustain Time.
    uint8_t                     envelope_times           [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];

    //! Release time. Time until amplitude becoming zero from note off.
    uint8_t                     envelope_release_times   [KS_NUM_OPERATORS];


    //! Volume change degree by velocity.
    uint8_t                     velocity_sens           [KS_NUM_OPERATORS];

    //! Envelope speed change degree by note number.
    uint8_t                     ratescales              [KS_NUM_OPERATORS];

    //! Volume change degree by note number when note number is smaller than keyscale_mid_points.
    uint8_t                     keyscale_low_depths          [KS_NUM_OPERATORS];

    //! Volume change degree by note number when note number is bigger than keyscale_mid_points.
    uint8_t                     keyscale_high_depths         [KS_NUM_OPERATORS];

    //! Note number which is center of keyscale.
    uint8_t                     keyscale_mid_points           [KS_NUM_OPERATORS];

    //! Kind of keyscale curve. Upper 4 bits are value for the notes which bellow keyscale_mid_points, and lower 4 bits are value for the notes which above keyscale_mid_points.
    ks_synth_keyscale_curve_t   keyscale_curve_types    [KS_NUM_OPERATORS];
    

    //! Amplitude modulation sensitivity of LFO.
    uint8_t                     lfo_ams_depths          [KS_NUM_OPERATORS]; // zero : disabled


    //! algorithm number, 0~7:FM 8~10:PCM
    uint8_t                     algorithm;

    //! FM feed back degree.
    uint8_t                     feedback_level;


    //! Wave type of LFO.
    uint8_t                     lfo_wave_type;

    //! Frequency of LFO.
    uint8_t                     lfo_freq;

    //! Initial phase of LFO.
    uint8_t                     lfo_det;
    
    //! Frequency modulation sensitivity of LFO.
    uint8_t                     lfo_fms_depth;                                 // zero : disabled

}
ks_synth_binary;


/**
 * @struct ks_synth_binary
 * @brief Synthesizer data read for ease of calculation.
*/
typedef struct ks_synth
{
    uint32_t    phase_coarses           [KS_NUM_OPERATORS];
    uint32_t    phase_fines             [KS_NUM_OPERATORS];
    uint32_t    phase_dets              [KS_NUM_OPERATORS];

    int32_t     envelope_points          [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    uint32_t    envelope_samples         [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    uint32_t    envelope_release_samples [KS_NUM_OPERATORS];

    uint32_t    velocity_sens           [KS_NUM_OPERATORS];
    uint32_t    lfo_ams_depths          [KS_NUM_OPERATORS];

    uint32_t    ratescales              [KS_NUM_OPERATORS];
    uint32_t    keyscale_low_depths     [KS_NUM_OPERATORS];
    uint32_t    keyscale_high_depths    [KS_NUM_OPERATORS];
    uint8_t     keyscale_mid_points     [KS_NUM_OPERATORS];
    uint8_t     keyscale_curve_types    [2][KS_NUM_OPERATORS];

    uint8_t     algorithm;
    uint32_t    feedback_level;

    uint32_t    lfo_det;
    uint32_t    lfo_freq;
    uint32_t    lfo_fms_depth;

    bool        fixed_frequency         [KS_NUM_OPERATORS];

    uint8_t     lfo_wave_type;
    bool        lfo_ams_enabled;
    bool        lfo_fms_enabled;
}
ks_synth;

/**
 * @struct ks_synth_binary
 * @brief Note state.
*/
typedef  struct ks_synth_note
{
    int32_t     output_log              [KS_NUM_OPERATORS];

    uint32_t    phases                  [KS_NUM_OPERATORS];
    uint32_t    phase_deltas            [KS_NUM_OPERATORS];

    int32_t     envelope_points          [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    uint32_t    envelope_samples         [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    int32_t     envelope_deltas          [KS_ENVELOPE_NUM_POINTS][KS_NUM_OPERATORS];
    uint32_t    envelope_release_samples [KS_NUM_OPERATORS];

    int32_t     envelope_now_deltas      [KS_NUM_OPERATORS];
    int32_t     envelope_now_times       [KS_NUM_OPERATORS];
    int32_t     envelope_now_amps        [KS_NUM_OPERATORS];
    uint8_t     envelope_states          [KS_NUM_OPERATORS];
    uint8_t     envelope_now_points      [KS_NUM_OPERATORS];

    uint32_t    lfo_phase;
    uint32_t    lfo_delta;
    int32_t     lfo_log;
    int32_t     feedback_log;

    uint32_t    now_frame;
}
ks_synth_note;

/**
 * @brief ks_synth_new Allocate synth data from binary
 * @param data
 * @param sampling_rate Sampling rate
 * @return Allocated synth data
 */
ks_synth*                    ks_synth_new                     (ks_synth_binary* data, uint32_t sampling_rate);

/**
 * @brief ks_synth_array_new Allocate synth data array from binary array
 * @param length Length of data
 * @param data Binary data of array
 * @param sampling_rate Sampling rate
 * @return Allocated synth data array
 */
ks_synth*                    ks_synth_array_new               (uint32_t length, ks_synth_binary data[length], uint32_t sampling_rate);

/**
 * @brief ks_synth_free
 * @param synth Synth to free
 */
void                        ks_synth_free                    (ks_synth* synth);

/**
 * @fn ks_synth_binary_set_default
 * @brief Data initialize to default
 * @param data Initializing data
*/
void                        ks_synth_binary_set_default      (ks_synth_binary* data);

/**
 * @fn ks_synth_set
 * @brief Read data from binary.
 * @param synth Dist data
 * @param sampling_rate Sampling rate
 * @param data Source data
*/
void                        ks_synth_set                    (ks_synth* synth, uint32_t sampling_rate, const ks_synth_binary* data);

/**
 * @fn ks_synth_render
 * @brief Synthesize note of synth to buf.
 * @param synth Tone data of synthesizer
 * @param note Current note state
 * @param buf Write buffer
 * @param len Length of buffer
*/
void                        ks_synth_render                 (const ks_synth *synth, ks_synth_note* note, uint32_t pitchbend, int16_t *buf, unsigned len);

/**
 * @fn ks_synth_note_on
 * @brief Note on tone with note number and velocity.
 * @param note A note noteon
 * @param synth Tone data of synthesizer
 * @param sampling_rate Sampling rate
 * @param notenum Note number of note
 * @param velocity Velociry of note
*/
void                        ks_synth_note_on                 (ks_synth_note* note, const ks_synth *synth, uint32_t sampling_rate,  uint8_t notenum, uint8_t velocity);

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
inline static bool          ks_synth_note_is_enabled         (const ks_synth_note* note){
    return *((uint32_t*)note->envelope_states) != 0;
}


static inline uint32_t krsyn_exp_u(uint8_t val, uint32_t base, int num_v_bit)
{
    // ->[e]*(8-num_v_bit) [v]*(num_v_bit)
    // base * 1.(v) * 2^(e)
    int8_t v = val & ((1 << num_v_bit) - 1);
    v |= ((val== 0) ? 0 : 1) << num_v_bit;    // add 1
    int8_t e = val >> num_v_bit;

    uint64_t ret = (uint64_t)base * v;
    ret <<= e;
    ret >>= (8 - num_v_bit - 1);
    ret >>= num_v_bit;
    return ret;
}

static inline uint32_t krsyn_calc_envelope_times(uint32_t val)
{
    return krsyn_exp_u(val, 1<<(16-8), 4);// 2^8 <= x < 2^16
}

static inline uint32_t krsyn_calc_envelope_samples(uint32_t smp_freq, uint8_t val)
{
    uint32_t time = krsyn_calc_envelope_times(val);
    uint64_t samples = time;
    samples *= smp_freq;
    samples >>= 16;
    samples = MAX(1u, samples);  // note : maybe dont need this line
    return (uint32_t)samples;
}

// min <= val < max
static inline int64_t krsyn_linear(uint8_t val, int32_t MIN, int32_t MAX)
{
    int32_t range = MAX - MIN;
    int64_t ret = val;
    ret *= range;
    ret >>= 8;
    ret += MIN;

    return ret;
}

// min <= val <= max
static inline int64_t krsyn_linear2(uint8_t val, int32_t MIN, int32_t MAX)
{
    return krsyn_linear(val, MIN, MAX + MAX/256);
}

static inline uint32_t krsyn_fms_depth(int32_t depth){
    int64_t ret = ((int64_t)depth*depth)>>(KS_LFO_DEPTH_BITS + 2);
    ret += (depth >> 2) + (depth >> 1);
    ret += ks_1(KS_LFO_DEPTH_BITS);
    return ret;
}


#define krsyn_linear_i (int32_t)krsyn_linear
#define krsyn_linear_u (uint32_t)krsyn_linear
#define krsyn_linear2_i (int32_t)krsyn_linear2
#define krsyn_linear2_u (uint32_t)krsyn_linear2

#define calc_fixed_frequency(value)                     (value)
#define calc_phase_coarses(value)                       (value)
#define calc_phase_fines(value)                         krsyn_linear_u(value, 0, 1 << (KS_PHASE_FINE_BITS))
#define calc_phase_dets(value)                          krsyn_linear_u(value, 0, ks_1(KS_PHASE_MAX_BITS))
#define calc_envelope_points(value)                      krsyn_linear2_i(value, 0, 1 << KS_ENVELOPE_BITS)
#define calc_envelope_samples(smp_freq, value)           krsyn_calc_envelope_samples(smp_freq, value)
#define calc_envelope_release_samples(smp_freq, value)   calc_envelope_samples(smp_ferq,value)
#define calc_velocity_sens(value)                       krsyn_linear2_u(value, 0, 1 << KS_VELOCITY_SENS_BITS)
#define calc_ratescales(value)                          krsyn_linear2_u(value, 0, 1 << KS_RS_BITS)
#define calc_keyscale_low_depths(value)                 krsyn_linear2_u(value, 0, 1 << KS_KEYSCALE_DEPTH_BITS)
#define calc_keyscale_high_depths(value)                krsyn_linear2_u(value, 0, 1 << KS_KEYSCALE_DEPTH_BITS)
#define calc_keyscale_mid_points(value)                 (value & 0x7f)
#define calc_keyscale_curve_types_left(value)           (value & 0x0f)
#define calc_keyscale_curve_types_right(value)          (value >> 4)
#define calc_lfo_ams_depths(value)                      krsyn_linear2_u(value, 0, 1 << KS_LFO_DEPTH_BITS)
#define calc_algorithm(value)                           (value)
#define calc_feedback_level(value)                      krsyn_linear_u(value, 0, 2<<KS_FEEDBACK_LEVEL_BITS)
#define calc_lfo_wave_type(value)                      (value)
#define calc_lfo_fms_depth(value)                       krsyn_exp_u(value, 1 << (KS_LFO_DEPTH_BITS-12), 4)
#define calc_lfo_freq(value)                            krsyn_exp_u(value, 1<<(KS_FREQUENCY_BITS-5), 4)
#define calc_lfo_det(value)                             krsyn_linear_u(value, 0, ks_1(KS_PHASE_MAX_BITS))

