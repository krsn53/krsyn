#pragma once

#include "synth.h"


#include <stdint.h>
#include <stdbool.h>

#define KS_NUM_MAX_PROGRAMS          128u

typedef struct ks_tones_bank_number{
    uint16_t lsb:7;
    uint16_t msb: 7;
    uint16_t percussion : 1;
} ks_tones_bank_number;

typedef struct ks_tones_bank_binary{
    ks_tones_bank_number bank_number;
    ks_synth_binary      *programs[KS_NUM_MAX_PROGRAMS];
}ks_tones_bank_binary;

typedef struct ks_tones_binary{
    uint16_t            num_banks;
    ks_tones_bank_binary *banks;
}ks_tones_binary;

typedef struct ks_tones_bank{
    bool                emplaced;
    ks_tones_bank_number bank_number;
    ks_synth             *programs[KS_NUM_MAX_PROGRAMS];
}ks_tones_bank;

typedef struct ks_tones{
    uint32_t            num_banks;
    ks_tones_bank        *banks;
}ks_tones;


ks_tones* ks_tones_new_from_binary(uint32_t sampling_rate, const ks_tones_binary *bin);
ks_tones* ks_tones_new(uint32_t num_banks, ks_tones_bank banks[num_banks]);
void ks_tones_free(ks_tones* tones);
bool ks_tones_set_bank(ks_tones* tones, const ks_tones_bank* bank);
ks_tones_bank ks_tones_bank_of(uint8_t msb, uint8_t lsb, ks_synth* programs[KS_NUM_MAX_PROGRAMS]);
ks_tones_bank* ks_tones_find_bank(const ks_tones* tones, ks_tones_bank_number bank_number);


ks_tones_bank* ks_tones_banks_new(uint32_t num_banks);
void            ks_tones_banks_free(uint32_t num_banks, ks_tones_bank* banks);

static inline uint32_t ks_tones_bank_number_hash(ks_tones_bank_number bank_number){
    return ks_v(bank_number.msb, 7) +  bank_number.lsb;

}

static inline bool ks_tones_bank_is_empty(const ks_tones_bank* bank){
    return bank->emplaced == false;
}

// XG like ???
static inline ks_tones_bank_number ks_tones_bank_number_of(uint8_t msb, uint8_t lsb){
    return (ks_tones_bank_number){
        .msb = msb,
        .lsb = lsb,
        .percussion = (lsb==127 ? 1: 0)
    };
}
