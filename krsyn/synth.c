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
        ks_access(prop) = (u8)ret; \
        __RETURN ++; \
    } \
}


ks_io_begin_custom_func(ks_synth_common_data)
    ks_bit_u8(amp_ratescale);
    ks_arr_obj(envelopes[0], ks_synth_envelope_data);
    ks_bit_u8(filter_ratescale);
    ks_arr_obj(envelopes[1], ks_synth_envelope_data);
    ks_bit_u8(panpot);
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
    ks_bit_u8(use_custom_wave);
    ks_bit_u8(wave_type);
    ks_bit_u8(fixed_frequency);
    ks_bit_u8(phase_coarse);
    ks_bit_u8(phase_offset);
    ks_bit_u8(phase_fine);
    ks_bit_u8(semitones);

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

        ret->powerof2[i] = pow(2, i*4/128.0) * ks_1(KS_POWER_OF_2_BITS);
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
        data->operators[i].use_custom_wave = 0;
        data->operators[i].wave_type = 0;
        data->operators[i].fixed_frequency = false;
        data->operators[i].phase_coarse = 2;
        data->operators[i].phase_offset = 0;
        data->operators[i].phase_fine = 8;
        data->operators[i].semitones= ks_1(6)-13;

        data->operators[i].velocity_sens = 31;
        data->operators[i].lfo_ams_depth = 0;
    }

    for(u32 i=0; i<KS_NUM_OPERATORS-1; i++){
        data->mods[i].sync = false;
        data->mods[i].type = KS_MOD_PASS;
    }

    data->common.panpot = 7;

    data->common.lfo_use_custom_wave = 0;
    data->common.lfo_wave_type = KS_WAVE_TRIANGLE;
    data->common.lfo_fms_depth = 0;
    data->common.lfo_freq = 0;
    data->common.lfo_offset = 0;

    // amp
    data->common.amp_ratescale = 0;
    data->common.envelopes[0][0].time= 0;
    data->common.envelopes[0][1].time = 0;
    data->common.envelopes[0][2].time = 0;
    data->common.envelopes[0][3].time = 14;
    data->common.envelopes[0][0].amp = 7;
    data->common.envelopes[0][1].amp= 7;
    data->common.envelopes[0][2].amp = 7;
    data->common.envelopes[0][3].amp = 0;
    // filter
    data->common.filter_ratescale = 0;
    data->common.envelopes[1][0].time= 0;
    data->common.envelopes[1][1].time = 0;
    data->common.envelopes[1][2].time = 0;
    data->common.envelopes[1][3].time = 0;
    data->common.envelopes[1][0].amp = 7;
    data->common.envelopes[1][1].amp= 7;
    data->common.envelopes[1][2].amp = 7;
    data->common.envelopes[1][3].amp = 7;
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

KS_INLINE static void synth_op_set(const ks_synth_context* ctx, ks_synth* synth, const ks_synth_data* data)
{
    synth->lfo_enabled.ams= false;

    for(u32 i=0; i<KS_NUM_OPERATORS; i++)
    {
        synth->wave_tables[i] = ctx->wave_tables[ks_wave_index(data->operators[i].use_custom_wave, data->operators[i].wave_type)];
        if(synth->wave_tables[i] == NULL) {
            ks_error("Failed to set synth for not set wave table %d", ks_wave_index(data->operators[i].use_custom_wave, data->operators[i].wave_type));
            synth->enabled = false;
            return ;
        }
        synth->fixed_frequency[i] = calc_fixed_frequency(data->operators[i].fixed_frequency);
        synth->phase_coarses[i] = calc_phase_coarses(data->operators[i].phase_coarse);
        synth->phase_offsets[i] = calc_phase_offsets(data->operators[i].phase_offset);
        synth->phase_fines[i] = calc_phase_fines(data->operators[i].phase_fine);
        synth->semitones[i] = calc_semitones(data->operators[i].semitones);

        synth->velocity_sens[i] = calc_velocity_sens(data->operators[i].velocity_sens);
        synth->velocity_base[i] = synth->velocity_sens[i] < 0 ? 0: 127;

        synth->lfo_ams_depths[i] = calc_lfo_ams_depths(data->operators[i].lfo_ams_depth);
        synth->lfo_enabled.ams = synth->lfo_enabled.ams ||  data->operators[i].lfo_ams_depth != 0;
    }

    for(u32 i=0; i< KS_NUM_OPERATORS-1; i++){
        synth->mod_syncs[i] = data->mods[i].sync;
        synth->mod_types[i] = data->mods[i].type;
    }
}

KS_INLINE static void synth_common_set(const ks_synth_context* ctx, ks_synth* synth, const ks_synth_data* data)
{
    for(u32 i=0; i< 2; i++) {
        for(u32 e=0; e< KS_ENVELOPE_NUM_POINTS; e++){
            synth->envelope_samples[e][i] = calc_envelope_samples(ctx->sampling_rate, data->common.envelopes[i][e].time);
            synth->envelope_points[e][i] = calc_envelope_points(data->common.envelopes[i][e].amp);
        }
    }
    synth->ratescales[0] = calc_ratescales(data->common.amp_ratescale);
    synth->ratescales[1] = calc_ratescales(data->common.filter_ratescale);

    ks_calc_panpot(ctx, &synth->panpot_left, &synth->panpot_right, calc_panpot(data->common.panpot));

    synth->lfo_wave_table = ctx->wave_tables[ks_wave_index(data->common.lfo_use_custom_wave, data->common.lfo_wave_type)];
    if(synth->lfo_wave_table == NULL) {
        ks_error("Failed to set synth for not set wave table %d", ks_wave_index(data->common.lfo_use_custom_wave, data->common.lfo_wave_type));
        synth->enabled = false;
        return ;
    }
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
    const u32 tune_rate = ((u64)ctx->note_deltas[tune] << KS_FREQUENCY_BITS) / ctx->note_deltas[ks_1(6)-13];
    const u32 coarse_tune = ((u64)coarse * tune_rate)>> KS_PHASE_COARSE_BITS;
    const u32 freq = ctx->note_deltas[notenum]; // heltz << KS_FREQUENCY_BITS

    const u32 freq_rate = coarse_tune;
    const u32 freq_rate_tuned = ((u64)freq_rate * (ks_1(KS_PHASE_FINE_BITS) + fine)) >> KS_PHASE_FINE_BITS;

    u64 delta = freq;
    delta *= freq_rate_tuned;
    delta >>= KS_FREQUENCY_BITS;

    return (u32)delta;
}

void ks_synth_note_on(ks_synth_note* note, const ks_synth *synth, const ks_synth_context *ctx, u8 notenum, u8 vel)
{
    if(! synth->enabled) return;

    note->synth = synth;

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

    }

    for(u32 i=0; i<KS_NUM_ENVELOPES; i++){
        //rate scale
        i64 ratescales =  synth->ratescales[i];
        ratescales *= ctx->ratescales[notenum];
        ratescales >>= KS_RATESCALE_BITS;
        ratescales = ks_1(KS_RATESCALE_BITS) - ratescales;

        i64 target;
        i32 velocity;

        velocity = synth->velocity_sens[i];
        velocity *=  (i32)synth->velocity_base[i]-vel;
        velocity >>= 7;
        velocity = (1 << KS_VELOCITY_SENS_BITS) -  velocity;

        //envelope
        for(u32 j=0; j < KS_ENVELOPE_NUM_POINTS; j++)
        {
            target = synth->envelope_points[j][i];

            target *= velocity;
            target >>= KS_VELOCITY_SENS_BITS;

            note->envelope_points[j][i] = (i32)target;

            u64 frame = (synth->envelope_samples[j][i]);
            frame *= ratescales;
            frame >>= KS_RATESCALE_BITS;
            note->envelope_samples[j][i] = MAX((u32)frame, 1u);
        }

        note->envelope_deltas[0][i] = ks_1(KS_ENVELOPE_BITS);
        note->envelope_deltas[0][i] /= ((i32)note->envelope_samples[0][i] );
        note->envelope_diffs[0][i] = note->envelope_points[0][i];
        for(u32 j=1; j < KS_ENVELOPE_NUM_POINTS; j++)
        {
            i32 sub = note->envelope_points[j][i] - note->envelope_points[j-1][i];
            note->envelope_diffs[j][i] = sub;
            note->envelope_deltas[j][i] =  ks_1(KS_ENVELOPE_BITS);
            note->envelope_deltas[j][i] /=  ((i32)note->envelope_samples[j][i]);
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
    }
}

void ks_synth_note_off (ks_synth_note* note)
{
    for(u32 i=0; i< KS_NUM_ENVELOPES; i++)
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
        //note->lfo_log = synth->lfo_func(note);
        note->lfo_phase += note->lfo_delta;
    }
}


KS_FORCEINLINE static void ks_synth_envelope_process(const ks_synth_context*ctx, ks_synth_note* note, int i){
    note->envelope_now_remains[i] -= note->envelope_now_deltas[i];
    i64 amp = note->envelope_now_remains[i];
    amp = ctx->powerof2[amp >> (KS_ENVELOPE_BITS - 7)] - ks_1(KS_POWER_OF_2_BITS);
    amp *=note->envelope_now_diffs[i];
    amp >>= KS_ENVELOPE_BITS - KS_OUTPUT_BITS;
    note->envelope_now_amps[i] = note->envelope_now_point_amps[i] - amp;


    if(--note->envelope_now_times[i] <= 0)
    {
        note->envelope_now_remains[i] = ks_1(KS_ENVELOPE_BITS);
        switch(note->envelope_states[i])
        {
        case KS_ENVELOPE_ON:
            if(note->envelope_now_points[i] == KS_ENVELOPE_RELEASE_INDEX-1)
            {
                note->envelope_states[i] = KS_ENVELOPE_SUSTAINED;
                note->envelope_now_deltas[i] = 0;
                note->envelope_now_diffs[i] = 0;
                note->envelope_now_times[i] =-1;

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


static void KS_FORCEINLINE ks_synth_render_mod_base(const ks_synth_context* ctx, ks_synth_note* note, u32 op, u32 pitchbend, i32*buf, u32 len, u32 mod_type, bool sync, bool ams, bool fms){
    if(mod_type == KS_MOD_PASS) return;

    const ks_synth* synth = note->synth;
    const u32 preop = (op-1) & 3;

    const u32 bended_phase_delta = ((u64)note->phase_deltas[op] * pitchbend) >> KS_PITCH_BEND_BITS;

    u32 sync_phase ;
    if(sync){
        sync_phase = note->phases[preop] - len*note->phase_deltas[preop];
    }

    for(u32 i=0; i<len; i++){
        switch (mod_type) {
        case KS_MOD_FM:
            buf[i] = synth->wave_tables[op][ks_mask((note->phases[op] + (buf[i] << 15)) >> KS_PHASE_BITS, KS_TABLE_BITS)];
            break;
        case KS_MOD_MIX:{
            i32 out  = synth->wave_tables[op][ks_mask((note->phases[op]) >> KS_PHASE_BITS, KS_TABLE_BITS)];
            out += buf[i];
            out >>=1;
            buf[i] = out;
            break;
        }
        case KS_MOD_MUL:{
            i32 out  = synth->wave_tables[op][ks_mask((note->phases[op] + buf[i]) >> KS_PHASE_BITS, KS_TABLE_BITS)];
            out *=  buf[i];
            out >>= KS_OUTPUT_BITS;
            buf[i] = out;
            break;
        }
        case KS_MOD_AM:{
            i32 out  = synth->wave_tables[op][ks_mask((note->phases[op] + buf[i]) >> KS_PHASE_BITS, KS_TABLE_BITS)];
            out += ks_1(KS_OUTPUT_BITS);
            out *=  buf[i];
            out >>= KS_OUTPUT_BITS+1;
            buf[i] = out;
            break;
        }
        default:
            buf[i] = synth->wave_tables[0][ks_mask(note->phases[0] >> KS_PHASE_BITS, KS_TABLE_BITS)];
            break;
        }

        note->phases[op] += bended_phase_delta;

        if(sync){
            u32 next_sync_phase =  ks_mask(sync_phase + note->phase_deltas[preop], KS_PHASE_MAX_BITS);
            if(ks_mask(sync_phase, KS_PHASE_MAX_BITS) > next_sync_phase){
                note->phases[op] = 0;
            }
            sync_phase =next_sync_phase;
        }

    }
}

#define ks_synth_render_func(mod, sync, ams, fms) ks_synth_render_ ## mod ## _ ## sync ##  ams ## fms

#define ks_synth_render_impl(mod, sync, ams, fms) \
    static void KS_NOINLINE ks_synth_render_func(mod, sync, ams, fms) (const ks_synth_context* ctx, ks_synth_note* note, u32 op, u32 pitchbend, i32*buf, u32 len){\
        ks_synth_render_mod_base(ctx, note, op, pitchbend, buf, len, mod, sync, ams, fms); \
    }

#define ks_synth_render_impl_mod(sync, ams, fms) \
    ks_synth_render_impl(KS_MOD_FM, sync, ams, fms) \
    ks_synth_render_impl(KS_MOD_MIX, sync, ams, fms)  \
    ks_synth_render_impl(KS_MOD_MUL, sync, ams, fms) \
    ks_synth_render_impl(KS_MOD_AM, sync, ams, fms)


ks_synth_render_impl_mod(0, 0, 0)
ks_synth_render_impl_mod(0, 0, 1)
ks_synth_render_impl_mod(0, 1, 0)
ks_synth_render_impl_mod(0, 1, 1)
ks_synth_render_impl_mod(1, 0, 0)
ks_synth_render_impl_mod(1, 1, 0)
ks_synth_render_impl(KS_MOD_PASS, 0, 0, 0)


static void KS_FORCEINLINE ks_synth_render_branch(const ks_synth_context* ctx, ks_synth_note* note, u32 op, u32 pitchbend, i32* buf, u32 len){
#define sync_lfo_branch(mod, sync, ams, fms) \
    if(sync){ \
        if(ams){ \
           ks_synth_render_func(mod, 1, 0, 0)(ctx, note, op, pitchbend, buf, len); \
        } else { \
           ks_synth_render_func(mod, 1, 1, 0)(ctx, note, op, pitchbend, buf, len); \
        } \
    } else { \
        if(ams){ \
            if(fms){ \
               ks_synth_render_func(mod, 0, 1, 1)(ctx, note, op, pitchbend, buf, len); \
            } else { \
               ks_synth_render_func(mod, 0, 1, 0)(ctx, note, op, pitchbend, buf, len); \
            } \
        } else { \
            if(fms){ \
               ks_synth_render_func(mod, 0, 0, 1)(ctx, note, op, pitchbend, buf, len); \
            } else { \
               ks_synth_render_func(mod, 0, 0, 0)(ctx, note, op, pitchbend, buf, len); \
            } \
        } \
    }

    const ks_synth* synth = note->synth;
    switch (synth->mod_types[op-1]) {
    case KS_MOD_FM:
        sync_lfo_branch(KS_MOD_FM, synth->mod_syncs[op-1], false, false);
        break;
    case KS_MOD_MIX:
        sync_lfo_branch(KS_MOD_MIX, synth->mod_syncs[op-1], false, false);
        break;
    case KS_MOD_MUL:
        sync_lfo_branch(KS_MOD_MUL, synth->mod_syncs[op-1], false, false);
        break;
    case KS_MOD_AM:
        sync_lfo_branch(KS_MOD_AM, synth->mod_syncs[op-1], false, false);
        break;
    case KS_MOD_PASS:
        ks_synth_render_func(KS_MOD_PASS, 0, 0, 0)(ctx, note, op, pitchbend, buf, len);
        break;
    }
#undef sync_lfo_branch
}

static void KS_NOINLINE ks_synth_render_0(const ks_synth_context* ctx, ks_synth_note* note, u32 pitchbend, i32*buf, u32 len){
    ks_synth_render_mod_base(ctx, note, 0, pitchbend, buf, len, KS_NUM_MODS, false, false, false);
}


static void KS_NOINLINE ks_synth_render_envelope(const ks_synth_context* ctx, ks_synth_note* note, i32*buf, u32 len){
    for(u32 i=0; i<len; i++){
        ks_synth_envelope_process(ctx, note, 0);

        u64 amp = note->envelope_now_amps[0];
        amp *= buf[i];
        amp >>= KS_ENVELOPE_BITS;

        buf[i] = amp;
    }
}

void ks_synth_render(const ks_synth_context*ctx, ks_synth_note* note, u32 volume, u32 pitchbend, i32 *buf, u32 len)
{
    const size_t tmpbuf_len = len / 2;
    i32* tmpbuf = (i32*)malloc(sizeof(i32) * tmpbuf_len);
    if(*(u32*)note->envelope_states != 0){
        const ks_synth* synth = note->synth;
        ks_synth_render_0(ctx, note, pitchbend, tmpbuf, tmpbuf_len);
        ks_synth_render_branch(ctx, note, 1, pitchbend, tmpbuf, tmpbuf_len);
        ks_synth_render_branch(ctx, note, 2, pitchbend, tmpbuf, tmpbuf_len);
        ks_synth_render_branch(ctx, note, 3, pitchbend, tmpbuf, tmpbuf_len);
        ks_synth_render_envelope(ctx, note, tmpbuf, tmpbuf_len);

        for(u32 i=0; i<tmpbuf_len; i++){
            buf[2*i] = ks_apply_panpot(tmpbuf[i], synth->panpot_left);
            buf[2*i+1] = ks_apply_panpot(tmpbuf[i], synth->panpot_right);
        }
    } else {
        memset(buf, 0, len*sizeof(i32));
    }
    free(tmpbuf);
}

