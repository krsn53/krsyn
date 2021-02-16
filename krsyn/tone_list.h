#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <ksio/io.h>
#include "./synth.h"


#define KS_NUM_MAX_PROGRAMS          128u
#define KS_NOTENUMBER_ALL            0x80
#define KS_PROGRAM_CUSTOM_WAVE       0x80

typedef struct ks_tone_list_bank_number{
    u16         msb: 7;
    u16         lsb:7;
    u16         percussion : 1;
    u16         enabled : 1;
} ks_tone_list_bank_number;

typedef struct ks_tone_data{
    u8              msb;
    u8              lsb;
    u8              program; // if bigger than 128, custom wave table
    u8              note; // if program is begger than 128, custom wave table velocity, for delicate wave
    char            name    [32];
    ks_synth_data   synth;
}ks_tone_data;

typedef struct ks_tone_list_data{
    u32             length;
    u32             capacity;
    ks_tone_data    *data;
}ks_tone_list_data;

typedef struct ks_tone_list_bank{
    ks_tone_list_bank_number    bank_number;
    ks_synth                    *programs           [KS_NUM_MAX_PROGRAMS];
}ks_tone_list_bank;

typedef struct ks_tone_list{
    u32                     length;
    u32                     capacity;
    ks_tone_list_bank       *data;
}ks_tone_list;

ks_io_decl_custom_func(ks_tone_data);
ks_io_decl_custom_func(ks_tone_list_data);

ks_tone_list_data*          ks_tone_list_data_new               ();
void                        ks_tone_list_data_free              (ks_tone_list_data* d);

ks_tone_list*               ks_tone_list_new_from_data          (ks_synth_context *ctx, const ks_tone_list_data *bin);
ks_tone_list*               ks_tone_list_new                    ();
void                        ks_tone_list_free                   (ks_tone_list* tones);
void                        ks_tone_list_reserve                (ks_tone_list* tones, u32 capacity);
ks_tone_list_bank*          ks_tone_list_emplace_bank           (ks_tone_list* tones, ks_tone_list_bank_number bank_number);
ks_tone_list_bank           ks_tone_list_bank_of                (u8 msb, u8 lsb, bool percussion, ks_synth* programs[KS_NUM_MAX_PROGRAMS]);
ks_tone_list_bank*          ks_tone_list_find_bank              (const ks_tone_list* tones, ks_tone_list_bank_number bank_number);

ks_tone_list_bank*          ks_tone_list_banks_new              (u32 num_banks);
void                        ks_tone_list_banks_free             (u32 num_banks, ks_tone_list_bank* banks);

u32                         ks_tone_list_bank_number_hash       (ks_tone_list_bank_number bank_number);
bool                        ks_tone_list_bank_number_equals     (ks_tone_list_bank_number b1, ks_tone_list_bank_number b2);

bool                        ks_tone_list_bank_is_empty          (const ks_tone_list_bank* bank);

// XG like ???
ks_tone_list_bank_number    ks_tone_list_bank_number_of         (u8 msb, u8 lsb, bool percussion);

void                        ks_tone_list_sort                   (ks_tone_list_data* v, i32* current);
void                        ks_tone_list_insert                 (ks_tone_list_data* v, ks_tone_data d, i32 *current);
void                        ks_tone_list_insert_empty           (ks_tone_list_data*v, i32 *current);


#ifdef __cplusplus
}
#endif
