#pragma once
#include <stdint.h>
#include <stdbool.h>

//define int type
typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif


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


i16 ks_sin              (u32 phase, bool linear_interpolution);
i16 ks_saw              (u32 phase);
i16 ks_triangle         (u32 phase);
i16 ks_fake_triangle    (u32 phase, u32 shift);
i16 ks_noise            (u32 phase, u32 begin);
u32 ks_notefreq         (u8 notenumber);
i32 ks_ratescale        (u8 notenumber);
u16 ks_keyscale_curves  (u8 type, u8 index);
