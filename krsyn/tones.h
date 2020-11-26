#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "./synth.h"
#include "./io.h"

#define KS_NUM_MAX_PROGRAMS          128u

typedef struct ks_tones_bank_number{
    uint16_t msb: 7;
    uint16_t lsb:7;
    uint16_t percussion : 1;
    uint16_t enabled : 1;
} ks_tones_bank_number;

typedef struct ks_tone_data{
    uint8_t msb;
    uint8_t lsb;
    uint8_t program;
    uint8_t note;
    char    name[64];
    ks_synth_data synth;
}ks_tone_data;

typedef struct ks_tones_data{
    uint32_t           num_tones;
    ks_tone_data      *tones;
}ks_tones_data;

typedef struct ks_tones_bank{
    ks_tones_bank_number bank_number;
    ks_synth             *programs[KS_NUM_MAX_PROGRAMS];
}ks_tones_bank;

typedef struct ks_tones{
    uint32_t            num_banks;
    uint32_t            capacity;
    ks_tones_bank        *banks;
}ks_tones;

ks_io_decl_custom_func(ks_tone_data);
ks_io_decl_custom_func(ks_tones_data);

ks_tones* ks_tones_new_from_data(uint32_t sampling_rate, const ks_tones_data *bin);
ks_tones* ks_tones_new();
void ks_tones_free(ks_tones* tones);
void ks_tones_reserve(ks_tones* tones, uint32_t capacity);
ks_tones_bank *ks_tones_emplace_bank(ks_tones* tones, ks_tones_bank_number bank_number);
ks_tones_bank ks_tones_bank_of(uint8_t msb, uint8_t lsb, ks_synth* programs[KS_NUM_MAX_PROGRAMS]);
ks_tones_bank* ks_tones_find_bank(const ks_tones* tones, ks_tones_bank_number bank_number);


ks_tones_bank* ks_tones_banks_new(uint32_t num_banks);
void            ks_tones_banks_free(uint32_t num_banks, ks_tones_bank* banks);

uint32_t ks_tones_bank_number_hash(ks_tones_bank_number bank_number);
bool ks_tones_bank_number_equals(ks_tones_bank_number b1, ks_tones_bank_number b2);

bool ks_tones_bank_is_empty(const ks_tones_bank* bank);

// XG like ???
ks_tones_bank_number ks_tones_bank_number_of(uint8_t msb, uint8_t lsb);
