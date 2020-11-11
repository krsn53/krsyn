#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define ks_v(a, x)                          ((a) << (x))
#define ks_1(x)                             ks_v(1, x)
#define ks_m(x)                             (ks_1(x) - 1)
#define ks_mask(a, x)                       ((a) & ks_m(x))

#define KS_TABLE_BITS                   10u
#define KS_OUTPUT_BITS                  15u
#define KS_PHASE_BITS                   16u
#define KS_PHASE_MAX_BITS               (KS_PHASE_BITS + KS_TABLE_BITS)

#define KS_NOISE_PHASE_BITS             (KS_PHASE_MAX_BITS - 5)


// liniar interpolution
int16_t ks_table_value_li(const int16_t* table, uint32_t phase, uint32_t mask);

// sin table value
int16_t ks_sin(uint32_t phase, bool linear_interpolution);

int16_t ks_saw(uint32_t phase);

int16_t ks_triangle(uint32_t phase);

int16_t ks_fake_triangle(uint32_t phase, uint32_t shift);

int16_t ks_noise(uint32_t phase, uint32_t begin);

uint32_t ks_notefreq(uint8_t notenumber);

int32_t ks_ratescale(uint8_t index);

int32_t ks_ratescale(uint8_t index);

uint16_t ks_keyscale_curves(uint8_t type, uint8_t index);
