#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h> // NULL
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
    KS_VALUE_STRING_ELEM,
    KS_VALUE_OBJECT,
    KS_VALUE_ARRAY,
}ks_value_type ;

typedef struct ks_io_funcs {
    bool (* key)(ks_io*, const ks_io_funcs*, const char*, bool);
    bool (* value)(ks_io*, const ks_io_funcs*,  void*, ks_value_type , uint32_t);
    bool (* string)(ks_io*, const ks_io_funcs*, uint32_t , ks_string* );
    bool (* array_num)(ks_io*, const ks_io_funcs*, uint32_t*);
    bool (* array_begin)(ks_io*, const ks_io_funcs*, ks_array_data*);
    bool (* array_elem)(ks_io*,const ks_io_funcs*,  ks_array_data*, uint32_t );
    bool (* array_end)(ks_io*, const ks_io_funcs*, ks_array_data*);
    bool (* object_begin)(ks_io*,const ks_io_funcs*,  ks_object_data*);
    bool (* object_end)(ks_io*, const ks_io_funcs*, ks_object_data*);

}ks_io_funcs;

typedef bool (* ks_value_func)(ks_io*, const ks_io_funcs*, void*,  uint32_t);


typedef struct ks_value{
    ks_value_type type;
    void * data;
}ks_value;

typedef struct ks_property{
    void* name;
    ks_value value;
}ks_property;

typedef struct ks_array_data{
    uint32_t length;
    uint32_t elem_size;
    ks_value value;
    bool (*check_enabled)(const void*);
    bool fixed_length;
}ks_array_data;

typedef struct ks_object_data{
    const char* type;
    ks_value_func serializer, deserializer;
    void* data;
}ks_object_data;



bool ks_io_print_indent(ks_io* io,  char indent, bool serialize);
bool ks_io_print_endl(ks_io* io, bool serialize);
bool ks_io_print_space(ks_io* io, bool serialize);


uint32_t ks_io_value_text(ks_io* io, void* v, ks_value_type type, uint32_t offset, bool serialize);


uint32_t ks_io_prop_text(ks_io* io, const char* str, const char* delims, bool serialize);
uint32_t ks_io_text_len(ks_io* io, uint32_t length, const char* str, bool serialize);
uint32_t ks_io_text(ks_io* io, const char* str, bool serialize);
uint32_t ks_io_fixed_text(ks_io* io, const char* str, bool serialize);

bool ks_io_fixed_property(ks_io* io, const ks_io_funcs* funcs,  ks_property prop, bool serialize);


bool ks_io_magic_number(ks_io* io, const ks_io_funcs* funcs, void*data);

bool ks_io_array(ks_io* io, const ks_io_funcs* funcs, ks_array_data array, uint32_t offset, bool serialize);

bool ks_io_object(ks_io* io, const ks_io_funcs* funcs, ks_object_data obj, uint32_t offset, bool serialize);

bool ks_io_key_clike(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize);
bool ks_io_array_num_clike(ks_io* io, const ks_io_funcs* funcs, uint32_t* num, bool serialize);
bool ks_io_string_clike(ks_io* io, const ks_io_funcs* funcs, uint32_t length, ks_string* str, bool serialize);
bool ks_io_value_clike(ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type, uint32_t offset,  bool serialize);
bool ks_io_array_begin_clike(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr,  bool serialize);
bool ks_io_array_elem_clike(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, uint32_t index, bool serialize);
bool ks_io_object_begin_clike(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj,  bool serialize);
bool ks_io_array_end_clike(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr,  bool serialize);
bool ks_io_object_end_clike(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize);


uint32_t ks_io_fixed_bin(ks_io* io, const char* str, bool serialize);
bool ks_io_value_bin(ks_io* io, uint32_t length, char* c, bool swap_endian, bool serialize);

bool ks_io_key_binary(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool swap_endian, bool serialize);
bool ks_io_value_binary(ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type, uint32_t offset,  bool swap_endian, bool serialize);
bool ks_io_string_binary(ks_io* io, const ks_io_funcs* funcs, uint32_t length, ks_string* str, bool swap_endian, bool serialize);
bool ks_io_array_num_binary(ks_io* io, const ks_io_funcs* funcs, uint32_t* num, bool swap_endian, bool serialize);
bool ks_io_array_begin_binary(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool swap_endian,  bool serialize);
bool ks_io_array_elem_binary(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, uint32_t index, bool swap_endian, bool serialize);
bool ks_io_array_end_binary(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool swap_endian, bool serialize);
bool ks_io_object_begin_binary(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj,  bool swap_endian, bool serialize);
bool ks_io_object_end_binary(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool swap_endian, bool serialize);

bool ks_io_key_binary_little_endian(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize);
bool ks_io_value_binary_little_endian(ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type, uint32_t offset,  bool serialize);
bool ks_io_string_binary_little_endian(ks_io* io, const ks_io_funcs* funcs, uint32_t length, ks_string* str, bool serialize);
bool ks_io_array_num_binary_little_endian(ks_io* io, const ks_io_funcs* funcs, uint32_t* num, bool serialize);
bool ks_io_array_begin_binary_little_endian(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize);
bool ks_io_array_elem_binary_little_endian(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, uint32_t index, bool serialize);
bool ks_io_array_end_binary_little_endian(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize);
bool ks_io_object_begin_binary_little_endian(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj,  bool serialize);
bool ks_io_object_end_binary_little_endian(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize);

bool ks_io_key_binary_big_endian(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize);
bool ks_io_value_binary_big_endian(ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type, uint32_t offset,  bool serialize);
bool ks_io_string_binary_big_endian(ks_io* io, const ks_io_funcs* funcs, uint32_t length, ks_string* str, bool serialize);
bool ks_io_array_num_binary_big_endian(ks_io* io, const ks_io_funcs* funcs, uint32_t* num,bool serialize);
bool ks_io_array_begin_binary_big_endian(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr,  bool serialize);
bool ks_io_array_elem_binary_big_endian(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, uint32_t index, bool serialize);
bool ks_io_array_end_binary_big_endian(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr,bool serialize);
bool ks_io_object_begin_binary_big_endian(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj,bool serialize);
bool ks_io_object_end_binary_big_endian(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize);

#define ks_io_funcs_func(pre, name) ks_io_ ## pre ## _ ## name

#define ks_io_funcs_decl_ext(name) extern const ks_io_funcs name ## _serializer, name ## _deserializer;
#define ks_io_funcs_decl_with( name, with) \
    const ks_io_funcs name ## with = { \
        ks_io_funcs_func(key , name ## with), \
        ks_io_funcs_func(value , name ## with), \
        ks_io_funcs_func(string , name ## with), \
        ks_io_funcs_func(array_num , name ## with), \
        ks_io_funcs_func(array_begin , name ## with), \
        ks_io_funcs_func(array_elem , name ## with), \
        ks_io_funcs_func(array_end , name ## with), \
        ks_io_funcs_func(object_begin , name ## with), \
        ks_io_funcs_func(object_end , name ## with), \
    };
#define ks_io_funcs_decl_serializer( name ) ks_io_funcs_decl_with( name ,  _serializer)
#define ks_io_funcs_decl_deserializer( name ) ks_io_funcs_decl_with( name , _deserializer)


#define ks_io_funcs_impl_with_add( name, with, add) \
    bool ks_io_funcs_func(key , name ## with) (ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed) { \
        return ks_io_funcs_func(key, name ) (io, funcs, name, fixed, add); \
    } \
    bool ks_io_funcs_func(value , name ## with) (ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type, uint32_t offset) { \
        return ks_io_funcs_func(value, name ) (io, funcs, u, type, offset, add);\
    } \
    bool ks_io_funcs_func(string , name ## with) (ks_io* io, const ks_io_funcs* funcs, uint32_t length, ks_string* str) { \
        return ks_io_funcs_func(string, name) (io, funcs, length, str, add );\
    } \
    bool ks_io_funcs_func(array_num , name ## with) (ks_io* io, const ks_io_funcs* funcs, uint32_t* num) { \
        return ks_io_funcs_func(array_num, name) (io, funcs, num, add );\
    } \
    bool ks_io_funcs_func(array_begin , name ## with) (ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr) { \
        return ks_io_funcs_func(array_begin, name ) (io, funcs, arr, add); \
    } \
    bool ks_io_funcs_func(array_elem , name ## with) (ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, uint32_t index) { \
        return ks_io_funcs_func(array_elem, name ) (io,  funcs, arr,index, add); \
    } \
    bool ks_io_funcs_func(array_end , name ## with) (ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr) { \
        return ks_io_funcs_func(array_end, name ) (io, funcs, arr, add); \
    } \
    bool ks_io_funcs_func(object_begin , name ## with) (ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj) { \
        return ks_io_funcs_func(object_begin, name ) (io, funcs, obj, add); \
    } \
    bool ks_io_funcs_func(object_end , name ## with) (ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj) { \
    return ks_io_funcs_func(object_end, name ) (io, funcs, obj, add);\
    }

#define ks_io_funcs_impl_serializer( name ) ks_io_funcs_impl_with_add( name, _serializer, true )
#define ks_io_funcs_impl_deserializer( name ) ks_io_funcs_impl_with_add( name, _deserializer, false )

#define ks_io_funcs_impl(name) \
    ks_io_funcs_impl_serializer(name) \
    ks_io_funcs_impl_deserializer(name) \
    ks_io_funcs_decl_serializer(name) \
    ks_io_funcs_decl_deserializer(name)


ks_io_funcs_decl_ext(clike)
ks_io_funcs_decl_ext(binary_little_endian)
ks_io_funcs_decl_ext(binary_big_endian)


ks_value ks_value_ptr(void* ptr, ks_value_type type);

#define ks_begin_props(io, funcs, serialize, offset, type, obj) { \
    ks_io * __KS_IO = io; \
    const ks_io_funcs * __KS_IO_FUNCS = funcs; \
    uint32_t __OFFSET = offset; \
    bool __SERIALIZE = serialize; \
    type * __KS_OBJECT = obj;

#define ks_end_props }

#define ks_io_custom_func(type) ks_io_ ## type
#define ks_io_custom_func_serializer(type) ks_io_custom_func(type ## _serializer)
#define ks_io_custom_func_deserializer(type) ks_io_custom_func(type ## _deserializer)

#define ks_io_custom_func_args  ks_io* io, const ks_io_funcs* funcs,  void *v, uint32_t offset

#define ks_io_decl_custom_func(type) \
     bool ks_io_custom_func_serializer(type) (ks_io_custom_func_args); \
     bool ks_io_custom_func_deserializer(type) (ks_io_custom_func_args)

#define ks_io_begin_custom_func(type) \
    static inline bool ks_io_custom_func(type)( ks_io_custom_func_args, bool serialize ){ \
    ks_begin_props(io, funcs, serialize, offset, type, v);

#define ks_io_end_custom_func(type) ks_end_props return true; } \
bool ks_io_custom_func_serializer(type)(ks_io_custom_func_args){ return ks_io_custom_func(type)(io, funcs, v, offset, true); } \
bool ks_io_custom_func_deserializer(type)(ks_io_custom_func_args){ return ks_io_custom_func(type)(io, funcs, v, offset, false); }

ks_property ks_prop_v(void *name, ks_value value);

#ifdef __cplusplus
    #define ks_type(type) type
#else
    #define ks_type(type) (type)
#endif

#define ks_elem_access(elem) __KS_OBJECT[ __OFFSET ].  elem
#define ks_val(elem, type) ks_value_ptr(& ks_elem_access(elem), type)
#define ks_prop_f(name, var, type) ks_prop_v(name, ks_val(var, type))

#define ks_prop_obj_data(var, type) (ks_type(ks_object_data[]){ \
    (ks_type(ks_object_data) {\
        #type , \
        ks_io_custom_func_serializer(type) , \
        ks_io_custom_func_deserializer(type) , \
        & ks_elem_access(var),\
    }) \
})

#define ks_value_obj(var, type) ks_value_ptr(ks_prop_obj_data(var, type), KS_VALUE_OBJECT)

#define ks_value_str_elem(elem) ks_val(elem, KS_VALUE_STRING_ELEM)
#define ks_value_u64(elem) ks_val(elem, KS_VALUE_U64)
#define ks_value_u32(elem) ks_val(elem, KS_VALUE_U32)
#define ks_value_u16(elem) ks_val(elem, KS_VALUE_U16)
#define ks_value_u8(elem) ks_val(elem, KS_VALUE_U8)

#define ks_prop_u64_as(name, var) ks_prop_f(name, var, KS_VALUE_U64)
#define ks_prop_u32_as(name, var) ks_prop_f(name, var, KS_VALUE_U32)
#define ks_prop_u16_as(name, var) ks_prop_f(name, var, KS_VALUE_U16)
#define ks_prop_u8_as(name, var) ks_prop_f(name, var, KS_VALUE_U8)
#define ks_prop_obj_as(name, type, var) ks_prop_v(name, ks_value_obj( var, type ))

#define ks_prop_u64(name) ks_prop_u64_as(#name, name)
#define ks_prop_u32(name) ks_prop_u32_as(#name, name)
#define ks_prop_u16(name) ks_prop_u16_as(#name, name)
#define ks_prop_u8(name) ks_prop_u8_as(#name, name)
#define ks_prop_obj(name, type) ks_prop_obj_as(#name, type, name)

#define ks_prop_arr_data_size_len(len,  size, value, check_enabled, fixed) (ks_type(ks_array_data[]) { \
    (ks_type(ks_array_data) { \
        len, \
        size, \
        value, \
        check_enabled, \
        fixed, \
    }) \
})

#define ks_elem_size(var) (sizeof(*ks_elem_access(var)))

#define ks_prop_arr_data_len(len, var, value, check_enabled, fixed) ks_prop_arr_data_size_len(len, ks_elem_size(var), value, check_enabled, fixed)

#define ks_arr_size(var) (sizeof(ks_elem_access(var))/ sizeof(*ks_elem_access(var)))


#define ks_value_arr_len_sparse_fixed(len, var, value, check_enabled, fixed) ks_value_ptr(ks_prop_arr_data_len(len,  var, value, check_enabled, fixed), KS_VALUE_ARRAY)

#define ks_prop_arr_len_sparse_fixed_as(name, len, var, value, check_enabled, fixed) ks_prop_v(name, ks_value_arr_len_sparse_fixed( len, var, value, check_enabled, fixed) )

#define ks_prop_arr_as(name, var, value) ks_prop_v(name, ks_value_arr_len_sparse_fixed( ks_arr_size(var), var, value, NULL, true) )
#define ks_prop_arr(name, value)  ks_prop_arr_as(#name, name, value)
#define ks_prop_arr_u64(name) ks_prop_arr(name, ks_value_u64(name))
#define ks_prop_arr_u32(name) ks_prop_arr(name, ks_value_u32(name))
#define ks_prop_arr_u16(name) ks_prop_arr(name, ks_value_u16(name))
#define ks_prop_arr_u8(name) ks_prop_arr(name, ks_value_u8(name))
#define ks_prop_arr_obj(name, type) ks_prop_arr(name, ks_value_obj(name, type))
#define ks_prop_str(name) ks_prop_arr(name, ks_value_str_elem(name))

#define ks_prop_arr_len(name, len, value) ks_prop_v(#name, ks_value_arr_len_sparse_fixed( len, name, value, NULL, false) )
#define ks_prop_arr_len_u64(name, len) ks_prop_arr_len(name, len, ks_value_u64(name))
#define ks_prop_arr_len_u32(name, len) ks_prop_arr_len(name, len, ks_value_u32(name))
#define ks_prop_arr_len_u16(name, len) ks_prop_arr_len(name, len, ks_value_u16(name))
#define ks_prop_arr_len_u8(name, len) ks_prop_arr_len(name, len, ks_value_u8(name))
#define ks_prop_arr_len_obj(name, type, len) ks_prop_arr_len(name, len, ks_value_obj(name, type))
#define ks_prop_str_len(name, len) ks_prop_arr_len(name, len, ks_value_str_elem(name))

#define ks_prop_arr_sparse(name, value, check_enabled) ks_prop_v(#name, ks_value_arr_len_sparse_fixed( ks_arr_size(name), name, value, check_enabled, true) )
#define ks_prop_arr_sparse_u64(name, check_enabled) ks_prop_arr_sparse(name, ks_value_u64(name), check_enabled)
#define ks_prop_arr_sparse_u32(name, check_enabled) ks_prop_arr_sparse(name, ks_value_u32(name), check_enabled)
#define ks_prop_arr_sparse_u16(name, check_enabled) ks_prop_arr_sparse(name, ks_value_u16(name), check_enabled)
#define ks_prop_arr_sparse_u8(name, check_enabled) ks_prop_arr_sparse(name, ks_value_u8(name), check_enabled)
#define ks_prop_arr_sparse_obj(name, type, check_enabled) ks_prop_arr_sparse(name, ks_value_obj(name, type), check_enabled)

#define ks_prop_arr_len_sparse(name, len, value, check_enabled) ks_prop_v(#name, ks_value_arr_len_sparse_fixed( len, name, value, check_enabled, false) )
#define ks_prop_arr_len_sparse_u64(name, len, check_enabled) ks_prop_arr_len(name, len, ks_value_u64(name), check_enabled)
#define ks_prop_arr_len_sparse_u32(name, len, check_enabled) ks_prop_arr_len(name, len, ks_value_u32(name), check_enabled)
#define ks_prop_arr_len_sparse_u16(name, len, check_enabled) ks_prop_arr_len(name, len, ks_value_u16(name), check_enabled)
#define ks_prop_arr_len_sparse_u8(name, len, check_enabled) ks_prop_arr_len(name, len, ks_value_u8(name), check_enabled)
#define ks_prop_arr_len_sparse_obj(name, type, len, check_enabled) ks_prop_arr_len(name, len, ks_value_obj(name, type), check_enabled)

#define ks_fp(prop) if(!ks_io_fixed_property(__KS_IO, __KS_IO_FUNCS, prop, __SERIALIZE)) return false

#define ks_fp_u64(name) ks_fp(ks_prop_u64(name))
#define ks_fp_u32(name) ks_fp(ks_prop_u32(name))
#define ks_fp_u16(name) ks_fp(ks_prop_u16(name))
#define ks_fp_u8(name) ks_fp(ks_prop_u8(name))
#define ks_fp_obj(name, type) ks_fp(ks_prop_obj(name, type))

#define ks_fp_arr_u64(name) ks_fp(ks_prop_arr_u64(name))
#define ks_fp_arr_u32(name) ks_fp(ks_prop_arr_u32(name))
#define ks_fp_arr_u16(name) ks_fp(ks_prop_arr_u16(name))
#define ks_fp_arr_u8(name) ks_fp(ks_prop_arr_u8(name))
#define ks_fp_arr_obj(name, type) ks_fp(ks_prop_arr_obj(name, type))
#define ks_fp_str(name) ks_fp(ks_prop_str(name))

#define ks_fp_arr_len_u64(name, len) ks_fp(ks_prop_arr_len_u64(name, len))
#define ks_fp_arr_len_u32(name, len) ks_fp(ks_prop_arr_len_u32(name, len))
#define ks_fp_arr_len_u16(name, len) ks_fp( ks_prop_arr_len_u16(name, len))
#define ks_fp_arr_len_u8(name, len) ks_fp(ks_prop_arr_len_u8(name, len))
#define ks_fp_arr_len_obj(name, type, len) ks_fp(ks_prop_arr_len_obj(name, type, len))
#define ks_fp_str_p(name) ks_fp(ks_prop_str_len(name, __SERIALIZE ? strlen(ks_elem_access(name)) : 0))
#define ks_fp_str_len(name, len) ks_fp(ks_prop_str_len(name, len))

#define ks_fp_arr_sparse_u64(name, check_enabled) ks_fp(ks_prop_arr_sparse_u64(name, check_enabled))
#define ks_fp_arr_sparse_u32(name, check_enabled) ks_fp(ks_prop_arr_sparse_u32(name, check_enabled))
#define ks_fp_arr_sparse_u16(name, check_enabled) ks_fp(ks_prop_arr_sparse_u16(name, check_enabled))
#define ks_fp_arr_sparse_u8(name, check_enabled) ks_fp(ks_prop_arr_sparse_u8(name, check_enabled))
#define ks_fp_arr_sparse_obj(name, type, check_enabled) ks_fp(ks_prop_arr_sparse_obj(name, type, check_enabled))

#define ks_fp_arr_len_sparse_u64(name, len, check_enabled) ks_fp(ks_prop_arr_len_sparse_u64(name, len, check_enabled))
#define ks_fp_arr_len_sparse_u32(name, len, check_enabled) ks_fp(ks_prop_arr_len_sparse_u32(name, len, check_enabled))
#define ks_fp_arr_len_sparse_u16(name, len, check_enabled) ks_fp( ks_prop_arr_len_sparse_u16(name, len, check_enabled))
#define ks_fp_arr_len_sparse_u8(name, len, check_enabled) ks_fp(ks_prop_arr_len_sparse_u8(name, len, check_enabled))
#define ks_fp_arr_len_sparse_obj(name, type, len, check_enabled) ks_fp(ks_prop_arr_len_sparse_obj(name, type, len, check_enabled))


#define ks_magic_number(num) ks_io_magic_number(__KS_IO, __KS_IO_FUNCS, num)
