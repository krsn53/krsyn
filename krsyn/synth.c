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

ks_io_begin_custom_func(ks_lfo_data)
    ks_bit_u8(op_enabled);

    ks_bit_u8(freq);
    ks_bit_u8(level);
    ks_bit_u8(use_custom_wave);
    ks_bit_u8(wave);
    ks_bit_u8(offset);
ks_io_end_custom_func(ks_lfo_data)

ks_io_begin_custom_func(ks_envelope_point_data)
    ks_bit_u8(time);
    ks_bit_u8(amp);
ks_io_end_custom_func(ks_envelope_point_data)

ks_io_begin_custom_func(ks_envelope_data)
    ks_arr_obj(points, ks_envelope_point_data);
    ks_bit_u8(level);
    ks_bit_u8(ratescale);
ks_io_end_custom_func(ks_envelope_data)

ks_io_begin_custom_func(ks_common_data)
    ks_arr_obj(envelopes, ks_envelope_data);
    ks_arr_obj(lfos, ks_lfo_data);
    ks_bit_u8(panpot);

ks_io_end_custom_func(ks_common_data)

ks_io_begin_custom_func(ks_operator_data)
    ks_bit_u8(use_custom_wave);
    ks_bit_u8(wave_type);
    ks_bit_u8(fixed_frequency);
    ks_bit_u8(phase_coarse);
    ks_bit_u8(phase_offset);
    ks_bit_u8(phase_fine);
    ks_bit_u8(semitones);
    ks_bit_u8(velocity_sens);
ks_io_end_custom_func(ks_operator_data)

ks_io_begin_custom_func(ks_mod_data)
    ks_bit_u8(type);
    ks_bit_u8(sync);
    ks_bit_u8(fm_level);
    ks_bit_u8(mix);
ks_io_end_custom_func(ks_mod_data)

ks_io_begin_custom_func(ks_synth_data)
    ks_magic_number("KSYN");
    if(__METHODS->type == KS_SERIAL_BINARY){
        __RETURN += ks_io_binary_as_array(io, __METHODS, __OBJECT, sizeof(ks_synth_data), __SERIAL_TYPE);
    } else {
        ks_arr_obj(operators, ks_operator_data);
        ks_arr_obj(mods, ks_mod_data);
        ks_obj(common, ks_common_data);
    }
ks_io_end_custom_func(ks_synth_data)


ks_synth_context* ks_synth_context_new(u32 sampling_rate){
    ks_synth_context *ret = calloc(1, sizeof(ks_synth_context));
    ret->sampling_rate = sampling_rate;
    ret->sampling_rate_inv = ks_1(30) / sampling_rate;

    for(unsigned i=0; i< 128; i++){
        ret->ratescales[i] = ks_1(KS_RATESCALE_BITS) - ks_1(KS_RATESCALE_BITS) / exp2((double)i/12);

        u64 freq_30 = 440.0 * exp2(((double)i-69)/12) * ret->sampling_rate_inv;
        ret->note_deltas[i] = freq_30 >> (KS_SAMPLING_RATE_INV_BITS - KS_PHASE_MAX_BITS);

        ret->powerof2[i] = pow(2, i*4/128.0) * ks_1(KS_POWER_OF_2_BITS);
    }

    for(unsigned i=0; i< KS_NUM_WAVES; i++){
        ret->wave_tables[i] = malloc(sizeof(i16)* ks_1(KS_TABLE_BITS));
    }

    srand(0);
    for(unsigned i=0; i< ks_1(KS_TABLE_BITS); i++){
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
    for(unsigned i=0; i< KS_MAX_WAVES; i++){
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
    for(unsigned i=0; i<length; i++){
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
    for(unsigned i=0; i<KS_NUM_OPERATORS; i++)
    {
        data->operators[i].use_custom_wave = 0;
        data->operators[i].wave_type = 0;
        data->operators[i].fixed_frequency = false;
        data->operators[i].phase_coarse = 2;
        data->operators[i].phase_offset = 0;
        data->operators[i].phase_fine = 8;
        data->operators[i].semitones= ks_1(6)-13;

        data->operators[i].velocity_sens = 15;
    }

    for(unsigned i=0; i<KS_NUM_OPERATORS-1; i++){
        data->mods[i].sync = false;
        data->mods[i].type = KS_MOD_PASS;
        data->mods[i].fm_level =   0;
        data->mods[i].mix = 63;
    }

    data->common.panpot = 7;

    for(unsigned i =0 ; i<KS_NUM_LFOS; i++){
        data->common.lfos[i].op_enabled = 0;
        data->common.lfos[i].use_custom_wave = 0;
        data->common.lfos[i].wave = KS_WAVE_TRIANGLE;
        data->common.lfos[i].level = 0;
        data->common.lfos[i].freq = 0;
        data->common.lfos[i].offset = 0;
    }

    // amp
    data->common.envelopes[0].level = 31;
    data->common.envelopes[0].ratescale = 0;
    data->common.envelopes[0].points[0].time= 0;
    data->common.envelopes[0].points[1].time = 0;
    data->common.envelopes[0].points[2].time = 0;
    data->common.envelopes[0].points[3].time = 14;
    data->common.envelopes[0].points[0].amp = 7;
    data->common.envelopes[0].points[1].amp= 7;
    data->common.envelopes[0].points[2].amp = 7;
    data->common.envelopes[0].points[3].amp = 0;
    // filter
    data->common.envelopes[1].level = 31;
    data->common.envelopes[1].ratescale = 0;
    data->common.envelopes[1].points[0].time= 0;
    data->common.envelopes[1].points[1].time = 0;
    data->common.envelopes[1].points[2].time = 0;
    data->common.envelopes[1].points[3].time = 0;
    data->common.envelopes[1].points[0].amp = 7;
    data->common.envelopes[1].points[1].amp= 7;
    data->common.envelopes[1].points[2].amp = 7;
    data->common.envelopes[1].points[3].amp = 7;
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
    for(unsigned i=0; i<KS_NUM_OPERATORS; i++)
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
    }

    for(unsigned i=0; i< KS_NUM_OPERATORS-1; i++){
        synth->mod_syncs[i] = data->mods[i].sync;
        synth->mod_types[i] = data->mods[i].type;
        synth->output_mod_levels[i] = calc_mix_levels(data->mods[i].mix);
        synth->output_levels[i] = ks_1(KS_LEVEL_BITS)-ks_1(KS_LEVEL_BITS-7) - synth->output_mod_levels[i];
        synth->mod_fm_levels[i] = calc_fm_levels(data->mods[i].fm_level);
    }
}

KS_INLINE static void synth_common_set(const ks_synth_context* ctx, ks_synth* synth, const ks_synth_data* data)
{
    for(unsigned i=0; i< KS_NUM_ENVELOPES; i++) {
        synth->ratescales[i] = calc_ratescales(data->common.envelopes[i].ratescale);
         u32 amp_level = calc_levels(data->common.envelopes[i].level);
        for(u32 e=0; e< KS_ENVELOPE_NUM_POINTS; e++){
            synth->envelope_samples[e][i] = calc_envelope_samples(ctx->sampling_rate, data->common.envelopes[i].points[e].time);
            synth->envelope_points[e][i] = calc_envelope_points(data->common.envelopes[i].points[e].amp);

            u64 point = synth->envelope_points[e][i];
            point *= amp_level;
            point >>= KS_LEVEL_BITS;
            synth->envelope_points[e][i] = point;
        }
    }

    for(u32 e=0; e< KS_NUM_LFOS; e++){
        const u32 freq = calc_lfo_freq(data->common.lfos[e].freq);
        const u64 freq_11 = ((u64)freq) << KS_TABLE_BITS;
        u64 delta_11 = (u64)freq_11 * ctx->sampling_rate_inv;
        delta_11 >>= KS_SAMPLING_RATE_INV_BITS-(KS_PHASE_BITS - KS_FREQUENCY_BITS);

        synth->lfo_deltas[e]= delta_11;
        synth->lfo_offsets[e] =calc_lfo_offset(data->common.lfos[e].offset);
        synth->lfo_op_enables[e] = data->common.lfos[e].op_enabled;

        synth->lfo_wave_tables[e] = ctx->wave_tables[ks_wave_index(data->common.lfos[e].use_custom_wave, data->common.lfos[e].wave)];
        if(synth->lfo_wave_tables[e] == NULL) {
            ks_error("Failed to set synth for not set wave table %d", ks_wave_index(data->common.lfos[e].use_custom_wave, data->common.lfos[e].wave));
            synth->enabled = false;
            return ;
        }
    }

    synth->lfo_levels[0] = calc_lfo_depth(data->common.lfos[0].level);
    synth->lfo_levels[1] = calc_lfo_depth(data->common.lfos[1].level);

    ks_calc_panpot(ctx, &synth->panpot_left, &synth->panpot_right, calc_panpot(data->common.panpot));



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
    note->noise_table_offset = 0;
    for(unsigned i=0; i<KS_NUM_LFOS; i++){
        note->lfo_phases[i] = synth->lfo_offsets[i];
    }


    for(unsigned i=0; i<KS_NUM_OPERATORS; i++){
        note->phases[i] = synth->phase_offsets[i];
    }


    for(unsigned i=0; i< KS_NUM_OPERATORS; i++)
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

    for(unsigned i=0; i<KS_NUM_ENVELOPES; i++){
        //rate scale
        i64 ratescales =  synth->ratescales[i];
        ratescales *= ctx->ratescales[notenum];
        ratescales >>= KS_RATESCALE_BITS;
        ratescales = ks_1(KS_RATESCALE_BITS) - ratescales;

        i64 target;
        i32 velocity;

        velocity = synth->velocity_sens[i];
        velocity *=  vel;
        velocity >>= 7;
        velocity += (1 << KS_VELOCITY_SENS_BITS) -  synth->velocity_sens[i];

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
    for(unsigned i=0; i< KS_NUM_ENVELOPES; i++)
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

KS_FORCEINLINE static void ks_envelope_process(const ks_synth_context*ctx, ks_synth_note* note, int i){
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


static void KS_FORCEINLINE ks_synth_render_mod_base(const ks_synth_context* ctx, ks_synth_note* note, u32 op, u32 pitchbend, i32*buf[], u32 len, u32 mod_type, bool fm, bool sync, bool ams, bool fms, bool noise){
    if(mod_type == KS_MOD_PASS) return;

    i32* outbuf = buf[0];
    i32* lfo1 = buf[1];
    i32* lfo2 = buf[2];

    const ks_synth* synth = note->synth;
    const u32 preop = (op-1) & 3;

    u32 bend_phase_delta = ((u64)note->phase_deltas[op] * pitchbend) >> KS_PITCH_BEND_BITS;


    u32 sync_phase ;
    if(sync){
        sync_phase = note->phases[preop] - len*note->phase_deltas[preop];
    }
    const u32 shift = noise ? (KS_PHASE_BITS + (KS_TABLE_BITS-5)) : KS_PHASE_BITS;


    for(unsigned i=0; i<len; i++){
        i32 out;
        i64 fm_amount = 0;
        if(fm) {
            fm_amount = outbuf[i];
            fm_amount *= synth->mod_fm_levels[preop];
            fm_amount >>= (KS_LEVEL_BITS - 3);
            fm_amount = ks_v(fm_amount, KS_PHASE_MAX_BITS - KS_OUTPUT_BITS);
        }
        switch (mod_type) {
        case KS_MOD_MIX:{
            out  = synth->wave_tables[op][ks_mask((note->phases[op] + fm_amount) >> KS_PHASE_BITS, KS_TABLE_BITS)];
            break;
        }
        case KS_MOD_MUL:{
            out  = synth->wave_tables[op][ks_mask((note->phases[op] + fm_amount) >> KS_PHASE_BITS, KS_TABLE_BITS)];
            out *=  outbuf[i];
            out >>= KS_OUTPUT_BITS;
            break;
        }
        case KS_MOD_AM:{
            out  = synth->wave_tables[op][ks_mask((note->phases[op] + fm_amount) >> KS_PHASE_BITS, KS_TABLE_BITS)];
            out += ks_1(KS_OUTPUT_BITS);
            out *=  outbuf[i];
            out >>= KS_OUTPUT_BITS+1;
            break;
        }
        case KS_NUM_MODS:{
            out= synth->wave_tables[0][ks_mask((note->phases[0]>> shift ) + note->noise_table_offset, KS_TABLE_BITS)];
            break;
        }
        }

        if(mod_type != KS_NUM_MODS){
            const i32 mod = ((i64)outbuf[i]* synth->output_mod_levels[preop]) >> KS_LEVEL_BITS;
            const i32 car = ((i64)out * synth->output_levels[preop]) >> KS_LEVEL_BITS;
            outbuf[i] = mod + car;
        } else {
            outbuf[i] = out;

            if(noise){
                if(ks_mask(note->phases[0] >> shift, KS_TABLE_BITS) > ks_mask((note->phases[0] + note->phase_deltas[0]) >> shift, KS_TABLE_BITS)){
                    srand(note->noise_table_offset);
                    note->noise_table_offset = rand();
                }
            }
        }

        if(ams){
            i32 factor = ks_1(KS_OUTPUT_BITS) + lfo1[i];
            outbuf[i] = ((i64)factor*outbuf[i]) >> KS_OUTPUT_BITS;
        }

        u32 phase_delta = bend_phase_delta;

        if(fms) {
            i32 factor = lfo2[i] + ks_1(KS_OUTPUT_BITS);
            phase_delta = ((i64)factor* phase_delta) >> KS_OUTPUT_BITS;
        }

        note->phases[op] += phase_delta;

        if(sync){
            u32 next_sync_phase =  ks_mask(sync_phase + note->phase_deltas[preop], KS_PHASE_MAX_BITS);
            if(ks_mask(sync_phase, KS_PHASE_MAX_BITS) > next_sync_phase){
                note->phases[op] = 0;
            }
            sync_phase =next_sync_phase;
        }

    }
}

#define ks_synth_render_func(mod, fm, sync, ams, fms, noise) ks_synth_render_ ##  mod ## _ ## fm ## sync ##  ams ## fms ## noise

#define ks_synth_render_impl(mod, fm, sync, ams, fms, noise) \
    static void KS_NOINLINE ks_synth_render_func(mod, fm, sync, ams, fms, noise) (const ks_synth_context* ctx, ks_synth_note* note, u32 op, u32 pitchbend, i32*buf[], u32 len){\
        ks_synth_render_mod_base(ctx, note, op, pitchbend, buf, len, mod, fm, sync, ams, fms, noise); \
    }

#define ks_synth_render_impl_mod(fm, sync, ams, fms) \
    ks_synth_render_impl(KS_MOD_MIX, fm, sync, ams, fms, 0)  \
    ks_synth_render_impl(KS_MOD_MUL, fm, sync, ams, fms, 0) \
    ks_synth_render_impl(KS_MOD_AM, fm, sync, ams, fms, 0)

#define ks_synth_render_impl_mod_fm(sync, ams, fms) \
    ks_synth_render_impl_mod(0, sync, ams, fms) \
    ks_synth_render_impl_mod(1, sync, ams, fms)  \


ks_synth_render_impl_mod_fm(0, 0, 0)
ks_synth_render_impl_mod_fm(0, 0, 1)
ks_synth_render_impl_mod_fm(0, 1, 0)
ks_synth_render_impl_mod_fm(0, 1, 1)
ks_synth_render_impl_mod_fm(1, 0, 0)
ks_synth_render_impl_mod_fm(1, 1, 0)
ks_synth_render_impl(KS_MOD_PASS, 0, 0, 0, 0, 0)

ks_synth_render_impl(KS_NUM_MODS, 0, 0, 0, 0, 0)
ks_synth_render_impl(KS_NUM_MODS, 0, 0, 0, 1, 0)
ks_synth_render_impl(KS_NUM_MODS, 0, 0, 1, 0, 0)
ks_synth_render_impl(KS_NUM_MODS, 0, 0, 1, 1, 0)
ks_synth_render_impl(KS_NUM_MODS, 0, 0, 0, 0, 1)
ks_synth_render_impl(KS_NUM_MODS, 0, 0, 0, 1, 1)
ks_synth_render_impl(KS_NUM_MODS, 0, 0, 1, 0, 1)
ks_synth_render_impl(KS_NUM_MODS, 0, 0, 1, 1, 1)


static void KS_FORCEINLINE ks_synth_render_branch(const ks_synth_context* ctx, ks_synth_note* note, u32 op, u32 pitchbend, i32* buf[], u32 len){
#define fm_branch(mod, sync, ams, fms) \
if(fm) { \
    ks_synth_render_func(mod, 1, sync, ams, fms, 0)(ctx, note, op, pitchbend, buf, len); \
}else{ \
    ks_synth_render_func(mod, 0, sync, ams, fms, 0)(ctx, note, op, pitchbend, buf, len); \
}

#define fms_branch(mod, sync, ams) \
    if(fms){ \
       fm_branch(mod, sync, ams, 1); \
    } else { \
       fm_branch(mod, sync, ams, 0); \
    }

#define sync_lfo_branch(mod)  { \
        const u32 preop = op-1; \
        const bool fm = synth->mod_fm_levels[preop]!= 0; \
        const bool sync = synth->mod_syncs[preop]; \
        const bool ams = (synth->lfo_op_enables[0] & ks_1(op)) != 0 && synth->lfo_levels[0] != 0; \
        const bool fms = (synth->lfo_op_enables[1] & ks_1(op)) != 0 && synth->lfo_levels[1] != 0; \
        if(sync){ \
            if(ams){ \
               fm_branch(mod, 1, 1, 0); \
            } else { \
               fm_branch(mod, 1, 0, 0); \
            } \
        } else { \
            if(ams){ \
                fms_branch(mod, 0, 1); \
            } else { \
                fms_branch(mod, 0, 0); \
            } \
        } \
    }

    const ks_synth* synth = note->synth;
    switch (synth->mod_types[op-1]) {
    case KS_MOD_MIX:
        sync_lfo_branch(KS_MOD_MIX);
        break;
    case KS_MOD_MUL:
        sync_lfo_branch(KS_MOD_MUL);
        break;
    case KS_MOD_AM:
        sync_lfo_branch(KS_MOD_AM);
        break;
    case KS_MOD_PASS:
        ks_synth_render_func(KS_MOD_PASS, 0, 0, 0, 0, 0)(ctx, note, op, pitchbend, buf, len);
        break;
    }
#undef sync_lfo_branch
#undef fm_branch
#undef fms_branch
}

static void KS_NOINLINE ks_synth_render_0(const ks_synth_context* ctx, ks_synth_note* note, u32 pitchbend, i32*buf[], u32 len){
#define noise_table_branch(ams, fms) \
    if(noise_table) { \
        ks_synth_render_func(KS_NUM_MODS, 0, 0, ams, fms, 1)(ctx, note, 0, pitchbend, buf, len); \
    } else { \
        ks_synth_render_func(KS_NUM_MODS, 0, 0, ams, fms, 0)(ctx, note, 0, pitchbend, buf, len); \
    }

#define fms_branch(ams) \
    if(fms){ \
       noise_table_branch(ams, 1); \
    } else { \
       noise_table_branch(ams, 0); \
    }

    const ks_synth* synth = note->synth;
    const bool noise_table = synth->wave_tables[0] == ctx->wave_tables[KS_WAVE_NOISE];
    const bool ams = (synth->lfo_op_enables[0] & ks_1(0)) != 0 && synth->lfo_levels[0] != 0;
    const bool fms = (synth->lfo_op_enables[1] & ks_1(0)) != 0 && synth->lfo_levels[1] != 0;
    if(ams){
        fms_branch(1);
    } else {
        fms_branch(0);
    }

#undef noise_table_branch
#undef fms_branch
}


static void KS_NOINLINE ks_synth_render_envelope(const ks_synth_context* ctx, ks_synth_note* note, i32*buf, u32 len){
    for(unsigned i=0; i<len; i++){
        ks_envelope_process(ctx, note, 0);

        u64 amp = note->envelope_now_amps[0];
        amp *= buf[i];
        amp >>= KS_ENVELOPE_BITS;

        buf[i] = amp;
    }
}

static void KS_NOINLINE ks_synth_render_lfo(const ks_synth_context* ctx, ks_synth_note* note, u32 l, i32* buf, u32 len){
    const ks_synth* synth = note->synth;
    for(unsigned i =0; i<len; i++){
        i32 out = synth->lfo_wave_tables[l][ks_mask(note->lfo_phases[l] >> KS_PHASE_BITS, KS_TABLE_BITS)];
        out *= synth->lfo_levels[l];
        out >>= KS_LEVEL_BITS;
        buf[i] = out;
        note->lfo_phases[l] += synth->lfo_deltas[l];
    }
}

void ks_synth_render(const ks_synth_context*ctx, ks_synth_note* note, u32 volume, u32 pitchbend, i32 *buf, u32 len)
{

    if(*(u32*)note->envelope_states != 0){
        const ks_synth* synth = note->synth;
        const size_t tmpbuf_len = len / 2;
        i32* tmpbuf = (i32*)malloc(sizeof(i32) * tmpbuf_len);
        i32* lfo1buf = NULL;
        i32* lfo2buf = NULL;
        if(synth->lfo_op_enables[0]){
            lfo1buf = (i32*)malloc(sizeof(i32) * tmpbuf_len);
            ks_synth_render_lfo(ctx, note, 0, lfo1buf, tmpbuf_len);
        }
        if(synth->lfo_op_enables[1]){
            lfo2buf = (i32*)malloc(sizeof(i32)* tmpbuf_len);
            ks_synth_render_lfo(ctx, note, 1, lfo2buf, tmpbuf_len);
        }

        i32* bufs[3] = {tmpbuf, lfo1buf, lfo2buf};

        ks_synth_render_0(ctx, note, pitchbend, bufs, tmpbuf_len);
        for(unsigned i=1; i< KS_NUM_OPERATORS; i++){
            ks_synth_render_branch(ctx, note, i, pitchbend, bufs, tmpbuf_len);
        }
        ks_synth_render_envelope(ctx, note, tmpbuf, tmpbuf_len);

        for(unsigned i=0; i<tmpbuf_len; i++){
            buf[2*i] = ks_apply_panpot(tmpbuf[i], synth->panpot_left) >> 2;
            buf[2*i+1] = ks_apply_panpot(tmpbuf[i], synth->panpot_right) >> 2;
        }

        free(tmpbuf);
        if(lfo1buf != NULL){
            free(lfo1buf);
        }
        if(lfo2buf != NULL){
            free(lfo2buf);
        }
    } else {
        memset(buf, 0, len*sizeof(i32));
    }

}

