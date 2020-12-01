#include "tone_list.h"
#include "./synth.h"
#include "./logger.h"

#include <stdlib.h>
#include <stdarg.h>

ks_io_begin_custom_func(ks_tone_data)
    ks_fp_u8(msb);
    ks_fp_u8(lsb);
    ks_fp_u8(program);
    ks_fp_u8(note);
    ks_fp_str(name);
    ks_fp_obj(synth, ks_synth_data);
ks_io_end_custom_func(ks_tone_data)

ks_io_begin_custom_func(ks_tone_list_data)
    ks_magic_number("KSTN");
    ks_fp_u32(length);
    ks_fp_arr_obj_len(data, ks_tone_data, ks_access(length));
    if(!__SERIALIZE) ks_access(capacity) = ks_access(length);
ks_io_end_custom_func(ks_tone_list_data)


inline u32 tone_list_bank_number_hash(tone_list_bank_number bank_number){
    return ks_v(bank_number.msb, 7) +  bank_number.lsb;

}

bool tone_list_bank_number_equals(tone_list_bank_number b1, tone_list_bank_number b2){
    return *(u16*)(&b1) == *(u16*)(&b2);
}

inline bool tone_list_bank_is_empty(const tone_list_bank* bank){
    return !bank->bank_number.enabled;
}

// XG like ???
inline tone_list_bank_number tone_list_bank_number_of(u8 msb, u8 lsb){
    return (tone_list_bank_number){
        .msb = msb,
        .lsb = lsb,
        .percussion = (lsb==127 ? 1: 0),
        .enabled = 1,
    };
}

tone_list* tone_list_new_from_data(u32 sampling_rate, const ks_tone_list_data* bin){
    tone_list* ret= tone_list_new();

    u32 length = bin->length;

    for(u32 i = 0; i< length; i++){
        tone_list_bank_number bank_number = tone_list_bank_number_of(bin->data[i].msb, bin->data[i].lsb);
        tone_list_bank* bank = tone_list_find_bank(ret, bank_number);
        if(bank == NULL){
            bank = tone_list_emplace_bank(ret, bank_number);
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

tone_list* tone_list_new(){
    tone_list* ret = malloc(sizeof(tone_list));
    ret->data = calloc(1, sizeof(tone_list_bank));

    ret->length = 0;
    ret->capacity = 1;

    return ret;
}

tone_list_bank* tone_list_banks_new(u32 num_banks){
    tone_list_bank* ret = calloc(num_banks, sizeof(tone_list_bank));
    return ret;
}

void tone_list_free(tone_list* tones){
    tone_list_banks_free(tones->length, tones->data);
    free(tones);
}

void tone_list_banks_free(u32 num_banks, tone_list_bank* banks){
    for(u32 i=0; i<num_banks; i++){
        for(u32 p=0; p<KS_NUM_MAX_PROGRAMS; p++){
            if(banks[i].programs[p] != NULL) {
                free(banks[i].programs[p]);
            }
        }
    }
    free(banks);
}

tone_list_bank* tone_list_add_bank(u32 capacity, tone_list_bank* banks, tone_list_bank bank){
    u16 hash = tone_list_bank_number_hash(bank.bank_number);
    u16 masked_hash = hash & (capacity-1);

    int it = masked_hash;
    while(!tone_list_bank_is_empty(&banks[it])){
        it ++;
        it &= capacity-1;
        //if(it == masked_hash) return false;
    }
    banks[it] = bank;
    return &banks[it];
}

void tone_list_reserve(tone_list* tones, u32 cap){
    if(cap <= tones->capacity) return;

    tone_list_bank* banks = calloc(cap, sizeof(tone_list_bank));
    for(u32 i=0; i<tones->capacity; i++){
        if(!tones->data[i].bank_number.enabled) continue;
        tone_list_add_bank(cap, banks, tones->data[i]);
    }

    free(tones->data);
    tones->data = banks;

    tones->capacity = cap;
}

tone_list_bank* tone_list_emplace_bank(tone_list* tones, tone_list_bank_number bank_number){
    if(tones->length == tones->capacity){
        tone_list_reserve(tones, tones->capacity*2);
    }

    tone_list_bank bank = {
        .bank_number = bank_number,
        .programs = { 0 },
    };

    tones->length++;

    return tone_list_add_bank(tones->capacity, tones->data, bank);
}

tone_list_bank* tone_list_find_bank(const tone_list* tones, tone_list_bank_number bank_number){
    u16 hash = tone_list_bank_number_hash(bank_number);
    u16 masked_hash = hash & (tones->capacity-1);

    int it = masked_hash;
    while(!tone_list_bank_number_equals(tones->data[it].bank_number, bank_number)){
        if(tone_list_bank_is_empty(&tones->data[it])) return NULL;
        it ++;
        it &= (tones->capacity-1);
        if(it == hash) return NULL;
    }
    return & tones->data[it];
}

tone_list_bank tone_list_bank_of(u8 msb, u8 lsb, ks_synth *programs[KS_NUM_MAX_PROGRAMS]){
    tone_list_bank ret;
    ret.bank_number = tone_list_bank_number_of(msb,lsb);

    for(u32 i=0; i<KS_NUM_MAX_PROGRAMS; i++){
        ret.programs[i] = programs[i];
    }


    return ret;
}

