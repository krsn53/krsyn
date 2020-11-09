#include "string.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

ks_string *ks_string_new(unsigned capacity) {
    ks_string* ret = malloc(sizeof(ks_string));
    *ret = (ks_string){
            .capacity = capacity,
            .length = 0,
            .ptr = malloc(sizeof(char)*capacity),

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

void ks_string_add_n(ks_string* str, unsigned n, const char* ch){
    unsigned out_len = str->length + n;

    if(out_len >= str->capacity) {
        ks_string_reserve(str, str->capacity*2);
    }

    memcpy(str->ptr + str->length, ch, n);
    str->length = out_len;
}

void ks_string_add(ks_string* str, const char* ch){
    ks_string_add_n(str, strlen(ch), ch);
}

void ks_string_reserve(ks_string* str, unsigned cap){
    if(str->capacity < cap) {
        str->ptr = realloc(str->ptr, cap*sizeof (char));
        str->capacity = cap;
    }
}

void ks_string_set(ks_string* str, const char* ch){
    ks_string_clear(str);

    unsigned new_len = strlen(ch);
    ks_string_reserve(str, new_len + 1);
    str->length = new_len;
    strcpy(str->ptr, ch);
    str->ptr[str->length] = 0;
}


inline unsigned ks_string_first_not_of(const ks_string* str, unsigned start, const char *c){
    unsigned len = str->length;
    for(unsigned i=start; i< len; i++){
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

inline unsigned ks_string_first_c_of(const ks_string* str, unsigned start, char c){
    unsigned len = str->length;
    for(unsigned i=start; i< len; i++){
            if(c == str->ptr[start + i]) return i - start;
    }
    return str->length-start;
}

inline unsigned ks_string_first_of(const ks_string* str, unsigned start, const char * c){
    unsigned len = str->length;
    for(unsigned i=start; i< len; i++){
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

