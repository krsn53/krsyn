#include "string.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
#include "vector.h"

ks_string *ks_string_new() {
    u32 cap = 1;
    ks_string* ret = malloc(sizeof(ks_string));
    *ret = (ks_string){
            .capacity = cap,
            .length = 0,
            .data = calloc(cap, sizeof(char)),
        };
    return ret;
}

void ks_string_free(ks_string* str){
    free(str->data);
    free(str);
}


void ks_string_clear(ks_string* str){
    ks_vector_clear(str);
}

void ks_string_add_c(ks_string* str, char ch){
     ks_vector_push(str, ch);
}

void ks_string_add_n(ks_string* str, u32 n, const char* ch){
    ks_string_reserve(str, str->length + n + 1);
    str->data[str->length + n] = 0;
    ks_vector_push_range(str, n, ch);
}

void ks_string_add(ks_string* str, const char* ch){
    ks_vector_push_range(str, strlen(ch), ch);
}

void ks_string_resize(ks_string* str, u32 size){
    ks_vector_resize(str, size);
}

void ks_string_reserve(ks_string* str, u32 cap){
    ks_vector_reserve(str, cap);
}

void ks_string_set(ks_string* str, const char* ch){
    ks_string_set_n(str, strlen(ch), ch);
}

void ks_string_set_n(ks_string* str, u32 n, const char* ch){
    ks_string_clear(str);

    ks_string_reserve(str, n+ 1);
    str->length = n;
    strncpy(str->data, ch,n);
    str->data[str->length] = 0;
}


inline u32 ks_string_first_not_of(const ks_string* str, u32 start, const char *c){
    u32 len = str->length;
    for(u32 i=start; i< len; i++){
        const char* b = c;
        char now = str->data[i];
        do{
            if(*b == now) break;
            b++;
            if(*b == 0) return i - start;
        }while(true);
    }
    return str->length-start;
}

inline u32 ks_string_first_c_of(const ks_string* str, u32 start, char c){
    u32 len = str->length;
    for(u32 i=start; i< len; i++){
            char now = str->data[i];
            if(c == now) return i - start;
    }
    return str->length-start;
}

inline u32 ks_string_first_of(const ks_string* str, u32 start, const char * c){
    u32 len = str->length;
    for(u32 i=start; i< len; i++){
        const char* b = c;
        char now = str->data[i];
        do{
            if(*b == now) return i - start;
            b++;
            if(*b == 0) break;
        }while(true);
    }
    return str->length-start;
}



/*
old

ks_string *ks_string_new() {
    u32 cap = 1;
    ks_string* ret = malloc(sizeof(ks_string));
    *ret = (ks_string){
            .capacity = cap,
            .length = 0,
            .data = malloc(sizeof(char)*cap),
        };
    return ret;
}

void ks_string_free(ks_string* str){
    free(str->data);
    free(str);
}




void ks_string_clear(ks_string* str){
    memset(str->data, 0, str->capacity);
    str->length = 0;
}

void ks_string_add_c(ks_string* str, char ch){
     ks_string_add_n(str, 1, &ch);
}

void ks_string_add_n(ks_string* str, u32 n, const char* ch){
    u32 out_len = str->length + n;

    if(out_len >= str->capacity) {
        u32 cap = str->capacity;
        do{
            cap*=2;
        }while(out_len > cap);
        ks_string_reserve(str, cap);
    }

    memcpy(str->data + str->length, ch, n);
    str->length = out_len;
}

void ks_string_add(ks_string* str, const char* ch){
    ks_string_add_n(str, strlen(ch), ch);
}

void ks_string_resize(ks_string* str, u32 size){
    ks_string_reserve(str, size);
    str->length = size;
}

void ks_string_reserve(ks_string* str, u32 cap){
    if(str->capacity >= cap) return;
    str->data = realloc(str->data, cap*sizeof (char));
    str->capacity = cap;
}

*/
