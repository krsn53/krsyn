#pragma once


typedef struct ks_string{
    unsigned length;
    unsigned capacity;
    char* ptr;
} ks_string;

ks_string *ks_string_new(unsigned capacity);
void ks_string_clear(ks_string* str);
void ks_string_add_c(ks_string* str, char ch);
void ks_string_add_n(ks_string* str, unsigned n, const char* ch);
void ks_string_free(ks_string* str);
void ks_string_reserve(ks_string* str, unsigned cap);
void ks_string_set(ks_string* str, const char* ch);
void ks_string_add(ks_string* str, const char* ch);
unsigned ks_string_first_not_of(const ks_string* str, unsigned start, const char *c);
unsigned ks_string_first_c_of(const ks_string* str, unsigned start, char c);
unsigned ks_string_first_of(const ks_string* str, unsigned start, const char * c);
char* ks_char_array_fill(char v, unsigned length, char c[]);
