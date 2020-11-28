#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "./synth.h"
#include "./io.h"

#define KS_NUM_MAX_PROGRAMS          128u

typedef struct ks_tones_bank_number{
    u16 msb: 7;
    u16 lsb:7;
    u16 percussion : 1;
    u16 enabled : 1;
} ks_tones_bank_number;

typedef struct ks_tone_data{
    u8 msb;
    u8 lsb;
    u8 program;
    u8 note;
    char    name[64];
    ks_synth_data synth;
}ks_tone_data;

typedef struct ks_tones_data{
    u32           length;
    u32           capacity;
    ks_tone_data       *data;
}ks_tones_data;

typedef struct ks_tones_bank{
    ks_tones_bank_number bank_number;
    ks_synth             *programs[KS_NUM_MAX_PROGRAMS];
}ks_tones_bank;

typedef struct ks_tones{
    u32            length;
    u32            capacity;
    ks_tones_bank        *data;
}ks_tones;

ks_io_decl_custom_func(ks_tone_data);
ks_io_decl_custom_func(ks_tones_data);

ks_tones* ks_tones_new_from_data(u32 sampling_rate, const ks_tones_data *bin);
ks_tones* ks_tones_new();
void ks_tones_free(ks_tones* tones);
void ks_tones_reserve(ks_tones* tones, u32 capacity);
ks_tones_bank *ks_tones_emplace_bank(ks_tones* tones, ks_tones_bank_number bank_number);
ks_tones_bank ks_tones_bank_of(u8 msb, u8 lsb, ks_synth* programs[KS_NUM_MAX_PROGRAMS]);
ks_tones_bank* ks_tones_find_bank(const ks_tones* tones, ks_tones_bank_number bank_number);


ks_tones_bank* ks_tones_banks_new(u32 num_banks);
void            ks_tones_banks_free(u32 num_banks, ks_tones_bank* banks);

u32 ks_tones_bank_number_hash(ks_tones_bank_number bank_number);
bool ks_tones_bank_number_equals(ks_tones_bank_number b1, ks_tones_bank_number b2);

bool ks_tones_bank_is_empty(const ks_tones_bank* bank);

// XG like ???
ks_tones_bank_number ks_tones_bank_number_of(u8 msb, u8 lsb);
