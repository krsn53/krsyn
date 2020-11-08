#include "io.h"
#include "logger.h"
#include <stdarg.h>



inline bool ks_io_print_indent(ks_io* io,  char indent, bool serialize){
    if(!serialize) return true;
    char c[io->indent+1];
    ks_string_add(io->str, ks_char_array_fill(indent, io->indent+1, c));
    return true;
}

inline bool ks_io_print_endl(ks_io* io, bool serialize){
    if(!serialize) return true;
    ks_string_add(io->str, "\n");
    return true;
}

inline char* ks_char_array_fill(char v, unsigned length, char c[]){
    for(unsigned i=0; i<length-1; i++){
        c[i] = v;
    }
    c[length-1] = 0;
    return c;
}

inline unsigned ks_io_value_text(ks_io* io, void* v, ks_value_type type, bool serialize){
    if(!serialize){
        unsigned first = ks_string_first_not_of(io->str, io->seek, "\t\n ");
        int p;
        int ret = sscanf(io->str->ptr + io->seek + first, "%d", &p);
        unsigned read  = ks_string_first_not_of(io->str, io->seek + first, "0123456789");
        if((ret != 1 && io->seek + first + read < io->str->length) || io->seek + first + read >= io->str->length) {
            ks_error("Failed to read value");
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
        default:
            break;
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



inline unsigned ks_io_prop_text(ks_io* io, const char* str, const char* delims, bool serialize){
    if(!serialize){
        unsigned first = ks_string_first_not_of(io->str, io->seek, "\t\n ");
        unsigned prop_length = ks_string_first_of(io->str, io->seek + first, delims);

        if(prop_length == 0 && io->seek + first != io->str->length){
            ks_error("Failed to read property name");
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

inline unsigned ks_io_fixed_text(ks_io* io, const char* str, bool serialize){
    if(!serialize){
        unsigned first = ks_string_first_not_of(io->str, io->seek, "\t\n ");
        unsigned tex_length = strlen(str);
        bool ret = strncmp(io->str->ptr + io->seek + first, str, strlen(str)) == 0;
        if(!ret){
            ks_error("Unexcepted syntax, excepted \"%s\"", str);
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

inline bool ks_io_fixed_props_args(ks_io* io, bool serialize, const ks_io_funcs* funcs,  ...) {
    va_list va;
    va_start( va,  funcs );

    ks_property prop;

    for(prop = va_arg(va, ks_property); prop.name != NULL ;prop = va_arg(va, ks_property)){
        if(!ks_io_fixed_property(io, funcs, prop, serialize)){
            va_end(va);
            return false;
        }
    }

    va_end(va);

    return true;
}

inline bool ks_io_fixed_props(ks_io* io,  const ks_io_funcs* funcs, unsigned num_props, ks_property* props, bool serialize){
    for(unsigned i=0; i< num_props; i++){
       if(!ks_io_fixed_property(io, funcs, props[i], serialize)) return false;
    }
    return true;
}

inline bool ks_io_chunks_args(ks_io* io, bool serialize, const ks_io_funcs* funcs,  ...) {
    va_list va;

    ks_property prop;
    unsigned num_props = 0;

    va_start( va,  funcs );
    for(prop = va_arg(va, ks_property); prop.name != NULL ;prop = va_arg(va, ks_property)){
        num_props++;
    }
    va_end(va);

    ks_property props[num_props];

    va_start( va,  funcs );

    for(unsigned i=0; i< num_props; i++){
        props[i] = va_arg(va, ks_property);
    }
    va_end(va);

    return ks_io_chunks(io, funcs, num_props, props, serialize);
}

inline bool ks_io_chunks(ks_io* io,  const ks_io_funcs* funcs, unsigned num_props, ks_property* props, bool serialize){
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
        return ks_io_fixed_props(io, funcs, num_props, props, serialize);
    }

    return true;
}

inline bool ks_io_magic_number(ks_io* io, const ks_io_funcs* funcs, void*data, unsigned offset, bool serialize){
    return funcs->value(io, funcs, data, KS_VALUE_MAGIC_NUMBER, serialize);
}

inline bool ks_io_u(ks_io* io, const ks_io_funcs* funcs, void*data, unsigned offset, bool serialize){
    return funcs->value(io, funcs, (unsigned*)data + offset, KS_VALUE_U, serialize);
}

inline bool ks_io_u8(ks_io* io, const ks_io_funcs* funcs,  void*data, unsigned offset, bool serialize){
    return funcs->value(io, funcs, (uint8_t*)data + offset, KS_VALUE_U8, serialize);
}

inline bool ks_io_array(ks_io* io, const ks_io_funcs* funcs, void*data, unsigned offset, bool serialize){
    ks_array_data array = *(ks_array_data*)data;

    if(! funcs->array_begin(io, funcs, &array, serialize)) return false;
    io->indent++;
    if(! funcs->array_elem(io, funcs, &array, serialize)) return false;
    io->indent--;
    if(! funcs->array_end(io, funcs, &array, serialize)) return false;

    return true;
}

inline bool ks_io_object(ks_io* io, const ks_io_funcs* funcs,  void*data, unsigned offset, bool serialize){
    ks_object_data obj = *(ks_object_data*)data;

    if(! funcs->object_begin(io, funcs, &obj, serialize)) return false;
    io->indent++;
    if(! obj.value.func(io, funcs, obj.value.data, offset, serialize)) return false;
    io->indent--;
    if(! funcs->object_end(io, funcs, &obj, serialize)) return false;

    return true;
}

inline unsigned ks_io_prop_default(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize){
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
        ks_error("excepted property \"%s\"", name);
        return 0;
    }

    return ret;
}

inline bool ks_io_value_default(ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type,  bool serialize){
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

inline bool ks_io_array_begin_default(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return  ks_io_fixed_text(io, "{", serialize) && ks_io_print_endl(io, serialize);
}

inline bool ks_io_array_elem_default(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, bool serialize){
    for(unsigned i=0; i< arr->length; i++){
        ks_io_print_indent(io,'\t', serialize);
        if(!arr->value.func(io, funcs, arr->value.data, i, serialize)) return false;
    }
    return true;
}
inline bool ks_io_object_begin_default(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj, bool serialize){
    return ks_io_fixed_text(io, "{", serialize) && ks_io_print_endl(io, serialize);
}
inline bool ks_io_array_end_default(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return ks_io_fixed_text(io, "}", serialize)  &&  ks_io_fixed_text(io, ",", serialize) && ks_io_print_endl(io, serialize);
}

inline bool ks_io_object_end_default(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize){
    return  ks_io_print_indent(io,'\t', serialize)&& ks_io_fixed_text(io, "}", serialize)  &&  ks_io_fixed_text(io, ",", serialize)  && ks_io_print_endl(io, serialize);
}



const ks_io_funcs default_io ={
    .value = ks_io_value_default,
    .array_begin = ks_io_array_begin_default,
    .array_elem = ks_io_array_elem_default,
    .array_end = ks_io_array_end_default,
    .object_begin = ks_io_object_begin_default,
    .object_end = ks_io_object_end_default,
    .prop = ks_io_prop_default,
};


inline unsigned ks_io_fixed_bin(ks_io* io, const char* str, bool serialize){
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

inline bool ks_io_value_bin(ks_io* io, unsigned length, char* c, bool serialize){
    if(!serialize){
        if(io->seek + length > io->str->length) {
            ks_error("Unexcepted file end");
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

inline unsigned ks_io_prop_binary(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize){
    if(fixed) {
        return true;
    }
    unsigned step = ks_io_fixed_bin(io, name, serialize);
    return step;
}

inline bool ks_io_value_binary(ks_io* io, const ks_io_funcs* funcs, void* u, ks_value_type type,  bool serialize){
    switch (type) {
    case KS_VALUE_MAGIC_NUMBER:{
        if(!serialize){
            if(strncmp(u, io->str->ptr + io->seek, 4) != 0){
                char c[5] = { 0 };
                strncpy(c, io->str->ptr + io->seek, 4);
                ks_error("Excepted magic number \"%s\", detected \"%s\"", u, c);
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

inline bool ks_io_array_begin_binary(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return true;
}

inline bool ks_io_array_elem_binary(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, bool serialize){
    for(unsigned i=0; i< arr->length; i++){
        if(!arr->value.func(io, funcs, arr->value.data, i, serialize)) return false;
    }
    return true;
}
inline bool ks_io_object_begin_binary(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj, bool serialize){
    return true;
}
inline bool ks_io_array_end_binary(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return true;
}

inline bool ks_io_object_end_binary(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize){
    return true;
}

const ks_io_funcs binary_io ={
    .value = ks_io_value_binary,
    .array_begin = ks_io_array_begin_binary,
    .array_elem = ks_io_array_elem_binary,
    .array_end = ks_io_array_end_binary,
    .object_begin = ks_io_object_begin_binary,
    .object_end = ks_io_object_end_binary,
    .prop = ks_io_prop_binary,
};


