#include "tones.h"
#include "./synth.h"
#include "./logger.h"

#include <stdlib.h>
#include <stdarg.h>

ks_io_begin_custom_func(ks_tone_binary)
    ks_fp(ks_prop_u8(msb))
    ks_fp(ks_prop_u8(lsb))
    ks_fp(ks_prop_u8(program))
    ks_fp(ks_prop_u8(note))
    ks_fp(ks_prop_obj(synth, ks_synth_binary))
ks_io_end_custom_func

ks_io_begin_custom_func(ks_tones_binary)
    ks_magic_number("KTON")
    ks_fp(ks_prop_u32(num_tones))
    ks_fp(ks_prop_arr_len_obj(tones, ks_tone_binary, ks_elem_access(num_tones)))
ks_io_end_custom_func


inline uint32_t ks_tones_bank_number_hash(ks_tones_bank_number bank_number){
    return ks_v(bank_number.msb, 7) +  bank_number.lsb;

}

bool ks_tones_bank_number_equals(ks_tones_bank_number b1, ks_tones_bank_number b2){
    return *(uint16_t*)(&b1) == *(uint16_t*)(&b2);
}

inline bool ks_tones_bank_is_empty(const ks_tones_bank* bank){
    return !bank->bank_number.enabled;
}

// XG like ???
inline ks_tones_bank_number ks_tones_bank_number_of(uint8_t msb, uint8_t lsb){
    return (ks_tones_bank_number){
        .msb = msb,
        .lsb = lsb,
        .percussion = (lsb==127 ? 1: 0),
        .enabled = 1,
    };
}

ks_tones* ks_tones_new_from_binary(uint32_t sampling_rate, const ks_tones_binary* bin){
    ks_tones* ret= ks_tones_new();

    uint32_t num_tones = bin->num_tones;

    for(uint32_t i = 0; i< num_tones; i++){
        ks_tones_bank_number bank_number = ks_tones_bank_number_of(bin->tones[i].msb, bin->tones[i].lsb);
        ks_tones_bank* bank = ks_tones_find_bank(ret, bank_number);
        if(bank == NULL){
            bank = ks_tones_emplace_bank(ret, bank_number);
        }
        if(!bank_number.percussion && bank->programs[bin->tones[i].program] != NULL){
            ks_warning("Already exist synth with msb %d, lsb %d and program %d", bin->tones[i].msb, bin->tones[i].lsb, bin->tones[i].program);
            continue;
        }
        if(bank_number.percussion){
            if(bank->programs[bin->tones[i].program] == NULL){
                bank->programs[bin->tones[i].program] = malloc(sizeof(ks_synth)* 128);
            }
        }
        else {
            bank->programs[bin->tones[i].program] = malloc(sizeof(ks_synth));
        }
        ks_synth_set( &bank->programs[bin->tones[i].program][bin->tones[i].note & 0x7f], sampling_rate, &bin->tones[i].synth);
    }

    return ret;
}

ks_tones* ks_tones_new(){
    ks_tones* ret = malloc(sizeof(ks_tones));
    ret->banks = malloc(sizeof(ks_tones_bank));
    memset(ret->banks, 0, sizeof(ks_tones_bank));

    ret->num_banks = 0;
    ret->capacity = 1;

    return ret;
}

ks_tones_bank* ks_tones_banks_new(uint32_t num_banks){
    ks_tones_bank* ret = malloc(sizeof(ks_tones_bank)*num_banks);
    return ret;
}

void ks_tones_free(ks_tones* tones){
    ks_tones_banks_free(tones->num_banks, tones->banks);
    free(tones);
}

void ks_tones_banks_free(uint32_t num_banks, ks_tones_bank* banks){
    for(uint32_t i=0; i<num_banks; i++){
        for(uint32_t p=0; p<KS_NUM_MAX_PROGRAMS; p++){
            if(banks[i].programs[p] != NULL) {
                free(banks[i].programs[p]);
            }
        }
    }
    free(banks);
}

ks_tones_bank* ks_tones_add_bank(uint32_t capacity, ks_tones_bank* banks, ks_tones_bank bank){
    uint16_t hash = ks_tones_bank_number_hash(bank.bank_number);
    uint16_t masked_hash = hash & (capacity-1);

    int it = masked_hash;
    while(!ks_tones_bank_is_empty(&banks[it])){
        it ++;
        it &= capacity-1;
        //if(it == masked_hash) return false;
    }
    banks[it] = bank;
    return &banks[it];
}

void ks_tones_reserve(ks_tones* tones, uint32_t cap){
    if(cap <= tones->capacity) return;

    ks_tones_bank* banks = malloc(sizeof(ks_tones_bank)* cap);
    memset(banks, 0, sizeof(ks_tones_bank)*cap);
    for(uint32_t i=0; i<tones->capacity; i++){
        if(!tones->banks[i].bank_number.enabled) continue;
        ks_tones_add_bank(cap, banks, tones->banks[i]);
    }

    free(tones->banks);
    tones->banks = banks;

    tones->capacity = cap;
}

ks_tones_bank* ks_tones_emplace_bank(ks_tones* tones, ks_tones_bank_number bank_number){
    if(tones->num_banks == tones->capacity){
        ks_tones_reserve(tones, tones->capacity*2);
    }

    ks_tones_bank bank = {
        .bank_number = bank_number,
        .programs = { 0 },
    };

    tones->num_banks++;

    return ks_tones_add_bank(tones->capacity, tones->banks, bank);
}

ks_tones_bank* ks_tones_find_bank(const ks_tones* tones, ks_tones_bank_number bank_number){
    uint16_t hash = ks_tones_bank_number_hash(bank_number);
    uint16_t masked_hash = hash & (tones->capacity-1);

    int it = masked_hash;
    while(!ks_tones_bank_number_equals(tones->banks[it].bank_number, bank_number)){
        if(ks_tones_bank_is_empty(&tones->banks[it])) return NULL;
        it ++;
        it &= (tones->capacity-1);
        if(it == hash) return NULL;
    }
    return & tones->banks[it];
}

ks_tones_bank ks_tones_bank_of(uint8_t msb, uint8_t lsb, ks_synth *programs[KS_NUM_MAX_PROGRAMS]){
    ks_tones_bank ret;
    ret.bank_number = ks_tones_bank_number_of(msb,lsb);

    for(uint32_t i=0; i<KS_NUM_MAX_PROGRAMS; i++){
        ret.programs[i] = programs[i];
    }


    return ret;
}

