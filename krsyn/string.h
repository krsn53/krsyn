#pragma once

#include <stdint.h>
#include "./math.h"

typedef struct ks_string{
    u32     length;
    u32     capacity;
    char    *data;
} ks_string;

ks_string   *ks_string_new          ();
void        ks_string_clear         (ks_string* str);
void        ks_string_add_c         (ks_string* str, char ch);
void        ks_string_add_n         (ks_string* str, u32 n, const char* ch);
void        ks_string_free          (ks_string* str);
void        ks_string_resize        (ks_string* str, u32 size);
void        ks_string_reserve       (ks_string* str, u32 cap);
void        ks_string_set           (ks_string* str, const char* ch);
void        ks_string_set_n         (ks_string* str, u32 n, const char* ch);
void        ks_string_add           (ks_string* str, const char* ch);
u32         ks_string_first_not_of  (const ks_string* str, u32 start, const char *c);
u32         ks_string_first_c_of    (const ks_string* str, u32 start, char c);
u32         ks_string_first_of      (const ks_string* str, u32 start, const char * c);
char*       ks_char_array_fill      (char v, u32 length, char c[]);
