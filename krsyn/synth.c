#include "synth.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

ks_io_begin_custom_func(ks_synth_data)
    ks_magic_number("KSYN");
    ks_arr_u8(phase_coarses.b);
    ks_arr_u8(phase_tunes);
    ks_arr_u8(phase_fines);
    ks_arr_u8(levels);

    ks_arr_u8(envelope_points[0]);
    ks_arr_u8(envelope_points[1]);
    ks_arr_u8(envelope_points[2]);
    ks_arr_u8(envelope_points[3]);

    ks_arr_u8(envelope_times[0]);
    ks_arr_u8(envelope_times[1]);
    ks_arr_u8(envelope_times[2]);
    ks_arr_u8(envelope_times[3]);

    ks_arr_u8(velocity_sens);
    ks_arr_u8(ratescales);
    ks_arr_u8(keyscale_low_depths);
    ks_arr_u8(keyscale_high_depths);
    ks_arr_u8(keyscale_mid_points);
    ks_arr_u8(keyscale_curve_types.b);
    ks_arr_u8(lfo_ams_depths);

    ks_u8(algorithm);
    ks_u8(feedback_level);
    ks_u8(panpot);
    ks_u8(lfo_wave_type);
    ks_u8(lfo_freq);
    ks_u8(lfo_det);
    ks_u8(lfo_fms_depth);
ks_io_end_custom_func(ks_synth_data)


ks_synth* ks_synth_new(ks_synth_data* data, u32 sampling_rate){
    ks_synth* ret = calloc(1 , sizeof(ks_synth));
    ks_synth_set(ret, sampling_rate, data);
    return ret;
}

ks_synth* ks_synth_array_new(u32 length, ks_synth_data data[], u32 sampling_rate){
    ks_synth* ret = calloc(length, sizeof(ks_synth));
    for(u32 i=0; i<length; i++){
        ks_synth_set(ret+i, sampling_rate, data+i);
    }
    return ret;
}

void ks_synth_free(ks_synth* synth){
    free(synth);
}

void ks_synth_data_set_default(ks_synth_data* data)
{
    *data = (ks_synth_data){ 0 };
    for(u32 i=0; i<KS_NUM_OPERATORS; i++)
    {
        data->phase_coarses.st[i].fixed_frequency = false;
        data->phase_coarses.st[i].value = 2;
        data->phase_fines[i] = 0;
        data->phase_tunes[i] = 127;
        data->levels[i] = 255;

        data->envelope_times[0][i] = 0;
        data->envelope_times[1][i] = UINT8_MAX;
        data->envelope_times[2][i] = UINT8_MAX;
        data->envelope_times[3][i] = 128;

        data->envelope_points[0][i] = UINT8_MAX;
        data->envelope_points[1][i] = UINT8_MAX;
        data->envelope_points[2][i] = UINT8_MAX;
        data->envelope_points[3][i] = 0;

        data->velocity_sens[i] = UINT8_MAX;

        data->ratescales[i] = 32; // about 0.125
        data->keyscale_low_depths[i] = 0;
        data->keyscale_high_depths[i] = 0;
        data->keyscale_mid_points[i] = 69;
        data->keyscale_curve_types.st[i].left = 0;
        data->keyscale_curve_types.st[i].right = 0;

        data->lfo_ams_depths[i] = 0;
    }

    data->algorithm = 0;
    data->feedback_level = 0;
    data->panpot = 64;

    data->lfo_wave_type = KS_LFO_WAVE_TRIANGLE;
    data->lfo_fms_depth = 0;
    data->lfo_freq = 128; // 1 Hz ?
    data->lfo_det = 0;
}

KS_INLINE  bool  ks_synth_note_is_enabled     (const ks_synth_note* note){
    return *((u32*)note->envelope_states) != 0;
}

KS_INLINE bool ks_synth_note_is_on (const ks_synth_note* note){
    return (*((u32*)note->envelope_states) & 0x0f0f0f0f) != 0;
}

KS_INLINE u32 ks_exp_u(u8 val, u32 base, int num_v_bit)
{
    // ->[e]*(8-num_v_bit) [v]*(num_v_bit)
    // base * 1.(v) * 2^(e)
    i8 v = val & ((1 << num_v_bit) - 1);
    v |= ((val== 0) ? 0 : 1) << num_v_bit;    // add 1
    i8 e = val >> num_v_bit;

    u64 ret = (u64)base * v;
    ret <<= e;
    ret >>= 7;
    return ret;
}

KS_INLINE u32 ks_calc_envelope_times(u32 val)
{
    return ks_exp_u(val, 1<<(KS_TIME_BITS-8), 4);
}

KS_INLINE u32 ks_calc_envelope_samples(u32 smp_freq, u8 val)
{
    u32 time = ks_calc_envelope_times(val);
    u64 samples = time;
    samples *= smp_freq;
    samples >>= KS_TIME_BITS;
    samples = MAX(1u, samples);  // note : maybe dont need this line
    return (u32)samples;
}

// min <= val < max
KS_INLINE i64 ks_linear(u8 val, i32 MIN, i32 MAX)
{
    i32 range = MAX - MIN;
    i64 ret = val;
    ret *= range;
    ret >>= 8;
    ret += MIN;

    return ret;
}


// min <= val <= max
KS_INLINE i64 ks_linear2(u8 val, i32 MIN, i32 MAX)
{
    return ks_linear(val, MIN, MAX + MAX/256);
}


KS_INLINE u32 ks_fms_depth(i32 depth){
    i64 ret = ((i64)depth*depth)>>(KS_LFO_DEPTH_BITS + 2);
    ret += (depth >> 2) + (depth >> 1);
    ret += ks_1(KS_LFO_DEPTH_BITS);
    return ret;
}

KS_INLINE void ks_calc_panpot(i16* left, i16 * right, u8 val){
    *left =  ks_sin(ks_v(val + ks_v(2, KS_PANPOT_BITS - 1), KS_PHASE_MAX_BITS - KS_PANPOT_BITS - 2 ), false);
    *right = ks_sin(ks_v(val + ks_v(0, KS_PANPOT_BITS - 1), KS_PHASE_MAX_BITS - KS_PANPOT_BITS - 2 ), false);
}

KS_INLINE i32 ks_apply_panpot(i32 in, i16 pan){
    i32 out = in;
    out *= pan;
    out >>= KS_OUTPUT_BITS;
    return out;
}

static KS_INLINE void synth_op_set(u32 sampling_rate, ks_synth* synth, const ks_synth_data* data)
{
    for(u32 i=0; i<KS_NUM_OPERATORS; i++)
    {
        synth->fixed_frequency[i] = calc_fixed_frequency(data->phase_coarses.st[i].fixed_frequency);
        synth->phase_coarses[i] = calc_phase_coarses(data->phase_coarses.st[i].value);

        synth->phase_fines[i] = calc_phase_fines(data->phase_fines[i]);
        synth->phase_tunes[i] = calc_phase_tunes(data->phase_tunes[i]);
        synth->levels[i] = calc_levels(data->levels[i]);

        for(u32 e=0; e < KS_ENVELOPE_NUM_POINTS; e++)
        {
            synth->envelope_points[e][i] = calc_envelope_points(data->envelope_points[e][i]);
            synth->envelope_samples[e][i] = calc_envelope_samples(sampling_rate, data->envelope_times[e][i]);
        }

        synth->velocity_sens[i] = calc_velocity_sens(data->velocity_sens[i]);

        synth->ratescales[i] = calc_ratescales(data->ratescales[i]);
        synth->keyscale_low_depths[i] = calc_keyscale_low_depths(data->keyscale_low_depths[i]);
        synth->keyscale_high_depths[i] = calc_keyscale_high_depths(data->keyscale_high_depths[i]);
        synth->keyscale_mid_points[i] = calc_keyscale_mid_points(data->keyscale_mid_points[i]);
        synth->keyscale_curve_types[0][i] = calc_keyscale_curve_types_left(data->keyscale_curve_types.st[i].left);
        synth->keyscale_curve_types[1][i] = calc_keyscale_curve_types_right(data->keyscale_curve_types.st[i].right);

        synth->lfo_ams_depths[i] = calc_lfo_ams_depths(data->lfo_ams_depths[i]);
    }
    synth->lfo_ams_enabled = *(const u32*)data->lfo_ams_depths != 0;


}

static KS_INLINE void synth_common_set(ks_synth* synth, const ks_synth_data* data)
{
    synth->algorithm = calc_algorithm(data->algorithm);
    synth->feedback_level = calc_feedback_level(data->feedback_level);
    ks_calc_panpot(&synth->panpot_left, &synth->panpot_right, data->panpot);

    synth->lfo_wave_type = calc_lfo_wave_type(data->lfo_wave_type);
    synth->lfo_fms_depth = calc_lfo_fms_depth(data->lfo_fms_depth);
    synth->lfo_fms_enabled = data->lfo_fms_depth != 0;
    synth->lfo_freq = calc_lfo_freq(data->lfo_freq);
    synth->lfo_det = ks_linear_u(data->lfo_det, 0, ks_1(KS_PHASE_MAX_BITS));
}


void ks_synth_set(ks_synth* synth, u32 sampling_rate, const ks_synth_data* data)
{
    synth_op_set(sampling_rate, synth, data);
    synth_common_set(synth, data);
}


static KS_INLINE u32 phase_lfo_delta(u32 sampling_rate, u32 freq)
{
    u64 freq_11 = ((u64)freq) << KS_TABLE_BITS;
    u64 delta_11 = (u32)(freq_11 / sampling_rate);

    return (u32)delta_11;
}

static KS_INLINE u32 phase_delta_fix_freq(u32 sampling_rate, u32 coarse, u32 fine, u32 tune)
{
    const u32 freq = ks_1(KS_FREQUENCY_BITS);
    const u64 freq_11 = ((u64)freq) << KS_TABLE_BITS;

    const u32 coarse_exp = ks_exp_u(coarse, 64, 4) -7; // max 2048 ?
    const u32 freq_rate = (coarse_exp * (ks_1(KS_FREQUENCY_BITS) + 9 * fine + tune));

    u64 delta_11 = (u32)(freq_11 / sampling_rate);
    delta_11 *= freq_rate;
    delta_11 >>= KS_FREQUENCY_BITS;

    return (u32)delta_11;
}

static KS_INLINE u32 phase_delta(u32 sampling_rate, u8 notenum, u32 coarse, u32 fine, u32 tune)
{
    const u32 freq = ks_notefreq(notenum); // heltz << KS_FREQUENCY_BITS
    const u64 freq_11 = ((u64)freq) << KS_TABLE_BITS;

    const u32 freq_rate = (coarse * (ks_1(KS_FREQUENCY_BITS) + fine + tune)) >> KS_PHASE_COARSE_BITS;
    u64 delta_11 = (u32)(freq_11 / sampling_rate);
    delta_11 *= freq_rate;
    delta_11 >>= KS_FREQUENCY_BITS;

    return (u32)delta_11;
}



static KS_INLINE i32 keyscale_v(i32 v, u32 d){
    i64 v2 = (i64)v* d;
    v2 >>= KS_KEYSCALE_DEPTH_BITS;
    return v2;
}

static KS_INLINE i32 keyscale(const ks_synth *synth, u8 notenum, u32 i)
{
    // <-- |mid point|
    if(notenum < synth->keyscale_mid_points[i]){
        const i32 sub = synth->keyscale_mid_points[i] - notenum;

        switch (synth->keyscale_curve_types[0][i]) {
        case KS_KEYSCALE_CURVE_ED:{
            return keyscale_v(sub * ks_1(KS_KEYSCALE_CURVE_BITS) / (sub - 127), synth->keyscale_low_depths[i]) + ks_1(KS_KEYSCALE_CURVE_BITS);
        }
        case KS_KEYSCALE_CURVE_EU:{
            return keyscale_v( sub * ks_1(KS_KEYSCALE_CURVE_BITS) / (127 - sub), synth->keyscale_low_depths[i])  + ks_1(KS_KEYSCALE_CURVE_BITS);
        }
        case KS_KEYSCALE_CURVE_LD:{
            return keyscale_v((u64)ks_notefreq(0) * ks_1(KS_KEYSCALE_CURVE_BITS) / ks_notefreq(((i64)sub * synth->keyscale_low_depths[i])>> KS_KEYSCALE_DEPTH_BITS),  ks_1(KS_KEYSCALE_DEPTH_BITS));
        }
        case KS_KEYSCALE_CURVE_LU:{
            return keyscale_v((u64)ks_notefreq(sub) * ks_1(KS_KEYSCALE_CURVE_BITS) / ks_notefreq(0), synth->keyscale_low_depths[i])  + ks_1(KS_KEYSCALE_CURVE_BITS); // ?
        }
        }

    }
    // |mid point| -->
    else {
        const i32 sub = notenum - synth->keyscale_mid_points[i];

        switch (synth->keyscale_curve_types[1][i]) {
        case KS_KEYSCALE_CURVE_ED:{
            return keyscale_v(sub * ks_1(KS_KEYSCALE_CURVE_BITS) / (sub - 127), synth->keyscale_high_depths[i]) + ks_1(KS_KEYSCALE_CURVE_BITS);
        }
        case KS_KEYSCALE_CURVE_EU:{
            return keyscale_v( sub * ks_1(KS_KEYSCALE_CURVE_BITS) / (127 - sub), synth->keyscale_high_depths[i])  + ks_1(KS_KEYSCALE_CURVE_BITS);
        }
        case KS_KEYSCALE_CURVE_LD:{
            return keyscale_v((u64)ks_notefreq(0) * ks_1(KS_KEYSCALE_CURVE_BITS) / ks_notefreq(((i64)sub * synth->keyscale_high_depths[i])>> KS_KEYSCALE_DEPTH_BITS),  ks_1(KS_KEYSCALE_DEPTH_BITS));
        }
        case KS_KEYSCALE_CURVE_LU:{
            return keyscale_v((u64)ks_notefreq(sub) * ks_1(KS_KEYSCALE_CURVE_BITS) / ks_notefreq(0), synth->keyscale_high_depths[i])  + ks_1(KS_KEYSCALE_CURVE_BITS); // ?
        }
        }
    }


    return 0;
}

void ks_synth_note_on( ks_synth_note* note, const ks_synth *synth, u32 sampling_rate, u8 notenum, u8 vel)
{
    note->now_frame = 0;
    note->feedback_log =0;

    note->lfo_log = 0;
    note->lfo_phase= synth->lfo_det;
    note->lfo_delta = phase_lfo_delta(sampling_rate, synth->lfo_freq);

    if(synth->algorithm == 8){
        note->phases[0] = note->phases[1] =  0;
        note->phases[2] = synth->levels[2] << (KS_PHASE_MAX_BITS - KS_LEVEL_BITS);
        note->phases[3] = synth->levels[3] << (KS_PHASE_MAX_BITS - KS_LEVEL_BITS);
    } else {
        note->phases[0] = note->phases[1] = note->phases[2] = note->phases[3] = 0;
    }

    for(u32 i=0; i< KS_NUM_OPERATORS; i++)
    {
        if(synth->fixed_frequency[i])
        {
            note->phase_deltas[i] = phase_delta_fix_freq(sampling_rate, synth->phase_coarses[i], synth->phase_fines[i], synth->phase_tunes[i]);
        }
        else
        {
            note->phase_deltas[i] = phase_delta(sampling_rate, notenum, synth->phase_coarses[i], synth->phase_fines[i], synth->phase_tunes[i]);
        }

        note->output_log[i] = 0;

        //rate scale
        i64 ratescales = synth->ratescales[i];
        ratescales *= ks_ratescale(notenum);
        ratescales >>= KS_RATESCALE_BITS;
        ratescales += 1<<KS_RATESCALE_BITS;

        //key scale
        i64 keysc = keyscale(synth, notenum, i);

        i64 target;
        u32 velocity;

        //envelope
        for(u32 j=0; j < KS_ENVELOPE_NUM_POINTS; j++)
        {
            velocity = synth->velocity_sens[i];
            velocity *=  127-vel;
            velocity >>= 7;
            velocity = (1 << KS_VELOCITY_SENS_BITS) -  velocity;

            target = keysc;
            target *= synth->envelope_points[j][i];
            target >>= KS_KEYSCALE_CURVE_BITS;
            target *= synth->levels[i];
            target >>= KS_LEVEL_BITS;
            target = MAX(MIN(target, synth->envelope_points[j][i]), 0);

            target *= velocity;
            target >>= KS_VELOCITY_SENS_BITS;

            note->envelope_points[j][i] = (i32)target;

            u64 frame = (synth->envelope_samples[j][i]);
            frame *= ratescales;
            frame >>= KS_RATESCALE_BITS;
            frame >>= KS_SAMPLE_PER_FRAMES_BITS;
            note->envelope_samples[j][i] = MAX((u32)frame, 1u);
        }

        note->envelope_deltas[0][i] = note->envelope_points[0][i] / (note->envelope_samples[0][i] << KS_SAMPLE_PER_FRAMES_BITS);
        for(u32 j=1; j < KS_ENVELOPE_NUM_POINTS; j++)
        {
            note->envelope_deltas[j][i] = (note->envelope_points[j][i] - note->envelope_points[j-1][i]);
            note->envelope_deltas[j][i] /= (i32)note->envelope_samples[j][i] << KS_SAMPLE_PER_FRAMES_BITS;
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
    for(u32 i=0; i< KS_NUM_OPERATORS; i++)
    {
        note->envelope_states[i] = KS_ENVELOPE_RELEASED;
        note->envelope_now_times[i] = note->envelope_samples[KS_ENVELOPE_RELEASE_INDEX][i];
        note->envelope_now_deltas[i] =  (note->envelope_points[KS_ENVELOPE_RELEASE_INDEX][i] - note->envelope_now_amps[i]) / ((i32)note->envelope_now_times[i] << KS_SAMPLE_PER_FRAMES_BITS);
    }
}



static KS_INLINE i32 envelope_apply(u32 amp, i32 in)
{
    i64 out = amp;
    out *=in;
    out >>= KS_ENVELOPE_BITS;
    return out;
}

static KS_INLINE i32 output_mod(u32 phase, i32 mod, u32 envelope)
{
    // mod<<(KS_TABLE_BITS + 2) = double table length = 4pi
    i32 out = ks_sin(phase + (mod<<(KS_TABLE_BITS + 2)), true);
    return envelope_apply(envelope, out);
}

static KS_INLINE i16 synth_frame(const ks_synth* synth, ks_synth_note* note, u8 algorithm)
{
    i32 out = 0;
    i32 feedback = 0;
    i32 output[KS_NUM_OPERATORS] = { 0 };

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
            const u32 p1 = note->phases[0];
            const u32 p2 = note->phases[0] + (note->phase_deltas[0]>>2);
            output[0] =(i32)ks_saw(p1) +ks_saw(p2);
            const u32 p3 = note->phases[1] + (note->phase_deltas[1]>>1);
            const u32 p4 = note->phases[1] + (note->phase_deltas[1]>>2);
            output[1] =(i32)ks_saw(p3)+ks_saw(p4);
        }
        {
            const u32 p1 = note->phases[2];
            const u32 p2 = note->phases[2] + (note->phase_deltas[2]>>2);
            output[2] =(i32)ks_saw(p1) +ks_saw(p2);
            const u32 p3 = note->phases[3] + (note->phase_deltas[3]>>1);
            const u32 p4 = note->phases[3] + (note->phase_deltas[3]>>2);
            output[3] =(i32)ks_saw(p3)+ks_saw(p4);
        }

        out =  envelope_apply(note->envelope_now_amps[0], (output[0]+output[1]) >> 1) -
                envelope_apply(note->envelope_now_amps[1], (output[2]+output[3]) >> 1);
    }
    if(algorithm ==9)
    {
        // triangle (fake triangle)
        {
            const u32 p1 = note->phases[0];
            const u32 p2 = note->phases[0] + (note->phase_deltas[0]>>2);
            output[0] =(i32)ks_fake_triangle( p1, synth->phase_coarses[2]) +ks_fake_triangle( p2, synth->phase_coarses[2]);
            const u32 p3 = note->phases[1] + (note->phase_deltas[1]>>1);
            const u32 p4 = note->phases[1] + (note->phase_deltas[1]>>2);
            output[1] =(i32)ks_fake_triangle(p3, synth->phase_coarses[3])+ks_fake_triangle( p4, synth->phase_coarses[3]);
        }

        out =  envelope_apply(note->envelope_now_amps[0], (output[0]+output[1]) >> 1);
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
        output[0] = envelope_apply(note->envelope_now_amps[0], ks_noise(note->phases[0], output[1]) );
        out = output[0];
    }

    for(u32 i =0; i< KS_NUM_OPERATORS; i++)
    {
        note->output_log[i] = output[i];
    }
    note->feedback_log = feedback;

    return out >> 2;
}

static KS_INLINE void envelope_next(ks_synth_note* note)
{
    for(u32 i=0; i<KS_NUM_OPERATORS; i++)
    {
        if(--note->envelope_now_times[i] <= 0)
        {
            switch(note->envelope_states[i])
            {
            case KS_ENVELOPE_ON:
                note->envelope_now_amps[i] = note->envelope_points[note->envelope_now_points[i]][i];
                note->envelope_now_points[i] ++;
                if(note->envelope_now_points[i] == KS_ENVELOPE_RELEASE_INDEX)
                {
                    note->envelope_states[i] = KS_ENVELOPE_SUSTAINED;
                    note->envelope_now_deltas[i] = 0;
                    note->envelope_now_times[i] = -1;
                } else{
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

static KS_INLINE void lfo_frame(const ks_synth* synth, ks_synth_note* note, u32 delta[],
                                      u8 lfo_wave_type, bool ams, bool fms)
{
    if(fms)
    {
        i64 depth = synth->lfo_fms_depth;
        depth *= note->lfo_log;
        depth >>= KS_OUTPUT_BITS;
        depth = ks_fms_depth(depth);

        for(u32 j=0; j<KS_NUM_OPERATORS; j++)
        {
            i64 d = note->phase_deltas[j];
            d *= depth;
            d >>= KS_LFO_DEPTH_BITS;
            delta[j] = (u32)d;
        }
    }
    else
    {
        for(u32 j=0; j<KS_NUM_OPERATORS; j++)
        {
            delta[j] = note->phase_deltas[j];
        }
    }

    if(ams)
    {
        for(u32 j=0; j<KS_NUM_OPERATORS; j++)
        {
            i64 depth = note->lfo_log;      // -1.0 ~ 1.0
            depth *= synth->lfo_ams_depths[j];
            depth >>= KS_LFO_DEPTH_BITS;
            depth >>= 1;
            depth += 1<<(KS_OUTPUT_BITS);     // 0 ~ 2.0


            u32 ams_size =  synth->lfo_ams_depths[j];
            ams_size >>= (KS_LFO_DEPTH_BITS - KS_OUTPUT_BITS) + 1; // MAX 0.0 ~ 1.0, mean 1-ams_depth/2

            depth -= ams_size;

            depth *= note->output_log[j];
            depth >>= KS_LFO_DEPTH_BITS;
            note->output_log[j] = (i32)depth;
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

static KS_INLINE void ks_synth_process(const ks_synth* synth, ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len,
                                    u8 algorithm, u8 lfo_type, bool ams, bool fms)
{
    u32 delta[KS_NUM_OPERATORS];

    for(u32 i=0; i<len; i+=2)
    {
        i32 out = synth_frame(synth, note, algorithm) * volume;
        out >>= KS_VOLUME_BITS;
        buf[i] = buf[i+1] = out;
        buf[i]= ks_apply_panpot(buf[i], synth->panpot_left);
        buf[i+1]= ks_apply_panpot(buf[i+1], synth->panpot_right);
        lfo_frame(synth, note, delta, lfo_type, ams, fms);

        for(u32 j=0; j<KS_NUM_OPERATORS; j++)
        {
            u64 d = delta[j];
            d*= pitchbend;
            d >>= KS_LFO_DEPTH_BITS;

            note->phases[j] = note->phases[j] + d;
        }

        for(u32 j=0; j<KS_NUM_OPERATORS; j++)
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
void ks_synth_render_ ## algorithm ## _ ## wave ##_0_0(const ks_synth* synth, ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len){ \
    ks_synth_process(synth, note, volume, pitchbend, buf, len, algorithm, wave, false, false); \
} \
void ks_synth_render_ ## algorithm ## _ ## wave  ##_0_1(const ks_synth* synth, ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len) {\
    ks_synth_process(synth, note, volume, pitchbend,  buf, len, algorithm, wave, false, true); \
} \
void ks_synth_render_ ## algorithm ## _ ## wave  ##_1_0(const ks_synth* synth, ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len) {\
    ks_synth_process(synth, note, volume, pitchbend, buf, len, algorithm, wave, true, false); \
} \
    void ks_synth_render_ ## algorithm ## _ ## wave  ##_1_1(const ks_synth* synth, ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len){ \
    ks_synth_process(synth, note, volume, pitchbend, buf, len, algorithm, wave, true, true); \
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
            ks_synth_render_ ## algorithm ## _ ## wave ## _1_1 (synth, note, volume, pitchbend,  buf, len); \
        } \
        else \
        { \
            ks_synth_render_ ## algorithm ## _ ## wave ## _1_0 (synth, note, volume, pitchbend, buf, len); \
        } \
    } \
    else \
    { \
        if(synth->lfo_fms_enabled) \
        { \
            ks_synth_render_ ## algorithm ## _ ## wave ## _0_1 (synth, note, volume, pitchbend, buf, len); \
        } \
        else \
        { \
            ks_synth_render_ ## algorithm ## _ ## wave ## _0_0 (synth, note, volume, pitchbend, buf, len); \
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

void ks_synth_render(const ks_synth* synth, ks_synth_note* note, u32 volume, u32 pitchbend, i32 *buf, u32 len)
{
    if(*(u32*)note->envelope_states != 0){
        switch(synth->algorithm)
        {
            _algorithm_list
        }
    } else {
        memset(buf, 0, len*sizeof(i32));
    }
}


#undef ks_synth_lfo_branch
#undef _

