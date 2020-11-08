#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "./string.h"

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
#define ks_fixed_props_len(len, ...)  { \
    ks_property __PROPS[len] = { \
        __VA_ARGS__ \
    }; \
    if(!ks_io_fixed_props(__KS_IO, __KS_IO_FUNCS, sizeof(__PROPS) / sizeof(ks_property), __PROPS, __SERIALIZE)) return false; \
}
#define ks_fixed_props2(...) ks_io_fixed_props_args(__KS_IO, __SERIALIZE, __KS_IO_FUNCS, __VA_ARGS__ , ks_type(ks_property){NULL, { 0 }});


#define ks_chunks_len(len, ...) { \
    ks_property __CHUNKS[len] = { \
        __VA_ARGS__ \
    }; \
    if(!ks_io_chunks(__KS_IO, __KS_IO_FUNCS, sizeof(__CHUNKS) / sizeof(ks_property), __CHUNKS, __SERIALIZE)) return false; \
}
#define ks_chunks2(...) ks_io_chunks_args(__KS_IO, __SERIALIZE, __KS_IO_FUNCS, __VA_ARGS__, ks_type(ks_property){NULL, { 0 }});

#define ks_magic_number(num) ks_io_magic_number(__KS_IO, __KS_IO_FUNCS, num, 0, __SERIALIZE);



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





bool ks_io_print_indent(ks_io* io,  char indent, bool serialize);

bool ks_io_print_endl(ks_io* io, bool serialize);




unsigned ks_io_value_text(ks_io* io, void* v, ks_value_type type, bool serialize);



unsigned ks_io_prop_text(ks_io* io, const char* str, const char* delims, bool serialize);

unsigned ks_io_fixed_text(ks_io* io, const char* str, bool serialize);

bool ks_io_fixed_props_args(ks_io* io, bool serialize, const ks_io_funcs* funcs, ...);

bool ks_io_fixed_props(ks_io* io,  const ks_io_funcs* funcs, unsigned num_props, ks_property* props, bool serialize);

 bool ks_io_chunks_args(ks_io* io, bool serialize, const ks_io_funcs* funcs,  ...);

bool ks_io_chunks(ks_io* io,  const ks_io_funcs* funcs, unsigned num_props, ks_property* props, bool serialize);

bool ks_io_magic_number(ks_io* io, const ks_io_funcs* funcs, void*data, unsigned offset, bool serialize);

bool ks_io_u(ks_io* io, const ks_io_funcs* funcs, void*data, unsigned offset, bool serialize);

bool ks_io_u8(ks_io* io, const ks_io_funcs* funcs,  void*data, unsigned offset, bool serialize);

bool ks_io_array(ks_io* io, const ks_io_funcs* funcs, void*data, unsigned offset, bool serialize);

bool ks_io_object(ks_io* io, const ks_io_funcs* funcs,  void*data, unsigned offset, bool serialize);
unsigned ks_io_prop_default(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize);

bool ks_io_value_default(ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type,  bool serialize);
bool ks_io_array_begin_default(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize);
bool ks_io_array_elem_default(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, bool serialize);
bool ks_io_object_begin_default(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj, bool serialize);
bool ks_io_array_end_default(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize);
bool ks_io_object_end_default(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize);


const ks_io_funcs default_io ;


unsigned ks_io_fixed_bin(ks_io* io, const char* str, bool serialize);
bool ks_io_value_bin(ks_io* io, unsigned length, char* c, bool serialize);
unsigned ks_io_prop_binary(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize);

bool ks_io_value_binary(ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type,  bool serialize);
bool ks_io_array_begin_binary(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize);
bool ks_io_array_elem_binary(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, bool serialize);
bool ks_io_object_begin_binary(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj, bool serialize);
bool ks_io_array_end_binary(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize);
bool ks_io_object_end_binary(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize);

const ks_io_funcs binary_io ;


