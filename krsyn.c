#include "krsyn.h"


krsyn_core* krsyn_new(uint32_t freq)
{
    krsyn_core* ret = malloc(sizeof(krsyn_core));
    ret->smp_freq = freq;

    for(int i=0; i< KRSYN_TABLE_LENGTH; i++)
    {
        double phase = 2.0 * M_PI * i / KRSYN_TABLE_LENGTH;
        ret->sin_table[i] = (int16_t)(sin( phase ) * INT16_MAX);
    }

    for(int i=0; i<128; i++)
    {
        double exp = (69-i)/12.0;
        ret->ratescale[i] = (uint32_t)((1<<KRSYN_RS_SHIFT) * pow(2.0, exp));
        ret->ratescale[i] -= 1 << KRSYN_RS_SHIFT;

        ret->note_freq[i] = (uint32_t)(440.0 * (1<<KRSYN_FREQUENCY_SHIFT) * pow(2.0, -exp));
    }

    double amp = (1 << KRSYN_KS_CURVE_SHIFT) - 1;
    for(int i=0; i<KRSYN_KS_CURVE_TABLE_LENGTH; i++)
    {
        ret->ks_curves[KRSYN_KS_CURVE_ED][i] = KRSYN_KS_CURVE_TABLE_LENGTH-i;
        ret->ks_curves[KRSYN_KS_CURVE_ED][i] *= (int32_t)(amp / KRSYN_KS_CURVE_TABLE_LENGTH);


        ret->ks_curves[KRSYN_KS_CURVE_EU][i] = i+1;
        ret->ks_curves[KRSYN_KS_CURVE_EU][i] *= (int32_t)(amp / KRSYN_KS_CURVE_TABLE_LENGTH);

        double exp = -i/12.0;

        ret->ks_curves[KRSYN_KS_CURVE_LD][i] =  (int32_t)(pow(2.0, exp) * amp);
        ret->ks_curves[KRSYN_KS_CURVE_LU][i] =  (int32_t)(amp - ret->ks_curves[KRSYN_KS_CURVE_LD][i]);
    }

    for(int i=0; i<KRSYN_LFO_TABLE_LENGTH; i++)
    {
        if(i < KRSYN_LFO_TABLE_LENGTH/2)
        {
            ret->lfo_tables[KRSYN_LFO_WAVE_TRIANGLE][i] = i < KRSYN_LFO_TABLE_LENGTH/4
                ? i * INT16_MAX / (KRSYN_LFO_TABLE_LENGTH/4)
                : (KRSYN_LFO_TABLE_LENGTH/2-i) * INT16_MAX / (KRSYN_LFO_TABLE_LENGTH/4);
            ret->lfo_tables[KRSYN_LFO_WAVE_SAW_UP][i] = i * INT16_MAX / (KRSYN_LFO_TABLE_LENGTH/2);
            ret->lfo_tables[KRSYN_LFO_WAVE_SQUARE][i] = INT16_MAX;
        }
        else
        {
            int i2 = i-KRSYN_LFO_TABLE_LENGTH/2;
            ret->lfo_tables[KRSYN_LFO_WAVE_TRIANGLE][i] = - ret->lfo_tables[KRSYN_LFO_WAVE_TRIANGLE][i2];
            ret->lfo_tables[KRSYN_LFO_WAVE_SAW_UP][i] = (i2) * INT16_MAX / (KRSYN_LFO_TABLE_LENGTH/2) - INT16_MAX;
            ret->lfo_tables[KRSYN_LFO_WAVE_SQUARE][i] = 0;
        }

        ret->lfo_tables[KRSYN_LFO_WAVE_SAW_DOWN][i] = - ret->lfo_tables[KRSYN_LFO_WAVE_SAW_UP][i];

        double phase = 2.0 * M_PI * i / KRSYN_LFO_TABLE_LENGTH;
        ret->lfo_tables[KRSYN_LFO_WAVE_SIN][i] = (int16_t)(sin(phase) * INT16_MAX);
     }

     for(int i=0; i<KRSYN_MIPMAP_TABLE_LENGTH; i++)
     {
         if(i < KRSYN_MIPMAP_TABLE_LENGTH/2)
         {
             ret->triangle_table[i] = i < KRSYN_MIPMAP_TABLE_LENGTH/4
                 ? i * INT16_MAX / (KRSYN_MIPMAP_TABLE_LENGTH/4)
                 : (KRSYN_MIPMAP_TABLE_LENGTH/2-i) * INT16_MAX / (KRSYN_MIPMAP_TABLE_LENGTH/4);
         }
         else
         {
             int i2 = i-KRSYN_MIPMAP_TABLE_LENGTH/2;
             ret->triangle_table[i] = - ret->triangle_table[i2];
         }
         ret->saw_table[i] = INT16_MAX - INT16_MAX * i / (KRSYN_MIPMAP_TABLE_LENGTH/2);
     }

    return ret;
}

void krsyn_free(krsyn_core* core)
{
    free(core);
}





void krsyn_fm_set_data_default(krsyn_fm_data* data)
{
    for(unsigned i=0; i<KRSYN_NUM_OPERATORS; i++)
    {
        data->phase_coarses[i].fixed_frequency = false;
        data->phase_coarses[i].value = 2;
        data->phase_fines[i] = 0;
        data->phase_dets[i] = 0;

        data->envelop_times[0][i] = 0;
        data->envelop_times[1][i] = UINT8_MAX;
        data->envelop_times[2][i] = UINT8_MAX;
        data->envelop_times[3][i] = UINT8_MAX;

        data->envelop_points[0][i] = UINT8_MAX;
        data->envelop_points[1][i] = UINT8_MAX;
        data->envelop_points[2][i] = UINT8_MAX;
        data->envelop_points[3][i] = UINT8_MAX;

        data->envelop_release_times[i] = 128;

        data->velocity_sens[i] = UINT8_MAX;

        data->ratescales[i] = 32; // about 0.125
        data->ks_low_depths[i] = 0;
        data->ks_high_depths[i] = 0;
        data->ks_mid_points[i] = 69;
        data->ks_curve_types[i].left = 0;
        data->ks_curve_types[i].right = 0;

        data->lfo_ams_depths[i] = 0;
    }

    data->algorithm = 0;
    data->feedback_level = 0;

    data->lfo_table_type = KRSYN_LFO_WAVE_TRIANGLE;
    data->lfo_fms_depth = 0;
    data->lfo_freq = 128; // 1 Hz ?
    data->lfo_det = 0;
}

static inline void fm_op_set(const krsyn_core* core, krsyn_fm* fm, const krsyn_fm_data* data)
{
    for(unsigned i=0; i<KRSYN_NUM_OPERATORS; i++)
    {
        fm->fixed_frequency[i] = calc_fixed_frequency(data->phase_coarses[i].fixed_frequency);
        fm->phase_coarses[i] = calc_phase_coarses(data->phase_coarses[i].value);

        fm->phase_fines[i] = calc_phase_fines(data->phase_fines[i]);
        fm->phase_dets[i] = calc_phase_dets(data->phase_dets[i]);

        for(unsigned e=0; e < KRSYN_ENVELOP_NUM_POINTS; e++)
        {
            fm->envelop_points[e][i] = calc_envelop_points(data->envelop_points[e][i]);
            fm->envelop_samples[e][i] = calc_envelop_samples(core->smp_freq, data->envelop_times[e][i]);
        }

        fm->envelop_release_samples[i] =  calc_envelop_samples(core->smp_freq, data->envelop_release_times[i]);

        fm->velocity_sens[i] = calc_velocity_sens(data->velocity_sens[i]);

        fm->ratescales[i] = calc_ratescales(data->ratescales[i]);
        fm->ks_low_depths[i] = calc_ks_low_depths(data->ks_low_depths[i]);
        fm->ks_high_depths[i] = calc_ks_high_depths(data->ks_high_depths[i]);
        fm->ks_mid_points[i] = calc_ks_mid_points(data->ks_mid_points[i]);
        fm->ks_curve_types[0][i] = calc_ks_curve_types_left(data->ks_curve_types[i].left);
        fm->ks_curve_types[1][i] = calc_ks_curve_types_right(data->ks_curve_types[i].right);

        fm->lfo_ams_depths[i] = calc_lfo_ams_depths(data->lfo_ams_depths[i]);
    }
    fm->lfo_ams_enabled = *(const uint32_t*)data->lfo_ams_depths != 0;


}

static inline void fm_common_set(krsyn_fm* fm, const krsyn_fm_data* data)
{
    fm->algorithm = calc_algorithm(data->algorithm);
    fm->feedback_level = calc_feedback_level(data->feedback_level);

    fm->lfo_table_type = calc_lfo_table_type(data->lfo_table_type);
    fm->lfo_fms_depth = calc_lfo_fms_depth(data->lfo_fms_depth);
    fm->lfo_fms_enabled = fm->lfo_fms_depth != 0;
    fm->lfo_freq = krsyn_exp_u(data->lfo_freq, 1<<(KRSYN_FREQUENCY_SHIFT-5), 4);
    fm->lfo_det = krsyn_linear_u(data->lfo_det, 0, KRSYN_PHASE_MAX);
}


void krsyn_fm_set(const krsyn_core* core, krsyn_fm* fm, const krsyn_fm_data* data)
{
    fm_op_set(core, fm, data);
    fm_common_set(fm, data);
}


static inline uint32_t phase_lfo_delta(const krsyn_core* core, uint32_t freq)
{
    uint64_t freq_11 = ((uint64_t)freq) << KRSYN_TABLE_SHIFT;
    uint64_t delta_11 = (uint32_t)(freq_11 / core->smp_freq);

    return (uint32_t)delta_11;
}

static inline uint32_t phase_delta_fix_freq(const krsyn_core* core, uint32_t coarse)
{
    uint32_t freq = core->note_freq[coarse];
    uint64_t freq_11 = ((uint64_t)freq) << KRSYN_TABLE_SHIFT;

    uint64_t delta_11 = (uint32_t)(freq_11 / core->smp_freq);
    return delta_11;
}

static inline uint32_t phase_delta(const krsyn_core* core, uint8_t notenum, uint32_t coarse, uint32_t fine)
{
    uint32_t freq = core->note_freq[notenum];
    uint64_t freq_11 = ((uint64_t)freq) << KRSYN_TABLE_SHIFT;

    uint32_t freq_rate = (coarse << (KRSYN_PHASE_FINE_SHIFT - KRSYN_PHASE_COARSE_SHIFT)) + fine;
    uint64_t delta_11 = (uint32_t)(freq_11 / core->smp_freq);
    delta_11 *= freq_rate;
    delta_11 >>= KRSYN_PHASE_FINE_SHIFT;

    return (uint32_t)delta_11;
}

static inline uint32_t ks_lc(const krsyn_core* core, uint32_t index_16, int curve_type)
{
    uint32_t index_m = (uint32_t)(index_16>> KRSYN_KS_CURVE_SHIFT);

    if(index_m >= KRSYN_KS_CURVE_TABLE_LENGTH-1)
    {
        return core->ks_curves[curve_type][KRSYN_KS_CURVE_TABLE_LENGTH-1];
    }

    uint32_t index_b = index_m+1;

    uint32_t under_fixed_b = index_16 & ((1<<KRSYN_KS_CURVE_SHIFT)-1);
    uint32_t under_fixed_m = (1<<KRSYN_KS_CURVE_SHIFT) - under_fixed_b;

    uint32_t ret = core->ks_curves[curve_type][index_m] * under_fixed_m +
        core->ks_curves[curve_type][index_b] * under_fixed_b;

    ret >>= KRSYN_KS_CURVE_SHIFT;

    return ret;
}

static inline uint32_t ks_value(const krsyn_core* core, const krsyn_fm *fm, uint32_t table_index, uint32_t table_range, bool low_note, unsigned i)
{
    uint64_t index_16 = table_index;
    index_16 <<= KRSYN_KS_CURVE_MAX_SHIFT;
    index_16 /= table_range;
    index_16 *= (low_note ? fm->ks_low_depths[i] : fm->ks_high_depths[i]);
    index_16 >>= KRSYN_KS_DEPTH_SHIFT;

    int curve_type = fm->ks_curve_types[low_note ? 0 : 1][i];
    return ks_lc(core, (uint32_t)index_16, curve_type);
}

static inline uint32_t keyscale(const krsyn_core* core, const krsyn_fm *fm, uint8_t notenum, unsigned i)
{
    if(notenum < fm->ks_mid_points[i])
    {
        if(fm->ks_curve_types[0][i] <= KRSYN_KS_CURVE_ED || fm->ks_curve_types[1][i] > KRSYN_KS_CURVE_ED)
        {
            return ks_value(core, fm, fm->ks_mid_points[i] - notenum - 1, fm->ks_mid_points[i], true, i);
        }
        else
        {
            return 1<< KRSYN_KS_CURVE_SHIFT;
        }
    }

    if(fm->ks_curve_types[1][i] <= KRSYN_KS_CURVE_ED || fm->ks_curve_types[0][i] > KRSYN_KS_CURVE_ED )
    {
        return ks_value(core, fm, notenum - fm->ks_mid_points[i], KRSYN_KS_CURVE_TABLE_LENGTH - fm->ks_mid_points[i], false, i);
    }
    return 1<< KRSYN_KS_CURVE_SHIFT;
}

void krsyn_fm_noteon(const krsyn_core* core, const krsyn_fm *fm, krsyn_fm_note* note, uint8_t notenum, uint8_t vel)
{
    note->now_frame = 0;
    note->feedback_log =0;

    note->lfo_log=0;
    note->lfo_phase= fm->lfo_det;
    note->lfo_delta = phase_lfo_delta(core, fm->lfo_freq);

    for(unsigned i=0; i< KRSYN_NUM_OPERATORS; i++)
    {
        //phase
        note->phases[i] = fm->phase_dets[i];
        if(fm->fixed_frequency[i])
        {
            note->phase_deltas[i] = phase_delta_fix_freq(core, fm->phase_coarses[i]);
        }
        else
        {
            note->phase_deltas[i] = phase_delta(core, notenum, fm->phase_coarses[i], fm->phase_fines[i]);
        }

        note->output_log[i] = 0;

        //rate scale
        int64_t ratescales = fm->ratescales[i];
        ratescales *= core->ratescale[notenum];
        ratescales >>= KRSYN_RS_SHIFT;
        ratescales += 1<<KRSYN_RS_SHIFT;

        //key scale
        uint64_t keysc = keyscale(core, fm, notenum, i);

        int64_t target;
        uint32_t velocity;

        //envelop
        for(unsigned j=0; j < KRSYN_ENVELOP_NUM_POINTS; j++)
        {
            velocity = fm->velocity_sens[i];
            velocity *=  127-vel;
            velocity >>= 7;
            velocity = (1 << KRSYN_VELOCITY_SENS_SHIFT) -  velocity;

            target = fm->envelop_points[j][i];
            target *= keysc;
            target >>= KRSYN_KS_CURVE_SHIFT;
            target *= velocity;
            target >>= KRSYN_VELOCITY_SENS_SHIFT;

            note->envelop_points[j][i] = (int32_t)target;

            uint64_t frame = (fm->envelop_samples[j][i]);
            frame *= ratescales;
            frame >>= KRSYN_RS_SHIFT;
            note->envelop_samples[j][i] = MAX((uint32_t)frame, 1u);
        }

        uint64_t frame = (fm->envelop_release_samples[i]);
        frame *= ratescales;
        frame >>= KRSYN_RS_SHIFT;
        note->envelop_release_samples[i] = MAX((uint32_t)frame, 1u);

        note->envelop_deltas[0][i] = note->envelop_points[0][i] / note->envelop_samples[0][i];
        for(unsigned j=1; j < KRSYN_ENVELOP_NUM_POINTS; j++)
        {
            note->envelop_deltas[j][i] = (note->envelop_points[j][i] - note->envelop_points[j-1][i]);
            note->envelop_deltas[j][i] /= (int32_t)note->envelop_samples[j][i];
        }

        //envelop state init
        note->envelop_now_amps[i] = 0;
        note->envelop_now_times[i] = note->envelop_samples[0][i];
        note->envelop_now_deltas[i] = note->envelop_deltas[0][i];
        note->envelop_now_points[i] = 0;
        note->envelop_states[i] = KRSYN_ENVELOP_ON;

    }
}

void krsyn_fm_noteoff (krsyn_fm_note* note)
{
    for(unsigned i=0; i< KRSYN_NUM_OPERATORS; i++)
    {
        note->envelop_states[i] = KRSYN_ENVELOP_RELEASED;
        note->envelop_now_times[i] = note->envelop_release_samples[i];
        note->envelop_now_deltas[i] = - note->envelop_now_amps[i] / (int32_t)note->envelop_now_times[i];
    }
}


// liniar complimentation
static inline int16_t table_value_lc(const int16_t* table, uint32_t phase, unsigned mask)
{
    unsigned index_m = phase >> KRSYN_PHASE_SHIFT;
    unsigned index_b = (index_m + 1);

    uint32_t under_fixed_b = phase & KRSYN_PHASE_SHIFT_MASK;
    uint32_t under_fixed_m = KRSYN_PHASE_1 - under_fixed_b;

    int64_t sin_31 = table[index_m & mask] * under_fixed_m +
        table[index_b & mask] * under_fixed_b;
    int16_t sin_15 = sin_31 >> KRSYN_PHASE_SHIFT;

    return sin_15;
}

// sin table value
static inline int16_t sin_t(const krsyn_core* core, uint32_t phase)
{
    return  table_value_lc(core->sin_table, phase,  KRSYN_TABLE_MASK);
}

// complimentation ato de naosu
static inline int16_t table_value_sc(const krsyn_core* core, const int16_t* table, uint32_t phase, unsigned mask, unsigned stride_shift)
{
    uint32_t index_m = phase;
    index_m >>= KRSYN_PHASE_SHIFT + stride_shift;
    index_m <<= stride_shift;
    index_m += 1<<(stride_shift-1);
    uint32_t index_b = (index_m + (1<<stride_shift));

    int32_t under_fixed_b = phase & ((1 << (KRSYN_PHASE_SHIFT + stride_shift)) - 1);
    under_fixed_b >>= stride_shift + KRSYN_PHASE_SHIFT + 1 - KRSYN_TABLE_SHIFT;
    under_fixed_b = core->sin_table[(under_fixed_b + KRSYN_TABLE_LENGTH*3/4) & KRSYN_TABLE_MASK];
    under_fixed_b += 1<<KRSYN_OUTPUT_SHIFT;
    int32_t under_fixed_m = (1 << (KRSYN_PHASE_SHIFT)) - under_fixed_b;

    int64_t sin_31 = table[index_m & mask] * under_fixed_m +
        table[index_b & mask] * under_fixed_b;
    sin_31 >>= KRSYN_PHASE_SHIFT;

    uint32_t amp = 1<<KRSYN_NUM_TABLE_MIPMAPS;
    amp -= (1<<stride_shift) - 1;
    amp = (1l<<30) / amp;
    amp >>= (31 - (KRSYN_NUM_TABLE_MIPMAPS*2));

    sin_31 *= amp;
    sin_31 >>= KRSYN_NUM_TABLE_MIPMAPS;

    return (int16_t)sin_31;
}

static inline int16_t lfo_t(const krsyn_core*core, uint8_t type, uint32_t phase)
{
    return table_value_lc(core->lfo_tables[type], phase >> ( KRSYN_TABLE_SHIFT - KRSYN_LFO_TABLE_SHIFT ), KRSYN_LFO_TABLE_MASK);
}

static inline int16_t saw_t(const krsyn_core*core, uint8_t mipmap, uint32_t phase)
{

    return table_value_sc(core, core->saw_table, phase >> (KRSYN_TABLE_SHIFT - KRSYN_NUM_TABLE_MIPMAPS), KRSYN_MIPMAP_TABLE_MASK, mipmap);
}

static inline int16_t triangle_t(const krsyn_core*core, uint8_t mipmap, uint32_t phase)
{
    return table_value_sc(core, core->triangle_table, phase >> (KRSYN_TABLE_SHIFT - KRSYN_NUM_TABLE_MIPMAPS), KRSYN_MIPMAP_TABLE_MASK, mipmap);
}

static inline int32_t envelop_apply(uint32_t amp, int32_t in)
{
    int32_t out = amp;
    out *=in;
    out >>= 16;
    return out;
}

static inline int32_t output_mod(const krsyn_core* core, uint32_t phase, int32_t mod, uint32_t envelop)
{
    // mod<<(KRSYN_TABLE_SHIFT + 1) = double table length = 4pi
    int32_t out = sin_t(core, phase + (mod<<(KRSYN_TABLE_SHIFT + 1)));
    return envelop_apply(envelop, out);
}

static inline int16_t fm_frame(const krsyn_core* core, const krsyn_fm* fm, krsyn_fm_note* note, uint8_t algorithm)
{
    int32_t out;
    int32_t feedback;
    int32_t output[KRSYN_NUM_OPERATORS];

    if(algorithm == 0)
    {
        // +-----+
        // +-[1]-+-[2]---[3]---[4]--->

        output[3] = output_mod(core, note->phases[3], note->output_log[2], note->envelop_now_amps[3]);
        output[2] = output_mod(core, note->phases[2], note->output_log[1], note->envelop_now_amps[2]);
        output[1] = output_mod(core, note->phases[1], note->output_log[0], note->envelop_now_amps[1]);
        output[0] = output_mod(core, note->phases[0], note->feedback_log , note->envelop_now_amps[0]);
        out = note->output_log[3];

        feedback = fm->feedback_level;
        feedback *= output[0];
        feedback >>= KRSYN_FEEDBACK_LEVEL_SHIFT;

    }
    if(algorithm == 1)
    {
        // +-----+
        // +-[1]-+
        //       +-[3]---[4]--->
        //   [2]-+
        output[3] = output_mod(core, note->phases[3], note->output_log[2]                      , note->envelop_now_amps[3]);
        output[2] = output_mod(core, note->phases[2], note->output_log[1] + note->output_log[0], note->envelop_now_amps[2]);
        output[1] = output_mod(core, note->phases[1], 0                                        , note->envelop_now_amps[1]);
        output[0] = output_mod(core, note->phases[0], note->feedback_log                       , note->envelop_now_amps[0]);
        out = note->output_log[3];

        feedback = fm->feedback_level;
        feedback *= output[0];
        feedback >>= KRSYN_FEEDBACK_LEVEL_SHIFT;
    }
    if(algorithm == 4)
    {
        // +-----+
        // +-[1]-+-[2]-+
        //             +--->
        // +-[3]---[4]-+
        output[3] = output_mod(core, note->phases[3], note->output_log[2] , note->envelop_now_amps[3]);
        output[2] = output_mod(core, note->phases[2], 0                   , note->envelop_now_amps[2]);
        output[1] = output_mod(core, note->phases[1], note->output_log[0] , note->envelop_now_amps[1]);
        output[0] = output_mod(core, note->phases[0], note->feedback_log  , note->envelop_now_amps[0]);
        out = note->output_log[1] + note->output_log[3];

        feedback = fm->feedback_level;
        feedback *= output[0];
        feedback >>= KRSYN_FEEDBACK_LEVEL_SHIFT;
    }
    if(algorithm ==7)
    {
        // +-----+
        // +-[1]-+ [2]   [3]   [4]
        //    +-----+-----+-----+--->
        output[3] = output_mod(core, note->phases[3], 0                     , note->envelop_now_amps[3]);
        output[2] = output_mod(core, note->phases[2], 0                     , note->envelop_now_amps[2]);
        output[1] = output_mod(core, note->phases[1], 0                     , note->envelop_now_amps[1]);
        output[0] = output_mod(core, note->phases[0], note->feedback_log    , note->envelop_now_amps[0]);
        out = note->output_log[0] + note->output_log[1] + note->output_log[2] + note->output_log[3];

        feedback = fm->feedback_level;
        feedback *= output[0];
        feedback >>= KRSYN_FEEDBACK_LEVEL_SHIFT;
    }
    if(algorithm == 8)
    {
        output[0] = envelop_apply(note->envelop_now_amps[0], table_value_lc(core->triangle_table, note->phases[0] >> (KRSYN_TABLE_SHIFT - 1), 1));
        out = note->output_log[0];
    }
    if(algorithm >= 10 && algorithm < 10 + KRSYN_NUM_TABLE_MIPMAPS)
    {
        out = envelop_apply(note->envelop_now_amps[0], triangle_t(core, algorithm-10, note->phases[0]));
    }
    if(algorithm >= 20 && algorithm < 20 + KRSYN_NUM_TABLE_MIPMAPS)
    {
        output[0] = envelop_apply(note->envelop_now_amps[0], saw_t(core, algorithm-20, note->phases[0]));
        output[1] = envelop_apply(note->envelop_now_amps[1], saw_t(core, algorithm-20, note->phases[1]));

        out = note->output_log[0] - note->output_log[1];
    }


    for(unsigned i =0; i< KRSYN_NUM_OPERATORS; i++)
    {
        note->output_log[i] = output[i];
    }
    note->feedback_log = feedback;

    return out >> 1;
}

static inline void envelop_next(krsyn_fm_note* note)
{
    for(unsigned i=0; i<KRSYN_NUM_OPERATORS; i++)
    {
        if(note->envelop_now_times[i] == 0)
        {
            switch(note->envelop_states[i])
            {
            case KRSYN_ENVELOP_ON:
                note->envelop_now_amps[i] = note->envelop_points[note->envelop_now_points[i]][i];
                note->envelop_now_points[i] ++;
                if(note->envelop_now_points[i] == KRSYN_ENVELOP_NUM_POINTS)
                {
                    note->envelop_states[i] = KRSYN_ENVELOP_SUSTAINED;
                    note->envelop_now_deltas[i] = 0;
                }
                else
                {
                    note->envelop_now_deltas[i] = note->envelop_deltas[note->envelop_now_points[i]][i];
                    note->envelop_now_times[i] = note->envelop_samples[note->envelop_now_points[i]][i];
                }
                break;
            case KRSYN_ENVELOP_RELEASED:
                note->envelop_now_amps[i] = 0;
                note->envelop_now_deltas[i] = 0;
                note->envelop_states[i] = KRSYN_ENVELOP_OFF;
                break;
            }
        }
    }
}

static inline void fm_lfo_frame(const krsyn_core* core, const krsyn_fm* fm, krsyn_fm_note* note, uint32_t delta[],
                                      bool ams, bool fms)
{
    if(fms)
    {
        int64_t depth = note->lfo_log;      // -1.0 ~ 1.0
        depth *= fm->lfo_fms_depth;
        depth >>= KRSYN_LFO_DEPTH_SHIFT;    // MAX -1.0 ~ 1.0
        depth += 1<<KRSYN_OUTPUT_SHIFT;     // MAX 0.0 ~ 2.0, mean 1.0

        for(unsigned j=0; j<KRSYN_NUM_OPERATORS; j++)
        {
            int64_t d = note->phase_deltas[j];
            d *= depth;
            d >>= KRSYN_LFO_DEPTH_SHIFT;
            delta[j] = (uint32_t)d;
        }
    }
    else
    {
        for(unsigned j=0; j<KRSYN_NUM_OPERATORS; j++)
        {
            delta[j] = note->phase_deltas[j];
        }
    }

    if(ams)
    {
        for(unsigned j=0; j<KRSYN_NUM_OPERATORS; j++)
        {
            int64_t depth = note->lfo_log;      // -1.0 ~ 1.0
            depth *= fm->lfo_ams_depths[j];
            depth >>= KRSYN_LFO_DEPTH_SHIFT;    // MAX -1.0 ~ 1.0
            depth += (2<<(KRSYN_OUTPUT_SHIFT));

            uint32_t ams_size =  fm->lfo_ams_depths[j];
            ams_size >>= KRSYN_LFO_DEPTH_SHIFT - KRSYN_OUTPUT_SHIFT; // MAX 0.0 ~ 1.0, mean 1-ams_depth/2

            depth -= ams_size;

            depth *= note->output_log[j];
            depth >>= KRSYN_LFO_DEPTH_SHIFT;
            note->output_log[j] = (int32_t)depth;
        }
    }

    if(fms || ams)
    {
        note->lfo_log = lfo_t(core, fm->lfo_table_type, note->lfo_phase); // 0.0 ~ 1.0
        note->lfo_phase += note->lfo_delta;
    }
}

static inline void krsyn_fm_process(const krsyn_core* core, const krsyn_fm* fm, krsyn_fm_note* note, int16_t* buf, unsigned len,
                                    uint8_t algorithm, bool ams, bool fms)
{
    uint32_t delta[KRSYN_NUM_OPERATORS];

    for(unsigned i=0; i<len; i+=2)
    {
        buf[i+1] = buf[i] = fm_frame(core,fm, note, algorithm);
        fm_lfo_frame(core, fm, note, delta, ams, fms);

        for(unsigned j=0; j<KRSYN_NUM_OPERATORS; j++)
        {
            note->phases[j] = note->phases[j] + delta[j];
        }

        if((note->now_frame & KRSYN_SAMPLE_PER_FRAMES_MASK) == 0)
        {
            for(unsigned j=0; j<KRSYN_NUM_OPERATORS; j++)
            {
                note->envelop_now_amps[j] += note->envelop_now_deltas[j];
                note->envelop_now_times[j]--;
            }
            envelop_next(note);
        }
        note->now_frame ++;
    }
}

#define krsyn_define_fm_render(algorithm) \
void krsyn_fm_render_ ## algorithm ##_0_0(const krsyn_core* core, const krsyn_fm* fm, krsyn_fm_note* note, int16_t* buf, unsigned len) \
{ \
    krsyn_fm_process(core, fm, note, buf, len, algorithm, false, false); \
} \
void krsyn_fm_render_ ## algorithm ##_0_1(const krsyn_core* core, const krsyn_fm* fm, krsyn_fm_note* note, int16_t* buf, unsigned len) \
{ \
    krsyn_fm_process(core, fm, note, buf, len, algorithm, false, true); \
} \
void krsyn_fm_render_ ## algorithm ##_1_0(const krsyn_core* core, const krsyn_fm* fm, krsyn_fm_note* note, int16_t* buf, unsigned len) \
{ \
    krsyn_fm_process(core, fm, note, buf, len, algorithm, true, false); \
} \
void krsyn_fm_render_ ## algorithm ##_1_1(const krsyn_core* core, const krsyn_fm* fm, krsyn_fm_note* note, int16_t* buf, unsigned len) \
{ \
    krsyn_fm_process(core, fm, note, buf, len, algorithm, true, true); \
}

krsyn_define_fm_render(0)
krsyn_define_fm_render(1)
krsyn_define_fm_render(4)
krsyn_define_fm_render(7)
krsyn_define_fm_render(8)
krsyn_define_fm_render(10)
krsyn_define_fm_render(11)
krsyn_define_fm_render(12)
krsyn_define_fm_render(13)
krsyn_define_fm_render(14)
krsyn_define_fm_render(15)
krsyn_define_fm_render(16)
krsyn_define_fm_render(17)
krsyn_define_fm_render(18)
krsyn_define_fm_render(19)
krsyn_define_fm_render(20)
krsyn_define_fm_render(21)
krsyn_define_fm_render(22)
krsyn_define_fm_render(23)
krsyn_define_fm_render(24)
krsyn_define_fm_render(25)
krsyn_define_fm_render(26)
krsyn_define_fm_render(27)
krsyn_define_fm_render(28)
krsyn_define_fm_render(29)

#undef krsyn_define_fm_render


static inline int calc_mipmap_num(const krsyn_core* core, const krsyn_fm_note* note)
{
    // max frequency x : (table_length/2) * x < (sampling_frequency/2)
    //                                      x < sampling_frequency/table_length
    uint32_t freq = MAX(note->phase_deltas[0], note->phase_deltas[1]);
    freq >>= KRSYN_TABLE_SHIFT;
    freq <<= KRSYN_NUM_TABLE_MIPMAPS;
    int i=0;
    while(freq > KRSYN_PHASE_1 && i< KRSYN_NUM_TABLE_MIPMAPS)
    {
        i++;
        freq >>= 1;
    }

    return i;
}

void krsyn_fm_render(const krsyn_core* core, const krsyn_fm* fm, krsyn_fm_note* note, int16_t* buf, unsigned len)
{

#define krsyn_lfo_branch(algorithm) \
    if(fm->lfo_ams_enabled) \
    { \
        if(fm->lfo_fms_enabled) \
        { \
            krsyn_fm_render_ ## algorithm ## _1_1 (core, fm, note, buf, len); \
        } \
        else \
        { \
            krsyn_fm_render_ ## algorithm ## _1_0 (core, fm, note, buf, len); \
        } \
    } \
    else \
    { \
        if(fm->lfo_fms_enabled) \
        { \
            krsyn_fm_render_ ## algorithm ## _0_1 (core, fm, note, buf, len); \
        } \
        else \
        { \
            krsyn_fm_render_ ## algorithm ## _0_0 (core, fm, note, buf, len); \
        } \
    }

    switch(fm->algorithm)
    {
    case 0:
        krsyn_lfo_branch(0)
        break;
    case 1:
        krsyn_lfo_branch(1)
        break;
    case 4:
        krsyn_lfo_branch(4)
        break;
    case 7:
        krsyn_lfo_branch(7)
        break;
    case 8:
    {
        int i = calc_mipmap_num(core, note);
        switch(i)
        {
        case 0:krsyn_lfo_branch(10) break;
        case 1:krsyn_lfo_branch(11) break;
        case 2:krsyn_lfo_branch(12) break;
        case 3:krsyn_lfo_branch(13) break;
        case 4:krsyn_lfo_branch(14) break;
        case 5:krsyn_lfo_branch(15) break;
        case 6:krsyn_lfo_branch(16) break;
        case 7:krsyn_lfo_branch(17) break;
        case 8:krsyn_lfo_branch(18) break;
        case 9:krsyn_lfo_branch(19) break;
        }
        break;
    }
    case 9:
    {
        int i= calc_mipmap_num(core, note);
        switch(i)
        {
        case 0:krsyn_lfo_branch(20) break;
        case 1:krsyn_lfo_branch(21) break;
        case 2:krsyn_lfo_branch(22) break;
        case 3:krsyn_lfo_branch(23) break;
        case 4:krsyn_lfo_branch(24) break;
        case 5:krsyn_lfo_branch(25) break;
        case 6:krsyn_lfo_branch(26) break;
        case 7:krsyn_lfo_branch(27) break;
        case 8:krsyn_lfo_branch(28) break;
        case 9:krsyn_lfo_branch(29) break;
        }
        break;
    }
    }

#undef krsyn_lfo_branch
}


