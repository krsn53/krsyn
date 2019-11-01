/**
 * @file krsyn.h
 * @brief これはkrsynシンセを使うためにAPIです。
*/
#ifndef KRSYN_H
#define KRSYN_H

//#include <xmmintrin.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#ifndef __cplusplus
#include <stdbool.h>
#else
extern "C" {
#endif

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#if defined _MSC_VER
#define KRSYN_ALIGNED(x) __declspec( align( x ) )
#elif defined __GNUC__
#define KRSYN_ALIGNED(x) __attribute__((aligned(x)))
#endif

#define KRSYN_TABLE_SHIFT                   10u
#define KRSYN_TABLE_LENGTH                  (1<<KRSYN_TABLE_SHIFT)
#define KRSYN_TABLE_MASK                    (KRSYN_TABLE_LENGTH - 1u)

#define KRSYN_OUTPUT_SHIFT                  15u

#define KRSYN_PHASE_SHIFT                   16u
#define KRSYN_PHASE_1                       (1<<KRSYN_PHASE_SHIFT)
#define KRSYN_PHASE_MAX                     (KRSYN_PHASE_1 << KRSYN_TABLE_SHIFT)
#define KRSYN_PHASE_MASK                    (KRSYN_PHASE_MAX - 1u)
#define KRSYN_PHASE_SHIFT_MASK              (KRSYN_PHASE_1 - 1u)

#define KRSYN_PHASE_COARSE_SHIFT            1u
#define KRSYN_PHASE_FINE_SHIFT              16u

#define KRSYN_LFO_TABLE_SHIFT               7u
#define KRSYN_LFO_TABLE_LENGTH              (1<<KRSYN_LFO_TABLE_SHIFT)
#define KRSYN_LFO_TABLE_MASK                (KRSYN_LFO_TABLE_LENGTH - 1u)

#define KRSYN_NUM_TABLE_MIPMAPS             10u
#define KRSYN_MIPMAP_TABLE_LENGTH           (1u<<KRSYN_NUM_TABLE_MIPMAPS)
#define KRSYN_MIPMAP_TABLE_MASK             (KRSYN_MIPMAP_TABLE_LENGTH-1)

#define KRSYN_NUM_OPERATORS                 4u

#define KRSYN_ENVELOP_SHIFT                 16u
#define KRSYN_ENVELOP_NUM_POINTS            4u

#define KRSYN_FREQUENCY_SHIFT               16u
#define KRSYN_RS_SHIFT                      16u
#define KRSYN_VELOCITY_SENS_SHIFT           16u

#define KRSYN_KS_CURVE_TABLE_SHIFT          7u
#define KRSYN_KS_CURVE_TABLE_LENGTH         (1<<KRSYN_KS_CURVE_TABLE_SHIFT)
#define KRSYN_KS_CURVE_SHIFT                16u
#define KRSYN_KS_CURVE_MAX_SHIFT            (KRSYN_KS_CURVE_SHIFT + KRSYN_KS_CURVE_TABLE_SHIFT)
#define KRSYN_KS_DEPTH_SHIFT                16u

#define KRSYN_LFO_DEPTH_SHIFT               16u
#define KRSYN_FEEDBACK_LEVEL_SHIFT          16u

#define KRSYN_SAMPLE_PER_FRAMES_SHIFT       5u
#define KRSYN_SAMPLE_PER_FRAMES             (1<<KRSYN_SAMPLE_PER_FRAMES_SHIFT)
#define KRSYN_SAMPLE_PER_FRAMES_MASK        (KRSYN_SAMPLE_PER_FRAMES-1)

#define KRSYN_DATA_SIZE                     sizeof(krsyn_fm_data)


/**
 * @enum krsyn_ks_type
 * @brief キースケールの効果曲線
 * ks_mid_point を中心に音量を変化させる。
 * LDとLUは周波数に対して、線形的に音量が減少、増加する。
 * 対して、EDとEUは周波数に対して指数的に音量が減少、増加する。
*/
enum krsyn_ks_type
{
    KRSYN_KS_CURVE_LD,
    KRSYN_KS_CURVE_ED,
    KRSYN_KS_CURVE_LU,
    KRSYN_KS_CURVE_EU,
    KRSYN_KS_CURVE_NUM_TYPES,
};

/**
 * @enum envelop_state
 * @brief エンベロープの現在の状態 
*/
enum envelop_state
{
    KRSYN_ENVELOP_OFF,
    KRSYN_ENVELOP_ON,
    KRSYN_ENVELOP_SUSTAINED,
    KRSYN_ENVELOP_RELEASED,
};

/**
 * @enum krsyn_lfo_wave_type
 * @brief LFO の波形
*/
enum krsyn_lfo_wave_type
{
    KRSYN_LFO_WAVE_TRIANGLE,
    KRSYN_LFO_WAVE_SAW_UP,
    KRSYN_LFO_WAVE_SAW_DOWN,
    KRSYN_LFO_WAVE_SQUARE,
    KRSYN_LFO_WAVE_SIN,

    KRSYN_LFO_NUM_WAVES,
};

/**
 * @struct krsyn_core
 * @brief シンセの共通情報（サンプリング周波数や波形テーブルなど）。
*/
typedef struct krsyn_core
{
    uint32_t    smp_freq;
    int16_t     sin_table[KRSYN_TABLE_LENGTH];
    uint32_t    note_freq[128];
    int32_t     ratescale[128];
    uint16_t    ks_curves[KRSYN_KS_CURVE_NUM_TYPES][KRSYN_KS_CURVE_TABLE_LENGTH];
    int16_t     lfo_tables[KRSYN_LFO_NUM_WAVES][KRSYN_LFO_TABLE_LENGTH]; // TODO: remove after
    int16_t     saw_table[1<<KRSYN_NUM_TABLE_MIPMAPS];
    int16_t     triangle_table[1<<KRSYN_NUM_TABLE_MIPMAPS];
}
krsyn_core;


typedef struct krsyn_phase_coarse_data
{
    uint8_t fixed_frequency: 1;
    uint8_t value :7;
}krsyn_phase_coarse_data;

typedef struct krsyn_ks_curve_type_data
{
    uint8_t left : 4;
    uint8_t right : 4;
}krsyn_ks_curve_type_data;

/**
 * @struct krsyn_fm_data
 * @brief FMシンセのバイナリデータ。
*/
typedef struct krsyn_fm_data
{
    //! 基本周波数の何倍を出力するか。
    krsyn_phase_coarse_data     phase_coarses           [KRSYN_NUM_OPERATORS];

    //! 周波数の微妙なズレ具合。範囲は0.0~0.5で、最大で半オクターブ音程が上がる。
    uint8_t                     phase_fines             [KRSYN_NUM_OPERATORS];

    //! 波形の初期位相。ノートオンされた際に初期化される。
    uint8_t                     phase_dets              [KRSYN_NUM_OPERATORS];

    //! envelop_times時のアンプの大きさ。いわゆるTotal LevelやSustain Levelなど。
    uint8_t                     envelop_points          [KRSYN_ENVELOP_NUM_POINTS][KRSYN_NUM_OPERATORS];

    //! envelop_pointsのアンプの大きさになるまでの時間。いわゆるAttack TimeやSustain Timeなど。
    uint8_t                     envelop_times           [KRSYN_ENVELOP_NUM_POINTS][KRSYN_NUM_OPERATORS];

    //! リリースタイム。ノートオフされてからアンプが0になるまでの時間。
    uint8_t                     envelop_release_times   [KRSYN_NUM_OPERATORS];


    //! ベロシティによる音量変化の度合い。
    uint8_t                     velocity_sens           [KRSYN_NUM_OPERATORS];

    //! ノートナンバーによるエンベロープのスピードの変化の度合い。
    uint8_t                     ratescales              [KRSYN_NUM_OPERATORS];

    //! ks_mid_pointsよりノートナンバーが小さい時のノートナンバーによる音量変化の度合い。
    uint8_t                     ks_low_depths           [KRSYN_NUM_OPERATORS];

    //! ks_mid_pointsよりノートナンバーが大きい時のノートナンバーによる音量変化の度合い。
    uint8_t                     ks_high_depths          [KRSYN_NUM_OPERATORS];

    //! キースケールの中心となるノートナンバー。
    uint8_t                     ks_mid_points           [KRSYN_NUM_OPERATORS];

    //! キースケールの曲線の種類。上位4bitがks_mid_pointsよりノートナンバーが小さい時、下位4bitが大きい時の曲線。
    krsyn_ks_curve_type_data    ks_curve_types          [KRSYN_NUM_OPERATORS];
    

    //! LFOの音量変化の度合い。
    uint8_t                     lfo_ams_depths          [KRSYN_NUM_OPERATORS]; // zero : disabled


    //! アルゴリズムの種類。
    uint8_t                     algorithm;

    //! フィードバックの度合い。
    uint8_t                     feedback_level;


    //! LFOの波形の種類。
    uint8_t                     lfo_table_type;

    //! LFOの周波数。
    uint8_t                     lfo_freq;

    //! LFOの初期位相。
    uint8_t                     lfo_det;
    
    //! LFOの音程変化の度合い。
    uint8_t                     lfo_fms_depth;                                 // zero : disabled

}
krsyn_fm_data;


/**
 * @struct krsyn_fm_data
 * @brief 読み込まれたFMシンセのデータ。
*/
typedef KRSYN_ALIGNED(16) struct krsyn_fm
{
    uint32_t    phase_coarses           [KRSYN_NUM_OPERATORS];
    uint32_t    phase_fines             [KRSYN_NUM_OPERATORS];
    uint32_t    phase_dets              [KRSYN_NUM_OPERATORS];

    int32_t     envelop_points          [KRSYN_ENVELOP_NUM_POINTS][KRSYN_NUM_OPERATORS];
    uint32_t    envelop_samples         [KRSYN_ENVELOP_NUM_POINTS][KRSYN_NUM_OPERATORS];
    uint32_t    envelop_release_samples [KRSYN_NUM_OPERATORS];

    uint32_t    velocity_sens           [KRSYN_NUM_OPERATORS];
    uint32_t    lfo_ams_depths           [KRSYN_NUM_OPERATORS];

    uint32_t    ratescales              [KRSYN_NUM_OPERATORS];
    uint32_t    ks_low_depths           [KRSYN_NUM_OPERATORS];
    uint32_t    ks_high_depths          [KRSYN_NUM_OPERATORS];
    uint8_t     ks_mid_points           [KRSYN_NUM_OPERATORS];
    uint8_t     ks_curve_types          [2][KRSYN_NUM_OPERATORS];

    uint8_t     algorithm;
    uint32_t    feedback_level;

    uint32_t    lfo_det;
    uint32_t    lfo_freq;
    uint32_t    lfo_fms_depth;

    bool        fixed_frequency         [KRSYN_NUM_OPERATORS];

    uint8_t     lfo_table_type;
    bool        lfo_ams_enabled;
    bool        lfo_fms_enabled;
}
krsyn_fm;

/**
 * @struct krsyn_fm_data
 * @brief ノートオンされたノートの状態。
*/
typedef  KRSYN_ALIGNED(16)  struct krsyn_fm_note
{
    int32_t     output_log              [KRSYN_NUM_OPERATORS];

    uint32_t    phases                  [KRSYN_NUM_OPERATORS];
    uint32_t    phase_deltas            [KRSYN_NUM_OPERATORS];

    int32_t     envelop_points          [KRSYN_ENVELOP_NUM_POINTS][KRSYN_NUM_OPERATORS];
    uint32_t    envelop_samples         [KRSYN_ENVELOP_NUM_POINTS][KRSYN_NUM_OPERATORS];
    int32_t     envelop_deltas          [KRSYN_ENVELOP_NUM_POINTS][KRSYN_NUM_OPERATORS];
    uint32_t    envelop_release_samples [KRSYN_NUM_OPERATORS];

    int32_t     envelop_now_deltas      [KRSYN_NUM_OPERATORS];
    uint32_t    envelop_now_times       [KRSYN_NUM_OPERATORS];
    int32_t     envelop_now_amps        [KRSYN_NUM_OPERATORS];
    uint8_t     envelop_states          [KRSYN_NUM_OPERATORS];
    uint8_t     envelop_now_points      [KRSYN_NUM_OPERATORS];

    uint32_t    lfo_phase;
    uint32_t    lfo_delta;
    int32_t     lfo_log;
    int32_t     feedback_log;

    uint32_t    now_frame;
}
krsyn_fm_note;

/**
 * @fn krsyn_new
 * @brief シンセの初期化を行う。
 * @param freq サンプリング周波数。
*/
krsyn_core*                 krsyn_new                       (uint32_t freq);

/**
 * @fn krsyn_free
 * @brief シンセの解放を行う。
 * @param core 解放するシンセ。
*/
void                        krsyn_free                      (krsyn_core* core);

/**
 * @fn krsyn_fm_set_data_default
 * @brief fmのデータを初期化する。
 * @param data 初期化するデータ。
*/
void                        krsyn_fm_set_data_default       (krsyn_fm_data* data);

/**
 * @fn krsyn_fm_set
 * @brief fmデータをバイナリから読み込む。
 * @param core シンセの基本データ。サンプリング周波数を参照する。
 * @param fm 読み込まれるfmデータ。
 * @param data 読み込むバイナリ。
*/
void                        krsyn_fm_set                    (const krsyn_core* core, krsyn_fm* fm, const krsyn_fm_data* data);

/**
 * @fn krsyn_fm_render
 * @brief fmの音色をバッファに書き込む。
 * @param　core シンセの基本データ。
 * @param fm シンセの音色。
 * @param note 現在のノートオンの状態。
 * @param buf 書き込まれるバッファ。チャンネル数は常に2。
 * @param len 書き込まれるバッファの要素数。
*/
void                        krsyn_fm_render                 (const krsyn_core* core, const krsyn_fm *fm, krsyn_fm_note* note, int16_t* buf, unsigned len);

/**
 * @fn krsyn_fm_noteon
 * @brief ノートオンする。
 * @param core シンセの基本データ。
 * @param fm シンセの音色。ノートの初期化に使う。
 * @param note ノートオンされるノート。
 * @param notenum ノートオンされるノートのノートナンバー。
 * @param velocity ノートオンされるノートのベロシティ。
*/
void                        krsyn_fm_noteon                 (const krsyn_core *core, const krsyn_fm *fm, krsyn_fm_note* note, uint8_t notenum, uint8_t velocity);

/**
 * @fn krsyn_fm_noteoff
 * @brief ノートオフする。
 * @param note ノートオフされるノート。
*/
void                        krsyn_fm_noteoff                (krsyn_fm_note* note);




static inline uint32_t krsyn_exp_u(uint8_t val, uint32_t base, int num_v_bit)
{
    // ->[e]*(8-num_v_bit) [v]*(num_v_bit)
    // base * 1.(v) * 2^(e)
    int8_t v = val & ((1 << num_v_bit) - 1);
    v |= 1 << num_v_bit;    // add 1
    int8_t e = val >> num_v_bit;

    uint64_t ret = (uint64_t)base * v;
    ret <<= e;
    ret >>= (8 - num_v_bit - 1);
    ret >>= num_v_bit;
    return ret;
}

static inline uint32_t krsyn_calc_envelop_times(uint32_t val)
{
    return krsyn_exp_u(val, 1<<(16-8), 4);// 2^8 <= x < 2^16
}

static inline uint32_t krsyn_calc_envelop_samples(uint32_t smp_freq, uint8_t val)
{
    uint32_t time = krsyn_calc_envelop_times(val);
    uint64_t samples = time;
    samples *= smp_freq;
    samples /= KRSYN_SAMPLE_PER_FRAMES;
    samples >>= 16;
    samples = MAX(1u, samples);  // note : maybe dont need this line
    return (uint32_t)samples;
}

// min <= val < max
static inline int64_t krsyn_linear(uint8_t val, int32_t MIN, int32_t MAX)
{
    int32_t range = MAX - MIN;
    int64_t ret = val;
    ret *= range;
    ret >>= 8;
    ret += MIN;

    return ret;
}

// min <= val <= max
static inline int64_t krsyn_linear2(uint8_t val, int32_t MIN, int32_t MAX)
{
    return krsyn_linear(val, MIN, MAX + MAX/256);
}

#define krsyn_linear_i (int32_t)krsyn_linear
#define krsyn_linear_u (uint32_t)krsyn_linear
#define krsyn_linear2_i (int32_t)krsyn_linear2
#define krsyn_linear2_u (uint32_t)krsyn_linear2

#define calc_fixed_frequency(value)                     (value)
#define calc_phase_coarses(value)                       (value)
#define calc_phase_fines(value)                         krsyn_linear_u(value, 0, 1 << (KRSYN_PHASE_FINE_SHIFT-1))
#define calc_phase_dets(value)                          krsyn_linear_u(value, 0, KRSYN_PHASE_MAX)
#define calc_envelop_points(value)                      krsyn_linear2_i(value, 0, 1 << KRSYN_ENVELOP_SHIFT)
#define calc_envelop_samples(smp_freq, value)           krsyn_calc_envelop_samples(smp_freq, value)
#define calc_envelop_release_samples(smp_freq, value)   calc_envelop_samples(smp_ferq,value)
#define calc_velocity_sens(value)                       krsyn_linear2_u(data->velocity_sens[i], 0, 1 << KRSYN_VELOCITY_SENS_SHIFT)
#define calc_ratescales(value)                          krsyn_linear2_u(value, 0, 1 << KRSYN_RS_SHIFT)
#define calc_ks_low_depths(value)                       krsyn_linear2_u(value, 0, 1 << KRSYN_KS_DEPTH_SHIFT);
#define calc_ks_high_depths(value)                      krsyn_linear2_u(value, 0, 1 << KRSYN_KS_DEPTH_SHIFT)
#define calc_ks_mid_points(value)                       (value & 0x7f)
#define calc_ks_curve_types_left(value)                 (value & 0x0f)
#define calc_ks_curve_types_right(value)                (value >> 4)
#define calc_lfo_ams_depths(value)                      krsyn_linear2_u(value, 0, 1 << KRSYN_LFO_DEPTH_SHIFT)
#define calc_algorithm(value)                           (value)
#define calc_feedback_level(value)                      krsyn_linear_u(value, 0, 2<<KRSYN_FEEDBACK_LEVEL_SHIFT)
#define calc_lfo_table_type(value)                      (value)
#define calc_lfo_fms_depth(value)                       krsyn_linear2_u(value, 0, 1 << KRSYN_LFO_DEPTH_SHIFT)
#define calc_lfo_freq(value)                            krsyn_exp_u(data->lfo_freq, 1<<(KRSYN_FREQUENCY_SHIFT-5), 4)
#define calc_lfo_det(value)                             krsyn_linear_u(data->lfo_det, 0, KRSYN_PHASE_MAX)

#ifdef __cplusplus
} // extern "C"
#endif


#endif // KRSYN_H
