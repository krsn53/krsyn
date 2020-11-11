#include "synth.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>


ks_io_begin_custom_func(ks_phase_coarse_t)
    ks_fixed_props_len( 1, ks_prop_u8(u8) )
ks_io_end_custom_func

ks_io_begin_custom_func(ks_keyscale_curve_t)
    ks_fixed_props_len( 1, ks_prop_u8(u8) )
ks_io_end_custom_func

ks_io_begin_custom_func(ks_synth_binary)
    ks_magic_number("KSYN")
    ks_fixed_props(
        ks_prop_arr_obj(phase_coarses, ks_phase_coarse_t),
        ks_prop_arr_u8(phase_fines),
        ks_prop_arr_u8(phase_dets),

        ks_prop_arr_u8(envelope_points[0]),
        ks_prop_arr_u8(envelope_points[1]),
        ks_prop_arr_u8(envelope_points[2]),
        ks_prop_arr_u8(envelope_points[3]),

        ks_prop_arr_u8(envelope_times[0]),
        ks_prop_arr_u8(envelope_times[1]),
        ks_prop_arr_u8(envelope_times[2]),
        ks_prop_arr_u8(envelope_times[3]),

        ks_prop_arr_u8(envelope_release_times),
        ks_prop_arr_u8(velocity_sens),
        ks_prop_arr_u8(ratescales),
        ks_prop_arr_u8(keyscale_low_depths),
        ks_prop_arr_u8(keyscale_high_depths),
        ks_prop_arr_u8(keyscale_mid_points),
        ks_prop_arr_obj(keyscale_curve_types, ks_keyscale_curve_t),
        ks_prop_arr_u8(lfo_ams_depths),

        ks_prop_u8(algorithm),
        ks_prop_u8(feedback_level),
        ks_prop_u8(lfo_wave_type),
        ks_prop_u8(lfo_freq),
        ks_prop_u8(lfo_det),
        ks_prop_u8(lfo_fms_depth)
    )
ks_io_end_custom_func


ks_synth* ks_synth_new(ks_synth_binary* data, uint32_t sampling_rate){
    ks_synth* ret = malloc(sizeof(ks_synth));
    ks_synth_set(ret, sampling_rate, data);
    return ret;
}

ks_synth* ks_synth_array_new(uint32_t length, ks_synth_binary data[], uint32_t sampling_rate){
    ks_synth* ret = malloc(sizeof(ks_synth)*length);
    for(uint32_t i=0; i<length; i++){
        ks_synth_set(ret+i, sampling_rate, data+i);
    }
    return ret;
}

void ks_synth_free(ks_synth* synth){
    free(synth);
}

void ks_synth_binary_set_default(ks_synth_binary* data)
{
    *data = (ks_synth_binary){ 0 };
    for(uint32_t i=0; i<KS_NUM_OPERATORS; i++)
    {
        data->phase_coarses[i].str.fixed_frequency = false;
        data->phase_coarses[i].str.value = 2;
        data->phase_fines[i] = 0;
        data->phase_dets[i] = 0;

        data->envelope_times[0][i] = 0;
        data->envelope_times[1][i] = UINT8_MAX;
        data->envelope_times[2][i] = UINT8_MAX;
        data->envelope_times[3][i] = UINT8_MAX;

        data->envelope_points[0][i] = UINT8_MAX;
        data->envelope_points[1][i] = UINT8_MAX;
        data->envelope_points[2][i] = UINT8_MAX;
        data->envelope_points[3][i] = UINT8_MAX;

        data->envelope_release_times[i] = 128;

        data->velocity_sens[i] = UINT8_MAX;

        data->ratescales[i] = 32; // about 0.125
        data->keyscale_low_depths[i] = 0;
        data->keyscale_high_depths[i] = 0;
        data->keyscale_mid_points[i] = 69;
        data->keyscale_curve_types[i].str.left = 0;
        data->keyscale_curve_types[i].str.right = 0;

        data->lfo_ams_depths[i] = 0;
    }

    data->algorithm = 0;
    data->feedback_level = 0;

    data->lfo_wave_type = KS_LFO_WAVE_TRIANGLE;
    data->lfo_fms_depth = 0;
    data->lfo_freq = 128; // 1 Hz ?
    data->lfo_det = 0;
}

inline  bool  ks_synth_note_is_enabled     (const ks_synth_note* note){
    return *((uint32_t*)note->envelope_states) != 0;
}


inline uint32_t ks_exp_u(uint8_t val, uint32_t base, int num_v_bit)
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

inline uint32_t ks_calc_envelope_times(uint32_t val)
{
    return ks_exp_u(val, 1<<(16-8), 4);// 2^8 <= x < 2^16
}

inline uint32_t ks_calc_envelope_samples(uint32_t smp_freq, uint8_t val)
{
    uint32_t time = ks_calc_envelope_times(val);
    uint64_t samples = time;
    samples *= smp_freq;
    samples >>= 16;
    samples = MAX(1u, samples);  // note : maybe dont need this line
    return (uint32_t)samples;
}

// min <= val < max
inline int64_t ks_linear(uint8_t val, int32_t MIN, int32_t MAX)
{
    int32_t range = MAX - MIN;
    int64_t ret = val;
    ret *= range;
    ret >>= 8;
    ret += MIN;

    return ret;
}

// min <= val <= max
inline int64_t ks_linear2(uint8_t val, int32_t MIN, int32_t MAX)
{
    return ks_linear(val, MIN, MAX + MAX/256);
}


inline uint32_t ks_fms_depth(int32_t depth){
    int64_t ret = ((int64_t)depth*depth)>>(KS_LFO_DEPTH_BITS + 2);
    ret += (depth >> 2) + (depth >> 1);
    ret += ks_1(KS_LFO_DEPTH_BITS);
    return ret;
}

static inline void synth_op_set(uint32_t sampling_rate, ks_synth* synth, const ks_synth_binary* data)
{
    for(uint32_t i=0; i<KS_NUM_OPERATORS; i++)
    {
        synth->fixed_frequency[i] = calc_fixed_frequency(data->phase_coarses[i].str.fixed_frequency);
        synth->phase_coarses[i] = calc_phase_coarses(data->phase_coarses[i].str.value);

        synth->phase_fines[i] = calc_phase_fines(data->phase_fines[i]);
        synth->phase_dets[i] = calc_phase_dets(data->phase_dets[i]);

        for(uint32_t e=0; e < KS_ENVELOPE_NUM_POINTS; e++)
        {
            synth->envelope_points[e][i] = calc_envelope_points(data->envelope_points[e][i]);
            synth->envelope_samples[e][i] = calc_envelope_samples(sampling_rate, data->envelope_times[e][i]);
        }

        synth->envelope_release_samples[i] =  calc_envelope_samples(sampling_rate, data->envelope_release_times[i]);

        synth->velocity_sens[i] = calc_velocity_sens(data->velocity_sens[i]);

        synth->ratescales[i] = calc_ratescales(data->ratescales[i]);
        synth->keyscale_low_depths[i] = calc_keyscale_low_depths(data->keyscale_low_depths[i]);
        synth->keyscale_high_depths[i] = calc_keyscale_high_depths(data->keyscale_high_depths[i]);
        synth->keyscale_mid_points[i] = calc_keyscale_mid_points(data->keyscale_mid_points[i]);
        synth->keyscale_curve_types[0][i] = calc_keyscale_curve_types_left(data->keyscale_curve_types[i].str.left);
        synth->keyscale_curve_types[1][i] = calc_keyscale_curve_types_right(data->keyscale_curve_types[i].str.right);

        synth->lfo_ams_depths[i] = calc_lfo_ams_depths(data->lfo_ams_depths[i]);
    }
    synth->lfo_ams_enabled = *(const uint32_t*)data->lfo_ams_depths != 0;


}

static inline void synth_common_set(ks_synth* synth, const ks_synth_binary* data)
{
    synth->algorithm = calc_algorithm(data->algorithm);
    synth->feedback_level = calc_feedback_level(data->feedback_level);

    synth->lfo_wave_type = calc_lfo_wave_type(data->lfo_wave_type);
    synth->lfo_fms_depth = calc_lfo_fms_depth(data->lfo_fms_depth);
    synth->lfo_fms_enabled = data->lfo_fms_depth != 0;
    synth->lfo_freq = calc_lfo_freq(data->lfo_freq);
    synth->lfo_det = ks_linear_u(data->lfo_det, 0, ks_1(KS_PHASE_MAX_BITS));
}


void ks_synth_set(ks_synth* synth, uint32_t sampling_rate, const ks_synth_binary* data)
{
    synth_op_set(sampling_rate, synth, data);
    synth_common_set(synth, data);
}


static inline uint32_t phase_lfo_delta(uint32_t sampling_rate, uint32_t freq)
{
    uint64_t freq_11 = ((uint64_t)freq) << KS_TABLE_BITS;
    uint64_t delta_11 = (uint32_t)(freq_11 / sampling_rate);

    return (uint32_t)delta_11;
}

static inline uint32_t phase_delta_fix_freq(uint32_t sampling_rate, uint32_t coarse, uint32_t fine)
{
    uint32_t freq = ks_notefreq(coarse);
    uint64_t freq_11 = ((uint64_t)freq) << KS_TABLE_BITS;
    // 5461 ~ 2^16 * 1/12
    uint32_t freq_rate = (1<<KS_FREQUENCY_BITS) + ((5461 * fine) >> KS_PHASE_FINE_BITS);

    freq_11 *= freq_rate;
    freq_11 >>= KS_FREQUENCY_BITS;

    uint32_t delta_11 = (uint32_t)(freq_11 / sampling_rate);
    return delta_11;
}

static inline uint32_t phase_delta(uint32_t sampling_rate, uint8_t notenum, uint32_t coarse, uint32_t fine)
{
    uint32_t freq = ks_notefreq(notenum);
    uint64_t freq_11 = ((uint64_t)freq) << KS_TABLE_BITS;

    uint32_t freq_rate = (coarse << (KS_FREQUENCY_BITS - KS_PHASE_COARSE_BITS)) + fine;
    uint64_t delta_11 = (uint32_t)(freq_11 / sampling_rate);
    delta_11 *= freq_rate;
    delta_11 >>= KS_FREQUENCY_BITS;

    return (uint32_t)delta_11;
}

static inline uint32_t keyscale_li(uint32_t index_16, int curve_type)
{
    uint32_t index_m = (uint32_t)(index_16>> KS_KEYSCALE_CURVE_BITS);

    if(index_m >= ks_1(KS_KEYSCALE_CURVE_TABLE_BITS)-1)
    {
        return ks_keyscale_curves(curve_type, ks_1(KS_KEYSCALE_CURVE_TABLE_BITS)-1);
    }

    uint32_t index_b = index_m+1;

    uint32_t under_fixed_b = index_16 & ((1<<KS_KEYSCALE_CURVE_BITS)-1);
    uint32_t under_fixed_m = (1<<KS_KEYSCALE_CURVE_BITS) - under_fixed_b;

    uint32_t ret = ks_keyscale_curves(curve_type, index_m) * under_fixed_m +
        ks_keyscale_curves(curve_type, index_b) * under_fixed_b;

    ret >>= KS_KEYSCALE_CURVE_BITS;

    return ret;
}

static inline uint32_t keyscale_value(const ks_synth *synth, uint32_t table_index, uint32_t table_range, bool low_note, uint32_t i)
{
    uint64_t index_16 = table_index;
    index_16 <<= KS_KEYSCALE_CURVE_MAX_BITS;
    index_16 /= table_range;
    index_16 *= (low_note ? synth->keyscale_low_depths[i] : synth->keyscale_high_depths[i]);
    index_16 >>= KS_KEYSCALE_DEPTH_BITS;

    int curve_type = synth->keyscale_curve_types[low_note ? 0 : 1][i];
    return keyscale_li((uint32_t)index_16, curve_type);
}

static inline uint32_t keyscale(const ks_synth *synth, uint8_t notenum, uint32_t i)
{
    if(notenum < synth->keyscale_mid_points[i])
    {
        if(synth->keyscale_curve_types[0][i] <= KS_KEYSCALE_CURVE_ED || synth->keyscale_curve_types[1][i] > KS_KEYSCALE_CURVE_ED)
        {
            return keyscale_value(synth, synth->keyscale_mid_points[i] - notenum - 1, synth->keyscale_mid_points[i], true, i);
        }
        else
        {
            return 1<< KS_KEYSCALE_CURVE_BITS;
        }
    }

    if(synth->keyscale_curve_types[1][i] <= KS_KEYSCALE_CURVE_ED || synth->keyscale_curve_types[0][i] > KS_KEYSCALE_CURVE_ED )
    {
        return keyscale_value(synth, notenum - synth->keyscale_mid_points[i], ks_1(KS_KEYSCALE_CURVE_TABLE_BITS) - synth->keyscale_mid_points[i], false, i);
    }
    return 1<< KS_KEYSCALE_CURVE_BITS;
}

void ks_synth_note_on( ks_synth_note* note, const ks_synth *synth, uint32_t sampling_rate, uint8_t notenum, uint8_t vel)
{
    note->now_frame = 0;
    note->feedback_log =0;

    note->lfo_log = 0;
    note->lfo_phase= synth->lfo_det;
    note->lfo_delta = phase_lfo_delta(sampling_rate, synth->lfo_freq);

    for(uint32_t i=0; i< KS_NUM_OPERATORS; i++)
    {
        //phase
        note->phases[i] = synth->phase_dets[i];
        if(synth->fixed_frequency[i])
        {
            note->phase_deltas[i] = phase_delta_fix_freq(sampling_rate, synth->phase_coarses[i], synth->phase_fines[i]);
        }
        else
        {
            note->phase_deltas[i] = phase_delta(sampling_rate, notenum, synth->phase_coarses[i], synth->phase_fines[i]);
        }

        note->output_log[i] = 0;

        //rate scale
        int64_t ratescales = synth->ratescales[i];
        ratescales *= ks_ratescale(notenum);
        ratescales >>= KS_RATESCALE_BITS;
        ratescales += 1<<KS_RATESCALE_BITS;

        //key scale
        uint64_t keysc = keyscale(synth, notenum, i);

        int64_t target;
        uint32_t velocity;

        //envelope
        for(uint32_t j=0; j < KS_ENVELOPE_NUM_POINTS; j++)
        {
            velocity = synth->velocity_sens[i];
            velocity *=  127-vel;
            velocity >>= 7;
            velocity = (1 << KS_VELOCITY_SENS_BITS) -  velocity;

            target = synth->envelope_points[j][i];
            target *= keysc;
            target >>= KS_KEYSCALE_CURVE_BITS;
            target *= velocity;
            target >>= KS_VELOCITY_SENS_BITS;

            note->envelope_points[j][i] = (int32_t)target;

            uint64_t frame = (synth->envelope_samples[j][i]);
            frame *= ratescales;
            frame >>= KS_RATESCALE_BITS;
            frame >>= KS_SAMPLE_PER_FRAMES_BITS;
            note->envelope_samples[j][i] = MAX((uint32_t)frame, 1u);
        }

        uint64_t frame = (synth->envelope_release_samples[i]);
        frame *= ratescales;
        frame >>= KS_RATESCALE_BITS;
        frame >>= KS_SAMPLE_PER_FRAMES_BITS;
        note->envelope_release_samples[i] = MAX((uint32_t)frame, 1u);

        note->envelope_deltas[0][i] = note->envelope_points[0][i] / (note->envelope_samples[0][i] << KS_SAMPLE_PER_FRAMES_BITS);
        for(uint32_t j=1; j < KS_ENVELOPE_NUM_POINTS; j++)
        {
            note->envelope_deltas[j][i] = (note->envelope_points[j][i] - note->envelope_points[j-1][i]);
            note->envelope_deltas[j][i] /= (int32_t)note->envelope_samples[j][i] << KS_SAMPLE_PER_FRAMES_BITS;
        }

        //envelope state init
        note->envelope_now_amps[i] = 0;
        note->envelope_now_times[i] = note->envelope_samples[0][i];
        note->envelope_now_deltas[i] = note->envelope_deltas[0][i];
        note->envelope_now_points[i] = 0;
        note->envelope_states[i] = KS_ENVELOPE_ON;

    }
}

void ks_synth_note_off (ks_synth_note* note)
{
    for(uint32_t i=0; i< KS_NUM_OPERATORS; i++)
    {
        note->envelope_states[i] = KS_ENVELOPE_RELEASED;
        note->envelope_now_times[i] = note->envelope_release_samples[i];
        note->envelope_now_deltas[i] = - note->envelope_now_amps[i] / ((int32_t)note->envelope_now_times[i] << KS_SAMPLE_PER_FRAMES_BITS);
    }
}



static inline int32_t envelope_apply(uint32_t amp, int32_t in)
{
    int64_t out = amp;
    out *=in;
    out >>= KS_ENVELOPE_BITS;
    return out;
}

static inline int32_t output_mod(uint32_t phase, int32_t mod, uint32_t envelope)
{
    // mod<<(KS_TABLE_BITS + 1) = double table length = 4pi
    int32_t out = ks_sin(phase + (mod<<(KS_TABLE_BITS + 1)), true);
    return envelope_apply(envelope, out);
}

static inline int16_t synth_frame(const ks_synth* synth, ks_synth_note* note, uint8_t algorithm)
{
    int32_t out;
    int32_t feedback;
    int32_t output[KS_NUM_OPERATORS];

    if(algorithm == 0)
    {
        // +-----+
        // +-[1]-+-[2]---[3]---[4]--->

        output[3] = output_mod(note->phases[3], note->output_log[2], note->envelope_now_amps[3]);
        output[2] = output_mod(note->phases[2], note->output_log[1], note->envelope_now_amps[2]);
        output[1] = output_mod(note->phases[1], note->output_log[0], note->envelope_now_amps[1]);
        output[0] = output_mod(note->phases[0], note->feedback_log , note->envelope_now_amps[0]);
        out = note->output_log[3];

        feedback = synth->feedback_level;
        feedback *= output[0];
        feedback >>= KS_FEEDBACK_LEVEL_BITS;

    }
    if(algorithm == 1)
    {
        // +-----+
        // +-[1]-+
        //       +-[3]---[4]--->
        //   [2]-+
        output[3] = output_mod(note->phases[3], note->output_log[2]                      , note->envelope_now_amps[3]);
        output[2] = output_mod(note->phases[2], note->output_log[1] + note->output_log[0], note->envelope_now_amps[2]);
        output[1] = output_mod(note->phases[1], 0                                        , note->envelope_now_amps[1]);
        output[0] = output_mod(note->phases[0], note->feedback_log                       , note->envelope_now_amps[0]);
        out = note->output_log[3];

        feedback = synth->feedback_level;
        feedback *= output[0];
        feedback >>= KS_FEEDBACK_LEVEL_BITS;
    }
    if(algorithm == 2)
    {
        // +-----+
        // +-[1]-+-----+
        //             +-[4]--->
        //   [2]---[3]-+
        output[3] = output_mod(note->phases[3], note->output_log[2] + note->output_log[0], note->envelope_now_amps[3]);
        output[2] = output_mod(note->phases[2], note->output_log[1]                      , note->envelope_now_amps[2]);
        output[1] = output_mod(note->phases[1], 0                                        , note->envelope_now_amps[1]);
        output[0] = output_mod(note->phases[0], note->feedback_log                       , note->envelope_now_amps[0]);
        out = note->output_log[3];

        feedback = synth->feedback_level;
        feedback *= output[0];
        feedback >>= KS_FEEDBACK_LEVEL_BITS;
    }
    if(algorithm == 3)
    {
        // +-----+
        // +-[1]-+-[2]-+
        //             +-[4]--->
        //         [3]-+
        output[3] = output_mod(note->phases[3], note->output_log[1] + note->output_log[2], note->envelope_now_amps[3]);
        output[2] = output_mod(note->phases[2], 0                                        , note->envelope_now_amps[2]);
        output[1] = output_mod(note->phases[1], note->output_log[0]                      , note->envelope_now_amps[1]);
        output[0] = output_mod(note->phases[0], note->feedback_log                       , note->envelope_now_amps[0]);
        out = note->output_log[3];

        feedback = synth->feedback_level;
        feedback *= output[0];
        feedback >>= KS_FEEDBACK_LEVEL_BITS;
    }
    if(algorithm == 4)
    {
        // +-----+
        // +-[1]-+-[2]-+
        //             +--->
        // +-[3]---[4]-+
        output[3] = output_mod(note->phases[3], note->output_log[2] , note->envelope_now_amps[3]);
        output[2] = output_mod(note->phases[2], 0                   , note->envelope_now_amps[2]);
        output[1] = output_mod(note->phases[1], note->output_log[0] , note->envelope_now_amps[1]);
        output[0] = output_mod(note->phases[0], note->feedback_log  , note->envelope_now_amps[0]);
        out = note->output_log[1] + note->output_log[3];

        feedback = synth->feedback_level;
        feedback *= output[0];
        feedback >>= KS_FEEDBACK_LEVEL_BITS;
    }
    if(algorithm == 5)
    {
        //         +-[2]-+
        // +-----+ |     |
        // +-[1]-+-+-[3]-+--->
        //         |     |
        //         +-[4]-+
        output[3] = output_mod(note->phases[3], note->output_log[0]                      , note->envelope_now_amps[3]);
        output[2] = output_mod(note->phases[2], note->output_log[0]                      , note->envelope_now_amps[2]);
        output[1] = output_mod(note->phases[1], note->output_log[0]                      , note->envelope_now_amps[1]);
        output[0] = output_mod(note->phases[0], note->feedback_log                       , note->envelope_now_amps[0]);
        out = note->output_log[1] + note->output_log[2] + note->output_log[3];

        feedback = synth->feedback_level;
        feedback *= output[0];
        feedback >>= KS_FEEDBACK_LEVEL_BITS;
    }
    if(algorithm == 6)
    {
        // +-----+
        // +-[1]-+-[2]-+
        //             |
        //         [3]-+--->
        //             |
        //         [4]-+
        output[3] = output_mod(note->phases[3], 0                   , note->envelope_now_amps[3]);
        output[2] = output_mod(note->phases[2], 0                   , note->envelope_now_amps[2]);
        output[1] = output_mod(note->phases[1], note->output_log[0] , note->envelope_now_amps[1]);
        output[0] = output_mod(note->phases[0], note->feedback_log  , note->envelope_now_amps[0]);
        out = note->output_log[1] + note->output_log[2] + note->output_log[3];

        feedback = synth->feedback_level;
        feedback *= output[0];
        feedback >>= KS_FEEDBACK_LEVEL_BITS;
    }
    if(algorithm ==7)
    {
        // +-----+
        // +-[1]-+ [2]   [3]   [4]
        //    +-----+-----+-----+--->
        output[3] = output_mod(note->phases[3], 0                     , note->envelope_now_amps[3]);
        output[2] = output_mod(note->phases[2], 0                     , note->envelope_now_amps[2]);
        output[1] = output_mod(note->phases[1], 0                     , note->envelope_now_amps[1]);
        output[0] = output_mod(note->phases[0], note->feedback_log    , note->envelope_now_amps[0]);
        out = note->output_log[0] + note->output_log[1] + note->output_log[2] + note->output_log[3];

        feedback = synth->feedback_level;
        feedback *= output[0];
        feedback >>= KS_FEEDBACK_LEVEL_BITS;
    }
    if(algorithm ==8)
    {
        // saw wave * 2 (saw - saw = square)
        {
            const uint32_t p1 = note->phases[0];
            const uint32_t p2 = note->phases[0] + (note->phase_deltas[0]>>2);
            output[0] =(int32_t)ks_saw(p1) +ks_saw(p2);
            const uint32_t p3 = note->phases[1] + (note->phase_deltas[1]>>1);
            const uint32_t p4 = note->phases[1] + (note->phase_deltas[1]>>2);
            output[1] =(int32_t)ks_saw(p3)+ks_saw(p4);
        }
        {
            const uint32_t p1 = note->phases[2];
            const uint32_t p2 = note->phases[2] + (note->phase_deltas[2]>>2);
            output[2] =(int32_t)ks_saw(p1) +ks_saw(p2);
            const uint32_t p3 = note->phases[3] + (note->phase_deltas[3]>>1);
            const uint32_t p4 = note->phases[3] + (note->phase_deltas[3]>>2);
            output[3] =(int32_t)ks_saw(p3)+ks_saw(p4);
        }

        out =  envelope_apply(note->envelope_now_amps[0], (output[0]+output[1]) >> 2) -
                envelope_apply(note->envelope_now_amps[2], (output[2]+output[3]) >> 2);
    }
    if(algorithm ==9)
    {
        // triangle (fake triangle)
        {
            const uint32_t p1 = note->phases[0];
            const uint32_t p2 = note->phases[0] + (note->phase_deltas[0]>>2);
            output[0] =(int32_t)ks_fake_triangle( p1, synth->phase_coarses[2]) +ks_fake_triangle( p2, synth->phase_coarses[2]);
            const uint32_t p3 = note->phases[1] + (note->phase_deltas[1]>>1);
            const uint32_t p4 = note->phases[1] + (note->phase_deltas[1]>>2);
            output[1] =(int32_t)ks_fake_triangle(p3, synth->phase_coarses[3])+ks_fake_triangle( p4, synth->phase_coarses[3]);
        }

        out =  envelope_apply(note->envelope_now_amps[0], (output[0]+output[1]) >> 2);
    }
    if(algorithm ==10)
    {
        // noise
        if((ks_mask(note->phases[0], KS_PHASE_MAX_BITS)) < note->phase_deltas[0]) {
            srand(note->output_log[1]);
            output[1] = rand();
        } else {
            output[1] = note->output_log[1];
        }
        output[0] = envelope_apply(note->envelope_now_amps[0], ks_noise(note->phases[0], output[1]));
        out = output[0];
    }

    for(uint32_t i =0; i< KS_NUM_OPERATORS; i++)
    {
        note->output_log[i] = output[i];
    }
    note->feedback_log = feedback;

    return out >> 2; //
}

static inline void envelope_next(ks_synth_note* note)
{
    for(uint32_t i=0; i<KS_NUM_OPERATORS; i++)
    {
        if(--note->envelope_now_times[i] <= 0)
        {
            switch(note->envelope_states[i])
            {
            case KS_ENVELOPE_ON:
                note->envelope_now_amps[i] = note->envelope_points[note->envelope_now_points[i]][i];
                note->envelope_now_points[i] ++;
                if(note->envelope_now_points[i] == KS_ENVELOPE_NUM_POINTS)
                {
                    note->envelope_states[i] = KS_ENVELOPE_SUSTAINED;
                    note->envelope_now_deltas[i] = 0;
                }
                else
                {
                    note->envelope_now_deltas[i] = note->envelope_deltas[note->envelope_now_points[i]][i];
                    note->envelope_now_times[i] = note->envelope_samples[note->envelope_now_points[i]][i];
                }
                break;
            case KS_ENVELOPE_RELEASED:
                note->envelope_now_amps[i] = 0;
                note->envelope_now_deltas[i] = 0;
                note->envelope_states[i] = KS_ENVELOPE_OFF;
                break;
            }
        }
    }
}

static inline void lfo_frame(const ks_synth* synth, ks_synth_note* note, uint32_t delta[],
                                      uint8_t lfo_wave_type, bool ams, bool fms)
{
    if(fms)
    {
        int64_t depth = synth->lfo_fms_depth;
        depth *= note->lfo_log;
        depth >>= KS_OUTPUT_BITS;
        depth = ks_fms_depth(depth);

        for(uint32_t j=0; j<KS_NUM_OPERATORS; j++)
        {
            int64_t d = note->phase_deltas[j];
            d *= depth;
            d >>= KS_LFO_DEPTH_BITS;
            delta[j] = (uint32_t)d;
        }
    }
    else
    {
        for(uint32_t j=0; j<KS_NUM_OPERATORS; j++)
        {
            delta[j] = note->phase_deltas[j];
        }
    }

    if(ams)
    {
        for(uint32_t j=0; j<KS_NUM_OPERATORS; j++)
        {
            int64_t depth = note->lfo_log;      // -1.0 ~ 1.0
            depth *= synth->lfo_ams_depths[j];
            depth >>= KS_LFO_DEPTH_BITS;
            depth >>= 1;
            depth += 1<<(KS_OUTPUT_BITS);     // 0 ~ 2.0


            uint32_t ams_size =  synth->lfo_ams_depths[j];
            ams_size >>= (KS_LFO_DEPTH_BITS - KS_OUTPUT_BITS) + 1; // MAX 0.0 ~ 1.0, mean 1-ams_depth/2

            depth -= ams_size;

            depth *= note->output_log[j];
            depth >>= KS_LFO_DEPTH_BITS;
            note->output_log[j] = (int32_t)depth;
        }
    }

    if(fms || ams)
    {

        switch(lfo_wave_type)
        {
        case KS_LFO_WAVE_TRIANGLE:
            note->lfo_log = ks_triangle(note->lfo_phase);
            break;
        case KS_LFO_WAVE_SAW_UP:
            note->lfo_log = ks_saw(note->lfo_phase);
            break;
        case KS_LFO_WAVE_SAW_DOWN:
            note->lfo_log = - ks_saw(note->lfo_phase);
            break;
        case KS_LFO_WAVE_SQUARE:
            note->lfo_log = ks_saw(note->lfo_phase);
            note->lfo_log -= ks_saw(note->lfo_phase + (ks_1(KS_PHASE_MAX_BITS) >> 1));
            break;
        case KS_LFO_WAVE_SIN:
            note->lfo_log = ks_sin(note->lfo_phase, true);
            break;
        }
        note->lfo_phase += note->lfo_delta;
    }
}

static inline void ks_synth_process(const ks_synth* synth, ks_synth_note* note, uint32_t pitchbend, int16_t* buf, uint32_t len,
                                    uint8_t algorithm, uint8_t lfo_type, bool ams, bool fms)
{
    uint32_t delta[KS_NUM_OPERATORS];

    for(uint32_t i=0; i<len; i++)
    {
        buf[i] = synth_frame(synth, note, algorithm);
        lfo_frame(synth, note, delta, lfo_type, ams, fms);

        for(uint32_t j=0; j<KS_NUM_OPERATORS; j++)
        {
            uint64_t d = delta[j];
            d*= pitchbend;
            d >>= KS_LFO_DEPTH_BITS;

            note->phases[j] = note->phases[j] + d;
        }

        for(uint32_t j=0; j<KS_NUM_OPERATORS; j++)
        {
            note->envelope_now_amps[j] += note->envelope_now_deltas[j];
        }
        if(ks_mask(note->now_frame, KS_SAMPLE_PER_FRAMES_BITS) == 0)
        {
            envelope_next(note);
        }
        note->now_frame ++;
    }
}

#define _algorithm_list _(0) _(1) _(2) _(3) _(4) _(5) _(6) _(7) _(8) _(9) _(10)

#define ks_synth_define_synth_render_aw(algorithm, wave) \
void ks_synth_render_ ## algorithm ## _ ## wave ##_0_0(const ks_synth* synth, ks_synth_note* note, uint32_t pitchbend, int16_t* buf, uint32_t len){ \
    ks_synth_process(synth, note, pitchbend, buf, len, algorithm, wave, false, false); \
} \
void ks_synth_render_ ## algorithm ## _ ## wave  ##_0_1(const ks_synth* synth, ks_synth_note* note, uint32_t pitchbend, int16_t* buf, uint32_t len) {\
    ks_synth_process(synth, note,pitchbend,  buf, len, algorithm, wave, false, true); \
} \
void ks_synth_render_ ## algorithm ## _ ## wave  ##_1_0(const ks_synth* synth, ks_synth_note* note, uint32_t pitchbend, int16_t* buf, uint32_t len) {\
    ks_synth_process(synth, note, pitchbend, buf, len, algorithm, wave, true, false); \
} \
    void ks_synth_render_ ## algorithm ## _ ## wave  ##_1_1(const ks_synth* synth, ks_synth_note* note, uint32_t pitchbend, int16_t* buf, uint32_t len){ \
    ks_synth_process(synth, note, pitchbend, buf, len, algorithm, wave, true, true); \
}

#define ks_synth_define_synth_render(algorithm) \
    ks_synth_define_synth_render_aw(algorithm, KS_LFO_WAVE_TRIANGLE) \
    ks_synth_define_synth_render_aw(algorithm, KS_LFO_WAVE_SAW_UP) \
    ks_synth_define_synth_render_aw(algorithm, KS_LFO_WAVE_SAW_DOWN) \
    ks_synth_define_synth_render_aw(algorithm, KS_LFO_WAVE_SQUARE) \
    ks_synth_define_synth_render_aw(algorithm, KS_LFO_WAVE_SIN)

#undef _
#define _(x) ks_synth_define_synth_render(x)
#define split

_algorithm_list

#undef ks_synth_define_synth_render


#define ks_synth_lfo_branch(algorithm, wave) \
    if(synth->lfo_ams_enabled) \
    { \
        if(synth->lfo_fms_enabled) \
        { \
            ks_synth_render_ ## algorithm ## _ ## wave ## _1_1 (synth, note, pitchbend,  buf, len); \
        } \
        else \
        { \
            ks_synth_render_ ## algorithm ## _ ## wave ## _1_0 (synth, note, pitchbend, buf, len); \
        } \
    } \
    else \
    { \
        if(synth->lfo_fms_enabled) \
        { \
            ks_synth_render_ ## algorithm ## _ ## wave ## _0_1 (synth, note, pitchbend, buf, len); \
        } \
        else \
        { \
            ks_synth_render_ ## algorithm ## _ ## wave ## _0_0 (synth, note, pitchbend, buf, len); \
        } \
    }

#define ks_synth_render_branch(algorithm) \
    switch(synth->lfo_wave_type) {\
    case KS_LFO_WAVE_TRIANGLE: \
        ks_synth_lfo_branch(algorithm, KS_LFO_WAVE_TRIANGLE); break; \
    case KS_LFO_WAVE_SAW_UP: \
        ks_synth_lfo_branch(algorithm, KS_LFO_WAVE_SAW_UP); break; \
    case KS_LFO_WAVE_SAW_DOWN: \
        ks_synth_lfo_branch(algorithm, KS_LFO_WAVE_SAW_DOWN); break; \
    case KS_LFO_WAVE_SQUARE: \
        ks_synth_lfo_branch(algorithm, KS_LFO_WAVE_SQUARE); break; \
    case KS_LFO_WAVE_SIN: \
        ks_synth_lfo_branch(algorithm, KS_LFO_WAVE_SIN); break; \
    }

#undef _
#define _(x) case x : ks_synth_render_branch(x) break;

void ks_synth_render(const ks_synth* synth, ks_synth_note* note, uint32_t pitchbend, int16_t *buf, uint32_t len)
{
    if(*(uint32_t*)note->envelope_states != 0){
        switch(synth->algorithm)
        {
            _algorithm_list
        }
    } else {
        memset(buf, 0, len*sizeof(int16_t));
    }
}


#undef ks_synth_lfo_branch
#undef _

