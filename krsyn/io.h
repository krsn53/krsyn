#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "./string.h"

typedef struct ks_io_funcs ks_io_funcs;

typedef struct ks_io{
    ks_string *str;
    uint32_t seek;
    uint32_t indent;
}ks_io;

typedef struct ks_object_data ks_object_data;
typedef struct ks_array_data ks_array_data;
typedef struct ks_value ks_value;

typedef enum ks_value_type{
    KS_VALUE_MAGIC_NUMBER,
    KS_VALUE_U64,
    KS_VALUE_U32,
    KS_VALUE_U16,
    KS_VALUE_U8,

}ks_value_type ;

typedef struct ks_io_funcs {
    uint32_t (* prop)(ks_io*,const ks_io_funcs*, const char*, bool,  bool);
    bool (* value)(ks_io*, const ks_io_funcs*,  void*, ks_value_type , uint32_t, bool);
    bool (* array_begin)(ks_io*, const ks_io_funcs*, ks_array_data* , bool);
    bool (* array_elem)(ks_io*,const ks_io_funcs*,  ks_array_data* ,  bool);
    bool (* array_end)(ks_io*, const ks_io_funcs*, ks_array_data* , bool);
    bool (* object_begin)(ks_io*,const ks_io_funcs*,  ks_object_data* , bool);
    bool (* object_end)(ks_io*, const ks_io_funcs*, ks_object_data* , bool);

}ks_io_funcs;

typedef bool (* ks_value_func)(ks_io*, const ks_io_funcs*, void*,  uint32_t, bool);


typedef struct ks_value{
    ks_value_func func;
    void * data;
}ks_value;

typedef struct ks_property{
    const char* name;
    ks_value value;
}ks_property;

typedef struct ks_array_data{
    uint32_t length;
    ks_value value;
    uint32_t elem_size;
    bool fixed_length;
}ks_array_data;

typedef struct ks_object_data{
    const char* type;
    ks_value value;
}ks_object_data;



bool ks_io_print_indent(ks_io* io,  char indent, bool serialize);

bool ks_io_print_endl(ks_io* io, bool serialize);



uint32_t ks_io_value_text(ks_io* io, void* v, ks_value_type type, uint32_t offset, bool serialize);



uint32_t ks_io_prop_text(ks_io* io, const char* str, const char* delims, bool serialize);

uint32_t ks_io_fixed_text(ks_io* io, const char* str, bool serialize);

bool ks_io_fixed_property(ks_io* io, const ks_io_funcs* funcs,  ks_property prop, bool serialize);

bool ks_io_fixed_props_args(ks_io* io, bool serialize, const ks_io_funcs* funcs, ...);

bool ks_io_fixed_props(ks_io* io,  const ks_io_funcs* funcs, uint32_t num_props, ks_property* props, bool serialize);

bool ks_io_chunks_args(ks_io* io, bool serialize, const ks_io_funcs* funcs,  ...);

bool ks_io_chunks(ks_io* io,  const ks_io_funcs* funcs, uint32_t num_props, ks_property* props, bool serialize);

bool ks_io_magic_number(ks_io* io, const ks_io_funcs* funcs, void*data, bool serialize);

bool ks_io_u64(ks_io* io, const ks_io_funcs* funcs, void*data, uint32_t offset, bool serialize);
bool ks_io_u32(ks_io* io, const ks_io_funcs* funcs, void*data, uint32_t offset, bool serialize);
bool ks_io_u16(ks_io* io, const ks_io_funcs* funcs,  void*data,uint32_t offset,  bool serialize);
bool ks_io_u8(ks_io* io, const ks_io_funcs* funcs,  void*data, uint32_t offset, bool serialize);

bool ks_io_array(ks_io* io, const ks_io_funcs* funcs, void*data, uint32_t offset, bool serialize);

bool ks_io_object(ks_io* io, const ks_io_funcs* funcs,  void*data, uint32_t offset, bool serialize);
uint32_t ks_io_prop_default(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed,   bool serialize);

bool ks_io_value_default(ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type, uint32_t offset,  bool serialize);
bool ks_io_array_begin_default(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr,  bool serialize);
bool ks_io_array_elem_default(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr,  bool serialize);
bool ks_io_object_begin_default(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj,  bool serialize);
bool ks_io_array_end_default(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr,  bool serialize);
bool ks_io_object_end_default(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize);


extern const ks_io_funcs default_io ;


uint32_t ks_io_fixed_bin(ks_io* io, const char* str, bool serialize);
bool ks_io_value_bin(ks_io* io, uint32_t length, char* c, bool serialize);
uint32_t ks_io_prop_binary(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize);

bool ks_io_value_binary(ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type, uint32_t offset,  bool serialize);
bool ks_io_array_begin_binary(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr,  bool serialize);
bool ks_io_array_elem_binary(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr,  bool serialize);
bool ks_io_object_begin_binary(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj,  bool serialize);
bool ks_io_array_end_binary(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize);
bool ks_io_object_end_binary(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize);

extern const ks_io_funcs binary_io ;






ks_value ks_value_ptr(void* ptr, ks_value_func func);

#define ks_begin_props(io, funcs, serialize, offset, type, obj) { \
    ks_io * __KS_IO = io; \
    const ks_io_funcs * __KS_IO_FUNCS = funcs; \
    uint32_t __OFFSET = offset; \
    bool __SERIALIZE = serialize; \
    type * __KS_OBJECT = obj;

#define ks_end_props }

#define ks_io_custom_func(type) ks_io_ ## type
#define ks_io_decl_custom_func(type) bool ks_io_custom_func(type) ( ks_io* io, const ks_io_funcs* funcs,  void *v, uint32_t offset, bool serialize )

#define ks_io_begin_custom_func(type) ks_io_decl_custom_func(type){ \
    ks_begin_props(io, funcs, serialize, offset, type, v);

#define ks_io_end_custom_func ks_end_props return true; }

ks_property ks_prop_v(const char* name, ks_value value);

#ifdef __cplusplus
    #define ks_type(type) type
#else
    #define ks_type(type) (type)
#endif

#define ks_elem_access(elem) __KS_OBJECT[ __OFFSET ].  elem
#define ks_val(elem, func) ks_value_ptr(& ks_elem_access(elem), func)
#define ks_prop_f(name, var, func) ks_prop_v(name, ks_val(var, func))

#define ks_prop_arr_data_len(len, value, size, fixed) (ks_type(ks_array_data[]) { \
    (ks_type(ks_array_data) { \
        len, \
        value, \
        size, \
        fixed, \
    }) \
})

#define ks_prop_arr_data(var, value) ks_prop_arr_data_len(sizeof(ks_elem_access(var))/ sizeof(*ks_elem_access(var)), value, sizeof(*ks_elem_access(var)), true)

#define ks_prop_obj_data(type, var) (ks_type(ks_object_data[]){ \
    (ks_type(ks_object_data) {\
        #type , \
        ks_val(var, ks_io_custom_func(type)), \
    }) \
})

#define ks_value_obj(type, var) ks_value_ptr(ks_prop_obj_data(type, var), ks_io_object)

#define ks_value_arr(var, value) ks_value_ptr(ks_prop_arr_data(var, value), ks_io_array)
#define ks_value_arr_len(len, value, size) ks_value_ptr(ks_prop_arr_data_len(len, value, size, false), ks_io_array)

#define ks_value_u64(elem) ks_val(elem, ks_io_u64)
#define ks_value_u32(elem) ks_val(elem, ks_io_u32)
#define ks_value_u16(elem) ks_val(elem, ks_io_u16)
#define ks_value_u8(elem) ks_val(elem, ks_io_u8)

#define ks_prop_u64_as(name, var) ks_prop_f(name, var, ks_io_u64)
#define ks_prop_u32_as(name, var) ks_prop_f(name, var, ks_io_u32)
#define ks_prop_u16_as(name, var) ks_prop_f(name, var, ks_io_u16)
#define ks_prop_u8_as(name, var) ks_prop_f(name, var, ks_io_u8)
#define ks_prop_obj_as(name, type, var) ks_prop_v(name, ks_value_obj( type, var ))

#define ks_prop_u64(name) ks_prop_u64_as(#name, name)
#define ks_prop_u32(name) ks_prop_u32_as(#name, name)
#define ks_prop_u16(name) ks_prop_u16_as(#name, name)
#define ks_prop_u8(name) ks_prop_u8_as(#name, name)
#define ks_prop_obj(name, type) ks_prop_obj_as(#name, type, name)

#define ks_prop_arr_as(name, var, value) ks_prop_v(name, ks_value_arr( var, value ) )
#define ks_prop_arr_u64_as(name, var) ks_prop_arr_as(name, var,  ks_value_u64(var))
#define ks_prop_arr_u32_as(name, var) ks_prop_arr_as(name, var,  ks_value_u32(var))
#define ks_prop_arr_u16_as(name, var) ks_prop_arr_as(name, var,  ks_value_u16(var))
#define ks_prop_arr_u8_as(name, var) ks_prop_arr_as(name, var,  ks_value_u8(var))
#define ks_prop_arr_obj_as(name, type, var) ks_prop_arr_as(name, var,  ks_value_obj(type, var))

#define ks_prop_arr_len_as(name, len, value, size) ks_prop_v(name, ks_value_arr_len( len, value, size ) )
#define ks_prop_arr_len_u64_as(name, len, var) ks_prop_arr_len_as(name, len, ks_value_u64(var),  sizeof(*ks_elem_access(var)))
#define ks_prop_arr_len_u32_as(name, len, var) ks_prop_arr_len_as(name, len, ks_value_u32(var),  sizeof(*ks_elem_access(var)))
#define ks_prop_arr_len_u16_as(name, len, var) ks_prop_arr_len_as(name, len, ks_value_u16(var),  sizeof(*ks_elem_access(var)))
#define ks_prop_arr_len_u8_as(name, len, var) ks_prop_arr_len_as(name, len,  ks_value_u8(var), sizeof(*ks_elem_access(var)))
#define ks_prop_arr_len_obj_as(name, type, len, var) ks_prop_arr_len_as(name, len, ks_value_obj(type, var),  sizeof(*ks_elem_access(var)))

#define ks_prop_arr(name, value) ks_prop_arr_as(#name, name, value)
#define ks_prop_arr_u64(name) ks_prop_arr_u64_as(#name, name)
#define ks_prop_arr_u32(name) ks_prop_arr_u32_as(#name, name)
#define ks_prop_arr_u16(name) ks_prop_arr_u16_as(#name, name)
#define ks_prop_arr_u8(name) ks_prop_arr_u8_as(#name, name)
#define ks_prop_arr_obj(name, type) ks_prop_arr_obj_as(#name, type, name)


#define ks_prop_arr_len_u64(name, len) ks_prop_arr_len_u64_as(#name, len, name)
#define ks_prop_arr_len_u32(name, len) ks_prop_arr_len_u32_as(#name, len, name)
#define ks_prop_arr_len_u16(name, len) ks_prop_arr_len_u16_as(#name, len, name)
#define ks_prop_arr_len_u8(name, len) ks_prop_arr_len_u8_as(#name, len, name)
#define ks_prop_arr_len_obj(name, type, len) ks_prop_arr_len_obj_as(#name,  type, len, name)

#define ks_fixed_prop(prop) ks_io_fixed_property(__KS_IO, __KS_IO_FUNCS, prop, __SERIALIZE);
#define ks_fixed_props_len(len, ...)  { \
    ks_property __PROPS[len] = { \
        __VA_ARGS__ \
    }; \
    if(!ks_io_fixed_props(__KS_IO, __KS_IO_FUNCS, sizeof(__PROPS) / sizeof(ks_property), __PROPS, __SERIALIZE)) return false; \
}
#define ks_chunks_len(len, ...) { \
    ks_property __CHUNKS[len] = { \
        __VA_ARGS__ \
    }; \
    if(!ks_io_chunks(__KS_IO, __KS_IO_FUNCS, sizeof(__CHUNKS) / sizeof(ks_property), __CHUNKS, __SERIALIZE)) return false; \
}

#ifdef __TINYC__
#define ks_fixed_props(...) ks_io_fixed_props_args(__KS_IO, __SERIALIZE, __KS_IO_FUNCS, __VA_ARGS__ , ks_type(ks_property){NULL, { 0 }});
#define ks_chunks(...) ks_io_chunks_args(__KS_IO, __SERIALIZE, __KS_IO_FUNCS, __VA_ARGS__, ks_type(ks_property){NULL, { 0 }});
#else
#define ks_fixed_props(...) ks_fixed_props_len(, __VA_ARGS__)
#define ks_chunks(...) ks_chunks_len(, __VA_ARGS__)
#endif



#define ks_magic_number(num) ks_io_magic_number(__KS_IO, __KS_IO_FUNCS, num,  __SERIALIZE);