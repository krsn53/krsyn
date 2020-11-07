#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "logger.h"

#ifdef __cplusplus
    #define ks_type(type) type
#else
    #define ks_type(type) (type)
#endif

#define ks_prop_access(prop) __KS_OBJECT[__OFFSET]. prop



#define ks_val_const(prop, func) (ks_type(ks_value) { \
    func, \
    prop, \
})

#define ks_val(prop, func) ks_val_const(& ks_prop_access(prop), func)

#define ks_val_u(prop) ks_val(prop, ks_io_u)
#define ks_val_u8(prop) ks_val(prop, ks_io_u8)

#define ks_begin_props(io, funcs, offset, serialize, type, obj) { \
    ks_io * __KS_IO = io; \
    const ks_io_funcs * __KS_IO_FUNCS = funcs; \
    unsigned __OFFSET = offset; \
    bool __SERIALIZE = serialize; \
    type * __KS_OBJECT = obj;

#define ks_end_props }

#define ks_io_custom_func(type) ks_io_ ## type
#define ks_io_decl_custom_func(type) bool ks_io_custom_func(type) ( ks_io* io, const ks_io_funcs* funcs,  void *v, unsigned offset,  bool serialize )

#define ks_io_begin_custom_func(type) ks_io_decl_custom_func(type){ \
    ks_begin_props(io, funcs, offset, serialize, type, v);

#define ks_io_end_custom_func ks_end_props return true; }

#define ks_prop_v(name, val) (ks_type(ks_property){ \
    name, \
    val, \
})

#define ks_prop_f(name, var, func) ks_prop_v(name, ks_val(var, func))


#define ks_prop_arr_data(var, val) (ks_type(ks_array_data[]) { \
    (ks_type(ks_array_data){ \
        sizeof(ks_prop_access(var))/ sizeof(*ks_prop_access(var)), \
        val, \
    }) \
})

#define ks_prop_obj_data(type, var) (ks_type(ks_object_data[]){ \
    (ks_type(ks_object_data) {\
        #type , \
        ks_val(var, ks_io_custom_func(type)), \
    }) \
})

#define ks_val_obj(type, var) ks_val_const(ks_prop_obj_data(type, var), ks_io_object)

#define ks_val_arr(var, val) ks_val_const(ks_prop_arr_data(var, val), ks_io_array)

#define ks_prop_u_as(name, var) ks_prop_f(name, var, ks_io_u)
#define ks_prop_u8_as(name, var) ks_prop_f(name, var, ks_io_u8)
#define ks_prop_obj_as(name, type, var) ks_prop_v(name, ks_val_obj( type, var ))

#define ks_prop_u(name) ks_prop_u_as(#name, name)
#define ks_prop_u8(name) ks_prop_u8_as(#name, name)
#define ks_prop_obj(name, type) ks_prop_obj_as(#name, type, name)

#define ks_prop_arr_as(name, var, val) ks_prop_v(name, ks_val_arr( var, val ) )
#define ks_prop_arr_u_as(name, var) ks_prop_arr_as(name, var,  ks_val_u(var))
#define ks_prop_arr_u8_as(name, var) ks_prop_arr_as(name, var,  ks_val_u8(var))
#define ks_prop_arr_obj_as(name, type, var) ks_prop_arr_as(name, var,  ks_val_obj(type, var))

#define ks_prop_arr(name, val) ks_prop_arr_as(#name, name, val)
#define ks_prop_arr_u(name) ks_prop_arr_u_as(#name, name)
#define ks_prop_arr_u8(name) ks_prop_arr_u8_as(#name, name)
#define ks_prop_arr_obj(name, type) ks_prop_arr_obj_as(#name, type, name)

#define ks_fixed_prop(prop) ks_io_fixed_property(__KS_IO, __KS_IO_FUNCS, prop, __SERIALIZE);
#define ks_fixed_props(...)  { \
    ks_property __PROPS[] = { \
        __VA_ARGS__ \
    }; \
    if(!ks_io_fixed_props(__KS_IO, __KS_IO_FUNCS, sizeof(__PROPS) / sizeof(ks_property), __PROPS, __SERIALIZE)) return false; \
}

#define ks_chunks(...) { \
    ks_property __CHUNKS[] = { \
        __VA_ARGS__ \
    }; \
    if(!ks_io_chunks(__KS_IO, __KS_IO_FUNCS, sizeof(__CHUNKS) / sizeof(ks_property), __CHUNKS, __SERIALIZE)) return false; \
}

#define ks_magic_number(num) ks_io_magic_number(__KS_IO, __KS_IO_FUNCS, num, 0, __SERIALIZE);

typedef struct ks_string{
    unsigned capacity;
    unsigned length;
    char* ptr;
} ks_string;

typedef struct ks_io_funcs ks_io_funcs;

typedef struct ks_io{
    ks_string *str;
    unsigned seek;
    unsigned indent;
}ks_io;

typedef struct ks_object_data ks_object_data;
typedef struct ks_array_data ks_array_data;
typedef struct ks_value ks_value;

typedef enum ks_value_type{
    KS_VALUE_MAGIC_NUMBER,
    KS_VALUE_U,
    KS_VALUE_U8,

}ks_value_type ;

typedef struct ks_io_funcs {
    unsigned (* prop)(ks_io*,const ks_io_funcs*, const char*, bool, bool);
    bool (* value)(ks_io*, const ks_io_funcs*,  void*, ks_value_type , bool);
    bool (* array_begin)(ks_io*, const ks_io_funcs*, ks_array_data* , bool);
    bool (* array_elem)(ks_io*,const ks_io_funcs*,  ks_array_data* , bool);
    bool (* array_end)(ks_io*, const ks_io_funcs*, ks_array_data* , bool);
    bool (* object_begin)(ks_io*,const ks_io_funcs*,  ks_object_data* , bool);
    bool (* object_end)(ks_io*, const ks_io_funcs*, ks_object_data* , bool);

}ks_io_funcs;

typedef bool (* ks_value_func)(ks_io*, const ks_io_funcs* funcs, void*, unsigned, bool);


typedef struct ks_value{
    ks_value_func func;
    void * data;
}ks_value;

typedef struct ks_property{
    const char* name;
    ks_value value;
}ks_property;

typedef struct ks_array_data{
    unsigned length;
    ks_value value;
}ks_array_data;

typedef struct ks_object_data{
    const char* type;
    ks_value value;
}ks_object_data;



ks_string *ks_string_new(unsigned capacity);
void ks_string_clear(ks_string* str);
void ks_string_add_c(ks_string* str, char ch);
void ks_string_add_n(ks_string* str, unsigned n, const char* ch);
void ks_string_free(ks_string* str);
void ks_string_reserve(ks_string* str, unsigned cap);
void ks_string_set(ks_string* str, const char* ch);
void ks_string_add(ks_string* str, const char* ch);

static inline char* ks_string_fill(char v, unsigned length, char c[length+1]){
    for(unsigned i=0; i<length; i++){
        c[i] = v;
    }
    c[length] = 0;
    return c;
}

static inline bool ks_io_print_indent(ks_io* io,  char indent, bool serialize){
    if(!serialize) return true;
    char c[io->indent+1];
    ks_string_add(io->str, ks_string_fill(indent, io->indent, c));
    return true;
}

static inline bool ks_io_print_endl(ks_io* io, bool serialize){
    if(!serialize) return true;
    ks_string_add(io->str, "\n");
    return true;
}

static inline unsigned ks_string_first_not_of(const ks_string* str, unsigned start, const char *c){
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

static inline unsigned ks_string_first_c_of(const ks_string* str, unsigned start, char c){
    unsigned len = str->length;
    for(unsigned i=start; i< len; i++){
            if(c == str->ptr[start + i]) return i - start;
    }
    return str->length-start;
}

static inline unsigned ks_string_first_of(const ks_string* str, unsigned start, const char * c){
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


static inline unsigned ks_io_value_text(ks_io* io, void* v, ks_value_type type, bool serialize){
    if(!serialize){
        unsigned first = ks_string_first_not_of(io->str, io->seek, "\t\n ");
        int p;
        int ret = sscanf(io->str->ptr + io->seek + first, "%d", &p);
        unsigned read  = ks_string_first_not_of(io->str, io->seek + first, "0123456789");
        if((ret != 1 && io->seek + first + read < io->str->length) || io->seek + first + read >= io->str->length) {
            krsyn_error("Failed to read value");
            return 0;
        }

        switch (type) {
        case KS_VALUE_U:
        {
            unsigned *u = v;
            *u=p;
            break;
        }
        case KS_VALUE_U8:
        {
            uint8_t* u = v;
            *u = p;
            break;
        }
        }

        io->seek += first + read;
        return  first + read;
    } else {
        unsigned len = 10;
        char s[len];

        int p;

        switch (type) {
        case KS_VALUE_MAGIC_NUMBER:
        case KS_VALUE_U:
        {
            unsigned *u = v;
            p = *u;
            break;
        }
        case KS_VALUE_U8:
        {
            uint8_t* u = v;
            p = *u;
            break;
        }
        }

        snprintf(s, len, "%d", p);
        ks_string_add(io->str, s);

    }
    return 1;
}



static inline unsigned ks_io_prop_text(ks_io* io, const char* str, const char* delims, bool serialize){
    if(!serialize){
        unsigned first = ks_string_first_not_of(io->str, io->seek, "\t\n ");
        unsigned prop_length = ks_string_first_of(io->str, io->seek + first, delims);

        if(prop_length == 0 && io->seek + first != io->str->length){
            krsyn_error("Failed to read property name");
            return 0;
        }


        io->seek += first + prop_length;
        return first + prop_length;
    }
    else {
        ks_string_add(io->str, str);

    }
    return 1;
}

static inline unsigned ks_io_fixed_text(ks_io* io, const char* str, bool serialize){
    if(!serialize){
        unsigned first = ks_string_first_not_of(io->str, io->seek, "\t\n ");
        unsigned tex_length = strlen(str);
        bool ret = strncmp(io->str->ptr + io->seek + first, str, strlen(str)) == 0;
        if(!ret){
            krsyn_error("Unexcepted syntax, excepted \"%s\"", str);
            return false;
        }

        io->seek += first + tex_length;
        return first + tex_length;
    }
    else {
        ks_string_add(io->str, str);

    }
    return 1;
}

static inline bool ks_io_fixed_property(ks_io* io, const ks_io_funcs* funcs,  ks_property prop, bool serialize){
    unsigned prop_length = funcs->prop(io, funcs, prop.name,  true, serialize);
    if(prop_length == 0){
        return false;
    }
    if(!prop.value.func(io, funcs, prop.value.data, 0, serialize)) {return false;}
    return true;
}

static inline bool ks_io_fixed_props(ks_io* io,  const ks_io_funcs* funcs, unsigned num_props, ks_property* props, bool serialize){
    for(unsigned i=0; i< num_props; i++){
       if(!ks_io_fixed_property(io, funcs, props[i], serialize)) return false;
    }
    return true;
}

static inline bool ks_io_chunks(ks_io* io,  const ks_io_funcs* funcs, unsigned num_props, ks_property* props, bool serialize){
    if(!serialize){
        int begin=0;
        int i=0;
        unsigned prop_length=0;
        while ((prop_length = funcs->prop(io, funcs, "",  false, serialize)) != 0){
               begin = i;
               do{
                   if(strncmp(props[i].name, io->str->ptr + io->seek - prop_length, strlen(props[i].name)) == 0) {
                       if(!props[i].value.func(io, funcs, props[i].value.data, 0, serialize)){
                            return false;
                        }

                        i++;
                        i%= num_props;
                        break;
                   }
                    i++;
                    i %= num_props;
                    if(i == begin) break;// unknown property
                }while(true);

                if(i == begin) { i++; i%= num_props; continue; }
        }
    }
    else {
        for(unsigned i=0; i< num_props; i++){
            if(!funcs->prop(io, funcs, props[i].name, false, serialize)){
                return false;
            }
            if(!props[i].value.func(io, funcs, props[i].value.data, 0, serialize)) {return false;}
        }
    }

    return true;
}

static inline bool ks_io_magic_number(ks_io* io, const ks_io_funcs* funcs, void*data, unsigned offset, bool serialize){
    return funcs->value(io, funcs, data, KS_VALUE_MAGIC_NUMBER, serialize);
}

static inline bool ks_io_u(ks_io* io, const ks_io_funcs* funcs, void*data, unsigned offset, bool serialize){
    return funcs->value(io, funcs, (unsigned*)data + offset, KS_VALUE_U, serialize);
}

static inline bool ks_io_u8(ks_io* io, const ks_io_funcs* funcs,  void*data, unsigned offset, bool serialize){
    return funcs->value(io, funcs, (uint8_t*)data + offset, KS_VALUE_U8, serialize);
}

static inline bool ks_io_array(ks_io* io, const ks_io_funcs* funcs, void*data, unsigned offset, bool serialize){
    ks_array_data array = *(ks_array_data*)data;

    if(! funcs->array_begin(io, funcs, &array, serialize)) return false;
    io->indent++;
    if(! funcs->array_elem(io, funcs, &array, serialize)) return false;
    io->indent--;
    if(! funcs->array_end(io, funcs, &array, serialize)) return false;

    return true;
}

static inline bool ks_io_object(ks_io* io, const ks_io_funcs* funcs,  void*data, unsigned offset, bool serialize){
    ks_object_data obj = *(ks_object_data*)data;

    if(! funcs->object_begin(io, funcs, &obj, serialize)) return false;
    io->indent++;
    if(! obj.value.func(io, funcs, obj.value.data, offset, serialize)) return false;
    io->indent--;
    if(! funcs->object_end(io, funcs, &obj, serialize)) return false;

    return true;
}

static inline unsigned ks_io_prop_default(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize){
    ks_io_print_indent(io, '\t', serialize);

    if(!serialize){
        io->seek += ks_string_first_not_of(io->str, io->seek, "\t\n ");
        if( io->seek >= io->str->length || io->str->ptr[io->seek] == '}'){
            return 0;
        }
    }
    unsigned dot = ks_io_fixed_text(io, ".", serialize );
    unsigned ret =   ks_io_prop_text(io, name, "\t\n =", serialize) + ks_io_fixed_text(io, "=", serialize );

    if(!serialize && fixed && strncmp(name, io->str->ptr + io->seek - ret, strlen(name)) != 0){
        krsyn_error("excepted property \"%s\"", name);
        return 0;
    }

    return ret;
}

static inline bool ks_io_value_default(ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type,  bool serialize){
    switch(type){
    case KS_VALUE_MAGIC_NUMBER:
        ks_io_fixed_text(io, "// Magic number : ", serialize);
        return ks_io_fixed_text(io, u, serialize) && ks_io_print_endl(io, serialize);
    default:
        break;
    }
    unsigned ret = ks_io_value_text(io, u, type, serialize);
    if(ret == 0) return false;
    ret += ks_io_fixed_text(io, ", ", serialize);
    ks_io_print_endl(io, serialize);
    return ret != 0;
}

static inline bool ks_io_array_begin_default(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return  ks_io_fixed_text(io, "{", serialize) && ks_io_print_endl(io, serialize);
}

static inline bool ks_io_array_elem_default(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, bool serialize){
    for(unsigned i=0; i< arr->length; i++){
        ks_io_print_indent(io,'\t', serialize);
        if(!arr->value.func(io, funcs, arr->value.data, i, serialize)) return false;
    }
    return true;
}
static inline bool ks_io_object_begin_default(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj, bool serialize){
    return ks_io_fixed_text(io, "{", serialize) && ks_io_print_endl(io, serialize);
}
static inline bool ks_io_array_end_default(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return ks_io_fixed_text(io, "}", serialize)  &&  ks_io_fixed_text(io, ",", serialize) && ks_io_print_endl(io, serialize);
}

static inline bool ks_io_object_end_default(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize){
    return  ks_io_print_indent(io,'\t', serialize)&& ks_io_fixed_text(io, "}", serialize)  &&  ks_io_fixed_text(io, ",", serialize)  && ks_io_print_endl(io, serialize);
}



static const ks_io_funcs default_io ={
    .value = ks_io_value_default,
    .array_begin = ks_io_array_begin_default,
    .array_elem = ks_io_array_elem_default,
    .array_end = ks_io_array_end_default,
    .object_begin = ks_io_object_begin_default,
    .object_end = ks_io_object_end_default,
    .prop = ks_io_prop_default,
};


static inline unsigned ks_io_fixed_bin(ks_io* io, const char* str, bool serialize){
    if(!serialize){
        unsigned first = ks_string_first_c_of(io->str, io->seek, 0) + 1;
        if(strncmp(io->str->ptr + io->seek, str, first) == 0){
            io->seek += first;
            return first;
        }
        return 0;
    }
    else {
        ks_string_add_n(io->str, strlen(str)+1, str);
    }
    return 1;
}

static inline bool ks_io_value_bin(ks_io* io, unsigned length, char* c, bool serialize){
    if(!serialize){
        if(io->seek + length > io->str->length) {
            krsyn_error("Unexcepted file end");
            return false;
        }
        strncpy(c, io->str->ptr + io->seek, length);
        io->seek += length;
    }
    else {
        ks_string_add_n(io->str, length, c);
    }
    return true;
}

static inline unsigned ks_io_prop_binary(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize){
    if(fixed) {
        return true;
    }
    unsigned step = ks_io_fixed_bin(io, name, serialize);
    return step;
}

static inline bool ks_io_value_binary(ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type,  bool serialize){
    switch (type) {
    case KS_VALUE_MAGIC_NUMBER:{
        if(!serialize){
            if(strncmp(u, io->str->ptr + io->seek, 4) != 0){
                char c[5] = { 0 };
                strncpy(c, io->str->ptr + io->seek, 4);
                krsyn_error("Excepted magic number \"%s\", detected \"%s\"", u, c);
               return false;
            }
            io->seek += 4;
        } else {
            ks_string_add_n(io->str, 4, u);
        }

        break;
    }
    case KS_VALUE_U:{
        union {
            unsigned* u;
            char(* c)[sizeof(unsigned)];
        } v = {.u = (unsigned*)u};
        return ks_io_value_bin(io, sizeof(*v.c), *v.c, serialize);
    }
    case KS_VALUE_U8:{
        union {
            uint8_t* u;
            char(* c)[sizeof(uint8_t)];
        } v = {.u = (uint8_t*)u};
        return ks_io_value_bin(io, sizeof(*v.c), *v.c, serialize);
    }
    }
    return true;
}

static inline bool ks_io_array_begin_binary(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return true;
}

static inline bool ks_io_array_elem_binary(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, bool serialize){
    for(unsigned i=0; i< arr->length; i++){
        if(!arr->value.func(io, funcs, arr->value.data, i, serialize)) return false;
    }
    return true;
}
static inline bool ks_io_object_begin_binary(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj, bool serialize){
    return true;
}
static inline bool ks_io_array_end_binary(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return true;
}

static inline bool ks_io_object_end_binary(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize){
    return true;
}

static const ks_io_funcs binary_io ={
    .value = ks_io_value_binary,
    .array_begin = ks_io_array_begin_binary,
    .array_elem = ks_io_array_elem_binary,
    .array_end = ks_io_array_end_binary,
    .object_begin = ks_io_object_begin_binary,
    .object_end = ks_io_object_end_binary,
    .prop = ks_io_prop_binary,
};


