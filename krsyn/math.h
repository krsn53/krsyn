#pragma once

#include <stdbool.h>
#include <ksio/types.h>

// macro utils
// a * 2^x
#define ks_v(a, x)                      ((a) << (x))
// 2^x
#define ks_1(x)                         ks_v(1, x)
// 2^x -1
#define ks_m(x)                         (ks_1(x) - 1)
// a & (2^x - 1)
#define ks_mask(a, x)                   ((a) & ks_m(x))

// fixed point macros
#define KS_TABLE_BITS                   10u
#define KS_OUTPUT_BITS                  15u

#define KS_FREQUENCY_BITS               16u
#define KS_RATESCALE_BITS               16u

#define KS_PHASE_BITS                   16u
#define KS_PHASE_MAX_BITS               (KS_PHASE_BITS + KS_TABLE_BITS)
#define KS_NOISE_PHASE_BITS             (KS_PHASE_MAX_BITS - 5)


const i16*  ks_get_wave_table   (u8 index, u8 notenumber);
i16         ks_sin              (u32 phase);
i16         ks_saw              (u32 phase);
i16         ks_saw_down         (u32 phase);
i16         ks_triangle         (u32 phase);
i16         ks_fake_triangle    (u32 phase, u32 shift);
i16         ks_square           (u32 phase);
i16         ks_noise            (u32 phase);
u32         ks_notefreq         (u8 notenumber);
i32         ks_ratescale        (u8 notenumber);
