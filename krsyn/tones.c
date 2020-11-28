#include "tones.h"
#include "./synth.h"
#include "./logger.h"

#include <stdlib.h>
#include <stdarg.h>

ks_io_begin_custom_func(ks_tone_data)
    ks_fp_u8(msb);
    ks_fp_u8(lsb);
    ks_fp_u8(program);
    ks_fp_u8(note);
    ks_fp_str_p(name);
    ks_fp_obj(synth, ks_synth_data);
ks_io_end_custom_func(ks_tone_data)

ks_io_begin_custom_func(ks_tones_data)
    ks_magic_number("KTON");
    ks_fp_u32(length);
    ks_fp_arr_obj_len(data, ks_tone_data, ks_access(length));
    if(!__SERIALIZE) ks_access(capacity) = ks_access(length);
ks_io_end_custom_func(ks_tones_data)


inline u32 ks_tones_bank_number_hash(ks_tones_bank_number bank_number){
    return ks_v(bank_number.msb, 7) +  bank_number.lsb;

}

bool ks_tones_bank_number_equals(ks_tones_bank_number b1, ks_tones_bank_number b2){
    return *(u16*)(&b1) == *(u16*)(&b2);
}

inline bool ks_tones_bank_is_empty(const ks_tones_bank* bank){
    return !bank->bank_number.enabled;
}

// XG like ???
inline ks_tones_bank_number ks_tones_bank_number_of(u8 msb, u8 lsb){
    return (ks_tones_bank_number){
        .msb = msb,
        .lsb = lsb,
        .percussion = (lsb==127 ? 1: 0),
        .enabled = 1,
    };
}

ks_tones* ks_tones_new_from_data(u32 sampling_rate, const ks_tones_data* bin){
    ks_tones* ret= ks_tones_new();

    u32 length = bin->length;

    for(u32 i = 0; i< length; i++){
        ks_tones_bank_number bank_number = ks_tones_bank_number_of(bin->data[i].msb, bin->data[i].lsb);
        ks_tones_bank* bank = ks_tones_find_bank(ret, bank_number);
        if(bank == NULL){
            bank = ks_tones_emplace_bank(ret, bank_number);
        }
        if(!bank_number.percussion && bank->programs[bin->data[i].program] != NULL){
            ks_warning("Already exist synth with msb %d, lsb %d and program %d", bin->data[i].msb, bin->data[i].lsb, bin->data[i].program);
            continue;
        }
        if(bank_number.percussion){
            if(bank->programs[bin->data[i].program] == NULL){
                bank->programs[bin->data[i].program] = calloc(128, sizeof(ks_synth));
            }
        }
        else {
            bank->programs[bin->data[i].program] = malloc(sizeof(ks_synth));
        }
        ks_synth_set( &bank->programs[bin->data[i].program][bin->data[i].note & 0x7f], sampling_rate, &bin->data[i].synth);
    }

    return ret;
}

ks_tones* ks_tones_new(){
    ks_tones* ret = malloc(sizeof(ks_tones));
    ret->data = calloc(1, sizeof(ks_tones_bank));

    ret->length = 0;
    ret->capacity = 1;

    return ret;
}

ks_tones_bank* ks_tones_banks_new(u32 num_banks){
    ks_tones_bank* ret = calloc(num_banks, sizeof(ks_tones_bank));
    return ret;
}

void ks_tones_free(ks_tones* tones){
    ks_tones_banks_free(tones->length, tones->data);
    free(tones);
}

void ks_tones_banks_free(u32 num_banks, ks_tones_bank* banks){
    for(u32 i=0; i<num_banks; i++){
        for(u32 p=0; p<KS_NUM_MAX_PROGRAMS; p++){
            if(banks[i].programs[p] != NULL) {
                free(banks[i].programs[p]);
            }
        }
    }
    free(banks);
}

ks_tones_bank* ks_tones_add_bank(u32 capacity, ks_tones_bank* banks, ks_tones_bank bank){
    u16 hash = ks_tones_bank_number_hash(bank.bank_number);
    u16 masked_hash = hash & (capacity-1);

    int it = masked_hash;
    while(!ks_tones_bank_is_empty(&banks[it])){
        it ++;
        it &= capacity-1;
        //if(it == masked_hash) return false;
    }
    banks[it] = bank;
    return &banks[it];
}

void ks_tones_reserve(ks_tones* tones, u32 cap){
    if(cap <= tones->capacity) return;

    ks_tones_bank* banks = calloc(cap, sizeof(ks_tones_bank));
    for(u32 i=0; i<tones->capacity; i++){
        if(!tones->data[i].bank_number.enabled) continue;
        ks_tones_add_bank(cap, banks, tones->data[i]);
    }

    free(tones->data);
    tones->data = banks;

    tones->capacity = cap;
}

ks_tones_bank* ks_tones_emplace_bank(ks_tones* tones, ks_tones_bank_number bank_number){
    if(tones->length == tones->capacity){
        ks_tones_reserve(tones, tones->capacity*2);
    }

    ks_tones_bank bank = {
        .bank_number = bank_number,
        .programs = { 0 },
    };

    tones->length++;

    return ks_tones_add_bank(tones->capacity, tones->data, bank);
}

ks_tones_bank* ks_tones_find_bank(const ks_tones* tones, ks_tones_bank_number bank_number){
    u16 hash = ks_tones_bank_number_hash(bank_number);
    u16 masked_hash = hash & (tones->capacity-1);

    int it = masked_hash;
    while(!ks_tones_bank_number_equals(tones->data[it].bank_number, bank_number)){
        if(ks_tones_bank_is_empty(&tones->data[it])) return NULL;
        it ++;
        it &= (tones->capacity-1);
        if(it == hash) return NULL;
    }
    return & tones->data[it];
}

ks_tones_bank ks_tones_bank_of(u8 msb, u8 lsb, ks_synth *programs[KS_NUM_MAX_PROGRAMS]){
    ks_tones_bank ret;
    ret.bank_number = ks_tones_bank_number_of(msb,lsb);

    for(u32 i=0; i<KS_NUM_MAX_PROGRAMS; i++){
        ret.programs[i] = programs[i];
    }


    return ret;
}

