#include "synth.h"

#include <ksio/serial/binary.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

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
    ks_bit_u8(wave_type);
    ks_bit_u8(fixed_frequency);
    ks_bit_u8(phase_coarse);
    ks_bit_u8(phase_offset);
    ks_bit_u8(phase_fine);
    ks_bit_u8(phase_detune);
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
    if((__SERIAL_TYPE == KS_IO_DESERIALIZER &&(__METHODS == &binary_big_endian_deserializer || __METHODS == &binary_little_endian_deserializer))||
        (__SERIAL_TYPE == KS_IO_SERIALIZER &&(__METHODS == &binary_big_endian_serializer || __METHODS == &binary_little_endian_serializer))){
        __RETURN += ks_io_binary_as_array(io, __METHODS, __OBJECT, sizeof(ks_synth_data), __SERIAL_TYPE);
    } else {
        ks_arr_obj(operators, ks_synth_operator_data);
        ks_obj(common, ks_synth_common_data);
    }
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
        data->operators[i].mod_sync = false;
        data->operators[i].mod_src = ks_mask(i-1,3);
        data->operators[i].mod_type = 0;
        data->operators[i].wave_type = 0;
        data->operators[i].fixed_frequency = false;
        data->operators[i].phase_coarse = 2;
        data->operators[i].phase_offset = 0;
        data->operators[i].phase_fine = 8;
        data->operators[i].phase_detune= 0;
        data->operators[i].level = 127;

        data->operators[i].envelopes[0].time= 0;
        data->operators[i].envelopes[1].time = 31;
        data->operators[i].envelopes[2].time = 31;
        data->operators[i].envelopes[3].time = 8;

        data->operators[i].envelopes[0].amp = 7;
        data->operators[i].envelopes[1].amp= 7;
        data->operators[i].envelopes[2].amp = 7;
        data->operators[i].envelopes[3].amp = 0;

        data->operators[i].ratescale = 1;
        data->operators[i].keyscale_low_depth = 0;
        data->operators[i].keyscale_low_depth = 0;
        data->operators[i].keyscale_mid_point = 17;
        data->operators[i].keyscale_left = KS_KEYSCALE_CURVE_LD;
        data->operators[i].keyscale_right = KS_KEYSCALE_CURVE_LD;

        data->operators[i].velocity_sens = 15;
        data->operators[i].lfo_ams_depth = 0;
    }

    data->common.output = 0;
    data->common.feedback_level = 0;
    data->common.panpot = 7;

    data->common.lfo_wave_type = KS_WAVE_TRIANGLE;
    data->common.lfo_fms_depth = 0;
    data->common.lfo_freq = 0;
    data->common.lfo_offset = 0;
}

KS_INLINE  bool  ks_synth_note_is_enabled     (const ks_synth_note* note){
    return *((u32*)note->envelope_states) != 0;
}

KS_INLINE bool ks_synth_note_is_on (const ks_synth_note* note){
    return (*((u32*)note->envelope_states) & 0x0f0f0f0f) != 0;
}

KS_INLINE  u64 ks_u2f(u32 val, int num_v){
    u32 v = ks_mask(val, num_v);
    u32 e = val >> num_v;
    if(e!= 0){
        e--;
        v |= 1 << num_v;
    }
    return (u64)v << e; // fixed point (value bit + e)
}

KS_INLINE u32 ks_exp_u(u32 val, u32 base, int num_v_bit)
{
    u64 ret = ks_u2f(val, num_v_bit);
    ret *= base;
    ret >>= 7;
    return ret;
}

KS_INLINE u32 ks_calc_envelope_times(u32 val)
{
    return ks_exp_u(val, 1<<(KS_TIME_BITS-6), 4);
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
    out >>=KS_ENVELOPE_BITS;
    return out;
}



KS_FORCEINLINE static i16 ks_fm_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note,i32 in){
    return ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op] + (in<<(KS_TABLE_BITS + (KS_PHASE_BITS - KS_OUTPUT_BITS) + 2))));
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



KS_FORCEINLINE static i16 ks_mul_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
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

KS_FORCEINLINE static i16 ks_none_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    return ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op]));
}

KS_FORCEINLINE static i16 ks_add_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
    i64 ret = ks_envelope_apply(note->envelope_now_amps[op], wave_func(op, note, note->phases[op]));
    ret +=  in;
    return ret;
}

KS_FORCEINLINE static i16 ks_sub_mod(ks_wave_func wave_func, u8 op, ks_synth_note* note, i32 in){
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

KS_FORCEINLINE static i32 ks_output_lpf_base(u8 op, ks_synth_note* note, i32 in, bool lfo){
    i64 out=  note->envelope_now_amps[op] >> (KS_ENVELOPE_BITS - KS_OUTPUT_BITS);
    if(lfo){
        const i32 lfo_factor = note->synth->phase_offsets[op] >> (KS_PHASE_MAX_BITS - KS_OUTPUT_BITS);
        const i32 lfo_level = (note->lfo_log * lfo_factor) >> KS_OUTPUT_BITS;
        out += lfo_level; // should lfo be influenced to envelope ?
    }
    out = ks_1(KS_OUTPUT_BITS) - out;
    out = ks_u2f(out, 12) >> 8;
    out *= note->phase_deltas[op];
    out >>= KS_PHASE_BITS; // +- 5 octave of note key ???
    out = MIN(out, ks_1(KS_OUTPUT_BITS));


    out = out*(in - note->mod_func_logs[op]);
    out >>= KS_OUTPUT_BITS;
    note->mod_func_logs[op] += out;

    return note->mod_func_logs[op] ;
}

KS_FORCEINLINE static i32 ks_output_hpf_base(u8 op, ks_synth_note* note, i32 in, bool lfo){
    i64 out=  note->envelope_now_amps[op] >> (KS_ENVELOPE_BITS - KS_OUTPUT_BITS);
    if(lfo){
        const i32 lfo_factor = note->synth->phase_offsets[op] >> (KS_PHASE_MAX_BITS - KS_OUTPUT_BITS);
        const i32 lfo_level = (note->lfo_log * lfo_factor) >> KS_OUTPUT_BITS;
        out += lfo_level;
    }
    out = ks_u2f(out, 12) >> 8;
    out *= note->phase_deltas[op];
    out >>= KS_PHASE_BITS; // +- 5 octave of note key ???
    out = MIN(out, ks_1(KS_OUTPUT_BITS));

    out = out*(in - note->mod_func_logs[op]);
    out >>= KS_OUTPUT_BITS;
    note->mod_func_logs[op] += out;

    return in - note->mod_func_logs[op] ;
}

static i32 ks_output_lpf(u8 op, ks_synth_note* note, i32 in){
     return ks_output_lpf_base(op, note, in, false);
}

static i32 ks_output_lpf_f(u8 op, ks_synth_note* note, i32 in){
     return ks_output_lpf_base(op, note, in, true);
}

static i32 ks_output_hpf(u8 op, ks_synth_note* note, i32 in){
    return ks_output_hpf_base(op, note, in, false);
}

static i32 ks_output_hpf_f(u8 op, ks_synth_note* note, i32 in){
    return ks_output_hpf_base(op, note, in, true);
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
ks_output_mod_impl(ks_wave_func_minus, mod) \
ks_output_mod_impl(ks_wave_func_square, mod) \
ks_output_mod_impl(ks_wave_func_noise, mod)

#define ks_output_wave_syncless_impl(mod) \
ks_output_mod_syncless_impl(ks_wave_func_default, mod) \
ks_output_mod_syncless_impl(ks_wave_func_minus, mod) \
ks_output_mod_syncless_impl(ks_wave_func_square, mod) \
ks_output_mod_syncless_impl(ks_wave_func_noise, mod)

ks_output_wave_impl(ks_fm_mod)
ks_output_wave_impl(ks_mix_mod)
ks_output_wave_impl(ks_mul_mod)
ks_output_wave_impl(ks_add_mod)
ks_output_wave_impl(ks_sub_mod)
ks_output_wave_impl(ks_am_mod)
ks_output_wave_impl(ks_none_mod)


#define ks_mod_sync_branch(func, mod, sync) (sync ? ks_output_mod_func(func, mod, true) : ks_output_mod_func(func, mod, false))

#define ks_output_wave_syncless_branch(mod, wave_type) \
switch (wave_type) { \
case KS_WAVE_SIN: \
case KS_WAVE_SAW_UP: \
case KS_WAVE_TRIANGLE: return ks_output_mod_func(ks_wave_func_default, mod, false); \
case KS_WAVE_SAW_DOWN:return ks_output_mod_func(ks_wave_func_minus, mod, false); \
case KS_WAVE_SQUARE: return ks_output_mod_func(ks_wave_func_square, mod, false); \
case KS_WAVE_NOISE: return ks_output_mod_func(ks_wave_func_noise, mod, false); \
}


#define ks_output_wave_branch(mod, wave_type, sync) \
switch (wave_type) { \
case KS_WAVE_SIN: \
case KS_WAVE_SAW_UP: \
case KS_WAVE_TRIANGLE: return ks_mod_sync_branch(ks_wave_func_default, mod, sync); \
case KS_WAVE_SAW_DOWN:return ks_mod_sync_branch(ks_wave_func_minus, mod, sync); \
case KS_WAVE_SQUARE: return ks_mod_sync_branch(ks_wave_func_square, mod, sync); \
case KS_WAVE_NOISE: return ks_mod_sync_branch(ks_wave_func_noise, mod, sync); \
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
    case KS_MOD_MIX:
        ks_output_wave_branch(ks_mix_mod, op->wave_type, op->mod_sync)
    case KS_MOD_MUL:
        ks_output_wave_branch(ks_mul_mod, op->wave_type, op->mod_sync)
    case KS_MOD_AM:
        ks_output_wave_branch(ks_am_mod, op->wave_type, op->mod_sync)
    case KS_MOD_LPF:
        return op->phase_offset!=0 ? ks_output_lpf_f : ks_output_lpf;
    case KS_MOD_HPF:
        return op->phase_offset!=0 ? ks_output_hpf_f : ks_output_hpf;
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
    synth->lfo_enabled.ams= false;
    synth->lfo_enabled.filter = false;
    for(u32 i=0; i<KS_NUM_OPERATORS; i++)
    {
        synth->mod_funcs[i] = ks_get_mod_func( &data->operators[i]);
        synth->wave_types[i] = data->operators[i].wave_type;
        synth->mod_srcs[i] = data->operators[i].mod_src;
        synth->fixed_frequency[i] = calc_fixed_frequency(data->operators[i].fixed_frequency);
        synth->phase_coarses[i] = calc_phase_coarses(data->operators[i].phase_coarse);
        synth->phase_offsets[i] = calc_phase_offsets(data->operators[i].phase_offset);
        synth->phase_fines[i] = calc_phase_fines(data->operators[i].phase_fine);
        synth->phase_tunes[i] = calc_phase_tunes(data->operators[i].phase_detune);
        synth->levels[i] = calc_levels(data->operators[i].level);

        for(u32 e=0; e< KS_ENVELOPE_NUM_POINTS; e++){
            synth->envelope_samples[e][i] = calc_envelope_samples(sampling_rate, data->operators[i].envelopes[e].time);
            synth->envelope_points[e][i] = calc_envelope_points(data->operators[i].envelopes[e].amp);
        }

        synth->velocity_sens[i] = calc_velocity_sens(data->operators[i].velocity_sens);

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

KS_INLINE static void synth_common_set(ks_synth* synth, const ks_synth_data* data)
{
    synth->output = calc_output(data->common.output);
    synth->feedback_level = calc_feedback_level(data->common.feedback_level);
    ks_calc_panpot(&synth->panpot_left, &synth->panpot_right, data->common.panpot << (7-4));

    synth->lfo_wave_type = calc_lfo_wave_type(data->common.lfo_wave_type);
    synth->lfo_fms_depth = calc_lfo_fms_depth(data->common.lfo_fms_depth);
    synth->lfo_enabled.fms = data->common.lfo_fms_depth != 0;
    synth->lfo_freq = calc_lfo_freq(data->common.lfo_freq);
    synth->lfo_offset =calc_lfo_offset(ks_v(data->common.lfo_offset, (8-4)));
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
    u64 freq_11 = ((u64)freq) << (KS_PHASE_BITS - KS_FREQUENCY_BITS + KS_TABLE_BITS);
    u64 delta_11 = (u32)(freq_11 / sampling_rate);

    return (u32)delta_11;
}

KS_INLINE static u32 phase_delta_fix_freq(u32 sampling_rate, u32 coarse, u32 tune, i32 fine)
{
    const u32 freq = ks_v(calc_frequency_fixed(coarse), KS_FREQUENCY_BITS);
    const u64 freq_11 = ((u64)freq) << KS_TABLE_BITS;

    const u32 freq_rate =  (ks_1(KS_FREQUENCY_BITS));
    const u32 freq_rate_tuned = ((u64)freq_rate * (ks_1(KS_PHASE_FINE_BITS) + 9 * tune + fine)) >> KS_PHASE_FINE_BITS;

    u64 delta_11 = (u32)(freq_11 / sampling_rate);
    delta_11 *= freq_rate_tuned;
    delta_11 >>= KS_FREQUENCY_BITS-(KS_PHASE_BITS - KS_FREQUENCY_BITS);

    return (u32)delta_11;
}

KS_INLINE static u32 phase_delta(u32 sampling_rate, u8 notenum, u32 coarse, u32 tune, i32 fine)
{
    const u32 freq = ks_notefreq(notenum); // heltz << KS_FREQUENCY_BITS
    const u64 freq_11 = ((u64)freq) << KS_TABLE_BITS;

    const u32 freq_rate = (coarse * (ks_1(KS_FREQUENCY_BITS))) >> KS_PHASE_COARSE_BITS;
    const u32 freq_rate_tuned = ((u64)freq_rate * (ks_1(KS_PHASE_FINE_BITS) + tune + fine)) >> KS_PHASE_FINE_BITS;

    u64 delta_11 = (u32)(freq_11 / sampling_rate);
    delta_11 *= freq_rate_tuned;
    delta_11 >>= KS_FREQUENCY_BITS-(KS_PHASE_BITS - KS_FREQUENCY_BITS);

    return (u32)delta_11;
}



KS_FORCEINLINE static i32 keyscale_v(i32 v, u32 d){
    i64 v2 = (i64)v* d;
    v2 >>= KS_KEYSCALE_DEPTH_BITS;
    return v2;
}

KS_INLINE static i32 keyscale(const ks_synth *synth, u8 notenum, u32 i)
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
        return keyscale_v((u64)ks_notefreq(0) * ks_1(KS_KEYSCALE_CURVE_BITS) / ks_notefreq(((i64)sub * depth)>> KS_KEYSCALE_DEPTH_BITS),  ks_1(KS_KEYSCALE_DEPTH_BITS));
    }
    case KS_KEYSCALE_CURVE_LU:{
        return keyscale_v((u64)ks_notefreq(sub) * ks_1(KS_KEYSCALE_CURVE_BITS) / ks_notefreq(0), depth)  + ks_1(KS_KEYSCALE_CURVE_BITS); // ?
    }
    }

    return 0;
}

void ks_synth_note_on( ks_synth_note* note, const ks_synth *synth, u32 sampling_rate, u8 notenum, u8 vel)
{
    note->synth = synth;
    note->now_frame = 0;

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

        note->envelope_deltas[0][i] = ks_1(KS_ENVELOPE_BITS);
        note->envelope_deltas[0][i] /= ((i32)note->envelope_samples[0][i] << KS_SAMPLE_PER_FRAMES_BITS);
        note->envelope_point_subs[0][i] = note->envelope_points[0][i];
        for(u32 j=1; j < KS_ENVELOPE_NUM_POINTS; j++)
        {
            i32 sub = note->envelope_points[j][i] - note->envelope_points[j-1][i];
            note->envelope_point_subs[j][i] = sub;
            note->envelope_deltas[j][i] =  ks_1(KS_ENVELOPE_BITS);
            note->envelope_deltas[j][i] /=  ((i32)note->envelope_samples[j][i] << KS_SAMPLE_PER_FRAMES_BITS);
        }

        //envelope state init
        note->envelope_now_amps[i] = 0;
        note->envelope_now_times[i] = note->envelope_samples[0][i];
        note->envelope_now_deltas[i] = note->envelope_deltas[0][i];
        note->envelope_now_diff[i] = note->envelope_point_subs[0][i];
        note->envelope_now_remains[i] = ks_1(KS_ENVELOPE_BITS);
        note->envelope_now_points[i] = 0;
        note->envelope_states[i] = KS_ENVELOPE_ON;

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
        note->envelope_now_diff[i] =  sub;
        note->envelope_now_deltas[i] = note->envelope_deltas[KS_ENVELOPE_RELEASE_INDEX][i];
        note->envelope_now_point_amps[i] =  note->envelope_points[note->envelope_now_points[KS_ENVELOPE_RELEASE_INDEX]][i];
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
        out = note->output_logs[3];
    }
    if(output_op == 1)
    {
        out = note->output_logs[3];
        out += note->output_logs[2];
    }
    if(output_op == 2)
    {
        out = note->output_logs[3];
        out += note->output_logs[2];
        out += note->output_logs[1];

    }
    if(output_op == 3)
    {
        out = note->output_logs[3];
        out += note->output_logs[2];
        out += note->output_logs[1];
        out += note->output_logs[0];

    }

    for(u32 i =0; i< KS_NUM_OPERATORS; i++)
    {
        note->output_logs[i] = outputs[i];
    }
    return out >> 2;
}

KS_FORCEINLINE static void lfo_frame(const ks_synth* synth, ks_synth_note* note, u32 delta[], bool ams, bool fms, bool lfo)
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
            depth += 1<<(KS_OUTPUT_BITS);     // 0 ~ 2.0


            depth *= note->output_logs[j];
            depth >>= KS_LFO_DEPTH_BITS;
            note->output_logs[j] = (i32)depth;
        }
    }

    if(lfo)
    {
        note->lfo_log = note->lfo_func(note);
        note->lfo_phase += note->lfo_delta;
    }
}

KS_FORCEINLINE static void ks_synth_envelope_process(ks_synth_note* note){
    for(u32 j=0; j<KS_NUM_OPERATORS; j++)
    {
        note->envelope_now_remains[j] -= note->envelope_now_deltas[j];
        i64 amp = note->envelope_now_remains[j] >> 1;

        amp = (ks_u2f(amp, 27) );
        amp *=note->envelope_now_diff[j];
        amp >>= KS_ENVELOPE_BITS;
        note->envelope_now_amps[j] = note->envelope_now_point_amps[j] - amp;
    }

    if(ks_mask(note->now_frame, KS_SAMPLE_PER_FRAMES_BITS) == 0)
    {
        for(u32 i=0; i<KS_NUM_OPERATORS; i++)
        {
            if(--note->envelope_now_times[i] <= 0)
            {
                note->envelope_now_remains[i] = ks_1(KS_ENVELOPE_BITS);
                switch(note->envelope_states[i])
                {
                case KS_ENVELOPE_ON:
                    note->envelope_now_points[i] ++;
                    if(note->envelope_now_points[i] == KS_ENVELOPE_RELEASE_INDEX)
                    {
                        note->envelope_now_points[i] --;
                        note->envelope_states[i] = KS_ENVELOPE_SUSTAINED;
                        note->envelope_now_deltas[i] = 0;
                        note->envelope_now_diff[i] = 0;
                        note->envelope_now_times[i] =-1;
                    } else{
                        note->envelope_now_deltas[i] = note->envelope_deltas[note->envelope_now_points[i]][i];
                        note->envelope_now_times[i] = note->envelope_samples[note->envelope_now_points[i]][i];
                        note->envelope_now_diff[i] = note->envelope_point_subs[note->envelope_now_points[i]][i];
                    }
                    note->envelope_now_point_amps[i] =  note->envelope_points[note->envelope_now_points[i]][i];
                    break;
                case KS_ENVELOPE_RELEASED:
                    note->envelope_now_deltas[i] = 0;
                    note->envelope_now_diff[i] = 0;
                    note->envelope_now_point_amps[i] =  note->envelope_points[note->envelope_now_points[KS_ENVELOPE_RELEASE_INDEX]][i];
                    note->envelope_states[i] = KS_ENVELOPE_OFF;
                    break;
                }
            }
        }
    }
}
KS_FORCEINLINE static void ks_synth_process(const ks_synth* synth, ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len, u8 output, bool ams, bool fms, bool lfo)
{
    u32 delta[KS_NUM_OPERATORS];

    for(u32 i=0; i<len; i+=2)
    {
        i32 out = synth_frame(synth, note, output) * volume;
        out >>= KS_VOLUME_BITS;
        buf[i] = buf[i+1] = out;
        buf[i]= ks_apply_panpot(buf[i], synth->panpot_left);
        buf[i+1]= ks_apply_panpot(buf[i+1], synth->panpot_right);
        lfo_frame(synth, note, delta,  ams, fms, lfo);

        for(u32 j=0; j<KS_NUM_OPERATORS; j++)
        {
            u64 d = delta[j];
            d*= pitchbend;
            d >>= KS_LFO_DEPTH_BITS;

            note->phases[j] = note->phases[j] + d;
        }


        ks_synth_envelope_process(note);


        note->now_frame ++;
    }
}

#define _output_list _(0) _(1) _(2) _(3)

#define ks_synth_define_synth_render_aw(output) \
void KS_NOINLINE ks_synth_render_ ## output ## _ ## _0_0_0( ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len){ \
    ks_synth_process(note->synth, note, volume, pitchbend, buf, len, output, false, false, false); \
} \
void KS_NOINLINE ks_synth_render_ ## output ## _ ## _0_0_1( ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len){ \
    ks_synth_process(note->synth, note, volume, pitchbend, buf, len, output, false, false, true); \
} \
void KS_NOINLINE ks_synth_render_ ## output ## _ ##_0_1_1(ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len) {\
    ks_synth_process(note->synth, note, volume, pitchbend,  buf, len, output, false, true, true); \
} \
void KS_NOINLINE ks_synth_render_ ## output ## _ ## _1_0_1( ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len) {\
    ks_synth_process(note->synth, note, volume, pitchbend, buf, len, output, true, false, true); \
} \
void  KS_NOINLINE ks_synth_render_ ## output ## _ ## _1_1_1(ks_synth_note* note, u32 volume, u32 pitchbend, i32* buf, u32 len){ \
    ks_synth_process(note->synth, note, volume, pitchbend, buf, len, output, true, true, true); \
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
            ks_synth_render_ ## output ## _ ##  _1_1_1 (note, volume, pitchbend,  buf, len); \
        } \
        else \
        { \
            ks_synth_render_ ## output ## _ ## _1_0_1 (note, volume, pitchbend, buf, len); \
        } \
    } \
    else \
    { \
        if(synth->lfo_enabled.fms) \
        { \
            ks_synth_render_ ## output ## _ ##  _0_1_1 (note, volume, pitchbend, buf, len); \
        } \
        else \
        { \
            if(synth->lfo_enabled.filter) ks_synth_render_ ## output ## _ ##  _0_0_1 (note, volume, pitchbend, buf, len); \
            else ks_synth_render_ ## output ## _ ##  _0_0_0 (note, volume, pitchbend, buf, len); \
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

