#pragma once

#include <stdint.h>

typedef struct ks_string{
    uint32_t length;
    uint32_t capacity;
    char* data;
} ks_string;

ks_string *ks_string_new();
void ks_string_clear(ks_string* str);
void ks_string_add_c(ks_string* str, char ch);
void ks_string_add_n(ks_string* str, uint32_t n, const char* ch);
void ks_string_free(ks_string* str);
void ks_string_resize(ks_string* str, uint32_t size);
void ks_string_reserve(ks_string* str, uint32_t cap);
void ks_string_set(ks_string* str, const char* ch);
void ks_string_set_n(ks_string* str, uint32_t n, const char* ch);
void ks_string_add(ks_string* str, const char* ch);
uint32_t ks_string_first_not_of(const ks_string* str, uint32_t start, const char *c);
uint32_t ks_string_first_c_of(const ks_string* str, uint32_t start, char c);
uint32_t ks_string_first_of(const ks_string* str, uint32_t start, const char * c);
char* ks_char_array_fill(char v, uint32_t length, char c[]);
