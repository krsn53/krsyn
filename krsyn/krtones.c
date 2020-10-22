#include "krtones.h"
#include "krsynth.h"

#include <stdlib.h>
#include <stdarg.h>

krtones* krtones_new(uint32_t num_banks, krtones_bank banks[num_banks]){
    krtones* ret = malloc(sizeof(krtones));
    ret->banks = krtones_banks_new(num_banks);
    ret->num_banks = num_banks;

    for(unsigned i=0;i<num_banks; i++){
        krtones_set_bank(ret, &banks[i]);
    }

    return ret;
}

krtones_bank* krtones_banks_new(uint32_t num_banks){
    krtones_bank* ret = malloc(sizeof(krtones_bank)*num_banks);
    return ret;
}

void krtones_free(krtones* tones){
    krtones_banks_free(tones->num_banks, tones->banks);
    free(tones);
}

void krtones_banks_free(uint32_t num_banks, krtones_bank* banks){
    for(unsigned i=0; i<num_banks; i++){
        for(unsigned p=0; p<KRSYN_NUM_MAX_PROGRAMS; p++){
            if(banks[i].programs[p] != NULL) {
                free(banks[i].programs[p]);
            }
        }
    }
    free(banks);
}

bool krtones_set_bank(krtones* tones, const krtones_bank* bank){
    uint16_t hash = krtones_bank_number_hash(bank->bank_number);
    uint16_t masked_hash = hash % tones->num_banks;

    int it = masked_hash;
    while(!krtones_bank_is_empty(&tones->banks[it])){
        it ++;
        it %= tones->num_banks;
        if(it == hash) return false;
    }
    tones->banks[it] = *bank;
    return true;
}

krtones_bank* krtones_find_bank(krtones* tones, krtones_bank_number bank_number){
    uint16_t hash = krtones_bank_number_hash(bank_number);
    uint16_t masked_hash = hash % tones->num_banks;

    int it = masked_hash;
    while(krtones_bank_number_hash(tones->banks[it].bank_number) != hash){
        if(krtones_bank_is_empty(&tones->banks[it])) return NULL;
        it ++;
        it %= tones->num_banks;
        if(it == hash) return NULL;
    }
    return & tones->banks[it];
}

krtones_bank krtones_bank_of(uint8_t msb, uint8_t lsb, uint32_t num_programs, krtones_program programs[num_programs]){
    krtones_bank ret;
    ret.bank_number = krtones_bank_number_of(msb,lsb);

    for(unsigned i=0; i<KRSYN_NUM_MAX_PROGRAMS; i++){
        ret.programs[i] = NULL;
    }

    for(unsigned i=0; i<num_programs; i++){
        uint8_t program_number = programs[i].program_number;
        ret.programs[program_number] = programs[i].synth;
    }

    return ret;
}

