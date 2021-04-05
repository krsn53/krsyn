#include "synth.h"

#include <ksio/serial/binary.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

ks_io_begin_custom_func(ks_lfo_data)
    ks_bit_val(op_enabled);
    ks_bit_val(filter_enabled);
    ks_bit_val(panpot_enabled);

    ks_bit_val(freq);
    ks_bit_val(level);
    ks_bit_val(use_custom_wave);
    ks_bit_val(wave);
    ks_bit_val(offset);
ks_io_end_custom_func(ks_lfo_data)

ks_io_begin_custom_func(ks_envelope_point_data)
    ks_bit_val(time);
    ks_bit_val(amp);
ks_io_end_custom_func(ks_envelope_point_data)

ks_io_begin_custom_func(ks_envelope_data)
    ks_arr_obj(points, ks_envelope_point_data);
    ks_bit_val(level);
    ks_bit_val(ratescale);
    ks_bit_val(velocity_sens);
ks_io_end_custom_func(ks_envelope_data)


ks_io_begin_custom_func(ks_operator_data)
    ks_bit_val(use_custom_wave);
    ks_bit_val(wave_type);
    ks_bit_val(fixed_frequency);
    ks_bit_val(phase_coarse);
    ks_bit_val(phase_offset);
    ks_bit_val(phase_fine);
    ks_bit_val(semitones);

ks_io_end_custom_func(ks_operator_data)

ks_io_begin_custom_func(ks_mod_data)
    ks_bit_val(type);
    ks_bit_val(sync);
    ks_bit_val(fm_level);
    ks_bit_val(mix);
ks_io_end_custom_func(ks_mod_data)

ks_io_begin_custom_func(ks_synth_data)
    ks_magic_number("KSYN");
    if(__METHODS->type == KS_SERIAL_BINARY){
        __RETURN += ks_io_binary_as_array(io, __METHODS, __OBJECT, sizeof(ks_synth_data), __SERIAL_TYPE);
    } else {
        ks_arr_obj(operators, ks_operator_data);
        ks_arr_obj(mods, ks_mod_data);
        ks_arr_obj(envelopes, ks_envelope_data);
        ks_arr_obj(lfos, ks_lfo_data);
        ks_bit_val(filter_type);
        ks_bit_val(filter_cutoff);
        ks_bit_val(filter_q);
        ks_bit_val(filter_key_sens);
        ks_bit_val(panpot);
    }
ks_io_end_custom_func(ks_synth_data)


ks_synth_context* ks_synth_context_new(u32 sampling_rate){
    ks_synth_context *ret = calloc(1, sizeof(ks_synth_context));
    ret->sampling_rate = sampling_rate;
    ret->sampling_rate_inv = ks_1(30) / sampling_rate;

    for(unsigned i=0; i< 128; i++){
        u64 freq_30 = 440.0 * exp2(((double)i-69)/12) * ret->sampling_rate_inv;
        ret->note_deltas[i] = freq_30 >> (KS_SAMPLING_RATE_INV_BITS - KS_PHASE_MAX_BITS);
    }

    for(unsigned i=0; i< KS_NUM_WAVES; i++){
        ret->wave_tables[i] = malloc(sizeof(i16)* ks_1(KS_TABLE_BITS));
    }

    srand(0);
    for(unsigned i=0; i< ks_1(KS_TABLE_BITS); i++){
        ret->wave_tables[KS_WAVE_SIN][i] = sin(2* M_PI * i / ks_1(KS_TABLE_BITS)) * (ks_1(KS_OUTPUT_BITS)-1);
        int p = ks_mask(i + (ks_1(KS_TABLE_BITS-2)), KS_TABLE_BITS);

        ret->wave_tables[KS_WAVE_TRIANGLE][i] = p < ks_1(KS_TABLE_BITS-1) ?
                    -(ks_1(KS_OUTPUT_BITS)-1) + (int64_t)(ks_1(KS_OUTPUT_BITS)-1)* 2 * p / ks_1(KS_TABLE_BITS-1) :
                (ks_1(KS_OUTPUT_BITS)-1) - (ks_1(KS_OUTPUT_BITS)-1) * 2 * (int64_t)(p-ks_1(KS_TABLE_BITS-1)) / ks_1(KS_TABLE_BITS-1);

        ret->wave_tables[KS_WAVE_FAKE_TRIANGLE][i] = ret->wave_tables[KS_WAVE_TRIANGLE][i] >> 13 << 13;

        ret->wave_tables[KS_WAVE_SAW_DOWN][i] = INT16_MIN + ks_mask(ks_1(16) * p / ks_1(KS_TABLE_BITS), 16);
        ret->wave_tables[KS_WAVE_SAW_UP][i] = -ret->wave_tables[KS_WAVE_SAW_DOWN][i];

        ret->wave_tables[KS_WAVE_SQUARE][i] = i < ks_1(KS_TABLE_BITS-1) ? (ks_1(KS_OUTPUT_BITS)-1): -(ks_1(KS_OUTPUT_BITS)-1);
        ret->wave_tables[KS_WAVE_NOISE][i] = rand();

        ret->powerof2[i] = pow(2, i*4/(float)ks_1(KS_TABLE_BITS)) * ks_1(KS_POWER_OF_2_BITS);
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
    }

    for(unsigned i=0; i<KS_NUM_OPERATORS-1; i++){
        data->mods[i].sync = false;
        data->mods[i].type = KS_MOD_PASS;
        data->mods[i].fm_level =   0;
        data->mods[i].mix = 63;
    }

    data->panpot = 7;

    for(unsigned i =0 ; i<KS_NUM_LFOS; i++){
        data->lfos[i].filter_enabled =0;
        data->lfos[i].panpot_enabled=0;
        data->lfos[i].op_enabled = 0;
        data->lfos[i].use_custom_wave = 0;
        data->lfos[i].wave = KS_WAVE_TRIANGLE;
        data->lfos[i].level = 0;
        data->lfos[i].freq = 0;
        data->lfos[i].offset = 0;
    }

    // amp
    data->envelopes[0].level = 255;
    data->envelopes[0].ratescale = 0;
    data->envelopes[0].points[0].time= 0;
    data->envelopes[0].points[1].time = 0;
    data->envelopes[0].points[2].time = 0;
    data->envelopes[0].points[3].time = 14;
    data->envelopes[0].points[0].amp = 7;
    data->envelopes[0].points[1].amp= 7;
    data->envelopes[0].points[2].amp = 7;
    data->envelopes[0].points[3].amp = 0;

    data->envelopes[0].velocity_sens = 15;

    // filter
    data->envelopes[1].level = 255;
    data->envelopes[1].ratescale = 0;
    data->envelopes[1].points[0].time= 0;
    data->envelopes[1].points[1].time = 0;
    data->envelopes[1].points[2].time = 0;
    data->envelopes[1].points[3].time = 0;
    data->envelopes[1].points[0].amp = 7;
    data->envelopes[1].points[1].amp= 7;
    data->envelopes[1].points[2].amp = 7;
    data->envelopes[1].points[3].amp = 7;

    data->envelopes[1].velocity_sens = 15;

    data->filter_type = KS_LOW_PASS_FILTER;
    data->filter_cutoff = 31;
    data->filter_key_sens = 9;
    data->filter_q = 2;
}

KS_INLINE u64 ks_exp_u(u32 val, int num_v)
{
    u32 v = ks_mask(val, num_v);
    u32 e = val >> num_v;
    if(e!= 0){
        e--;
        v |= 1 << num_v;
    }
    i64 ret = (u64)v<< e;
    return ret;
}

KS_INLINE u32 ks_calc_envelope_time(u32 val)
{
    return (ks_exp_u(val, 4) * ks_1(KS_TIME_BITS)) >> 12;
}

KS_INLINE u32 ks_calc_envelope_samples(u32 smp_freq, u8 val)
{
    u32 time = ks_calc_envelope_time(val);
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

KS_INLINE void ks_calc_panpot(const ks_synth_context * ctx, i16* left, i16 * right, u32 val){
    *left =  ks_sin(ctx, ks_v(val + ks_v(2, KS_PANPOT_BITS - 1), KS_PHASE_MAX_BITS - KS_PANPOT_BITS - 2 ));
    *right = ks_sin(ctx, ks_v(val + ks_v(0, KS_PANPOT_BITS - 1), KS_PHASE_MAX_BITS - KS_PANPOT_BITS - 2 ));
}

KS_INLINE i32 ks_apply_panpot(i32 in, i16 pan){
    i32 out = in;
    out *= pan;
    out >>= KS_OUTPUT_BITS;
    return out;
}

void ks_synth_set(ks_synth* synth, const ks_synth_context* ctx, const ks_synth_data* data)
{
    synth->enabled = true;

    for(unsigned i=0; i<KS_NUM_OPERATORS; i++)
    {
        synth->operators[i].wave_table = ctx->wave_tables[ks_wave_index(data->operators[i].use_custom_wave, data->operators[i].wave_type)];
        if(synth->operators[i].wave_table == NULL) {
            ks_error("Failed to set synth for not set wave table %d", ks_wave_index(data->operators[i].use_custom_wave, data->operators[i].wave_type));
            synth->enabled = false;
            return ;
        }
        synth->operators[i].fixed_frequency = calc_fixed_frequency(data->operators[i].fixed_frequency);
        synth->operators[i].phase_coarse= calc_phase_coarses(data->operators[i].phase_coarse);
        synth->operators[i].phase_offset = calc_phase_offsets(data->operators[i].phase_offset);
        synth->operators[i].phase_fine = calc_phase_fines(data->operators[i].phase_fine);
        synth->operators[i].semitone = calc_semitones(data->operators[i].semitones);
    }

    for(unsigned i=0; i< KS_NUM_OPERATORS-1; i++){
        synth->mods[i].sync = data->mods[i].sync;
        synth->mods[i].type = data->mods[i].type;
        synth->mods[i].mod_level = calc_mix_levels(data->mods[i].mix);
        synth->mods[i].output_level = ks_1(KS_LEVEL_BITS)-ks_1(KS_LEVEL_BITS-7) - synth->mods[i].mod_level;
        synth->mods[i].fm_level = calc_fm_levels(data->mods[i].fm_level);
    }

    for(unsigned i=0; i< KS_NUM_ENVELOPES; i++) {
        synth->envelopes[i].ratescale = calc_ratescales(data->envelopes[i].ratescale);
        i32 amplevel = calc_levels(data->envelopes[i].level);
        synth->envelopes[i].level = amplevel;
        for(unsigned e=0; e< KS_ENVELOPE_NUM_POINTS; e++){
            synth->envelopes[i].samples[e] = calc_envelope_samples(ctx->sampling_rate, data->envelopes[i].points[e].time);
            synth->envelopes[i].points[e] = calc_envelope_points(data->envelopes[i].points[e].amp);

            i64 point = synth->envelopes[i].points[e];
            point *= amplevel;
            point >>= KS_LEVEL_BITS;
            synth->envelopes[i].points[e] = point;
        }

        synth->envelopes[i].velocity_sens = calc_velocity_sens(data->envelopes[i].velocity_sens);
    }

    for(unsigned e=0; e< KS_NUM_LFOS; e++){
        const u32 freq = calc_lfo_freq(data->lfos[e].freq);
        const u64 freq_11 = ((u64)freq) << KS_TABLE_BITS;
        u64 delta_11 = (u64)freq_11 * ctx->sampling_rate_inv;
        delta_11 >>= KS_SAMPLING_RATE_INV_BITS-(KS_PHASE_BITS - KS_FREQUENCY_BITS);

        synth->lfo_deltas[e]= delta_11;
        synth->lfo_offsets[e] =calc_lfo_offset(data->lfos[e].offset);
        for(unsigned i = 0; i< KS_NUM_OPERATORS; i++){
            synth->operators[i].lfo_op_enable[e] = (data->lfos[e].op_enabled & ks_1(i)) != 0;
        }
        synth->lfo_wave_tables[e] = ctx->wave_tables[ks_wave_index(data->lfos[e].use_custom_wave, data->lfos[e].wave)];
        if(synth->lfo_wave_tables[e] == NULL) {
            ks_error("Failed to set synth for not set wave table %d", ks_wave_index(data->lfos[e].use_custom_wave, data->lfos[e].wave));
            synth->enabled = false;
            return ;
        }

        synth->lfo_levels[e] = calc_lfo_depth(data->lfos[e].level);
    }

    if(data->envelopes[1].level < 128) {
        synth->filter_envelope_base = ks_1(KS_TABLE_BITS);
    } else {
        synth->filter_envelope_base = 0;
    }
    synth->filter_cutoff = calc_filter_cutoff(ctx, data->filter_cutoff);
    synth->filter_type = data->filter_type;
    synth->filter_key_sens = calc_filter_key_sens(data->filter_key_sens);
    synth->filter_q = calc_filter_q(data->filter_q);

    synth->lfo_filter_enabled = 0;
    for(unsigned i=0; i< KS_NUM_LFOS; i++){
        if(data->lfos[i].level == 0) continue;
        if(data->lfos[i].filter_enabled){
            synth->lfo_filter_enabled = i+1;
            break;
        }
    }

    synth->lfo_panpot_enabled = 0;
    for(unsigned i=0; i< KS_NUM_LFOS; i++){
        if(data->lfos[i].level == 0) continue;
        if(data->lfos[i].level != 0 && data->lfos[i].panpot_enabled){
            synth->lfo_panpot_enabled = i+1;
            break;
        }
    }

    synth->panpot = calc_panpot(data->panpot);

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
    for(unsigned i=0; i<KS_FILTER_NUM_LOGS; i++){
        note->filter_in_logs[i] = 0;
        note->filter_out_logs[i] = 0;
    }

    for(unsigned i=0; i<KS_NUM_LFOS; i++){
        note->lfo_phases[i] = synth->lfo_offsets[i];
    }


    for(unsigned i=0; i<KS_NUM_OPERATORS; i++){
        note->operators[i].phase = synth->operators[i].phase_offset;
    }


    for(unsigned i=0; i< KS_NUM_OPERATORS; i++)
    {
        if(synth->operators[i].fixed_frequency)
        {
            note->operators[i].phase_delta = phase_delta_fix_freq(ctx, synth->operators[i].phase_coarse, synth->operators[i].semitone, synth->operators[i].phase_fine);
        }
        else
        {
            note->operators[i].phase_delta = phase_delta(ctx, notenum, synth->operators[i].phase_coarse, synth->operators[i].semitone, synth->operators[i].phase_fine);
        }

    }

    for(unsigned i=0; i<KS_NUM_ENVELOPES; i++){
        note->envelopes[i].update_clock =0;

        i64 ratescales;
        const u32 exp = notenum / synth->envelopes[i].ratescale;
        const u32 val = notenum % synth->envelopes[i].ratescale;
        const i64 exp_val = (ctx->powerof2[ks_1(KS_TABLE_BITS-2) - ks_v(val, KS_TABLE_BITS-2) / synth->envelopes[i].ratescale])  >> exp; // val^ -exp
        //rate scale
        ratescales = exp_val >> 1; // if val == 0 then base = 2

        i64 target;
        i32 velocity;

        velocity = synth->envelopes[i].velocity_sens;
        velocity *=  vel;
        velocity >>= 7;
        velocity += (1 << KS_VELOCITY_SENS_BITS) -  synth->envelopes[i].velocity_sens;

        target = synth->envelopes[i].level;
        target *= velocity;
        target >>= KS_VELOCITY_SENS_BITS;

        note->envelopes[i].level = target;

        //envelope
        for(u32 j=0; j < KS_ENVELOPE_NUM_POINTS; j++)
        {
            target = synth->envelopes[i].points[j];

            target *= velocity;
            target >>= KS_VELOCITY_SENS_BITS;

            note->envelopes[i].points[j] = (i32)target;

            u64 frame = (synth->envelopes[i].samples[j]);
            frame *= ratescales;
            frame >>= KS_RATESCALE_BITS;
            note->envelopes[i].samples[j] = MAX((u32)frame >> KS_UPDATE_PER_FRAMES_BITS, 1u);
        }

        note->envelopes[i].deltas[0] = ks_1(KS_ENVELOPE_BITS);
        note->envelopes[i].deltas[0] /= (i32)note->envelopes[i].samples[0];
        note->envelopes[i].diffs[0] = note->envelopes[i].points[0];
        for(u32 j=1; j < KS_ENVELOPE_NUM_POINTS; j++)
        {
            i32 sub = note->envelopes[i].points[j] - note->envelopes[i].points[j-1];
            note->envelopes[i].diffs[j] = sub;
            note->envelopes[i].deltas[j] =  ks_1(KS_ENVELOPE_BITS);
            note->envelopes[i].deltas[j] /=  ((i32)note->envelopes[i].samples[j]);
        }

        //envelope state init
        note->envelopes[i].now_amp = 0;
        note->envelopes[i].now_point_amp= note->envelopes[i].points[0];
        note->envelopes[i].now_time = note->envelopes[i].samples[0];
        note->envelopes[i].now_delta = note->envelopes[i].deltas[0];
        note->envelopes[i].now_diff = note->envelopes[i].diffs[0];
        note->envelopes[i].now_remain= ks_1(KS_ENVELOPE_BITS);
        note->envelopes[i].now_point = 0;
        note->envelopes[i].state = KS_ENVELOPE_ON;
    }

    note->noise_table_offset = 0;
    note->filter_seek = 0;

    const u32 key_sens = synth->filter_key_sens;
    const u32 exp = notenum / key_sens;
    const u32 val = notenum % key_sens;
    const i64 exp_val = ((u64)ctx->powerof2[ks_v(val, KS_TABLE_BITS-2) / key_sens]) << exp;
    const i64 cutoff = (exp_val * synth->filter_cutoff) >> (KS_POWER_OF_2_BITS+4);

    note->filter_cutoff = cutoff;

}

void ks_synth_note_off (ks_synth_note* note)
{
    for(unsigned i=0; i< KS_NUM_ENVELOPES; i++)
    {
        note->envelopes[i].now_time = note->envelopes[i].samples[KS_ENVELOPE_RELEASE_INDEX];
        note->envelopes[i].now_point = KS_ENVELOPE_RELEASE_INDEX;
        note->envelopes[i].state = KS_ENVELOPE_RELEASED;

        note->envelopes[i].now_remain= ks_1(KS_ENVELOPE_BITS);
        i32 sub = note->envelopes[i].points[KS_ENVELOPE_RELEASE_INDEX] - note->envelopes[i].now_amp;
        note->envelopes[i].now_diff =  sub;
        note->envelopes[i].now_delta = note->envelopes[i].deltas[KS_ENVELOPE_RELEASE_INDEX];
        note->envelopes[i].now_point_amp=  note->envelopes[i].points[KS_ENVELOPE_RELEASE_INDEX];

    }
}

KS_FORCEINLINE static void ks_envelope_update_clock(ks_synth_note* note, int i){
    note->envelopes[i].update_clock = ks_mask(--note->envelopes[i].update_clock, KS_UPDATE_PER_FRAMES_BITS);
}

KS_NOINLINE static void ks_calclate_envelope(const ks_synth_context*ctx, ks_synth_note* note, int i){
    note->envelopes[i].now_remain-= note->envelopes[i].now_delta;
    i64 amp = note->envelopes[i].now_remain;
    amp = (i32)ctx->powerof2[amp >> (KS_ENVELOPE_BITS - KS_TABLE_BITS)] - ks_1(KS_POWER_OF_2_BITS);
    amp *=note->envelopes[i].now_diff;
    amp >>= KS_ENVELOPE_BITS - KS_POWER_OF_2_BITS - 4;
    note->envelopes[i].now_amp = note->envelopes[i].now_point_amp- amp;

    if(--note->envelopes[i].now_time <= 0)
    {
        note->envelopes[i].now_remain= ks_1(KS_ENVELOPE_BITS);
        u32 point;
        switch(note->envelopes[i].state)
        {
        case KS_ENVELOPE_ON:
            if(note->envelopes[i].now_point == KS_ENVELOPE_RELEASE_INDEX-1)
            {
                point = KS_ENVELOPE_RELEASE_INDEX-1;
                note->envelopes[i].state = KS_ENVELOPE_SUSTAINED;
                note->envelopes[i].now_delta = 0;
                note->envelopes[i].now_diff = 0;
                note->envelopes[i].now_time =-1;

            } else{
                note->envelopes[i].now_point ++;
                point = note->envelopes[i].now_point;

                note->envelopes[i].now_delta = note->envelopes[i].deltas[point];
                note->envelopes[i].now_time = note->envelopes[i].samples[point];
                note->envelopes[i].now_diff = note->envelopes[i].diffs[point];
            }
            note->envelopes[i].now_point_amp=  note->envelopes[i].points[point];
            break;
        case KS_ENVELOPE_RELEASED:
            point = note->envelopes[i].now_point;
            note->envelopes[i].now_delta = 0;
            note->envelopes[i].now_diff = 0;
            note->envelopes[i].now_point_amp=  note->envelopes[i].points[point];
            note->envelopes[i].state = KS_ENVELOPE_OFF;
            break;
        }
    }
}

KS_FORCEINLINE static void ks_envelope_process(const ks_synth_context*ctx, ks_synth_note* note, int i){
    if(note->envelopes[i].update_clock == 0){
        ks_calclate_envelope(ctx, note, i);
    }
    ks_envelope_update_clock(note, i);
}

static i32 KS_FORCEINLINE ks_synth_biquad_filter_apply(ks_synth_note* note, i32 in, i32 b0a0, i32 b1a0, i32 b2a0, i32 a1a0, i32 a2a0){
    const unsigned pre1 = ks_mask(note->filter_seek, KS_FILTER_LOG_BITS);
    const unsigned pre2 = ks_mask(note->filter_seek-1, KS_FILTER_LOG_BITS);
    const i32 in0 = in;
    const i32 in1 = note->filter_in_logs[pre1];
    const i32 in2 = note->filter_in_logs[pre2];

    const i32 out1 = note->filter_out_logs[pre1];
    const i32 out2 = note->filter_out_logs[pre2];

    const i64 in0f = (i64)b0a0*in0;
    const i64 in1f = (i64)b1a0*in1;
    const i64 in2f = (i64)b2a0*in2;

    const i64 out1f = (i64)a1a0*out1;
    const i64 out2f = (i64)a2a0*out2;

    const i32 out = (in0f + in1f + in2f - out1f - out2f) >> KS_OUTPUT_BITS;

    note->filter_seek ++;

    const unsigned current = ks_mask(note->filter_seek, KS_FILTER_LOG_BITS);
    note->filter_in_logs[current] = in;
    note->filter_out_logs[current] = out;

    return out;
}

static void KS_FORCEINLINE ks_synth_filter_calclate(const ks_synth_context* ctx, ks_synth_note* note, i32* buf[], u32 i, u8 type, bool lfo, u32 lfo_index){
    const ks_synth* synth = note->synth;

    const u32 cutoff = note->filter_cutoff;
    i32 envelope_amp = (note->envelopes[1].level - note->envelopes[1].now_amp) >> (KS_ENVELOPE_BITS - KS_TABLE_BITS);
    envelope_amp = synth->filter_envelope_base - envelope_amp;
    envelope_amp = MAX(MIN(ks_1(KS_TABLE_BITS), envelope_amp), 0);

    i32 envelope_level = ctx->powerof2[envelope_amp];

    if(lfo) {
       i32 lfo_amp =(ks_1(KS_OUTPUT_BITS) + buf[lfo_index][i]) >> (KS_OUTPUT_BITS - KS_TABLE_BITS + 1);
       lfo_amp = ctx->powerof2[lfo_amp];
       envelope_level = ((i64)envelope_level * lfo_amp) >> (KS_POWER_OF_2_BITS+2);
    }

    const i64 omega0_ = ((i64)cutoff * envelope_level) >> (KS_POWER_OF_2_BITS+2);

    const u32 omega0 = MAX(MIN(omega0_, ks_1(KS_PHASE_MAX_BITS-1)), ks_1(KS_PHASE_BITS));
    const i32 sin_omega0 = ks_sin(ctx, omega0);
    const i32 cos_omega0 = ks_sin(ctx, omega0 + ks_1(KS_PHASE_BITS + KS_TABLE_BITS - 2));

    const i32 alpha = ks_v((i64)sin_omega0, KS_FILTER_Q_BITS) / synth->filter_q;

    i32 a0, a1, a2, b0, b1, b2;

    switch (type) {
    case KS_LOW_PASS_FILTER:
        a0 = ks_1(KS_OUTPUT_BITS) + alpha;
        a1 = (-cos_omega0) << 1;
        a2 = ks_1(KS_OUTPUT_BITS) - alpha;
        b1 = ks_1(KS_OUTPUT_BITS) - cos_omega0;
        b0 = b2 = b1 >> 1;
        break;
    case KS_HIGH_PASS_FILTER:
        a0 = ks_1(KS_OUTPUT_BITS) + alpha;
        a1 = (-cos_omega0) << 1;
        a2 = ks_1(KS_OUTPUT_BITS) - alpha;
        b1 = ks_1(KS_OUTPUT_BITS) + cos_omega0;
        b0 = b2 = b1 >> 1;
        b1 = -b1;
        break;
    case KS_BAND_PASS_FILTER:
        a0 = ks_1 (KS_OUTPUT_BITS)+ alpha;
        a1 = (-cos_omega0) << 1;
        a2 = ks_1(KS_OUTPUT_BITS) - alpha;
        b0 = sin_omega0 >> 1;
        b1 = 0;
        b2 = -b0;
        break;
    }

    i64 a0inv = ks_v(1ll, KS_OUTPUT_BITS*2 + KS_OUTPUT_BITS) / a0;
    note->a1a0 = ((i64)a1 * a0inv) >> (KS_OUTPUT_BITS*2);
    note->a2a0 = ((i64)a2 * a0inv) >> (KS_OUTPUT_BITS*2);

    note->b0a0 = ((i64)b0 * a0inv) >> (KS_OUTPUT_BITS*2);
    note->b1a0 = ((i64)b1 * a0inv) >> (KS_OUTPUT_BITS*2);
    note->b2a0 = ((i64)b2 * a0inv) >> (KS_OUTPUT_BITS*2);

}

static void KS_FORCEINLINE ks_synth_biquad_filter_base(const ks_synth_context* ctx, ks_synth_note* note, i32* buf[], u32 len, u8 type, bool lfo, u32 lfo_index){
    i32* outbuf = buf[0];
    i32* lfobufs[KS_NUM_LFOS];
    for(unsigned i=0; i<KS_NUM_LFOS; i++){
        lfobufs[i] = buf[i+1];
    }

    for(u32 i=0; i< len; i++){
        if(note->envelopes[1].update_clock == 0){
            ks_envelope_process(ctx, note, 1);
            ks_synth_filter_calclate(ctx, note, buf, i, type, lfo, lfo_index);
        }
        ks_envelope_update_clock(note, 1);

        outbuf[i] = ks_synth_biquad_filter_apply(note, outbuf[i], note->b0a0, note->b1a0, note->b2a0, note->a1a0, note->a2a0);
    }
}

#define ks_synth_apply_filter_func(type, lfo) ks_filter_apply_ ## type ## _ ## lfo

#define ks_synth_apply_filter_impl(type, lfo) static void KS_NOINLINE ks_synth_apply_filter_func(type, lfo) (const ks_synth_context* ctx, ks_synth_note* note, i32* buf[], u32 len, u32 lfo_index) { \
    ks_synth_biquad_filter_base(ctx, note, buf, len, type, lfo, lfo_index); \
}

#define ks_synth_apply_filter_impl_lfo(type) \
    ks_synth_apply_filter_impl(type, 0) \
    ks_synth_apply_filter_impl(type, 1)

ks_synth_apply_filter_impl_lfo(KS_LOW_PASS_FILTER)
ks_synth_apply_filter_impl_lfo(KS_HIGH_PASS_FILTER)
ks_synth_apply_filter_impl_lfo(KS_BAND_PASS_FILTER)

static void KS_FORCEINLINE ks_synth_apply_filter(const ks_synth_context* ctx, ks_synth_note* note, i32* buf[], u32 len){
#define lfo_branch(type)  \
    if(synth->lfo_filter_enabled == 0) { \
        ks_synth_apply_filter_func(type, 0)(ctx, note, buf, len, 0); \
    } else { \
        ks_synth_apply_filter_func(type, 1)(ctx, note, buf, len, synth->lfo_filter_enabled); \
    }

    const ks_synth* synth = note->synth;
    switch(note->synth->filter_type){
    case KS_LOW_PASS_FILTER:
        lfo_branch(KS_LOW_PASS_FILTER);
        break;
    case KS_HIGH_PASS_FILTER:
         lfo_branch(KS_HIGH_PASS_FILTER);
        break;
    case KS_BAND_PASS_FILTER:
         lfo_branch(KS_BAND_PASS_FILTER);
        break;
    }
}

static void KS_FORCEINLINE ks_synth_render_mod_base(const ks_synth_context* ctx, ks_synth_note* note, u32 op, u32 pitchbend, i32*buf[], u32 len, u32 mod_type, bool fm, bool sync, bool ams, bool fms, bool noise){
    if(mod_type == KS_MOD_PASS) return;

    i32* outbuf = buf[0];
    i32* lfobufs[KS_NUM_LFOS];
    for(unsigned i=0; i<KS_NUM_LFOS; i++){
        lfobufs[i] = buf[i+1];
    }

    const ks_synth* synth = note->synth;
    const u32 preop = (op-1) & 3;

    u32 bend_phase_delta = ((u64)note->operators[op].phase_delta * pitchbend) >> KS_PITCH_BEND_BITS;


    u32 sync_phase ;
    if(sync){
        sync_phase = note->operators[preop].phase - len*note->operators[preop].phase_delta;
    }
    const u32 shift = noise ? (KS_PHASE_BITS + (KS_TABLE_BITS-5)) : KS_PHASE_BITS;

    for(unsigned i=0; i<len; i++){
        i32 out;
        i64 fm_amount = 0;
        if(fm) {
            fm_amount = outbuf[i];
            fm_amount *= synth->mods[preop].fm_level;
            fm_amount >>= (KS_LEVEL_BITS - 3);
            fm_amount = ks_v(fm_amount, KS_PHASE_MAX_BITS - KS_OUTPUT_BITS);
        }
        switch (mod_type) {
        case KS_MOD_MIX:{
            out  = synth->operators[op].wave_table[ks_mask((note->operators[op].phase + fm_amount) >> KS_PHASE_BITS, KS_TABLE_BITS)];
            break;
        }
        case KS_MOD_MUL:{
            out  = synth->operators[op].wave_table[ks_mask((note->operators[op].phase + fm_amount) >> KS_PHASE_BITS, KS_TABLE_BITS)];
            out *=  outbuf[i];
            out >>= KS_OUTPUT_BITS;
            break;
        }
        case KS_MOD_AM:{
            out  = synth->operators[op].wave_table[ks_mask((note->operators[op].phase + fm_amount) >> KS_PHASE_BITS, KS_TABLE_BITS)];
            out += ks_1(KS_OUTPUT_BITS);
            out *=  outbuf[i];
            out >>= KS_OUTPUT_BITS+1;
            break;
        }
        case KS_NUM_MODS:{
            out= synth->operators[0].wave_table[ks_mask((note->operators[0].phase>> shift ) + (noise ? note->noise_table_offset : 0), KS_TABLE_BITS)];
            break;
        }
        }

        if(mod_type != KS_NUM_MODS){
            const i32 mod = ((i64)outbuf[i]* synth->mods[preop].mod_level) >> KS_LEVEL_BITS;
            const i32 car = ((i64)out * synth->mods[preop].output_level) >> KS_LEVEL_BITS;
            outbuf[i] = mod + car;
        } else {
            outbuf[i] = out;

            if(noise){
                if(ks_mask(note->operators[0].phase >> shift, KS_TABLE_BITS) > ks_mask((note->operators[0].phase + note->operators[0].phase_delta) >> shift, KS_TABLE_BITS)){
                    srand(note->noise_table_offset);
                    note->noise_table_offset = rand();
                }
            }
        }

        if(ams){
            i32 factor = ks_1(KS_OUTPUT_BITS) + lfobufs[0][i];
            outbuf[i] = ((i64)factor*outbuf[i]) >> KS_OUTPUT_BITS;
        }

        u32 phase_delta = bend_phase_delta;

        if(fms) {
            i32 factor = lfobufs[1][i] + ks_1(KS_OUTPUT_BITS);
            phase_delta = ((i64)factor* phase_delta) >> KS_OUTPUT_BITS;
        }

        note->operators[op].phase += phase_delta;

        if(sync){
            u32 next_sync_phase =  ks_mask(sync_phase + note->operators[preop].phase_delta, KS_PHASE_MAX_BITS);
            if(ks_mask(sync_phase, KS_PHASE_MAX_BITS) > next_sync_phase){
                note->operators[op].phase = 0;
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
        const bool fm = synth->mods[preop].fm_level != 0; \
        const bool sync = synth->mods[preop].sync; \
        const bool ams = synth->operators[op].lfo_op_enable[0]; \
        const bool fms = synth->operators[op].lfo_op_enable[1]; \
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
    switch (synth->mods[op-1].type) {
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
    const bool noise_table = synth->operators[0].wave_table == ctx->wave_tables[KS_WAVE_NOISE];
    const bool ams = synth->operators[0].lfo_op_enable[0];
    const bool fms = synth->operators[0].lfo_op_enable[1];
    if(ams){
        fms_branch(1);
    } else {
        fms_branch(0);
    }

#undef noise_table_branch
#undef fms_branch
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

static void KS_NOINLINE ks_synth_apply_envelope(const ks_synth_context* ctx, ks_synth_note* note, i32*buf, u32 len){
    for(unsigned i=0; i<len; i++){
        ks_envelope_process(ctx, note, 0);

        i64 amp = note->envelopes[0].now_amp;
        amp *= buf[i];
        amp >>= KS_ENVELOPE_BITS;

        buf[i] = amp;
    }
}

static void KS_FORCEINLINE ks_synth_apply_panpot_base(const ks_synth_context*ctx, ks_synth_note*note, i32* buf, i32*bufs[], u32 len, bool lfo, u32 lfo_index){
    const ks_synth* synth = note->synth;
    i32* inbuf = bufs[0];
    i32* lfobuf = bufs[lfo_index];

    i16 pan_left;
    i16 pan_right;

    if(!lfo){
        ks_calc_panpot(ctx, &pan_left, &pan_right, synth->panpot << (KS_PANPOT_BITS - 7));
    }

    for(unsigned i=0; i<len; i++){
        if(lfo){
            const u32 fac = 92682; // sqrt(2)<<16

            ks_calc_panpot(ctx, &pan_left, &pan_right, synth->panpot << (KS_PANPOT_BITS - 7));

            i32 pan = ks_1(KS_PANPOT_BITS-1) + (lfobuf[i] >> (KS_OUTPUT_BITS - KS_PANPOT_BITS +1));
            i16 pan_left2, pan_right2;

            ks_calc_panpot(ctx, &pan_left2, &pan_right2, pan);

            const i32 pan_left3 = ((i32)pan_left2 * fac) >> 16;
            const i32 pan_right3 = ((i32)pan_right2 * fac) >> 16;
            pan_left = ((i32)pan_left * pan_left3) >> KS_OUTPUT_BITS;
            pan_right = ((i32)pan_right* pan_right3) >> KS_OUTPUT_BITS;
        }

        buf[2*i] = ks_apply_panpot(inbuf[i], pan_left) >> 1;
        buf[2*i+1] = ks_apply_panpot(inbuf[i], pan_right) >> 1;
    }
}

#define ks_synth_apply_panpot_func(lfo) ks_apply_panpot_ ## lfo

#define ks_synth_apply_panpot_impl(lfo) \
    static void KS_NOINLINE ks_synth_apply_panpot_func(lfo) (const ks_synth_context* ctx, ks_synth_note* note, i32* buf, i32* bufs[], u32 len, u32 lfo_index) { \
        ks_synth_apply_panpot_base(ctx, note, buf, bufs, len, lfo, lfo_index); \
    }

ks_synth_apply_panpot_impl(0)
ks_synth_apply_panpot_impl(1)

KS_FORCEINLINE static void ks_synth_apply_panpot_branch(const ks_synth_context* ctx, ks_synth_note* note, i32* buf, i32*bufs[], u32 len) {
    const ks_synth* synth = note->synth;
    if(synth->lfo_panpot_enabled == 0) {
        ks_synth_apply_panpot_func(0)(ctx, note, buf, bufs, len, 0);
    } else {
        ks_synth_apply_panpot_func(1)(ctx, note, buf, bufs, len, synth->lfo_panpot_enabled);
    }
}

void ks_synth_render(const ks_synth_context*ctx, ks_synth_note* note, u32 volume, u32 pitchbend, i32 *buf, u32 len)
{
    if(note->envelopes[0].state != KS_ENVELOPE_OFF){
        const ks_synth* synth = note->synth;
        const size_t tmpbuf_len = len / 2;

        i32* bufs[KS_NUM_LFOS+1];

        bufs[0] = (i32*)malloc(sizeof(i32) * tmpbuf_len);


        for(unsigned i=0; i<KS_NUM_LFOS; i++){
            if(synth->lfo_levels[i] != 0){
                bufs[i+1]= (i32*)malloc(sizeof(i32) * tmpbuf_len);
                ks_synth_render_lfo(ctx, note, i, bufs[i+1], tmpbuf_len);
            }
        }

        ks_synth_render_0(ctx, note, pitchbend, bufs, tmpbuf_len);
        for(unsigned i=1; i< KS_NUM_OPERATORS; i++){
            ks_synth_render_branch(ctx, note, i, pitchbend, bufs, tmpbuf_len);
        }
        ks_synth_apply_filter(ctx, note, bufs, tmpbuf_len);
        ks_synth_apply_envelope(ctx, note, bufs[0], tmpbuf_len);

        for(unsigned i=0; i<tmpbuf_len; i++){
            bufs[0][i] = ((i64)bufs[0][i] * volume)>> KS_VOLUME_BITS;
        }

        ks_synth_apply_panpot_branch(ctx, note, buf, bufs, tmpbuf_len);
        free(bufs[0]);
        for(unsigned i=0; i<KS_NUM_LFOS; i++){
            if(synth->lfo_levels[i] != 0){
                free(bufs[i+1]);
            }
        }

    } else {
        memset(buf, 0, len*sizeof(i32));
    }

}

