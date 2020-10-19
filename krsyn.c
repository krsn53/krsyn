#include "krsyn.h"
#include "constants.h"

#include <memory.h>


void krsyn_binary_set_default(krsyn_binary* data)
{
    for(unsigned i=0; i<KRSYN_NUM_OPERATORS; i++)
    {
        data->phase_coarses[i].fixed_frequency = false;
        data->phase_coarses[i].value = 2;
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
        data->ks_low_depths[i] = 0;
        data->ks_high_depths[i] = 0;
        data->ks_mid_points[i] = 69;
        data->ks_curve_types[i].left = 0;
        data->ks_curve_types[i].right = 0;

        data->lfo_ams_depths[i] = 0;
    }

    data->algorithm = 0;
    data->feedback_level = 0;

    data->lfo_wave_type = KRSYN_LFO_WAVE_TRIANGLE;
    data->lfo_fms_depth = 0;
    data->lfo_freq = 128; // 1 Hz ?
    data->lfo_det = 0;
}

static inline void fm_op_set(uint32_t sampling_rate, krsyn* fm, const krsyn_binary* data)
{
    for(unsigned i=0; i<KRSYN_NUM_OPERATORS; i++)
    {
        fm->fixed_frequency[i] = calc_fixed_frequency(data->phase_coarses[i].fixed_frequency);
        fm->phase_coarses[i] = calc_phase_coarses(data->phase_coarses[i].value);

        fm->phase_fines[i] = calc_phase_fines(data->phase_fines[i]);
        fm->phase_dets[i] = calc_phase_dets(data->phase_dets[i]);

        for(unsigned e=0; e < KRSYN_ENVELOPE_NUM_POINTS; e++)
        {
            fm->envelope_points[e][i] = calc_envelope_points(data->envelope_points[e][i]);
            fm->envelope_samples[e][i] = calc_envelope_samples(sampling_rate, data->envelope_times[e][i]);
        }

        fm->envelope_release_samples[i] =  calc_envelope_samples(sampling_rate, data->envelope_release_times[i]);

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

static inline void fm_common_set(krsyn* fm, const krsyn_binary* data)
{
    fm->algorithm = calc_algorithm(data->algorithm);
    fm->feedback_level = calc_feedback_level(data->feedback_level);

    fm->lfo_wave_type = calc_lfo_wave_type(data->lfo_wave_type);
    fm->lfo_fms_depth = calc_lfo_fms_depth(data->lfo_fms_depth);
    fm->lfo_fms_enabled = fm->lfo_fms_depth != 0;
    fm->lfo_freq = calc_lfo_freq(data->lfo_freq);
    fm->lfo_det = krsyn_linear_u(data->lfo_det, 0, ks_1(KRSYN_PHASE_MAX_BITS));
}


void krsyn_set(krsyn* fm, uint32_t sampling_rate, const krsyn_binary* data)
{
    fm_op_set(sampling_rate, fm, data);
    fm_common_set(fm, data);
}


static inline uint32_t phase_lfo_delta(uint32_t sampling_rate, uint32_t freq)
{
    uint64_t freq_11 = ((uint64_t)freq) << KRSYN_TABLE_BITS;
    uint64_t delta_11 = (uint32_t)(freq_11 / sampling_rate);

    return (uint32_t)delta_11;
}

static inline uint32_t phase_delta_fix_freq(uint32_t sampling_rate, uint32_t coarse, uint32_t fine)
{
    uint32_t freq = note_freq[coarse];
    uint64_t freq_11 = ((uint64_t)freq) << KRSYN_TABLE_BITS;
    // 5461 ~ 2^16 * 1/12
    uint32_t freq_rate = (1<<KRSYN_FREQUENCY_BITS) + ((5461 * fine) >> KRSYN_PHASE_FINE_BITS);

    freq_11 *= freq_rate;
    freq_11 >>= KRSYN_FREQUENCY_BITS;

    uint32_t delta_11 = (uint32_t)(freq_11 / sampling_rate);
    return delta_11;
}

static inline uint32_t phase_delta(uint32_t sampling_rate, uint8_t notenum, uint32_t coarse, uint32_t fine)
{
    uint32_t freq = note_freq[notenum];
    uint64_t freq_11 = ((uint64_t)freq) << KRSYN_TABLE_BITS;

    uint32_t freq_rate = (coarse << (KRSYN_FREQUENCY_BITS - KRSYN_PHASE_COARSE_BITS)) + fine;
    uint64_t delta_11 = (uint32_t)(freq_11 / sampling_rate);
    delta_11 *= freq_rate;
    delta_11 >>= KRSYN_FREQUENCY_BITS;

    return (uint32_t)delta_11;
}

static inline uint32_t ks_li(uint32_t index_16, int curve_type)
{
    uint32_t index_m = (uint32_t)(index_16>> KRSYN_KS_CURVE_BITS);

    if(index_m >= ks_1(KRSYN_KS_CURVE_TABLE_BITS)-1)
    {
        return ks_curves[curve_type][ks_1(KRSYN_KS_CURVE_TABLE_BITS)-1];
    }

    uint32_t index_b = index_m+1;

    uint32_t under_fixed_b = index_16 & ((1<<KRSYN_KS_CURVE_BITS)-1);
    uint32_t under_fixed_m = (1<<KRSYN_KS_CURVE_BITS) - under_fixed_b;

    uint32_t ret = ks_curves[curve_type][index_m] * under_fixed_m +
        ks_curves[curve_type][index_b] * under_fixed_b;

    ret >>= KRSYN_KS_CURVE_BITS;

    return ret;
}

static inline uint32_t ks_value(const krsyn *fm, uint32_t table_index, uint32_t table_range, bool low_note, unsigned i)
{
    uint64_t index_16 = table_index;
    index_16 <<= KRSYN_KS_CURVE_MAX_BITS;
    index_16 /= table_range;
    index_16 *= (low_note ? fm->ks_low_depths[i] : fm->ks_high_depths[i]);
    index_16 >>= KRSYN_KS_DEPTH_BITS;

    int curve_type = fm->ks_curve_types[low_note ? 0 : 1][i];
    return ks_li((uint32_t)index_16, curve_type);
}

static inline uint32_t keyscale(const krsyn *fm, uint8_t notenum, unsigned i)
{
    if(notenum < fm->ks_mid_points[i])
    {
        if(fm->ks_curve_types[0][i] <= KRSYN_KS_CURVE_ED || fm->ks_curve_types[1][i] > KRSYN_KS_CURVE_ED)
        {
            return ks_value(fm, fm->ks_mid_points[i] - notenum - 1, fm->ks_mid_points[i], true, i);
        }
        else
        {
            return 1<< KRSYN_KS_CURVE_BITS;
        }
    }

    if(fm->ks_curve_types[1][i] <= KRSYN_KS_CURVE_ED || fm->ks_curve_types[0][i] > KRSYN_KS_CURVE_ED )
    {
        return ks_value(fm, notenum - fm->ks_mid_points[i], ks_1(KRSYN_KS_CURVE_TABLE_BITS) - fm->ks_mid_points[i], false, i);
    }
    return 1<< KRSYN_KS_CURVE_BITS;
}

void krsyn_note_on( krsyn_note* note, const krsyn *fm, uint32_t sampling_rate, uint8_t notenum, uint8_t vel)
{
    note->now_frame = 0;
    note->feedback_log =0;

    note->lfo_log = 0;
    note->lfo_phase= fm->lfo_det;
    note->lfo_delta = phase_lfo_delta(sampling_rate, fm->lfo_freq);

    for(unsigned i=0; i< KRSYN_NUM_OPERATORS; i++)
    {
        //phase
        note->phases[i] = fm->phase_dets[i];
        if(fm->fixed_frequency[i])
        {
            note->phase_deltas[i] = phase_delta_fix_freq(sampling_rate, fm->phase_coarses[i], fm->phase_fines[i]);
        }
        else
        {
            note->phase_deltas[i] = phase_delta(sampling_rate, notenum, fm->phase_coarses[i], fm->phase_fines[i]);
        }

        note->output_log[i] = 0;

        //rate scale
        int64_t ratescales = fm->ratescales[i];
        ratescales *= ratescale[notenum];
        ratescales >>= KRSYN_RS_BITS;
        ratescales += 1<<KRSYN_RS_BITS;

        //key scale
        uint64_t keysc = keyscale(fm, notenum, i);

        int64_t target;
        uint32_t velocity;

        //envelope
        for(unsigned j=0; j < KRSYN_ENVELOPE_NUM_POINTS; j++)
        {
            velocity = fm->velocity_sens[i];
            velocity *=  127-vel;
            velocity >>= 7;
            velocity = (1 << KRSYN_VELOCITY_SENS_BITS) -  velocity;

            target = fm->envelope_points[j][i];
            target *= keysc;
            target >>= KRSYN_KS_CURVE_BITS;
            target *= velocity;
            target >>= KRSYN_VELOCITY_SENS_BITS;

            note->envelope_points[j][i] = (int32_t)target;

            uint64_t frame = (fm->envelope_samples[j][i]);
            frame *= ratescales;
            frame >>= KRSYN_RS_BITS;
            frame >>= KRSYN_SAMPLE_PER_FRAMES_BITS;
            note->envelope_samples[j][i] = MAX((uint32_t)frame, 1u);
        }

        uint64_t frame = (fm->envelope_release_samples[i]);
        frame *= ratescales;
        frame >>= KRSYN_RS_BITS;
        frame >>= KRSYN_SAMPLE_PER_FRAMES_BITS;
        note->envelope_release_samples[i] = MAX((uint32_t)frame, 1u);

        note->envelope_deltas[0][i] = note->envelope_points[0][i] / (note->envelope_samples[0][i] << KRSYN_SAMPLE_PER_FRAMES_BITS);
        for(unsigned j=1; j < KRSYN_ENVELOPE_NUM_POINTS; j++)
        {
            note->envelope_deltas[j][i] = (note->envelope_points[j][i] - note->envelope_points[j-1][i]);
            note->envelope_deltas[j][i] /= (int32_t)note->envelope_samples[j][i] << KRSYN_SAMPLE_PER_FRAMES_BITS;
        }

        //envelope state init
        note->envelope_now_amps[i] = 0;
        note->envelope_now_times[i] = note->envelope_samples[0][i];
        note->envelope_now_deltas[i] = note->envelope_deltas[0][i];
        note->envelope_now_points[i] = 0;
        note->envelope_states[i] = KRSYN_ENVELOPE_ON;

    }
}

void krsyn_note_off (krsyn_note* note)
{
    for(unsigned i=0; i< KRSYN_NUM_OPERATORS; i++)
    {
        note->envelope_states[i] = KRSYN_ENVELOPE_RELEASED;
        note->envelope_now_times[i] = note->envelope_release_samples[i];
        note->envelope_now_deltas[i] = - note->envelope_now_amps[i] / ((int32_t)note->envelope_now_times[i] << KRSYN_SAMPLE_PER_FRAMES_BITS);
    }
}


// liniar interpolution
static inline int16_t table_value_li(const int16_t* table, uint32_t phase, unsigned mask)
{
    unsigned index_m = phase >> KRSYN_PHASE_BITS;
    unsigned index_b = (index_m + 1);

    uint32_t under_fixed_b = ks_mask(phase, KRSYN_PHASE_BITS);
    uint32_t under_fixed_m = ks_1(KRSYN_PHASE_BITS) - under_fixed_b;

    int64_t sin_31 = table[index_m & mask] * under_fixed_m +
        table[index_b & mask] * under_fixed_b;
     sin_31 >>= KRSYN_PHASE_BITS;

    return (int16_t)sin_31;
}

// sin table value
static inline int16_t sin_t(uint32_t phase, bool linear_interpolution)
{
    return linear_interpolution ? table_value_li(sin_table, phase,  ks_m(KRSYN_TABLE_BITS)) :
                                  sin_table[ks_mask(phase >> KRSYN_PHASE_BITS, KRSYN_TABLE_BITS)];
}

static inline int16_t saw_t(uint32_t phase)
{
    return saw_table[ks_mask((phase >> KRSYN_PHASE_BITS), KRSYN_TABLE_BITS)];
}

static inline int16_t triangle_t(uint32_t phase)
{
    return triangle_table[ks_mask((phase >> KRSYN_PHASE_BITS), KRSYN_TABLE_BITS)];
}

static inline int16_t fake_triangle_t(uint32_t phase, uint32_t shift)
{
    return triangle_table[ks_mask((phase >> (KRSYN_PHASE_BITS +shift) << (shift)), KRSYN_TABLE_BITS)];
}

static inline int16_t noise_t(uint32_t phase, uint32_t begin){
    return noise_table[ks_mask((begin + ((phase >> KRSYN_NOISE_PHASE_BITS))), KRSYN_TABLE_BITS)];
}

static inline int32_t envelope_apply(uint32_t amp, int32_t in)
{
    int64_t out = amp;
    out *=in;
    out >>= KRSYN_ENVELOPE_BITS;
    return out;
}

static inline int32_t output_mod(uint32_t phase, int32_t mod, uint32_t envelope)
{
    // mod<<(KRSYN_TABLE_BITS + 1) = double table length = 4pi
    int32_t out = sin_t(phase + (mod<<(KRSYN_TABLE_BITS + 1)), true);
    return envelope_apply(envelope, out);
}

static inline int16_t fm_frame(const krsyn* fm, krsyn_note* note, uint8_t algorithm)
{
    int32_t out;
    int32_t feedback;
    int32_t output[KRSYN_NUM_OPERATORS];

    if(algorithm == 0)
    {
        // +-----+
        // +-[1]-+-[2]---[3]---[4]--->

        output[3] = output_mod(note->phases[3], note->output_log[2], note->envelope_now_amps[3]);
        output[2] = output_mod(note->phases[2], note->output_log[1], note->envelope_now_amps[2]);
        output[1] = output_mod(note->phases[1], note->output_log[0], note->envelope_now_amps[1]);
        output[0] = output_mod(note->phases[0], note->feedback_log , note->envelope_now_amps[0]);
        out = note->output_log[3];

        feedback = fm->feedback_level;
        feedback *= output[0];
        feedback >>= KRSYN_FEEDBACK_LEVEL_BITS;

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

        feedback = fm->feedback_level;
        feedback *= output[0];
        feedback >>= KRSYN_FEEDBACK_LEVEL_BITS;
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

        feedback = fm->feedback_level;
        feedback *= output[0];
        feedback >>= KRSYN_FEEDBACK_LEVEL_BITS;
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

        feedback = fm->feedback_level;
        feedback *= output[0];
        feedback >>= KRSYN_FEEDBACK_LEVEL_BITS;
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

        feedback = fm->feedback_level;
        feedback *= output[0];
        feedback >>= KRSYN_FEEDBACK_LEVEL_BITS;
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

        feedback = fm->feedback_level;
        feedback *= output[0];
        feedback >>= KRSYN_FEEDBACK_LEVEL_BITS;
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

        feedback = fm->feedback_level;
        feedback *= output[0];
        feedback >>= KRSYN_FEEDBACK_LEVEL_BITS;
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

        feedback = fm->feedback_level;
        feedback *= output[0];
        feedback >>= KRSYN_FEEDBACK_LEVEL_BITS;
    }
    if(algorithm ==8)
    {
        // saw wave * 2 (saw - saw = square)
        {
            const uint32_t p1 = note->phases[0];
            const uint32_t p2 = note->phases[0] + (note->phase_deltas[0]>>2);
            output[0] =(int32_t)saw_t(p1) +saw_t(p2);
            const uint32_t p3 = note->phases[1] + (note->phase_deltas[1]>>1);
            const uint32_t p4 = note->phases[1] + (note->phase_deltas[1]>>2);
            output[1] =(int32_t)saw_t(p3)+saw_t(p4);
        }
        {
            const uint32_t p1 = note->phases[2];
            const uint32_t p2 = note->phases[2] + (note->phase_deltas[2]>>2);
            output[2] =(int32_t)saw_t(p1) +saw_t(p2);
            const uint32_t p3 = note->phases[3] + (note->phase_deltas[3]>>1);
            const uint32_t p4 = note->phases[3] + (note->phase_deltas[3]>>2);
            output[3] =(int32_t)saw_t(p3)+saw_t(p4);
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
            output[0] =(int32_t)fake_triangle_t( p1, fm->phase_coarses[2]) +fake_triangle_t( p2, fm->phase_coarses[2]);
            const uint32_t p3 = note->phases[1] + (note->phase_deltas[1]>>1);
            const uint32_t p4 = note->phases[1] + (note->phase_deltas[1]>>2);
            output[1] =(int32_t)fake_triangle_t(p3, fm->phase_coarses[3])+fake_triangle_t( p4, fm->phase_coarses[3]);
        }

        out =  envelope_apply(note->envelope_now_amps[0], (output[0]+output[1]) >> 2);
    }
    if(algorithm ==10)
    {
        // noise
        if((ks_mask(note->phases[0], KRSYN_PHASE_MAX_BITS)) < note->phase_deltas[0]) {
            srand(note->output_log[1]);
            output[1] = rand();
        } else {
            output[1] = note->output_log[1];
        }
        output[0] = envelope_apply(note->envelope_now_amps[0], noise_t(note->phases[0], output[1]));
        out = output[0];
    }

    for(unsigned i =0; i< KRSYN_NUM_OPERATORS; i++)
    {
        note->output_log[i] = output[i];
    }
    note->feedback_log = feedback;

    return out >> 2; //
}

static inline void envelope_next(krsyn_note* note)
{
    for(unsigned i=0; i<KRSYN_NUM_OPERATORS; i++)
    {
        if(--note->envelope_now_times[i] <= 0)
        {
            switch(note->envelope_states[i])
            {
            case KRSYN_ENVELOPE_ON:
                note->envelope_now_amps[i] = note->envelope_points[note->envelope_now_points[i]][i];
                note->envelope_now_points[i] ++;
                if(note->envelope_now_points[i] == KRSYN_ENVELOPE_NUM_POINTS)
                {
                    note->envelope_states[i] = KRSYN_ENVELOPE_SUSTAINED;
                    note->envelope_now_deltas[i] = 0;
                }
                else
                {
                    note->envelope_now_deltas[i] = note->envelope_deltas[note->envelope_now_points[i]][i];
                    note->envelope_now_times[i] = note->envelope_samples[note->envelope_now_points[i]][i];
                }
                break;
            case KRSYN_ENVELOPE_RELEASED:
                note->envelope_now_amps[i] = 0;
                note->envelope_now_deltas[i] = 0;
                note->envelope_states[i] = KRSYN_ENVELOPE_OFF;
                break;
            }
        }
    }
}

static inline void fm_lfo_frame(const krsyn* fm, krsyn_note* note, uint32_t delta[],
                                      uint8_t lfo_wave_type, bool ams, bool fms)
{
    if(fms)
    {
        int64_t depth = note->lfo_log;
        depth *= fm->lfo_fms_depth;
        if(lfo_wave_type != KRSYN_LFO_WAVE_SQUARE){
            depth >>= KRSYN_LFO_DEPTH_BITS - 1;
            depth += 2<<KRSYN_OUTPUT_BITS;     // MAX 0.0 ~ 2.0, mean 1.0
        }
        else {
            depth >>= KRSYN_LFO_DEPTH_BITS;
            depth += 3<<KRSYN_OUTPUT_BITS;     // MAX 0.0 ~ 2.0, mean 1.0
        }

        for(unsigned j=0; j<KRSYN_NUM_OPERATORS; j++)
        {
            int64_t d = note->phase_deltas[j];
            d *= depth;
            d >>= KRSYN_LFO_DEPTH_BITS;
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
            depth >>= KRSYN_LFO_DEPTH_BITS;
            depth >>= 1;
            depth += 1<<(KRSYN_OUTPUT_BITS);     // 0 ~ 2.0


            uint32_t ams_size =  fm->lfo_ams_depths[j];
            ams_size >>= (KRSYN_LFO_DEPTH_BITS - KRSYN_OUTPUT_BITS) + 1; // MAX 0.0 ~ 1.0, mean 1-ams_depth/2

            depth -= ams_size;

            depth *= note->output_log[j];
            depth >>= KRSYN_LFO_DEPTH_BITS;
            note->output_log[j] = (int32_t)depth;
        }
    }

    if(fms || ams)
    {

        switch(lfo_wave_type)
        {
        case KRSYN_LFO_WAVE_TRIANGLE:
            note->lfo_log = triangle_t(note->lfo_phase);
            break;
        case KRSYN_LFO_WAVE_SAW_UP:
            note->lfo_log = saw_t(note->lfo_phase);
            break;
        case KRSYN_LFO_WAVE_SAW_DOWN:
            note->lfo_log = - saw_t(note->lfo_phase);
            break;
        case KRSYN_LFO_WAVE_SQUARE:
            note->lfo_log = saw_t(note->lfo_phase);
            note->lfo_log -= saw_t(note->lfo_phase + (ks_1(KRSYN_PHASE_MAX_BITS) >> 1));
            break;
        case KRSYN_LFO_WAVE_SIN:
            note->lfo_log = sin_t(note->lfo_phase, true);
            break;
        }
        note->lfo_phase += note->lfo_delta;
    }
}

static inline void krsyn_process(const krsyn* fm, krsyn_note* note, int16_t* buf, unsigned len,
                                    uint8_t algorithm, uint8_t lfo_type, bool ams, bool fms)
{
    uint32_t delta[KRSYN_NUM_OPERATORS];

    for(unsigned i=0; i<len; i+=2)
    {
        buf[i+1] = buf[i] = fm_frame(fm, note, algorithm);
        fm_lfo_frame(fm, note, delta, lfo_type, ams, fms);

        for(unsigned j=0; j<KRSYN_NUM_OPERATORS; j++)
        {
            note->phases[j] = note->phases[j] + delta[j];
        }

        for(unsigned j=0; j<KRSYN_NUM_OPERATORS; j++)
        {
            note->envelope_now_amps[j] += note->envelope_now_deltas[j];
        }
        if(ks_mask(note->now_frame, KRSYN_SAMPLE_PER_FRAMES_BITS) == 0)
        {
            envelope_next(note);
        }
        note->now_frame ++;
    }
}

#define _algorithm_list _(0) _(1) _(2) _(3) _(4) _(5) _(6) _(7) _(8) _(9) _(10)

#define krsyn_define_fm_render_aw(algorithm, wave) \
void krsyn_render_ ## algorithm ## _ ## wave ##_0_0(const krsyn* fm, krsyn_note* note, int16_t* buf, unsigned len){ \
    krsyn_process(fm, note, buf, len, algorithm, wave, false, false); \
} \
void krsyn_render_ ## algorithm ## _ ## wave  ##_0_1(const krsyn* fm, krsyn_note* note, int16_t* buf, unsigned len) {\
    krsyn_process(fm, note, buf, len, algorithm, wave, false, true); \
} \
void krsyn_render_ ## algorithm ## _ ## wave  ##_1_0(const krsyn* fm, krsyn_note* note, int16_t* buf, unsigned len) {\
    krsyn_process(fm, note, buf, len, algorithm, wave, true, false); \
} \
    void krsyn_render_ ## algorithm ## _ ## wave  ##_1_1(const krsyn* fm, krsyn_note* note, int16_t* buf, unsigned len){ \
    krsyn_process(fm, note, buf, len, algorithm, wave, true, true); \
}

#define krsyn_define_fm_render(algorithm) \
    krsyn_define_fm_render_aw(algorithm, KRSYN_LFO_WAVE_TRIANGLE) \
    krsyn_define_fm_render_aw(algorithm, KRSYN_LFO_WAVE_SAW_UP) \
    krsyn_define_fm_render_aw(algorithm, KRSYN_LFO_WAVE_SAW_DOWN) \
    krsyn_define_fm_render_aw(algorithm, KRSYN_LFO_WAVE_SQUARE) \
    krsyn_define_fm_render_aw(algorithm, KRSYN_LFO_WAVE_SIN)

#undef _
#define _(x) krsyn_define_fm_render(x)
#define split

_algorithm_list

#undef krsyn_define_fm_render


#define krsyn_lfo_branch(algorithm, wave) \
    if(fm->lfo_ams_enabled) \
    { \
        if(fm->lfo_fms_enabled) \
        { \
            krsyn_render_ ## algorithm ## _ ## wave ## _1_1 (fm, note, buf, len); \
        } \
        else \
        { \
            krsyn_render_ ## algorithm ## _ ## wave ## _1_0 (fm, note, buf, len); \
        } \
    } \
    else \
    { \
        if(fm->lfo_fms_enabled) \
        { \
            krsyn_render_ ## algorithm ## _ ## wave ## _0_1 (fm, note, buf, len); \
        } \
        else \
        { \
            krsyn_render_ ## algorithm ## _ ## wave ## _0_0 (fm, note, buf, len); \
        } \
    }

#define krsyn_render_branch(algorithm) \
    switch(fm->lfo_wave_type) {\
    case KRSYN_LFO_WAVE_TRIANGLE: \
        krsyn_lfo_branch(algorithm, KRSYN_LFO_WAVE_TRIANGLE); break; \
    case KRSYN_LFO_WAVE_SAW_UP: \
        krsyn_lfo_branch(algorithm, KRSYN_LFO_WAVE_SAW_UP); break; \
    case KRSYN_LFO_WAVE_SAW_DOWN: \
        krsyn_lfo_branch(algorithm, KRSYN_LFO_WAVE_SAW_DOWN); break; \
    case KRSYN_LFO_WAVE_SQUARE: \
        krsyn_lfo_branch(algorithm, KRSYN_LFO_WAVE_SQUARE); break; \
    case KRSYN_LFO_WAVE_SIN: \
        krsyn_lfo_branch(algorithm, KRSYN_LFO_WAVE_SIN); break; \
    }

#undef _
#define _(x) case x : krsyn_render_branch(x) break;

void krsyn_render(const krsyn* fm, krsyn_note* note, int16_t* buf, unsigned len)
{
    if(*(uint32_t*)note->envelope_states != 0){
        switch(fm->algorithm)
        {
            _algorithm_list
        }
    } else {
        memset(buf, 0, len*sizeof(int16_t));
    }
}


#undef krsyn_lfo_branch
#undef _
