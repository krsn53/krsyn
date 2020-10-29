#include "tones.h"
#include "synth.h"

#include <stdlib.h>
#include <stdarg.h>

ks_tones* ks_tones_new_from_binary(uint32_t sampling_rate, const ks_tones_binary* bin){
    uint32_t num_banks = bin->num_banks;
    ks_tones_bank banks[num_banks];

    for(unsigned i=0; i<num_banks; i++){
        banks[i].bank_number = bin->banks[i].bank_number;
        banks[i].emplaced = true;

        for(unsigned p=0; p< KS_NUM_MAX_PROGRAMS; p++){
            if(bin->banks[i].programs[p] == NULL) continue;

            ks_synth* synth;
            if(bin->banks[i].bank_number.percussion){
                synth = ks_synth_array_new(KS_NUM_MAX_PROGRAMS, bin->banks[i].programs[p], sampling_rate);
            } else {
                synth = ks_synth_new(bin->banks[i].programs[p], sampling_rate);
            }

            banks[i].programs[p] = synth;
        }
    }

    return ks_tones_new(bin->num_banks, banks);
}

ks_tones* ks_tones_new(uint32_t num_banks, ks_tones_bank banks[num_banks]){
    ks_tones* ret = malloc(sizeof(ks_tones));
    ret->banks = ks_tones_banks_new(num_banks);
    ret->num_banks = num_banks;

    for(unsigned i=0;i<num_banks; i++){
        ks_tones_set_bank(ret, &banks[i]);
    }

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
    for(unsigned i=0; i<num_banks; i++){
        for(unsigned p=0; p<KS_NUM_MAX_PROGRAMS; p++){
            if(banks[i].programs[p] != NULL) {
                free(banks[i].programs[p]);
            }
        }
    }
    free(banks);
}

bool ks_tones_set_bank(ks_tones* tones, const ks_tones_bank* bank){
    uint16_t hash = ks_tones_bank_number_hash(bank->bank_number);
    uint16_t masked_hash = hash % tones->num_banks;

    int it = masked_hash;
    while(!ks_tones_bank_is_empty(&tones->banks[it])){
        it ++;
        it %= tones->num_banks;
        if(it == hash) return false;
    }
    tones->banks[it] = *bank;
    return true;
}

ks_tones_bank* ks_tones_find_bank(const ks_tones* tones, ks_tones_bank_number bank_number){
    uint16_t hash = ks_tones_bank_number_hash(bank_number);
    uint16_t masked_hash = hash % tones->num_banks;

    int it = masked_hash;
    while(ks_tones_bank_number_hash(tones->banks[it].bank_number) != hash){
        if(ks_tones_bank_is_empty(&tones->banks[it])) return NULL;
        it ++;
        it %= tones->num_banks;
        if(it == hash) return NULL;
    }
    return & tones->banks[it];
}

ks_tones_bank ks_tones_bank_of(uint8_t msb, uint8_t lsb, ks_synth *programs[KS_NUM_MAX_PROGRAMS]){
    ks_tones_bank ret;
    ret.emplaced = true;
    ret.bank_number = ks_tones_bank_number_of(msb,lsb);

    for(unsigned i=0; i<KS_NUM_MAX_PROGRAMS; i++){
        ret.programs[i] = programs[i];
    }


    return ret;
}

