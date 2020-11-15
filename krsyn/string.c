#include "string.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

ks_string *ks_string_new() {
    uint32_t cap = 1;
    ks_string* ret = malloc(sizeof(ks_string));
    *ret = (ks_string){
            .capacity = cap,
            .length = 0,
            .ptr = malloc(sizeof(char)*cap),
        };
    return ret;
}

void ks_string_free(ks_string* str){
    free(str->ptr);
    free(str);
}




void ks_string_clear(ks_string* str){
    memset(str->ptr, 0, str->capacity);
    str->length = 0;
}

void ks_string_add_c(ks_string* str, char ch){
     ks_string_add_n(str, 1, &ch);
}

void ks_string_add_n(ks_string* str, uint32_t n, const char* ch){
    uint32_t out_len = str->length + n;

    if(out_len >= str->capacity) {
        uint32_t cap = str->capacity;
        do{
            cap*=2;
        }while(out_len > cap);
        ks_string_reserve(str, cap);
    }

    memcpy(str->ptr + str->length, ch, n);
    str->length = out_len;
}

void ks_string_add(ks_string* str, const char* ch){
    ks_string_add_n(str, strlen(ch), ch);
}

void ks_string_resize(ks_string* str, uint32_t size){
    ks_string_reserve(str, size);
    str->length = size;
}

void ks_string_reserve(ks_string* str, uint32_t cap){
    if(str->capacity >= cap) return;
    str->ptr = realloc(str->ptr, cap*sizeof (char));
    str->capacity = cap;
}

void ks_string_set(ks_string* str, const char* ch){
    ks_string_clear(str);

    uint32_t new_len = strlen(ch);
    ks_string_reserve(str, new_len + 1);
    str->length = new_len;
    strcpy(str->ptr, ch);
    str->ptr[str->length] = 0;
}


inline uint32_t ks_string_first_not_of(const ks_string* str, uint32_t start, const char *c){
    uint32_t len = str->length;
    for(uint32_t i=start; i< len; i++){
        const char* b = c;
        char now = str->ptr[i];
        do{
            if(*b == now) break;
            b++;
            if(*b == 0) return i - start;
        }while(true);
    }
    return str->length-start;
}

inline uint32_t ks_string_first_c_of(const ks_string* str, uint32_t start, char c){
    uint32_t len = str->length;
    for(uint32_t i=start; i< len; i++){
            if(c == str->ptr[start + i]) return i - start;
    }
    return str->length-start;
}

inline uint32_t ks_string_first_of(const ks_string* str, uint32_t start, const char * c){
    uint32_t len = str->length;
    for(uint32_t i=start; i< len; i++){
        const char* b = c;
        char now = str->ptr[i];
        do{
            if(*b == now) return i - start;
            b++;
            if(*b == 0) break;
        }while(true);
    }
    return str->length-start;
}

