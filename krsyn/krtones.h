#pragma once

#include "krsynth.h"


#include <stdint.h>
#include <stdbool.h>

#define KRSYN_NUM_MAX_PROGRAMS          128u

typedef struct krtones_bank_number{
    uint16_t lsb:7;
    uint16_t msb: 7;
    uint16_t percussion : 1;
} krtones_bank_number;


typedef struct krtones_bank{
    bool                emplaced;
    krtones_bank_number bank_number;
    krsynth             *programs[KRSYN_NUM_MAX_PROGRAMS];
}krtones_bank;

typedef struct krtones{
    uint32_t            num_banks;
    krtones_bank        *banks;
}krtones;

typedef struct krtones_program{
    uint32_t            program_number;
    krsynth             *synth;
}krtones_program;


krtones* krtones_new(uint32_t num_banks, krtones_bank banks[num_banks]);
void krtones_free(krtones* tones);
bool krtones_set_bank(krtones* tones, const krtones_bank* bank);
krtones_bank krtones_bank_of(uint8_t msb, uint8_t lsb, uint32_t num_programs, krtones_program programs[num_programs]);
krtones_bank* krtones_find_bank(const krtones* tones, krtones_bank_number bank_number);


krtones_bank* krtones_banks_new(uint32_t num_banks);
void            krtones_banks_free(uint32_t num_banks, krtones_bank* banks);

static inline uint32_t krtones_bank_number_hash(krtones_bank_number bank_number){
    return ks_v(bank_number.msb, 7) +  bank_number.lsb;

}

static inline bool krtones_bank_is_empty(const krtones_bank* bank){
    return bank->emplaced == false;
}

// XG like ???
static inline krtones_bank_number krtones_bank_number_of(uint8_t msb, uint8_t lsb){
    return (krtones_bank_number){
        .msb = msb,
        .lsb = lsb,
        .percussion = (lsb==127 ? 1: 0)
    };
}
