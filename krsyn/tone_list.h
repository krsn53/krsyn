#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "./synth.h"
#include "./io.h"

#define KS_NUM_MAX_PROGRAMS          128u
#define KS_NOTENUMBER_ALL            0x80

typedef struct tone_list_bank_number{
    u16 msb: 7;
    u16 lsb:7;
    u16 percussion : 1;
    u16 enabled : 1;
} tone_list_bank_number;

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

typedef struct tone_list_bank{
    tone_list_bank_number bank_number;
    ks_synth             *programs[KS_NUM_MAX_PROGRAMS];
}tone_list_bank;

typedef struct tone_list{
    u32            length;
    u32            capacity;
    tone_list_bank        *data;
}tone_list;

ks_io_decl_custom_func(ks_tone_data);
ks_io_decl_custom_func(ks_tone_list_data);

tone_list* tone_list_new_from_data(u32 sampling_rate, const ks_tone_list_data *bin);
tone_list* tone_list_new();
void tone_list_free(tone_list* tones);
void tone_list_reserve(tone_list* tones, u32 capacity);
tone_list_bank *tone_list_emplace_bank(tone_list* tones, tone_list_bank_number bank_number);
tone_list_bank tone_list_bank_of(u8 msb, u8 lsb, ks_synth* programs[KS_NUM_MAX_PROGRAMS]);
tone_list_bank* tone_list_find_bank(const tone_list* tones, tone_list_bank_number bank_number);


tone_list_bank* tone_list_banks_new(u32 num_banks);
void            tone_list_banks_free(u32 num_banks, tone_list_bank* banks);

u32 tone_list_bank_number_hash(tone_list_bank_number bank_number);
bool tone_list_bank_number_equals(tone_list_bank_number b1, tone_list_bank_number b2);

bool tone_list_bank_is_empty(const tone_list_bank* bank);

// XG like ???
tone_list_bank_number tone_list_bank_number_of(u8 msb, u8 lsb);
