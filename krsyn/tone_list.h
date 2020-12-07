#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "./synth.h"
#include "./io.h"

#define KS_NUM_MAX_PROGRAMS          128u
#define KS_NOTENUMBER_ALL            0x80

typedef struct ks_tone_list_bank_number{
    u16 msb: 7;
    u16 lsb:7;
    u16 percussion : 1;
    u16 enabled : 1;
} ks_tone_list_bank_number;

typedef struct ks_tone_data{
    u8 msb;
    u8 lsb;
    u8 program;
    u8 note;
    char    name    [32];
    ks_synth_data synth;
}ks_tone_data;

typedef struct ks_tone_list_data{
    u32           length;
    u32           capacity;
    ks_tone_data       *data;
}ks_tone_list_data;

typedef struct ks_tone_list_bank{
    ks_tone_list_bank_number bank_number;
    ks_synth             *programs[KS_NUM_MAX_PROGRAMS];
}ks_tone_list_bank;

typedef struct ks_tone_list{
    u32            length;
    u32            capacity;
    ks_tone_list_bank        *data;
}ks_tone_list;

ks_io_decl_custom_func(ks_tone_data);
ks_io_decl_custom_func(ks_tone_list_data);

ks_tone_list* ks_tone_list_new_from_data(u32 sampling_rate, const ks_tone_list_data *bin);
ks_tone_list* ks_tone_list_new();
void ks_tone_list_free(ks_tone_list* tones);
void ks_tone_list_reserve(ks_tone_list* tones, u32 capacity);
ks_tone_list_bank *ks_tone_list_emplace_bank(ks_tone_list* tones, ks_tone_list_bank_number bank_number);
ks_tone_list_bank ks_tone_list_bank_of(u8 msb, u8 lsb, ks_synth* programs[KS_NUM_MAX_PROGRAMS]);
ks_tone_list_bank* ks_tone_list_find_bank(const ks_tone_list* tones, ks_tone_list_bank_number bank_number);


ks_tone_list_bank* ks_tone_list_banks_new(u32 num_banks);
void            ks_tone_list_banks_free(u32 num_banks, ks_tone_list_bank* banks);

u32 ks_tone_list_bank_number_hash(ks_tone_list_bank_number bank_number);
bool ks_tone_list_bank_number_equals(ks_tone_list_bank_number b1, ks_tone_list_bank_number b2);

bool ks_tone_list_bank_is_empty(const ks_tone_list_bank* bank);

// XG like ???
ks_tone_list_bank_number ks_tone_list_bank_number_of(u8 msb, u8 lsb);

int ks_tone_data_compare1(const ks_tone_data* t1, const ks_tone_data* t2);
int ks_tone_data_compare2(const void* v1, const void* v2);
void ks_tone_list_sort(ks_tone_list_data* v, i32* current);
void ks_tone_list_insert(ks_tone_list_data* v, ks_tone_data d, i32 *current);
void ks_tone_list_insert_empty(ks_tone_list_data*v, i32 *current);
