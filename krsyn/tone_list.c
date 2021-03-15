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
    if(__SERIAL_TYPE == KS_IO_DESERIALIZER) ks_access(capacity) = ks_access(length);
ks_io_end_custom_func(ks_tone_list_data)

ks_tone_list_data* ks_tone_list_data_new(){
    ks_tone_list_data* ret = malloc(sizeof(ks_tone_list_data));
    ks_vector_init(ret);
    return ret;
}

void ks_tone_list_data_free(ks_tone_list_data* d){
    ks_vector_free(d);
    free(d);
}

KS_INLINE u32 ks_tone_list_bank_number_hash(ks_tone_list_bank_number bank_number){
    return ks_v(bank_number.percussion, 8) + ks_v(bank_number.msb, 7) +  bank_number.lsb;

}

bool ks_tone_list_bank_number_equals(ks_tone_list_bank_number b1, ks_tone_list_bank_number b2){
    return *(u16*)(&b1) == *(u16*)(&b2);
}

KS_INLINE bool ks_tone_list_bank_is_empty(const ks_tone_list_bank* bank){
    return !bank->bank_number.enabled;
}

// XG like ???
KS_INLINE ks_tone_list_bank_number ks_tone_list_bank_number_of(u8 msb, u8 lsb, bool percussion){
    return (ks_tone_list_bank_number){
        .msb = msb,
        .lsb = lsb,
        .percussion = percussion || msb == 127,
        .enabled = 1,
    };
}

void ks_synth_context_add_custom_wave(ks_synth_context* ctx, const ks_tone_data* bin){
    ks_synth synth;
    ks_synth_data data = bin->synth;

    // reset not require parametors
    data.common.panpot = 0;
    data.common.amp_ratescale = 0;
    data.common.filter_ratescale = 0;
    ks_synth_set(&synth, ctx, &data);

    const u8 index = bin->program - KS_PROGRAM_CUSTOM_WAVE;
    const u8 velocity = bin->note;

    ks_synth_note note;
    ks_synth_note_on(&note, &synth, ctx,  69, velocity); // 440 helz, 1 cycle= sampling_rate/440

    const u64 f = ks_v((u64)ctx->sampling_rate, 30) / 440;

    for(u32 i=0; i< KS_NUM_OPERATORS; i++){
        u64 p = note.phase_deltas[i];
        p *= f;
        p >>= 30 + KS_TABLE_BITS;

        note.phase_deltas[i] = p; // normalize 1 cycle = 1024
    }

    if(ctx->wave_tables[ks_wave_index(1, index)] != NULL){
        ks_warning("Already set wave table %d, table is overrided", ks_wave_index(1, index));
        free(ctx->wave_tables[ks_wave_index(1, index)]);
    }
    i16* table = ctx->wave_tables[ks_wave_index(1, index)] = malloc(sizeof(i16) * ks_1(KS_TABLE_BITS));
    i32 tmp[ks_v(2, KS_TABLE_BITS)];

    // render and write to table
    ks_synth_render(ctx, &note, ks_1(KS_VOLUME_BITS), ks_1(KS_LFO_DEPTH_BITS), tmp, ks_v(2, KS_TABLE_BITS));
    for(u32 i=0 ;i< ks_1(KS_TABLE_BITS); i++){
         table[i] = MIN(MAX((tmp[2*i]) << 2, INT16_MIN), INT16_MAX);
    }
}

ks_tone_list* ks_tone_list_new_from_data(ks_synth_context* ctx, const ks_tone_list_data* bin){
    ks_tone_list* ret= ks_tone_list_new();

    u32 length = bin->length;

    for(u32 i = 0; i< length; i++){
        if(bin->data[i].program >= KS_PROGRAM_CUSTOM_WAVE){
            ks_synth_context_add_custom_wave(ctx, &bin->data[i]);
        } else {
            ks_tone_list_bank_number bank_number = ks_tone_list_bank_number_of(bin->data[i].msb, bin->data[i].lsb, bin->data[i].note != KS_NOTENUMBER_ALL);
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
            ks_synth_set( &bank->programs[bin->data[i].program][bin->data[i].note & 0x7f], ctx, &bin->data[i].synth);
        }
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

ks_tone_list_bank ks_tone_list_bank_of(u8 msb, u8 lsb, bool percussion, ks_synth *programs[KS_NUM_MAX_PROGRAMS]){
    ks_tone_list_bank ret;
    ret.bank_number = ks_tone_list_bank_number_of(msb,lsb, percussion);

    for(u32 i=0; i<KS_NUM_MAX_PROGRAMS; i++){
        ret.programs[i] = programs[i];
    }


    return ret;
}

static int ks_tone_data_compare1(const ks_tone_data* t1, const ks_tone_data* t2){
    if(t1->program >= KS_PROGRAM_CUSTOM_WAVE && t2->program < KS_PROGRAM_CUSTOM_WAVE){
        return -1;
    }
    else if(t1->program < KS_PROGRAM_CUSTOM_WAVE && t2->program >= KS_PROGRAM_CUSTOM_WAVE){
        return 1;
    }
    if(t1->program >= KS_PROGRAM_CUSTOM_WAVE){
        int ret = t1->program - t2->program;
        if(ret != 0) return ret;
        ret = t1->lsb - t2->lsb;
        if(ret != 0) return ret;
        ret = t1->note - t2->note;
        if(ret != 0) return ret;
    }
    else {
        int ret = t1->note - t2->note;
        if(ret != 0) return -ret;
        ret = t1->program - t2->program;
        if(ret != 0) return ret;
        ret = t1->lsb - t2->lsb;
        if(ret != 0) return ret;
        ret = t1->msb - t2->msb;
        if(ret != 0) return ret;
    }
    return strcmp(t1->name, t2->name);
}


static int ks_tone_data_compare2(const void* v1, const void* v2){
    return ks_tone_data_compare1(v1, v2);
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
    if(v->length != 0){
        tone.msb = v->data[*current].msb;
        tone.lsb = v->data[*current].lsb;
        tone.program = v->data[*current].program;
        tone.note = v->data[*current].note;
    }

    strcpy(tone.name, "Noname Tone\0");
    ks_synth_data_set_default(&tone.synth);

    ks_tone_list_insert(v, tone, current);
}
