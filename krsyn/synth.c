#include "synth.h"

#include <ksio/serial/binary.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

KS_FORCEINLINE bool ks_io_binary_as_array(ks_io* io, const ks_io_methods* methods, void* data, u32 data_size, ks_io_serial_type serial_type){
    ks_value v = ks_val_ptr(data, KS_VALUE_U8);
    ks_array_data arr = ks_prop_arr_data_size_len(data_size, 1, v, true);
    return ks_io_array(io, methods, arr, 0, serial_type);
}

KS_FORCEINLINE i16 ks_io_bit_value(ks_io* io, const ks_io_methods* methods, u8 val, const char* prop_name, ks_io_serial_type serial_type){
    ks_value v = ks_val_ptr(&val, KS_VALUE_U8);
    ks_property p = ks_prop_v(prop_name, v);
    return ks_io_property(io, methods, p, serial_type) ? val : -1;
}


#define ks_bit_u8(prop) { \
    i16 ret = ks_io_bit_value(__IO, __METHODS, ks_access(prop), #prop, __SERIAL_TYPE); \
    if(ret != -1) { \
        ks_access(prop) = ret; \
        __RETURN ++; \
    } \
}


ks_io_begin_custom_func(ks_synth_common_data)
    ks_bit_u8(feedback_level);
    ks_bit_u8(panpot);
    ks_bit_u8(output);
    ks_bit_u8(lfo_fms_depth);
    ks_bit_u8(lfo_use_custom_wave);
    ks_bit_u8(lfo_wave_type);
    ks_bit_u8(lfo_freq);
    ks_bit_u8(lfo_offset);
ks_io_end_custom_func(ks_synth_common_data)



ks_io_begin_custom_func(ks_synth_envelope_data)
    ks_bit_u8(time);
    ks_bit_u8(amp);
ks_io_end_custom_func(ks_synth_envelope_data)

ks_io_begin_custom_func(ks_synth_operator_data)
    ks_bit_u8(mod_type);
    ks_bit_u8(mod_src);
    ks_bit_u8(mod_sync);
    ks_bit_u8(use_custom_wave);
    ks_bit_u8(wave_type);
    ks_bit_u8(fixed_frequency);
    ks_bit_u8(phase_coarse);
    ks_bit_u8(phase_offset);
    ks_bit_u8(phase_fine);
    ks_bit_u8(semitones);
    ks_bit_u8(repeat_envelope);
    ks_bit_u8(repeat_envelope_amp);
    ks_arr_obj(envelopes, ks_synth_envelope_data);
    ks_bit_u8(level);
    ks_bit_u8(ratescale);
    ks_bit_u8(keyscale_mid_point);
    ks_bit_u8(keyscale_left);
    ks_bit_u8(keyscale_right);
    ks_bit_u8(keyscale_low_depth);
    ks_bit_u8(keyscale_high_depth );

    ks_bit_u8(velocity_sens);
    ks_bit_u8(lfo_ams_depth);
ks_io_end_custom_func(ks_synth_operator_data)

ks_io_begin_custom_func(ks_synth_data)
    ks_magic_number("KSYN");
    if(__METHODS->type == KS_SERIAL_BINARY){
        __RETURN += ks_io_binary_as_array(io, __METHODS, __OBJECT, sizeof(ks_synth_data), __SERIAL_TYPE);
    } else {
        ks_arr_obj(operators, ks_synth_operator_data);
        ks_obj(common, ks_synth_common_data);
    }
ks_io_end_custom_func(ks_synth_data)


ks_synth_context* ks_synth_context_new(u32 sampling_rate){
    ks_synth_context *ret = calloc(1, sizeof(ks_synth_context));
    ret->sampling_rate = sampling_rate;
    ret->sampling_rate_inv = ks_1(30) / sampling_rate;

    for(u32 i=0; i< 128; i++){
        ret->ratescales[i] = ks_1(KS_RATESCALE_BITS) - ks_1(KS_RATESCALE_BITS) / exp2((double)i/12);

        u64 freq_30 = 440.0 * exp2(((double)i-69)/12) * ret->sampling_rate_inv;
        ret->note_deltas[i] = freq_30 >> (KS_SAMPLING_RATE_INV_BITS - KS_PHASE_MAX_BITS);
    }

    for(u32 i=0; i< KS_NUM_WAVES; i++){
        ret->wave_tables[i] = malloc(sizeof(i16)* ks_1(KS_TABLE_BITS));
    }

    srand(0);
    for(u32 i=0; i< ks_1(KS_TABLE_BITS); i++){
        ret->wave_tables[KS_WAVE_SIN][i] = sin(2* M_PI * i / ks_1(KS_TABLE_BITS)) * INT16_MAX;
        int p = ks_mask(i + (ks_1(KS_TABLE_BITS-2)), KS_TABLE_BITS);

        ret->wave_tables[KS_WAVE_TRIANGLE][i] = p < ks_1(KS_TABLE_BITS-1) ?
                    -INT16_MAX + (int64_t)INT16_MAX* 2 * p / ks_1(KS_TABLE_BITS-1) :
                INT16_MAX - INT16_MAX * 2 * (int64_t)(p-ks_1(KS_TABLE_BITS-1)) / ks_1(KS_TABLE_BITS-1);

        ret->wave_tables[KS_WAVE_FAKE_TRIANGLE][i] = ret->wave_tables[KS_WAVE_TRIANGLE][i] >> 13 << 13;

        ret->wave_tables[KS_WAVE_SAW_DOWN][i] = INT16_MIN + ks_mask(ks_1(16) * p / ks_1(KS_TABLE_BITS), 16);
        ret->wave_tables[KS_WAVE_SAW_UP][i] = -ret->wave_tables[KS_WAVE_SAW_DOWN][i];

        ret->wave_tables[KS_WAVE_SQUARE][i] = i < ks_1(KS_TABLE_BITS-1) ? INT16_MAX: -INT16_MAX;
        ret->wave_tables[KS_WAVE_NOISE][i] = rand();
    }

    //ret->num_waves = KS_NUM_WAVES;



    return ret;
}

void ks_synth_context_free(ks_synth_context * ctx){
    for(u32 i=0; i< KS_MAX_WAVES; i++){
        if(ctx->wave_tables[i] != NULL) free(ctx->wave_tables[i]);
    }

    free(ctx);
}

ks_synth* ks_synth_new(ks_synth_data* data, const ks_synth_context* ctx){
    ks_synth* ret = calloc(1 , sizeof(ks_synth));
    ks_synth_set(ret, ctx, data);
    return ret;
}

ks_synth* ks_synth_array_new(u32 length, ks_synth_data data[], const ks_synth_context* ctx){
    ks_synth* ret = calloc(length, sizeof(ks_synth));
    for(u32 i=0; i<length; i++){
        ks_synth_set(ret+i, ctx, data+i);
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
        data->operators[i].mod_sync = false;
        data->operators[i].mod_src = 0;
        data->operators[i].mod_type = i== 3 ? KS_MOD_NONE : KS_MOD_PASS;
        data->operators[i].use_custom_wave = 0;
        data->operators[i].wave_type = 0;
        data->operators[i].fixed_frequency = false;
        data->operators[i].phase_coarse = 2;
        data->operators[i].phase_offset = 0;
        data->operators[i].phase_fine = 8;
        data->operators[i].semitones= ks_1(5);
        data->operators[i].level = 127;
        if(i == 3){
            data->operators[i].envelopes[0].time= 0;
            data->operators[i].envelopes[1].time = 31;
            data->operators[i].envelopes[2].time = 31;
            data->operators[i].envelopes[3].time = 14;

            data->operators[i].envelopes[0].amp = 7;
            data->operators[i].envelopes[1].amp= 7;
            data->operators[i].envelopes[2].amp = 7;
            data->operators[i].envelopes[3].amp = 0;
        } else {
            data->operators[i].envelopes[0].time= 0;
            data->operators[i].envelopes[1].time = 0;
            data->operators[i].envelopes[2].time = 0;
            data->operators[i].envelopes[3].time = 0;

            data->operators[i].envelopes[0].amp = 0;
            data->operators[i].envelopes[1].amp= 0;
            data->operators[i].envelopes[2].amp = 0;
            data->operators[i].envelopes[3].amp = 0;
        }

        data->operators[i].ratescale = 0;
        data->operators[i].keyscale_low_depth = 0;
        data->operators[i].keyscale_low_depth = 0;
        data->operators[i].keyscale_mid_point = 17;
        data->operators[i].keyscale_left = KS_KEYSCALE_CURVE_LD;
        data->operators[i].keyscale_right = KS_KEYSCALE_CURVE_LD;

        data->operators[i].velocity_sens = 31;
        data->operators[i].lfo_ams_depth = 0;
    }

    data->common.output = 0;
    data->common.feedback_level = 0;
    data->common.panpot = 7;

    data->common.lfo_use_custom_wave = 0;
    data->common.lfo_wave_type = KS_WAVE_TRIANGLE;
    data->common.lfo_fms_depth = 0;
    data->common.lfo_freq = 0;
    data->common.lfo_offset = 0;
}



KS_INLINE  u64 ks_u2f(u32 val, int num_v){
    u32 v = ks_mask(val, num_v);
    u32 e = val >> num_v;
    v |= 1 << num_v;
    return (u64)(v << e) - (1<<(num_v)); // fixed point (value bit + e)
}

KS_INLINE u32 ks_exp_u(u32 val, u32 base, int num_v)
{
    u32 v = ks_mask(val, num_v);
    u32 e = val >> num_v;
    if(e!= 0){
        e--;
        v |= 1 << num_v;
    }
    i64 ret = (u64)v<< e;
    ret *= base;
    ret >>= 7;
    return ret;
}

KS_INLINE u32 ks_calc_envelope_times(u32 val)
{
    return ks_exp_u(val, 1<<(KS_TIME_BITS-5), 4);
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

 KS_INLINE static i16 ks_sin(const ks_synth_context* ctx, u32 phase){
    return ctx->wave_tables[KS_WAVE_SIN][ks_mask(phase>>(KS_PHASE_BITS), KS_TABLE_BITS)];
}

KS_INLINE void ks_calc_panpot(const ks_synth_context * ctx, i16* left, i16 * right, u8 val){
    *left =  ks_sin(ctx, ks_v(val + ks_v(2, KS_PANPOT_BITS - 1), KS_PHASE_MAX_BITS - KS_PANPOT_BITS - 2 ));
    *right = ks_sin(ctx, ks_v(val + ks_v(0, KS_PANPOT_BITS - 1), KS_PHASE_MAX_BITS - KS_PANPOT_BITS - 2 ));
}

KS_INLINE i32 ks_apply_panpot(i32 in, i16 pan){
    i32 out = in;
    out *= pan;
    out >>= KS_OUTPUT_BITS;
    return out;
}


typedef i32(*ks_mod_func_impl)(ks_wave_func, u8, ks_synth_note*, i32);

KS_FORCEINLINE static i32 ks_wave_func_default(u8 op,  ks_synth_note* note, u32 phase){
    return note->synth->wave_tables[op][ks_mask(phase >> KS_PHASE_BITS, KS_TABLE_BITS)];
}


KS_FORCEINLINE static i32 ks_wave_func_noise(u8 op,  ks_synth_note* note, u32 phase){
    i16 ret = note->synth->wave_tables[op][ks_mask((((phase >> (KS_NOISE_PHASE_BITS)) + note->mod_func_logs[op][0])), KS_TABLE_BITS)];
    // noise
    if(((note->phases[op]) >> (KS_PHASE_BITS)) > ks_1(10)) {
        note->phases[op] -= ks_1(10 + KS_PHASE_BITS);
        //srand((unsigned)note->mod_func_logs[op][0]);
        note->mod_func_logs[op][0] = rand();
    }
    return ret;
}

KS_FORCEINLINE static i32 ks_envelope_apply(u32 amp, i32 in)
{
    i64 out = amp;
    out *=in;
    out >>=KS_ENVELOPE_BITS;
    return out;
}



KS_FORCEINLINE static i32 ks_fm_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note,i32 in){
    return ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op] + (in<<(KS_TABLE_BITS + (KS_PHASE_BITS - KS_OUTPUT_BITS) + 1))));
}




KS_FORCEINLINE static i32 ks_am_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    i64 ret =  ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op]));
    ret += ks_1(KS_OUTPUT_BITS);
    ret *=  in;
    ret >>= KS_OUTPUT_BITS+1;
    return ret;
}



KS_FORCEINLINE static i32 ks_mul_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    i64 ret = ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op]));
    ret *=  in;
    ret >>= KS_OUTPUT_BITS;
    return ret;
}

KS_FORCEINLINE static void ks_sync_process(u8 op, ks_synth_note* note){
    const u8 mod_src = note->synth->mod_srcs[op];
    const u32 src_phase =note->phases[mod_src];
    const u32 src_phase_delta = note->phase_deltas[mod_src];

    if(ks_mask(src_phase, KS_PHASE_MAX_BITS) > ks_mask(src_phase + src_phase_delta, KS_PHASE_MAX_BITS)){
            note->phases[op] = 0;
    }
}

KS_FORCEINLINE static i32 ks_none_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    return ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op]));
}

KS_FORCEINLINE static i32 ks_add_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    i64 ret = ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op]));
    ret +=  in;
    return ret;
}

KS_FORCEINLINE static i32 ks_sub_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    i64 ret = ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op]));
    ret -=  in;
    return ret;
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


static i32 ks_output_lpf(u8 op, ks_synth_note* note, i32 in){
    i64  alpha = ks_1(KS_ENVELOPE_BITS) - MIN(note->envelope_now_amps[op], ks_1(KS_ENVELOPE_BITS));
    alpha >>= KS_ENVELOPE_BITS - KS_OUTPUT_BITS;

    alpha = ks_u2f(alpha, 12 ) >> 9;

    alpha *= note->phase_deltas[op] ;
    alpha >>= KS_PHASE_BITS;
    alpha = MAX(MIN(alpha, ks_1(KS_OUTPUT_BITS)),0);

    i64 out = alpha*(in - note->mod_func_logs[op][0]);
    out >>= KS_OUTPUT_BITS;
    note->mod_func_logs[op][0] += out;

    out = alpha*(note->mod_func_logs[op][0] - note->mod_func_logs[op][1]);
    out >>= KS_OUTPUT_BITS;
    note->mod_func_logs[op][1] += out;

    return note->mod_func_logs[op][1] ;
}


static i32 ks_output_hpf(u8 op, ks_synth_note* note, i32 in){
    i64  alpha=  MIN(note->envelope_now_amps[op], ks_1(KS_ENVELOPE_BITS));
     alpha >>= KS_ENVELOPE_BITS - KS_OUTPUT_BITS;

    alpha = ks_u2f( alpha, 12 ) >> 14; // 9 + 5 (phase coarse)

    alpha *= note->phase_deltas[op];
    alpha >>= KS_PHASE_BITS;
    alpha = MIN(alpha, ks_1(KS_OUTPUT_BITS));

    i64 out = alpha*(in - note->mod_func_logs[op][0]);
    out >>= KS_OUTPUT_BITS;
    note->mod_func_logs[op][0] += out;

    out = alpha*(note->mod_func_logs[op][0] - note->mod_func_logs[op][1]);
    out >>= KS_OUTPUT_BITS;
    note->mod_func_logs[op][1] += out;

    return in - note->mod_func_logs[op][1] ;
}


#define ks_output_mod_func(wave, mod, sync) ks_output_mod_ ## wave ## _ ## mod ## _ ## sync

#define ks_output_mod_syncless_impl(wave, mod) \
    static i32 ks_output_mod_func(wave, mod, false) (u8 op,  ks_synth_note* note, i32 in){ \
        return ks_output_mod_base(mod, wave, op, note, in); \
    }

#define ks_output_mod_impl(wave, mod) \
    static i32 ks_output_mod_func(wave, mod, false) (u8 op,  ks_synth_note* note, i32 in){ \
        return ks_output_mod_base(mod, wave, op, note, in); \
    } \
    static i32 ks_output_mod_func(wave, mod, true) (u8 op,  ks_synth_note* note, i32 in){ \
        i32 ret =  ks_output_mod_base(mod, wave, op, note, in); \
        ks_sync_process(op, note); \
        return ret; \
    }



#define ks_output_wave_impl(mod) \
ks_output_mod_impl(ks_wave_func_default, mod) \
ks_output_mod_impl(ks_wave_func_noise, mod)

#define ks_output_wave_syncless_impl(mod) \
ks_output_mod_syncless_impl(ks_wave_func_default, mod) \
ks_output_mod_syncless_impl(ks_wave_func_noise, mod)

ks_output_wave_impl(ks_fm_mod)
ks_output_wave_impl(ks_mul_mod)
ks_output_wave_impl(ks_add_mod)
ks_output_wave_impl(ks_sub_mod)
ks_output_wave_impl(ks_am_mod)
ks_output_wave_impl(ks_none_mod)


#define ks_mod_sync_branch(func, mod, sync) (sync ? ks_output_mod_func(func, mod, true) : ks_output_mod_func(func, mod, false))

#define ks_output_wave_syncless_branch(mod, wave_type) \
switch (wave_type) { \
case KS_WAVE_NOISE: return ks_output_mod_func(ks_wave_func_noise, mod, false); \
default: return ks_output_mod_func(ks_wave_func_default, mod, false); \
}


#define ks_output_wave_branch(mod, wave_type, sync) \
switch (wave_type) { \
case KS_WAVE_NOISE: return ks_mod_sync_branch(ks_wave_func_noise, mod, sync); \
default: return ks_mod_sync_branch(ks_wave_func_default, mod, sync); \
}

static ks_mod_func ks_get_mod_func(const ks_synth_operator_data* op){
    switch (op->mod_type) {
    case KS_MOD_FM:
        ks_output_wave_branch(ks_fm_mod, op->wave_type, op->mod_sync)
    case KS_MOD_NONE:
        ks_output_wave_branch(ks_none_mod,  op->wave_type, op->mod_sync)
    case KS_MOD_ADD:
        ks_output_wave_branch(ks_add_mod, op->wave_type, op->mod_sync)
    case KS_MOD_SUB:
        ks_output_wave_branch(ks_sub_mod, op->wave_type, op->mod_sync)
    case KS_MOD_MUL:
        ks_output_wave_branch(ks_mul_mod, op->wave_type, op->mod_sync)
    case KS_MOD_AM:
        ks_output_wave_branch(ks_am_mod, op->wave_type, op->mod_sync)
    case KS_MOD_LPF:
        return ks_output_lpf;
    case KS_MOD_HPF:
        return ks_output_hpf;
    case KS_MOD_PASS:
        return ks_output_pass;
    }

    return NULL;
}


static i32 ks_lfo_wave_func_default(ks_synth_note* note){
    return note->synth->lfo_wave_table[ks_mask(note->lfo_phase >> KS_PHASE_BITS, KS_TABLE_BITS)];
}

static i32 ks_lfo_wave_func_noise(ks_synth_note* note){
    i16 ret = note->synth->lfo_wave_table[ks_mask((((note->lfo_phase >> (KS_NOISE_PHASE_BITS)) + note->lfo_func_log)), KS_TABLE_BITS)];
    // noise
    if(((note->lfo_phase) >> KS_NOISE_PHASE_BITS) > ks_1(5)) {
        note->lfo_phase -= ks_1(5 + KS_NOISE_PHASE_BITS);
        srand((unsigned)note->lfo_func_log);
        note->lfo_func_log = rand();
    }
    return ret;
}




KS_INLINE static void synth_op_set(const ks_synth_context* ctx, ks_synth* synth, const ks_synth_data* data)
{
    synth->lfo_enabled.ams= false;
    synth->lfo_enabled.filter = false;

    for(u32 i=0; i<KS_NUM_OPERATORS; i++)
    {
        synth->mod_funcs[i] = ks_get_mod_func( &data->operators[i]);
        synth->wave_tables[i] = ctx->wave_tables[ks_wave_index(data->operators[i].use_custom_wave, data->operators[i].wave_type)];
        if(synth->wave_tables[i] == NULL) {
            ks_error("Failed to set synth for not set wave table %d", ks_wave_index(data->operators[i].use_custom_wave, data->operators[i].wave_type));
            synth->enabled = false;
            return ;
        }
        synth->mod_srcs[i] = data->operators[i].mod_src;
        synth->fixed_frequency[i] = calc_fixed_frequency(data->operators[i].fixed_frequency);
        synth->phase_coarses[i] = calc_phase_coarses(data->operators[i].phase_coarse);
        synth->phase_offsets[i] = calc_phase_offsets(data->operators[i].phase_offset);
        synth->phase_fines[i] = calc_phase_fines(data->operators[i].phase_fine);
        synth->semitones[i] = calc_semitones(data->operators[i].semitones);
        synth->levels[i] = calc_levels(data->operators[i].level);

        synth->repeat_envelopes[i] = data->operators[i].repeat_envelope;
        synth->repeat_envelope_amps[i] = calc_repeat_envelope_amp(data->operators[i].repeat_envelope_amp);
        for(u32 e=0; e< KS_ENVELOPE_NUM_POINTS; e++){
            synth->envelope_samples[e][i] = calc_envelope_samples(ctx->sampling_rate, data->operators[i].envelopes[e].time);
            synth->envelope_points[e][i] = calc_envelope_points(data->operators[i].envelopes[e].amp);
        }

        synth->velocity_sens[i] = calc_velocity_sens(data->operators[i].velocity_sens);
        synth->velocity_base[i] = synth->velocity_sens[i] < 0 ? 0: 127;

        synth->ratescales[i] = calc_ratescales(data->operators[i].ratescale);
        synth->keyscale_low_depths[i] = calc_keyscale_low_depths(data->operators[i].keyscale_low_depth);
        synth->keyscale_high_depths[i] = calc_keyscale_high_depths(data->operators[i].keyscale_high_depth);
        synth->keyscale_mid_points[i] = calc_keyscale_mid_points(data->operators[i].keyscale_mid_point);
        synth->keyscale_curve_types[0][i] = calc_keyscale_curve_types_left(data->operators[i].keyscale_left);
        synth->keyscale_curve_types[1][i] = calc_keyscale_curve_types_right(data->operators[i].keyscale_right);

        synth->lfo_ams_depths[i] = calc_lfo_ams_depths(data->operators[i].lfo_ams_depth);
        synth->lfo_enabled.ams = synth->lfo_enabled.ams ||  data->operators[i].lfo_ams_depth != 0;
        synth->lfo_enabled.filter = synth->lfo_enabled.filter ||
                ((data->operators[i].mod_type == KS_MOD_HPF || data->operators[i].mod_type == KS_MOD_LPF) &&
                 data->operators[i].phase_offset != 0);
    }

}

KS_INLINE static void synth_common_set(const ks_synth_context* ctx, ks_synth* synth, const ks_synth_data* data)
{
    synth->output = calc_output(data->common.output);
    synth->feedback_level = calc_feedback_level(data->common.feedback_level);
    ks_calc_panpot(ctx, &synth->panpot_left, &synth->panpot_right, calc_panpot(data->common.panpot));

    synth->lfo_wave_table = ctx->wave_tables[ks_wave_index(data->common.lfo_use_custom_wave, data->common.lfo_wave_type)];
    if(synth->lfo_wave_table == NULL) {
        ks_error("Failed to set synth for not set wave table %d", ks_wave_index(data->common.lfo_use_custom_wave, data->common.lfo_wave_type));
        synth->enabled = false;
        return ;
    }
    synth->lfo_func = data->common.lfo_wave_type  == KS_WAVE_NOISE ? ks_lfo_wave_func_noise : ks_lfo_wave_func_default;
    synth->lfo_fms_depth = calc_lfo_fms_depth(data->common.lfo_fms_depth);
    synth->lfo_enabled.fms = data->common.lfo_fms_depth != 0;

    const u32 freq = calc_lfo_freq(data->common.lfo_freq);
    const u64 freq_11 = ((u64)freq) << KS_TABLE_BITS;
    u64 delta_11 = (u64)freq_11 * ctx->sampling_rate_inv;
    delta_11 >>= KS_SAMPLING_RATE_INV_BITS-(KS_PHASE_BITS - KS_FREQUENCY_BITS);

    synth->lfo_delta= delta_11;
    synth->lfo_offset =calc_lfo_offset(data->common.lfo_offset);
}


void ks_synth_set(ks_synth* synth, const ks_synth_context* ctx, const ks_synth_data* data)
{
    synth->enabled = true;
    synth_op_set(ctx, synth, data);
    synth_common_set(ctx, synth, data);
}

KS_INLINE static u32 phase_delta_fix_freq(const ks_synth_context* ctx, u32 coarse, u32 tune, i32 fine)
{
    const u32 freq = ks_v(calc_frequency_fixed(coarse), KS_FREQUENCY_BITS);
    const u64 freq_11 = ((u64)freq) << KS_TABLE_BITS;

    const u32 freq_rate =  (ks_1(KS_FREQUENCY_BITS));
    const u32 freq_rate_tuned = ((u64)freq_rate * (ks_1(KS_PHASE_FINE_BITS) + 9*ks_v(tune, KS_PHASE_FINE_BITS - 6) + fine)) >> KS_PHASE_FINE_BITS;

    u64 delta_11 = (u64)freq_11 * ctx->sampling_rate_inv;
    delta_11 >>= KS_SAMPLING_RATE_INV_BITS;
    delta_11 *= freq_rate_tuned;
    delta_11 >>= KS_FREQUENCY_BITS-(KS_PHASE_BITS - KS_FREQUENCY_BITS);

    return (u32)delta_11;
}

KS_INLINE static u32 phase_delta(const ks_synth_context* ctx, u8 notenum, u32 coarse, u32 tune, i32 fine)
{
    const u32 freq = ctx->note_deltas[MAX(MIN(notenum + tune - ks_1(5), 127), 0)]; // heltz << KS_FREQUENCY_BITS

    const u32 freq_rate = (coarse * (ks_1(KS_FREQUENCY_BITS))) >> KS_PHASE_COARSE_BITS;
    const u32 freq_rate_tuned = ((u64)freq_rate * (ks_1(KS_PHASE_FINE_BITS) + fine)) >> KS_PHASE_FINE_BITS;

    u64 delta = freq;
    delta *= freq_rate_tuned;
    delta >>= KS_FREQUENCY_BITS;

    return (u32)delta;
}



KS_FORCEINLINE static i32 keyscale_v(i32 v, u32 d){
    i64 v2 = (i64)v* d;
    v2 >>= KS_KEYSCALE_DEPTH_BITS;
    return v2;
}

KS_INLINE static i32 keyscale(const ks_synth *synth, const ks_synth_context*ctx, u8 notenum, u32 i)
{
    i32 sub;
    u32 depth;
    u8 index;
    // <-- |mid point|
    if(notenum < synth->keyscale_mid_points[i]){
        sub = synth->keyscale_mid_points[i] - notenum;
        depth = synth->keyscale_low_depths[i];
        index = 0;
    }
    // |mid point| -->
    else {
        sub = notenum - synth->keyscale_mid_points[i];
        depth = synth->keyscale_high_depths[i];
        index = 1;
    }

    switch (synth->keyscale_curve_types[index][i]) {
    case KS_KEYSCALE_CURVE_ED:{
        return keyscale_v(sub * ks_1(KS_KEYSCALE_CURVE_BITS) / (sub - 127), depth) + ks_1(KS_KEYSCALE_CURVE_BITS);
    }
    case KS_KEYSCALE_CURVE_EU:{
        return keyscale_v( sub * ks_1(KS_KEYSCALE_CURVE_BITS) / (127 - sub), depth)  + ks_1(KS_KEYSCALE_CURVE_BITS);
    }
    case KS_KEYSCALE_CURVE_LD:{
        return keyscale_v((u64)ctx->note_deltas[0] * ks_1(KS_KEYSCALE_CURVE_BITS) / ctx->note_deltas[((i64)sub * depth)>> KS_KEYSCALE_DEPTH_BITS],  ks_1(KS_KEYSCALE_DEPTH_BITS));
    }
    case KS_KEYSCALE_CURVE_LU:{
        return keyscale_v((u64)ctx->note_deltas[sub] * ks_1(KS_KEYSCALE_CURVE_BITS) / ctx->note_deltas[0], depth)  + ks_1(KS_KEYSCALE_CURVE_BITS); // ?
    }
    }

    return 0;
}

void ks_synth_note_on(ks_synth_note* note, const ks_synth *synth, const ks_synth_context *ctx, u8 notenum, u8 vel)
{
    if(! synth->enabled) return;

    note->synth = synth;
    note->now_frame = 0;

    note->lfo_log = 0;
    note->lfo_phase= synth->lfo_offset;
    note->lfo_delta = synth->lfo_delta;


    for(unsigned i=0; i<KS_NUM_OPERATORS; i++){
        note->phases[i] = synth->phase_offsets[i];
    }


    for(u32 i=0; i< KS_NUM_OPERATORS; i++)
    {
        if(synth->fixed_frequency[i])
        {
            note->phase_deltas[i] = phase_delta_fix_freq(ctx, synth->phase_coarses[i], synth->semitones[i], synth->phase_fines[i]);
        }
        else
        {
            note->phase_deltas[i] = phase_delta(ctx, notenum, synth->phase_coarses[i], synth->semitones[i], synth->phase_fines[i]);
        }

        note->output_logs[i] = 0;

        //rate scale
        i64 ratescales =  synth->ratescales[i];
        ratescales *= ctx->ratescales[notenum];
        ratescales >>= KS_RATESCALE_BITS;
        ratescales = ks_1(KS_RATESCALE_BITS) - ratescales;

        //key scale
        i64 keysc = keyscale(synth, ctx, notenum, i);

        i64 target;
        i32 velocity;

        velocity = synth->velocity_sens[i];
        velocity *=  (i32)synth->velocity_base[i]-vel;
        velocity >>= 7;
        velocity = (1 << KS_VELOCITY_SENS_BITS) -  velocity;

        //envelope
        for(u32 j=0; j < KS_ENVELOPE_NUM_POINTS; j++)
        {


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

        note->envelope_deltas[0][i] = ks_1(KS_ENVELOPE_BITS);
        note->envelope_deltas[0][i] /= ((i32)note->envelope_samples[0][i] << KS_SAMPLE_PER_FRAMES_BITS);
        note->envelope_diffs[0][i] = note->envelope_points[0][i];
        for(u32 j=1; j < KS_ENVELOPE_NUM_POINTS; j++)
        {
            i32 sub = note->envelope_points[j][i] - note->envelope_points[j-1][i];
            note->envelope_diffs[j][i] = sub;
            note->envelope_deltas[j][i] =  ks_1(KS_ENVELOPE_BITS);
            note->envelope_deltas[j][i] /=  ((i32)note->envelope_samples[j][i] << KS_SAMPLE_PER_FRAMES_BITS);
        }

        //envelope state init
        note->envelope_now_amps[i] = 0;
        note->envelope_now_point_amps[i] = note->envelope_points[0][i];
        note->envelope_now_times[i] = note->envelope_samples[0][i];
        note->envelope_now_deltas[i] = note->envelope_deltas[0][i];
        note->envelope_now_diffs[i] = note->envelope_diffs[0][i];
        note->envelope_now_remains[i] = ks_1(KS_ENVELOPE_BITS);
        note->envelope_now_points[i] = 0;
        note->envelope_states[i] = KS_ENVELOPE_ON;
        note->mod_func_logs[i][0] = 0;
        note->mod_func_logs[i][1] = 0;
    }
}

void ks_synth_note_off (ks_synth_note* note)
{
    for(u32 i=0; i< KS_NUM_OPERATORS; i++)
    {
        note->envelope_now_times[i] = note->envelope_samples[KS_ENVELOPE_RELEASE_INDEX][i];
        note->envelope_now_points[i] = KS_ENVELOPE_RELEASE_INDEX;
        note->envelope_states[i] = KS_ENVELOPE_RELEASED;

        note->envelope_now_remains[i] = ks_1(KS_ENVELOPE_BITS);
        i32 sub = note->envelope_points[KS_ENVELOPE_RELEASE_INDEX][i] - note->envelope_now_amps[i];
        note->envelope_now_diffs[i] =  sub;
        note->envelope_now_deltas[i] = note->envelope_deltas[KS_ENVELOPE_RELEASE_INDEX][i];
        note->envelope_now_point_amps[i] =  note->envelope_points[KS_ENVELOPE_RELEASE_INDEX][i];
    }
}



KS_FORCEINLINE static i32 synth_frame(const ks_synth* synth, ks_synth_note* note, u8 output_op)
{
    i32 out = 0;
    i32 feedback = note->output_logs[synth->mod_srcs[0]];
    i32 outputs[KS_NUM_OPERATORS] = { 0 };

    feedback *= synth->feedback_level;
    feedback >>= KS_FEEDBACK_LEVEL_BITS;

    outputs[0] = synth->mod_funcs[0](0, note, feedback);
    outputs[1] = synth->mod_funcs[1](1, note, note->output_logs[synth->mod_srcs[1]]);
    outputs[2] = synth->mod_funcs[2](2, note, note->output_logs[synth->mod_srcs[2]]);
    outputs[3] = synth->mod_funcs[3](3, note, note->output_logs[synth->mod_srcs[3]]);

    feedback *= synth->feedback_level;
    feedback >>= KS_FEEDBACK_LEVEL_BITS;

    if(output_op == 0)
    {
        out = outputs[3];
    }
    if(output_op == 1)
    {
        out = outputs[3];
        out += outputs[2];
    }
    if(output_op == 2)
    {
        out = outputs[3];
        out += outputs[2];
        out += outputs[1];

    }
    if(output_op == 3)
    {
        out = outputs[3];
        out += outputs[2];
        out += outputs[1];
        out += outputs[0];

    }

    for(u32 i =0; i< KS_NUM_OPERATORS; i++)
    {
        note->output_logs[i] = outputs[i];
    }
    return out >> 2;
}

KS_FORCEINLINE static void lfo_frame(const ks_synth* synth, ks_synth_note* note, u32 delta[], bool ams, bool fms)
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
            depth += ks_1(KS_OUTPUT_BITS);     // 0 ~ 2.0

            depth *= note->envelope_now_amps[j];
            depth >>= KS_OUTPUT_BITS;
            note->envelope_now_amps[j] = (i32)depth;
        }
    }

    if(ams || fms)
    {
        note->lfo_log = synth->lfo_func(note);
        note->lfo_phase += note->lfo_delta;
    }
}


KS_FORCEINLINE static void ks_synth_envelope_process(ks_synth_note* note){
    for(u32 j=0; j<KS_NUM_OPERATORS; j++)
    {
        note->envelope_now_remains[j] -= note->envelope_now_deltas[j];
        i64 amp = note->envelope_now_remains[j];
        amp = (ks_u2f(amp, 28)) >> 2;
        amp *=note->envelope_now_diffs[j];
        amp >>= KS_ENVELOPE_BITS;
        note->envelope_now_amps[j] = note->envelope_now_point_amps[j] - amp;
    }

    if(ks_mask(note->now_frame, KS_SAMPLE_PER_FRAMES_BITS) == 0)
    {
        for(u32 i=0; i<KS_NUM_OPERATORS; i++)
        {
            if(--note->envelope_now_times[i] <= 0)
            {
                const ks_synth* synth = note->synth;

                note->envelope_now_remains[i] = ks_1(KS_ENVELOPE_BITS);
                switch(note->envelope_states[i])
                {
                case KS_ENVELOPE_ON:
                    if(note->envelope_now_points[i] == KS_ENVELOPE_RELEASE_INDEX-1)
                    {
                        if(synth->repeat_envelopes[i]){
                            note->envelope_now_points[i] =0;
                            note->envelope_now_deltas[i] = note->envelope_deltas[0][i];
                            note->envelope_now_times[i] = note->envelope_samples[0][i];
                            note->envelope_now_diffs[i] = note->envelope_diffs[0][i];

                            for(u32 e =0; e<KS_ENVELOPE_NUM_POINTS-1; e++){
                                u64 a = note->envelope_points[e][i];
                                a *= synth->repeat_envelope_amps[i];
                                a >>= KS_ENVELOPE_BITS;
                                note->envelope_points[e][i] = a;

                                a = note->envelope_diffs[e][i];
                                a *= synth->repeat_envelope_amps[i];
                                a >>= KS_ENVELOPE_BITS;
                                note->envelope_diffs[e][i] = a;
                            }
                        } else {
                            note->envelope_states[i] = KS_ENVELOPE_SUSTAINED;
                            note->envelope_now_deltas[i] = 0;
                            note->envelope_now_diffs[i] = 0;
                            note->envelope_now_times[i] =-1;
                        }
                    } else{
                        note->envelope_now_points[i] ++;
                        note->envelope_now_deltas[i] = note->envelope_deltas[note->envelope_now_points[i]][i];
                        note->envelope_now_times[i] = note->envelope_samples[note->envelope_now_points[i]][i];
                        note->envelope_now_diffs[i] = note->envelope_diffs[note->envelope_now_points[i]][i];
                    }
                    note->envelope_now_point_amps[i] =  note->envelope_points[note->envelope_now_points[i]][i];
                    break;
                case KS_ENVELOPE_RELEASED:
                    note->envelope_now_deltas[i] = 0;
                    note->envelope_now_diffs[i] = 0;
                    note->envelope_now_point_amps[i] =  note->envelope_points[note->envelope_now_points[i]][i];
                    note->envelope_states[i] = KS_ENVELOPE_OFF;
                    break;
                }
            }
        }
    }
}

KS_FORCEINLINE static void ks_synth_process(const ks_synth* synth, ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len, u8 output, bool ams, bool fms)
{
    // NOTE: for LFO, each outputs delay 1 frame.

    u32 delta[KS_NUM_OPERATORS];

    for(u32 i=0; i<len; i+=2)
    {
        ks_synth_envelope_process(note);
        lfo_frame(synth, note, delta,  ams, fms);

        for(u32 j=0; j<KS_NUM_OPERATORS; j++)
        {
            u64 d = delta[j];
            d*= pitchbend;
            d >>= KS_LFO_DEPTH_BITS;

            note->phases[j] = note->phases[j] + d;
        }
        i32 out = synth_frame(synth, note, output) * volume;
        out >>= KS_VOLUME_BITS;
        buf[i] = buf[i+1] = out;
        buf[i]= ks_apply_panpot(buf[i], synth->panpot_left);
        buf[i+1]= ks_apply_panpot(buf[i+1], synth->panpot_right);



        note->now_frame ++;
    }
}

#define _output_list _(0) _(1) _(2) _(3)

#define ks_synth_define_synth_render_aw(output) \
void KS_NOINLINE ks_synth_render_ ## output ## _ ## _0_0( ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len){ \
    ks_synth_process(note->synth, note, volume, pitchbend, buf, len, output, false, false); \
} \
void KS_NOINLINE ks_synth_render_ ## output ## _ ##_0_1(ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len) {\
    ks_synth_process(note->synth, note, volume, pitchbend,  buf, len, output, false, true); \
} \
void KS_NOINLINE ks_synth_render_ ## output ## _ ## _1_0( ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len) {\
    ks_synth_process(note->synth, note, volume, pitchbend, buf, len, output, true, false); \
} \
void  KS_NOINLINE ks_synth_render_ ## output ## _ ## _1_1(ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len){ \
    ks_synth_process(note->synth, note, volume, pitchbend, buf, len, output, true, true); \
}


#undef _
#define _(x) ks_synth_define_synth_render_aw(x)
#define split

_output_list

#undef ks_synth_define_synth_render


#define ks_synth_lfo_branch(output) \
    if(synth->lfo_enabled.ams) \
    { \
        if(synth->lfo_enabled.fms) \
        { \
            ks_synth_render_ ## output ## _ ##  _1_1 (note, volume, pitchbend,  buf, len); \
        } \
        else \
        { \
            ks_synth_render_ ## output ## _ ## _1_0 (note, volume, pitchbend, buf, len); \
        } \
    } \
    else \
    { \
        if(synth->lfo_enabled.fms) \
        { \
            ks_synth_render_ ## output ## _ ##  _0_1 (note, volume, pitchbend, buf, len); \
        } \
        else \
        { \
            ks_synth_render_ ## output ## _ ##  _0_0 (note, volume, pitchbend, buf, len); \
        } \
    }

#undef _
#define _(x) case x : ks_synth_lfo_branch(x) break;

void ks_synth_render(ks_synth_note* note, u32 volume, u32 pitchbend, i32 *buf, u32 len)
{
    if(*(u32*)note->envelope_states != 0){
        const ks_synth* synth = note->synth;
        switch(synth->output)
        {
            _output_list
        }
    } else {
        memset(buf, 0, len*sizeof(i32));
    }
}


#undef ks_synth_lfo_branch
#undef _

