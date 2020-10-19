/**
 * @file krsyn.h
 * @brief Synthesizer core
*/
#ifndef KRSYN_H
#define KRSYN_H

//#include <xmmintrin.h> perhaps, my implementation is so bad, so I have compiler vectorize automatically. It is faster.
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#ifndef __cplusplus
#include <stdbool.h>
#else
extern "C" {
#endif

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define ks_v(a, x)                          ((a) << (x))
#define ks_1(x)                             ks_v(1, x)
#define ks_m(x)                             (ks_1(x) - 1)
#define ks_mask(a, x)                       ((a) & ks_m(x))

#define KRSYN_TABLE_BITS                   10u
#define KRSYN_OUTPUT_BITS                  15u
#define KRSYN_PHASE_BITS                   16u
#define KRSYN_PHASE_MAX_BITS               (KRSYN_PHASE_BITS + KRSYN_TABLE_BITS)

#define KRSYN_NOISE_PHASE_BITS             (KRSYN_PHASE_MAX_BITS - 5)

#define KRSYN_FREQUENCY_BITS               16u

#define KRSYN_PHASE_COARSE_BITS            1u
#define KRSYN_PHASE_FINE_BITS              (KRSYN_FREQUENCY_BITS - 1u)

#define KRSYN_NUM_OPERATORS                 4u

#define KRSYN_ENVELOPE_BITS                 30u
#define KRSYN_ENVELOPE_NUM_POINTS            4u

#define KRSYN_RS_BITS                      16u
#define KRSYN_VELOCITY_SENS_BITS           16u

#define KRSYN_KS_CURVE_TABLE_BITS          7u
#define KRSYN_KS_CURVE_BITS                16u
#define KRSYN_KS_CURVE_MAX_BITS            (KRSYN_KS_CURVE_BITS + KRSYN_KS_CURVE_TABLE_BITS)
#define KRSYN_KS_DEPTH_BITS                16u

#define KRSYN_LFO_DEPTH_BITS               16u
#define KRSYN_FEEDBACK_LEVEL_BITS          16u

#define KRSYN_SAMPLE_PER_FRAMES_BITS       5u

#define KRSYN_DATA_SIZE                     sizeof(krsyn_binary)


/**
 * @enum krsyn_keyscale_t
 * @brief Key scale factor curve
 * ks_mid_point as center, volume is changed by key by key.
 * In LD and LU, volume changes linearly with frequency.
 * In ED and EU, volume changes exponentially.
*/
enum krsyn_keyscale_t
{
    KRSYN_KS_CURVE_LD,
    KRSYN_KS_CURVE_ED,
    KRSYN_KS_CURVE_LU,
    KRSYN_KS_CURVE_EU,
    KRSYN_KS_CURVE_NUM_TYPES,
};

/**
 * @enum krsyn_envelope_state
 * @brief Current envelope state
*/
typedef enum krsyn_envelope_state
{
    KRSYN_ENVELOPE_OFF = 0,
    KRSYN_ENVELOPE_ON,
    KRSYN_ENVELOPE_SUSTAINED,
    KRSYN_ENVELOPE_RELEASED,
}krsyn_envelope_state;

/**
 * @enum krsynLFOWaveType
 * @brief LFO wave type
*/
typedef enum krsyn_lfo_wave_t
{
    KRSYN_LFO_WAVE_TRIANGLE,
    KRSYN_LFO_WAVE_SAW_UP,
    KRSYN_LFO_WAVE_SAW_DOWN,
    KRSYN_LFO_WAVE_SQUARE,
    KRSYN_LFO_WAVE_SIN,

    KRSYN_LFO_NUM_WAVES,
}krsyn_lfo_wave_t;


typedef struct krsyn_phase_coarse
{
    uint8_t fixed_frequency: 1;
    uint8_t value :7;
}krsyn_phase_coarse;

typedef struct krsyn_keyscale_curve_t
{
    uint8_t left : 4;
    uint8_t right : 4;
}krsyn_keyscale_curve_t;

/**
 * @struct krsyn_binary
 * @brief Binary data of the synthesizer.
*/
typedef struct krsyn_binary
{
    //! Frequency magnification of output.
    krsyn_phase_coarse          phase_coarses           [KRSYN_NUM_OPERATORS];

    //! Detune of frequency. Max value is half octave.
    uint8_t                     phase_fines             [KRSYN_NUM_OPERATORS];

    //! Initial phase of wave. Every time note on event arises, phase is initialized.
    uint8_t                     phase_dets              [KRSYN_NUM_OPERATORS];

    //! Amplitude at envelope_times. i.e. Total Level and Sustain Level.
    uint8_t                     envelope_points          [KRSYN_ENVELOPE_NUM_POINTS][KRSYN_NUM_OPERATORS];

    //! Time until becoming envelop_points. i.e. Attack Time and Sustain Time.
    uint8_t                     envelope_times           [KRSYN_ENVELOPE_NUM_POINTS][KRSYN_NUM_OPERATORS];

    //! Release time. Time until amplitude becoming zero from note off.
    uint8_t                     envelope_release_times   [KRSYN_NUM_OPERATORS];


    //! Volume change degree by velocity.
    uint8_t                     velocity_sens           [KRSYN_NUM_OPERATORS];

    //! Envelope speed change degree by note number.
    uint8_t                     ratescales              [KRSYN_NUM_OPERATORS];

    //! Volume change degree by note number when note number is smaller than ks_mid_points.
    uint8_t                     ks_low_depths           [KRSYN_NUM_OPERATORS];

    //! Volume change degree by note number when note number is bigger than ks_mid_points.
    uint8_t                     ks_high_depths          [KRSYN_NUM_OPERATORS];

    //! Note number which is center of keyscale.
    uint8_t                     ks_mid_points           [KRSYN_NUM_OPERATORS];

    //! Kind of keyscale curve. Upper 4 bits are value for the notes which bellow ks_mid_points, and lower 4 bits are value for the notes which above ks_mid_points.
    krsyn_keyscale_curve_t        ks_curve_types          [KRSYN_NUM_OPERATORS];
    

    //! Amplitude modulation sensitivity of LFO.
    uint8_t                     lfo_ams_depths          [KRSYN_NUM_OPERATORS]; // zero : disabled


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
krsyn_binary;


/**
 * @struct krsyn_binary
 * @brief Synthesizer data read for ease of calculation.
*/
typedef struct krsyn
{
    uint32_t    phase_coarses           [KRSYN_NUM_OPERATORS];
    uint32_t    phase_fines             [KRSYN_NUM_OPERATORS];
    uint32_t    phase_dets              [KRSYN_NUM_OPERATORS];

    int32_t     envelope_points          [KRSYN_ENVELOPE_NUM_POINTS][KRSYN_NUM_OPERATORS];
    uint32_t    envelope_samples         [KRSYN_ENVELOPE_NUM_POINTS][KRSYN_NUM_OPERATORS];
    uint32_t    envelope_release_samples [KRSYN_NUM_OPERATORS];

    uint32_t    velocity_sens           [KRSYN_NUM_OPERATORS];
    uint32_t    lfo_ams_depths           [KRSYN_NUM_OPERATORS];

    uint32_t    ratescales              [KRSYN_NUM_OPERATORS];
    uint32_t    ks_low_depths           [KRSYN_NUM_OPERATORS];
    uint32_t    ks_high_depths          [KRSYN_NUM_OPERATORS];
    uint8_t     ks_mid_points           [KRSYN_NUM_OPERATORS];
    uint8_t     ks_curve_types          [2][KRSYN_NUM_OPERATORS];

    uint8_t     algorithm;
    uint32_t    feedback_level;

    uint32_t    lfo_det;
    uint32_t    lfo_freq;
    uint32_t    lfo_fms_depth;

    bool        fixed_frequency         [KRSYN_NUM_OPERATORS];

    uint8_t     lfo_wave_type;
    bool        lfo_ams_enabled;
    bool        lfo_fms_enabled;
}
krsyn;

/**
 * @struct krsyn_binary
 * @brief Note state.
*/
typedef  struct krsyn_note
{
    int32_t     output_log              [KRSYN_NUM_OPERATORS];

    uint32_t    phases                  [KRSYN_NUM_OPERATORS];
    uint32_t    phase_deltas            [KRSYN_NUM_OPERATORS];

    int32_t     envelope_points          [KRSYN_ENVELOPE_NUM_POINTS][KRSYN_NUM_OPERATORS];
    uint32_t    envelope_samples         [KRSYN_ENVELOPE_NUM_POINTS][KRSYN_NUM_OPERATORS];
    int32_t     envelope_deltas          [KRSYN_ENVELOPE_NUM_POINTS][KRSYN_NUM_OPERATORS];
    uint32_t    envelope_release_samples [KRSYN_NUM_OPERATORS];

    int32_t     envelope_now_deltas      [KRSYN_NUM_OPERATORS];
    int32_t     envelope_now_times       [KRSYN_NUM_OPERATORS];
    int32_t     envelope_now_amps        [KRSYN_NUM_OPERATORS];
    uint8_t     envelope_states          [KRSYN_NUM_OPERATORS];
    uint8_t     envelope_now_points      [KRSYN_NUM_OPERATORS];

    uint32_t    lfo_phase;
    uint32_t    lfo_delta;
    int32_t     lfo_log;
    int32_t     feedback_log;

    uint32_t    now_frame;
}
krsyn_note;


/**
 * @fn krsyn_binary_set_default
 * @brief FM data initialize to default
 * @param data Initializing data
*/
void                        krsyn_binary_set_default       (krsyn_binary* data);

/**
 * @fn krsyn_set
 * @brief Read FM data from binary.
 * @param fm Dist data
 * @param sampling_rate Sampling rate
 * @param data Source data
*/
void                        krsyn_set                    (krsyn* fm, uint32_t sampling_rate, const krsyn_binary* data);

/**
 * @fn krsyn_render
 * @brief Synthesize note of fm to buf.
 * @param fm Tone data of synthesizer
 * @param note Current note state
 * @param buf Write buffer
 * @param len Length of buffer
*/
void                        krsyn_render                 (const krsyn *fm, krsyn_note* note, int16_t* buf, unsigned len);

/**
 * @fn krsyn_note_on
 * @brief Note on FM tone with note number and velocity.
 * @param note A note noteon
 * @param fm Tone data of synthesizer
 * @param sampling_rate Sampling rate
 * @param notenum Note number of note
 * @param velocity Velociry of note
*/
void                        krsyn_note_on                 (krsyn_note* note, const krsyn *fm, uint32_t sampling_rate,  uint8_t notenum, uint8_t velocity);

/**
 * @fn krsyn_note_off
 * @brief Note off FM tone
 * @param note A note noteoff
*/
void                        krsyn_note_off                (krsyn_note* note);




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

#define krsyn_linear_i (int32_t)krsyn_linear
#define krsyn_linear_u (uint32_t)krsyn_linear
#define krsyn_linear2_i (int32_t)krsyn_linear2
#define krsyn_linear2_u (uint32_t)krsyn_linear2

#define calc_fixed_frequency(value)                     (value)
#define calc_phase_coarses(value)                       (value)
#define calc_phase_fines(value)                         krsyn_linear_u(value, 0, 1 << (KRSYN_PHASE_FINE_BITS))
#define calc_phase_dets(value)                          krsyn_linear_u(value, 0, ks_1(KRSYN_PHASE_MAX_BITS))
#define calc_envelope_points(value)                      krsyn_linear2_i(value, 0, 1 << KRSYN_ENVELOPE_BITS)
#define calc_envelope_samples(smp_freq, value)           krsyn_calc_envelope_samples(smp_freq, value)
#define calc_envelope_release_samples(smp_freq, value)   calc_envelope_samples(smp_ferq,value)
#define calc_velocity_sens(value)                       krsyn_linear2_u(data->velocity_sens[i], 0, 1 << KRSYN_VELOCITY_SENS_BITS)
#define calc_ratescales(value)                          krsyn_linear2_u(value, 0, 1 << KRSYN_RS_BITS)
#define calc_ks_low_depths(value)                       krsyn_linear2_u(value, 0, 1 << KRSYN_KS_DEPTH_BITS);
#define calc_ks_high_depths(value)                      krsyn_linear2_u(value, 0, 1 << KRSYN_KS_DEPTH_BITS)
#define calc_ks_mid_points(value)                       (value & 0x7f)
#define calc_ks_curve_types_left(value)                 (value & 0x0f)
#define calc_ks_curve_types_right(value)                (value >> 4)
#define calc_lfo_ams_depths(value)                      krsyn_linear2_u(value, 0, 1 << KRSYN_LFO_DEPTH_BITS)
#define calc_algorithm(value)                           (value)
#define calc_feedback_level(value)                      krsyn_linear_u(value, 0, 2<<KRSYN_FEEDBACK_LEVEL_BITS)
#define calc_lfo_wave_type(value)                      (value)
#define calc_lfo_fms_depth(value)                       krsyn_exp_u(value, 1 << (KRSYN_LFO_DEPTH_BITS-12), 4)
#define calc_lfo_freq(value)                            krsyn_exp_u(value, 1<<(KRSYN_FREQUENCY_BITS-5), 4)
#define calc_lfo_det(value)                             krsyn_linear_u(value, 0, ks_1(KRSYN_PHASE_MAX_BITS))

#ifdef __cplusplus
} // extern "C"
#endif


#endif // KRSYN_H
