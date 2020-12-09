#include "tone_list.h"
#include "./synth.h"
#include <ksio/logger.h>
#include <ksio/vector.h>

#include <stdlib.h>
#include <stdarg.h>

ks_io_begin_custom_func(ks_tone_data)
    ks_u8(msb);
    ks_u8(lsb);
    ks_u8(program);
    ks_u8(note);
    ks_str(name);
    ks_obj(synth, ks_synth_data);
ks_io_end_custom_func(ks_tone_data)

ks_io_begin_custom_func(ks_tone_list_data)
    ks_magic_number("KSTN");
    ks_u32(length);
    ks_arr_obj_len(data, ks_tone_data, ks_access(length));
    if(!__SERIALIZE) ks_access(capacity) = ks_access(length);
ks_io_end_custom_func(ks_tone_list_data)


inline u32 ks_tone_list_bank_number_hash(ks_tone_list_bank_number bank_number){
    return ks_v(bank_number.msb, 7) +  bank_number.lsb;

}

bool ks_tone_list_bank_number_equals(ks_tone_list_bank_number b1, ks_tone_list_bank_number b2){
    return *(u16*)(&b1) == *(u16*)(&b2);
}

inline bool ks_tone_list_bank_is_empty(const ks_tone_list_bank* bank){
    return !bank->bank_number.enabled;
}

// XG like ???
inline ks_tone_list_bank_number ks_tone_list_bank_number_of(u8 msb, u8 lsb){
    return (ks_tone_list_bank_number){
        .msb = msb,
        .lsb = lsb,
        .percussion = (lsb==127 ? 1: 0),
        .enabled = 1,
    };
}

ks_tone_list* ks_tone_list_new_from_data(u32 sampling_rate, const ks_tone_list_data* bin){
    ks_tone_list* ret= ks_tone_list_new();

    u32 length = bin->length;

    for(u32 i = 0; i< length; i++){
        ks_tone_list_bank_number bank_number = ks_tone_list_bank_number_of(bin->data[i].msb, bin->data[i].lsb);
        ks_tone_list_bank* bank = ks_tone_list_find_bank(ret, bank_number);
        if(bank == NULL){
            bank = ks_tone_list_emplace_bank(ret, bank_number);
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

ks_tone_list* ks_tone_list_new(){
    ks_tone_list* ret = malloc(sizeof(ks_tone_list));
    ret->data = calloc(1, sizeof(ks_tone_list_bank));

    ret->length = 0;
    ret->capacity = 1;

    return ret;
}

ks_tone_list_bank* ks_tone_list_banks_new(u32 num_banks){
    ks_tone_list_bank* ret = calloc(num_banks, sizeof(ks_tone_list_bank));
    return ret;
}

void ks_tone_list_free(ks_tone_list* tones){
    ks_tone_list_banks_free(tones->length, tones->data);
    free(tones);
}

void ks_tone_list_banks_free(u32 num_banks, ks_tone_list_bank* banks){
    for(u32 i=0; i<num_banks; i++){
        for(u32 p=0; p<KS_NUM_MAX_PROGRAMS; p++){
            if(banks[i].programs[p] != NULL) {
                free(banks[i].programs[p]);
            }
        }
    }
    free(banks);
}

ks_tone_list_bank* ks_tone_list_add_bank(u32 capacity, ks_tone_list_bank* banks, ks_tone_list_bank bank){
    u16 hash = ks_tone_list_bank_number_hash(bank.bank_number);
    u16 masked_hash = hash & (capacity-1);

    int it = masked_hash;
    while(!ks_tone_list_bank_is_empty(&banks[it])){
        it ++;
        it &= capacity-1;
        //if(it == masked_hash) return false;
    }
    banks[it] = bank;
    return &banks[it];
}

void ks_tone_list_reserve(ks_tone_list* tones, u32 cap){
    if(cap <= tones->capacity) return;

    ks_tone_list_bank* banks = calloc(cap, sizeof(ks_tone_list_bank));
    for(u32 i=0; i<tones->capacity; i++){
        if(!tones->data[i].bank_number.enabled) continue;
        ks_tone_list_add_bank(cap, banks, tones->data[i]);
    }

    free(tones->data);
    tones->data = banks;

    tones->capacity = cap;
}

ks_tone_list_bank* ks_tone_list_emplace_bank(ks_tone_list* tones, ks_tone_list_bank_number bank_number){
    if(tones->length == tones->capacity){
        ks_tone_list_reserve(tones, tones->capacity*2);
    }

    ks_tone_list_bank bank = {
        .bank_number = bank_number,
        .programs = { 0 },
    };

    tones->length++;

    return ks_tone_list_add_bank(tones->capacity, tones->data, bank);
}

ks_tone_list_bank* ks_tone_list_find_bank(const ks_tone_list* tones, ks_tone_list_bank_number bank_number){
    u16 hash = ks_tone_list_bank_number_hash(bank_number);
    u16 masked_hash = hash & (tones->capacity-1);

    int it = masked_hash;
    while(!ks_tone_list_bank_number_equals(tones->data[it].bank_number, bank_number)){
        if(ks_tone_list_bank_is_empty(&tones->data[it])) return NULL;
        it ++;
        it &= (tones->capacity-1);
        if(it == masked_hash) return NULL;
    }
    return & tones->data[it];
}

ks_tone_list_bank ks_tone_list_bank_of(u8 msb, u8 lsb, ks_synth *programs[KS_NUM_MAX_PROGRAMS]){
    ks_tone_list_bank ret;
    ret.bank_number = ks_tone_list_bank_number_of(msb,lsb);

    for(u32 i=0; i<KS_NUM_MAX_PROGRAMS; i++){
        ret.programs[i] = programs[i];
    }


    return ret;
}

int ks_tone_data_compare1(const ks_tone_data* t1, const ks_tone_data* t2){
    int ret = t1->program - t2->program;
    if(ret != 0) return ret;
    ret = t1->lsb - t1->lsb;
    if(ret != 0) return ret;
    ret = t1->msb - t2->msb;
    return ret;
}


int ks_tone_data_compare2(const void* v1, const void* v2){
    const ks_tone_data* t1 = v1, *t2 = v2;
    int ret = ks_tone_data_compare1(t1, t2);
    if(ret != 0) return ret;
    ret = t1->note - t2->note;
    if(ret != 0) return ret;
    return strcmp(t1->name, t2->name);
}

void ks_tone_list_sort(ks_tone_list_data* v, i32* current){
    qsort(v->data, v->length, sizeof(ks_tone_data), ks_tone_data_compare2);
    *current = 0;
}

void ks_tone_list_insert(ks_tone_list_data* v, ks_tone_data d, i32 *current){
    if(v->length >= 128*128*128) return;

    i32 pos = *current + 1;

    *current = pos;
    ks_vector_insert(v, pos, d);
}

void ks_tone_list_insert_empty(ks_tone_list_data*v, i32 *current){
    ks_tone_data tone;
    memset(&tone, 0, sizeof(tone));
    tone.msb = 0;
    tone.lsb = 0;
    tone.program = 0;
    tone.note = KS_NOTENUMBER_ALL;
    tone.note = KS_NOTENUMBER_ALL;

    strcpy(tone.name, "Noname Tone\0");
    ks_synth_data_set_default(&tone.synth);

    ks_tone_list_insert(v, tone, current);
}
