#include "synth.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

ks_io_begin_custom_func(ks_synth_data)
    ks_magic_number("KSYN");
    ks_arr_u8(b);
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
        data->st.operators[i].mod_type = 0;
        data->st.operators[i].wave_type = 0;
        data->st.operators[i].fixed_frequency = false;
        data->st.operators[i].phase_coarse = 2;
        data->st.operators[i].phase_offset = 0;
        data->st.operators[i].phase_fine = 8;
        data->st.operators[i].phase_detune= 0;
        data->st.operators[i].level = 255;

        data->st.operators[i].envelopes[0].time = 0;
        data->st.operators[i].envelopes[1].time = 31;
        data->st.operators[i].envelopes[2].time = 31;
        data->st.operators[i].envelopes[3].time = 16;

        data->st.operators[i].envelopes[0].amp = 7;
        data->st.operators[i].envelopes[1].amp = 7;
        data->st.operators[i].envelopes[2].amp = 7;
        data->st.operators[i].envelopes[3].amp = 0;

        data->st.operators[i].ratescale = 2; // about 0.125
        data->st.operators[i].keyscale_low_depth = 0;
        data->st.operators[i].keyscale_low_depth = 0;
        data->st.operators[i].keyscale_mid_point = 17;
        data->st.operators[i].keyscale_left = KS_KEYSCALE_CURVE_LD;
        data->st.operators[i].keyscale_right = KS_KEYSCALE_CURVE_LD;

        data->st.operators[i].velocity_sens = 15;
        data->st.operators[i].lfo_ams_depth = 0;
    }

    data->st.common.algorithm = 0;
    data->st.common.feedback_level = 0;
    data->st.common.panpot = 64;

    data->st.common.lfo_wave_type = KS_WAVE_TRIANGLE;
    data->st.common.lfo_fms_depth = 0;
    data->st.common.lfo_freq = 8; // 1 Hz ?
    data->st.common.lfo_offset = 0;
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

KS_INLINE u32 ks_fms_depth(i32 depth){
    i64 ret = ((i64)depth*depth)>>(KS_LFO_DEPTH_BITS + 2);
    ret += (depth >> 2) + (depth >> 1);
    ret += ks_1(KS_LFO_DEPTH_BITS);
    return ret;
}

KS_INLINE void ks_calc_panpot(i16* left, i16 * right, u8 val){
    *left =  ks_sin(ks_v(val + ks_v(2, KS_PANPOT_BITS - 1), KS_PHASE_MAX_BITS - KS_PANPOT_BITS - 2 ));
    *right = ks_sin(ks_v(val + ks_v(0, KS_PANPOT_BITS - 1), KS_PHASE_MAX_BITS - KS_PANPOT_BITS - 2 ));
}

KS_INLINE i32 ks_apply_panpot(i32 in, i16 pan){
    i32 out = in;
    out *= pan;
    out >>= KS_OUTPUT_BITS;
    return out;
}


typedef i16(*ks_mod_func_impl)(ks_wave_func, u8, ks_synth_note*, i32);
typedef void(*ks_mod_post_process)(u8, ks_synth_note*);

KS_FORCEINLINE static i16 ks_wave_func_default(u8 op,  ks_synth_note* note, u32 phase){
    return note->wave_tables[op][ks_mask(phase >> KS_PHASE_BITS, KS_TABLE_BITS)];
}

KS_FORCEINLINE static i16 ks_wave_func_minus(u8 op,  ks_synth_note* note, u32 phase){
    return -note->wave_tables[op][ks_mask(phase >> KS_PHASE_BITS, KS_TABLE_BITS)];
}

KS_FORCEINLINE static i16 ks_wave_func_noise(u8 op,  ks_synth_note* note, u32 phase){
    const i16* table = ks_get_noise_table();
    i16 ret = table[ks_mask((((phase >> KS_NOISE_PHASE_BITS))), KS_TABLE_BITS)];
    // noise
    if(((note->phases[op] - (u32)note->wave_tables[op]) >> KS_NOISE_PHASE_BITS) > ks_1(5)) {
        srand((unsigned)note->wave_tables[op]);
        note->phases[op] = rand();
        note->wave_tables[op] = (const i16*)(i64)note->phases[op];
    }
    return ret;
}

KS_FORCEINLINE static i16 ks_wave_func_square(u8 op,  ks_synth_note* note, u32 phase){
    return note->wave_tables[op][ks_mask(((phase) >> KS_PHASE_BITS), KS_TABLE_BITS)] - note->wave_tables[op][ks_mask(((phase + ks_1(KS_PHASE_MAX_BITS-1)) >> KS_PHASE_BITS), KS_TABLE_BITS)];
}

KS_FORCEINLINE static i32 ks_envelope_apply(u32 amp, i32 in)
{
    i64 out = amp;
    out *=in;
    out >>= KS_ENVELOPE_BITS;
    return out;
}


KS_FORCEINLINE static i16 ks_ifm_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note,i32 in){
    i64 add = note->synth->delta_time;
    add *= in;
    add >>= (KS_DELTA_TIME_BITS - KS_OUTPUT_BITS)- KS_TABLE_BITS;
    return ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op] += add));
}

KS_FORCEINLINE static i16 ks_fm_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note,i32 in){
    return ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op] + (in<<(KS_TABLE_BITS + 2))));
}


KS_FORCEINLINE static i16 ks_mix_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    i32 ret = wave_func(op, note, note->phases[op]);
    ret = ks_envelope_apply(note->envelope_now_amps[op], ret);
    u32 in_f = ks_1(KS_ENVELOPE_BITS) - note->envelope_now_amps[op];
    ret += ks_envelope_apply(in_f, in);
    return ret;
}


KS_FORCEINLINE static i16 ks_am_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    i64 ret =  ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op]));
    ret += ks_1(KS_OUTPUT_BITS);
    ret *=  in;
    ret >>= KS_OUTPUT_BITS+1;
    return ret;
}

KS_FORCEINLINE static i16 ks_lpf_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    i64 out=  ks_envelope_apply(note->envelope_now_amps[op],wave_func(op, note, note->phases[op]));
    out += ks_1(KS_OUTPUT_BITS);
    out= ks_v(2, KS_OUTPUT_BITS) - out;
    out >>= 1;

    out = out*(in - note->mod_func_logs[op]);
    out >>= KS_OUTPUT_BITS;
    note->mod_func_logs[op] += out;

    return note->mod_func_logs[op] ;
}

KS_FORCEINLINE static i16 ks_hpf_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    i64 out=  ks_envelope_apply(note->envelope_now_amps[op],wave_func(op, note, note->phases[op]));
    out += ks_1(KS_OUTPUT_BITS);
    out= ks_v(2, KS_OUTPUT_BITS) - out;
    out >>= 1;

    out = out*(in - note->mod_func_logs[op]);
    out >>= KS_OUTPUT_BITS;
    note->mod_func_logs[op] += out;

    return in - note->mod_func_logs[op] ;
}

KS_FORCEINLINE static i16 ks_mul_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    i64 ret = ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op]));
    ret *=  in;
    ret >>= KS_OUTPUT_BITS;
    return ret;
}

KS_FORCEINLINE static i16 ks_rsg_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    i32 ret = wave_func(op, note, note->phases[op] );
    ret = ks_envelope_apply(note->envelope_now_amps[op], ret);
    if(note->mod_func_logs[op] < 0 && in >= 0)note->phases[op] = 0;
    note->mod_func_logs[op] = in;
    return ret;
}

KS_FORCEINLINE static i16 ks_none_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    return ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op]));
}


KS_FORCEINLINE static i32 ks_output_mod_base(ks_mod_func_impl mod_func, ks_wave_func wave_func, u8 op,  ks_synth_note* note, i32 in)
{
    // mod<<(KS_TABLE_BITS + 2) = double table length = 4pi
    i32 ret =mod_func(wave_func, op, note, in);
    return ret;
}

static i32 ks_output_pass(u8 op, ks_synth_note* note, i32 in){
    return in;
}


#define ks_output_mod_func(wave, mod) ks_output_mod_ ## wave ## _ ## mod
#define ks_output_mod_impl(wave, mod) \
    static i32 ks_output_mod_func(wave, mod) (u8 op,  ks_synth_note* note, i32 in){ \
        return ks_output_mod_base(mod, wave, op, note, in); \
    }

KS_FORCEINLINE static i32 ks_output(u8 op, ks_synth_note* note, i32 in){
    return note->mod_funcs[op](op, note, in);
}


#define ks_output_wave_impl(mod) \
ks_output_mod_impl(ks_wave_func_default, mod) \
ks_output_mod_impl(ks_wave_func_minus, mod) \
ks_output_mod_impl(ks_wave_func_square, mod) \
ks_output_mod_impl(ks_wave_func_noise, mod)

ks_output_wave_impl(ks_fm_mod)
ks_output_wave_impl(ks_ifm_mod)
ks_output_wave_impl(ks_mix_mod)
ks_output_wave_impl(ks_mul_mod)
ks_output_wave_impl(ks_am_mod)
ks_output_wave_impl(ks_rsg_mod)
ks_output_wave_impl(ks_lpf_mod)
ks_output_wave_impl(ks_hpf_mod)
ks_output_wave_impl(ks_none_mod)


#define ks_output_wave_branch(mod) \
switch (wave_type) { \
case KS_WAVE_SIN: \
case KS_WAVE_SAW_UP: \
case KS_WAVE_TRIANGLE:return ks_output_mod_func(ks_wave_func_default, mod); \
case KS_WAVE_SAW_DOWN:return ks_output_mod_func(ks_wave_func_minus, mod); \
case KS_WAVE_SQUARE: return ks_output_mod_func(ks_wave_func_square, mod); \
case KS_WAVE_NOISE: return ks_output_mod_func(ks_wave_func_noise, mod); \
}

static ks_mod_func ks_get_mod_func(u8 mod_type, u8 wave_type){
    switch (mod_type) {
    case KS_MOD_FM:
        ks_output_wave_branch(ks_fm_mod)
    case KS_MOD_NONE:
        ks_output_wave_branch(ks_none_mod)
    case KS_MOD_IFM:
        ks_output_wave_branch(ks_ifm_mod)
    case KS_MOD_MIX:
        ks_output_wave_branch(ks_mix_mod)
    case KS_MOD_MUL:
        ks_output_wave_branch(ks_mul_mod)
    case KS_MOD_AM:
        ks_output_wave_branch(ks_am_mod)
    case KS_MOD_RSG:
        ks_output_wave_branch(ks_rsg_mod)
    case KS_MOD_LPF:
        ks_output_wave_branch(ks_lpf_mod)
    case KS_MOD_HPF:
        ks_output_wave_branch(ks_hpf_mod)
    case KS_MOD_PASS:
        return ks_output_pass;
    }

    return NULL;
}


static i16 ks_lfo_wave_func_default(ks_synth_note* note){
    return note->lfo_wave_table[ks_mask(note->lfo_phase >> KS_PHASE_BITS, KS_TABLE_BITS)];
}

static i16 ks_lfo_wave_func_minus( ks_synth_note* note){
    return -note->lfo_wave_table[ks_mask(note->lfo_phase >> KS_PHASE_BITS, KS_TABLE_BITS)];
}

static i16 ks_lfo_wave_func_noise(ks_synth_note* note){
    const i16* table = ks_get_noise_table();
    i16 ret = table[ks_mask((((note->lfo_phase >> KS_NOISE_PHASE_BITS))), KS_TABLE_BITS)];
    // noise
    if(((note->lfo_phase - (u32)note->lfo_wave_table) >> KS_NOISE_PHASE_BITS) > ks_1(5)) {
        srand((unsigned)note->lfo_wave_table);
        note->lfo_phase = rand();
        note->lfo_wave_table = (const i16*)(i64)note->lfo_phase;
    }
    return ret;
}

static i16 ks_lfo_wave_func_square(ks_synth_note* note){
    return note->lfo_wave_table[ks_mask((note->lfo_phase >> KS_PHASE_BITS), KS_TABLE_BITS)] - note->lfo_wave_table[ks_mask(((note->lfo_phase + ks_1(KS_PHASE_MAX_BITS-1)) >> KS_PHASE_BITS), KS_TABLE_BITS)];
}

static ks_lfo_wave_func ks_get_lfo_wave_func(u8 index){
    switch (index) {
    case KS_WAVE_SIN:
    case KS_WAVE_SAW_UP:
    case KS_WAVE_TRIANGLE:return ks_lfo_wave_func_default;
    case KS_WAVE_SAW_DOWN:return ks_lfo_wave_func_minus;
    case KS_WAVE_SQUARE: return ks_lfo_wave_func_square;
    case KS_WAVE_NOISE: return ks_lfo_wave_func_noise;
    }
    return NULL;
}


KS_INLINE static void synth_op_set(u32 sampling_rate, ks_synth* synth, const ks_synth_data* data)
{
    for(u32 i=0; i<KS_NUM_OPERATORS; i++)
    {
        synth->mod_types[i] = data->st.operators[i].mod_type;
        synth->wave_types[i] = data->st.operators[i].wave_type;
        synth->fixed_frequency[i] = calc_fixed_frequency(data->st.operators[i].fixed_frequency);
        synth->phase_coarses[i] = calc_phase_coarses(data->st.operators[i].phase_coarse);
        synth->phase_offsets[i] = calc_phase_offsets(data->st.operators[i].phase_offset);
        synth->phase_fines[i] = calc_phase_fines(data->st.operators[i].phase_fine);
        synth->phase_tunes[i] = calc_phase_tunes(data->st.operators[i].phase_detune);
        synth->levels[i] = calc_levels(data->st.operators[i].level);

        for(u32 e=0; e < KS_ENVELOPE_NUM_POINTS; e++)
        {
            synth->envelope_points[e][i] = calc_envelope_points(data->st.operators[i].envelopes[e].amp);
            synth->envelope_samples[e][i] = calc_envelope_samples(sampling_rate, data->st.operators[i].envelopes[e].time);
        }

        synth->velocity_sens[i] = calc_velocity_sens(data->st.operators[i].velocity_sens);

        synth->ratescales[i] = calc_ratescales(data->st.operators[i].ratescale);
        synth->keyscale_low_depths[i] = calc_keyscale_low_depths(data->st.operators[i].keyscale_low_depth);
        synth->keyscale_high_depths[i] = calc_keyscale_high_depths(data->st.operators[i].keyscale_high_depth);
        synth->keyscale_mid_points[i] = calc_keyscale_mid_points(data->st.operators[i].keyscale_mid_point);
        synth->keyscale_curve_types[0][i] = calc_keyscale_curve_types_left(data->st.operators[i].keyscale_left);
        synth->keyscale_curve_types[1][i] = calc_keyscale_curve_types_right(data->st.operators[i].keyscale_right);

        synth->lfo_ams_depths[i] = calc_lfo_ams_depths(data->st.operators[i].lfo_ams_depth);
    }
    synth->lfo_ams_enabled = memcmp(synth->lfo_ams_depths, (const u32[]){0,0,0,0}, sizeof(synth->lfo_ams_depths)) == 0;


}

KS_INLINE static void synth_common_set(ks_synth* synth, const ks_synth_data* data)
{
    synth->algorithm = calc_algorithm(data->st.common.algorithm);
    synth->feedback_level = calc_feedback_level(data->st.common.feedback_level);
    ks_calc_panpot(&synth->panpot_left, &synth->panpot_right, data->st.common.panpot);

    synth->lfo_wave_type = calc_lfo_wave_type(data->st.common.lfo_wave_type);
    synth->lfo_fms_depth = calc_lfo_fms_depth(data->st.common.lfo_fms_depth);
    synth->lfo_fms_enabled = data->st.common.lfo_fms_depth != 0;
    synth->lfo_freq = calc_lfo_freq(data->st.common.lfo_freq);
    synth->lfo_offset =calc_lfo_offset(ks_v(data->st.common.lfo_offset, (8-4)));
}


void ks_synth_set(ks_synth* synth, u32 sampling_rate, const ks_synth_data* data)
{
    synth->sampling_rate = sampling_rate;
    synth->delta_time = ks_1(KS_DELTA_TIME_BITS)  / sampling_rate;
    synth_op_set(sampling_rate, synth, data);
    synth_common_set(synth, data);
}


KS_INLINE static u32 phase_lfo_delta(u32 sampling_rate, u32 freq)
{
    u64 freq_11 = ((u64)freq) << KS_TABLE_BITS;
    u64 delta_11 = (u32)(freq_11 / sampling_rate);

    return (u32)delta_11;
}

KS_INLINE static u32 phase_delta_fix_freq(u32 sampling_rate, u32 coarse, u32 tune, u32 fine)
{
    const u32 freq = ks_1(KS_FREQUENCY_BITS);
    const u64 freq_11 = ((u64)freq) << KS_TABLE_BITS;

    const u32 coarse_exp = ks_exp_u(coarse, 64, 4) -7; // max 2048 ?
    const u32 freq_rate = (coarse_exp * (ks_1(KS_FREQUENCY_BITS) + 9 * tune + fine));

    u64 delta_11 = (u32)(freq_11 / sampling_rate);
    delta_11 *= freq_rate;
    delta_11 >>= KS_FREQUENCY_BITS;

    return (u32)delta_11;
}

KS_INLINE static u32 phase_delta(u32 sampling_rate, u8 notenum, u32 coarse, u32 tune, u32 fine)
{
    const u32 freq = ks_notefreq(notenum); // heltz << KS_FREQUENCY_BITS
    const u64 freq_11 = ((u64)freq) << KS_TABLE_BITS;

    const u32 freq_rate = (coarse * (ks_1(KS_FREQUENCY_BITS) + tune + fine)) >> KS_PHASE_COARSE_BITS;
    u64 delta_11 = (u32)(freq_11 / sampling_rate);
    delta_11 *= freq_rate;
    delta_11 >>= KS_FREQUENCY_BITS;

    return (u32)delta_11;
}



KS_FORCEINLINE static i32 keyscale_v(i32 v, u32 d){
    i64 v2 = (i64)v* d;
    v2 >>= KS_KEYSCALE_DEPTH_BITS;
    return v2;
}

KS_INLINE static i32 keyscale(const ks_synth *synth, u8 notenum, u32 i)
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
    note->synth = synth;
    note->now_frame = 0;
    note->feedback_log =0;

    note->lfo_log = 0;
    note->lfo_phase= synth->lfo_offset;
    note->lfo_delta = phase_lfo_delta(sampling_rate, synth->lfo_freq);
    note->lfo_wave_table = ks_get_wave_table(synth->lfo_wave_type);
    note->lfo_func = ks_get_lfo_wave_func(synth->lfo_wave_type);


    for(unsigned i=0; i<KS_NUM_OPERATORS; i++){
        note->phases[i] = synth->phase_offsets[i];
    }


    for(u32 i=0; i< KS_NUM_OPERATORS; i++)
    {
        note->wave_tables[i] = ks_get_wave_table( synth->wave_types[i] );
        note->mod_funcs[i] = ks_get_mod_func( synth->mod_types[i], synth->wave_types[i] );
        if(synth->fixed_frequency[i])
        {
            note->phase_deltas[i] = phase_delta_fix_freq(sampling_rate, synth->phase_coarses[i], synth->phase_tunes[i], synth->phase_fines[i]);
        }
        else
        {
            note->phase_deltas[i] = phase_delta(sampling_rate, notenum, synth->phase_coarses[i], synth->phase_tunes[i], synth->phase_fines[i]);
        }

        note->output_logs[i] = 0;

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



KS_FORCEINLINE static i32 synth_frame(const ks_synth* synth, ks_synth_note* note, u8 algorithm)
{
    i32 out = 0;
    i32 feedback = 0;
    i32 output[KS_NUM_OPERATORS] = { 0 };

    if(algorithm == 0)
    {
        // +-----+
        // +-[1]-+-[2]---[3]---[4]--->

        output[3] = ks_output( 3, note,note->output_logs[2]);
        output[2] = ks_output( 2, note,note->output_logs[1]);
        output[1] = ks_output( 1, note,note->output_logs[0]);
        output[0] = ks_output( 0, note,note->feedback_log );
        out = note->output_logs[3];

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
        output[3] = ks_output( 3, note,note->output_logs[2]                      );
        output[2] = ks_output( 2, note,note->output_logs[1] + note->output_logs[0]);
        output[1] = ks_output( 1, note,0                                        );
        output[0] = ks_output( 0, note,note->feedback_log                       );
        out = note->output_logs[3];

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
        output[3] = ks_output( 3, note,note->output_logs[2] + note->output_logs[0]);
        output[2] = ks_output( 2, note,note->output_logs[1]                      );
        output[1] = ks_output( 1, note,0                                        );
        output[0] = ks_output( 0, note,note->feedback_log                       );
        out = note->output_logs[3];

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
        output[3] = ks_output( 3, note,note->output_logs[1] + note->output_logs[2]);
        output[2] = ks_output( 2, note,0                                        );
        output[1] = ks_output( 1, note,note->output_logs[0]                      );
        output[0] = ks_output( 0, note,note->feedback_log                       );
        out = note->output_logs[3];

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
        output[3] = ks_output( 3, note,note->output_logs[2] );
        output[2] = ks_output( 2, note,0                   );
        output[1] = ks_output( 1, note,note->output_logs[0] );
        output[0] = ks_output( 0, note,note->feedback_log  );
        out = note->output_logs[1] + note->output_logs[3];

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
        output[3] = ks_output( 3, note,note->output_logs[0]                      );
        output[2] = ks_output( 2, note,note->output_logs[0]                      );
        output[1] = ks_output( 1, note,note->output_logs[0]                      );
        output[0] = ks_output( 0, note,note->feedback_log                       );
        out = note->output_logs[1] + note->output_logs[2] + note->output_logs[3];

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
        output[3] = ks_output( 3, note,0                   );
        output[2] = ks_output( 2, note,0                   );
        output[1] = ks_output( 1, note,note->output_logs[0] );
        output[0] = ks_output( 0, note,note->feedback_log  );
        out = note->output_logs[1] + note->output_logs[2] + note->output_logs[3];

        feedback = synth->feedback_level;
        feedback *= output[0];
        feedback >>= KS_FEEDBACK_LEVEL_BITS;
    }
    if(algorithm ==7)
    {
        // +-----+
        // +-[1]-+ [2]   [3]   [4]
        //    +-----+-----+-----+--->
        output[3] = ks_output( 3, note,0                     );
        output[2] = ks_output( 2, note,0                     );
        output[1] = ks_output( 1, note,0                     );
        output[0] = ks_output( 0, note,note->feedback_log    );
        out = note->output_logs[0] + note->output_logs[1] + note->output_logs[2] + note->output_logs[3];

        feedback = synth->feedback_level;
        feedback *= output[0];
        feedback >>= KS_FEEDBACK_LEVEL_BITS;
    }

    for(u32 i =0; i< KS_NUM_OPERATORS; i++)
    {
        note->output_logs[i] = output[i];
    }
    note->feedback_log = feedback;

    return out >> 1;
}

KS_INLINE static void envelope_next(ks_synth_note* note)
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

KS_FORCEINLINE static void lfo_frame(const ks_synth* synth, ks_synth_note* note, u32 delta[],
                                       bool ams, bool fms)
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

            depth *= note->output_logs[j];
            depth >>= KS_OUTPUT_BITS;
            note->output_logs[j] = (i32)depth;
        }
    }

    if(fms || ams)
    {
        note->lfo_log = note->lfo_func(note);
        note->lfo_phase += note->lfo_delta;
    }
}


KS_FORCEINLINE static void ks_synth_process(const ks_synth* synth, ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len,
                                    u8 algorithm, bool ams, bool fms)
{
    u32 delta[KS_NUM_OPERATORS];

    for(u32 i=0; i<len; i+=2)
    {
        i32 out = synth_frame(synth, note, algorithm) * volume;
        out >>= KS_VOLUME_BITS;
        buf[i] = buf[i+1] = out;
        buf[i]= ks_apply_panpot(buf[i], synth->panpot_left);
        buf[i+1]= ks_apply_panpot(buf[i+1], synth->panpot_right);
        lfo_frame(synth, note, delta,  ams, fms);

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

#define _algorithm_list _(0) _(1) _(2) _(3) _(4) _(5) _(6) _(7)

#define ks_synth_define_synth_render_aw(algorithm) \
void KS_NOINLINE ks_synth_render_ ## algorithm ## _ ## _0_0( ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len){ \
    ks_synth_process(note->synth, note, volume, pitchbend, buf, len, algorithm, false, false); \
} \
void KS_NOINLINE ks_synth_render_ ## algorithm ## _ ##_0_1(ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len) {\
    ks_synth_process(note->synth, note, volume, pitchbend,  buf, len, algorithm, false, true); \
} \
void KS_NOINLINE ks_synth_render_ ## algorithm ## _ ## _1_0( ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len) {\
    ks_synth_process(note->synth, note, volume, pitchbend, buf, len, algorithm, true, false); \
} \
void  KS_NOINLINE ks_synth_render_ ## algorithm ## _ ## _1_1(ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len){ \
    ks_synth_process(note->synth, note, volume, pitchbend, buf, len, algorithm, true, true); \
}


#undef _
#define _(x) ks_synth_define_synth_render_aw(x)
#define split

_algorithm_list

#undef ks_synth_define_synth_render


#define ks_synth_lfo_branch(algorithm) \
    if(synth->lfo_ams_enabled) \
    { \
        if(synth->lfo_fms_enabled) \
        { \
            ks_synth_render_ ## algorithm ## _ ##  _1_1 (note, volume, pitchbend,  buf, len); \
        } \
        else \
        { \
            ks_synth_render_ ## algorithm ## _ ## _1_0 (note, volume, pitchbend, buf, len); \
        } \
    } \
    else \
    { \
        if(synth->lfo_fms_enabled) \
        { \
            ks_synth_render_ ## algorithm ## _ ##  _0_1 (note, volume, pitchbend, buf, len); \
        } \
        else \
        { \
            ks_synth_render_ ## algorithm ## _ ##  _0_0 (note, volume, pitchbend, buf, len); \
        } \
    }

#undef _
#define _(x) case x : ks_synth_lfo_branch(x) break;

void ks_synth_render(ks_synth_note* note, u32 volume, u32 pitchbend, i32 *buf, u32 len)
{
    if(*(u32*)note->envelope_states != 0){
        const ks_synth* synth = note->synth;
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

