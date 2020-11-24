#include "io.h"
#include "math.h"
#include <stdarg.h>
#include <stdlib.h>


ks_io* ks_io_new(){
    ks_io* ret = malloc(sizeof(ks_io));
    ret->str = ks_string_new();
    ret->seek = 0;
    ret->indent = 0;

    return ret;
}

void ks_io_free(ks_io* io){
    ks_string_free(io->str);
    free(io);
}

void ks_io_reset(ks_io* io){
    ks_string_clear(io->str);
    io->seek =0;
}

inline bool ks_io_value(ks_io* io, const ks_io_funcs* funcs, ks_value value, uint32_t index, bool serialize){
    bool ret = false;

    switch (value.type) {
    case KS_VALUE_U8:
    case KS_VALUE_U16:
    case KS_VALUE_U32:
    case KS_VALUE_U64:
        ret = funcs->value(io, funcs, value, index);
        break;
    case KS_VALUE_ARRAY:{
        ks_array_data* array = value.ptr.arr;
        if(array->value.type == KS_VALUE_STRING_ELEM) {
            ret = ks_io_string(io, funcs, *array, index, serialize);
        }
        else {
            ret = ks_io_array(io, funcs, *array, index, serialize);
        }
        break;
    }
    case KS_VALUE_OBJECT:
        ret = ks_io_object(io, funcs, *(ks_object_data*)value.ptr.obj, index, serialize);
        break;
    default:
        break;
    }
    return ret;
}

bool ks_io_begin(ks_io* io, const ks_io_funcs* funcs, bool serialize, ks_property prop){
    // Redundancy for inline
    if(serialize){
        ks_string_clear(io->str);
        return ks_io_value(io, funcs, prop.value, 0, true);
    }
    else {
        io->seek = 0;
        return ks_io_value(io, funcs, prop.value, 0, false);
    }
}

bool ks_io_read_file(ks_io* io, const char* file){
    FILE* fp = fopen(file, "rb");
    if(!fp) return false;

    fseek(fp, 0, SEEK_END);
    uint32_t filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    ks_string_resize(io->str, filesize);
    uint32_t readsize =fread(io->str->data, 1, filesize, fp);

    fclose(fp);

    return readsize == filesize;
}
void ks_io_read_string_len(ks_io* io, uint32_t length, const char* data){
    ks_string_set_n(io->str, length, data);
}

void ks_io_read_string(ks_io* io, const char* data){
    ks_string_set_n(io->str, strlen(data), data);
}

inline ks_value ks_val_ptr(void* ptr, ks_value_type type) {
    return (ks_value){ type, {.data=ptr} };
}

inline ks_property ks_prop_v(void* name, ks_value value) {
    return (ks_property){name, value};
}

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

inline char* ks_char_array_fill(char v, uint32_t length, char c[]){
    for(uint32_t i=0; i<length-1; i++){
        c[i] = v;
    }
    c[length-1] = 0;
    return c;
}

inline uint32_t ks_io_value_text(ks_io* io, ks_value_ptr v, ks_value_type type, uint32_t offset,  bool serialize){
    if(!serialize){
        uint32_t first = ks_string_first_not_of(io->str, io->seek, "\t\n ");

        uint64_t p;
        uint32_t read;

        int ret = sscanf(io->str->data + io->seek + first,"%ld", &p);
        read  = ks_string_first_not_of(io->str, io->seek + first, "0123456789");
        if((ret != 1 && io->seek + first + read < io->str->length) || io->seek + first + read >= io->str->length) {
            ks_error("Failed to read value");
            return 0;
        }

        switch (type) {
        case KS_VALUE_U64:
        {
            uint64_t *u = v.u64;
            u+= offset;
            *u=p;
            break;
        }
        case KS_VALUE_U32:
        {
            uint32_t *u = v.u32;
            u+= offset;
            *u=p;
            break;
        }
        case KS_VALUE_U16:
        {
            uint16_t *u = v.u16;
            u+= offset;
            *u=p;
            break;
        }
        case KS_VALUE_U8:
        {
            uint8_t* u = v.u8;
            u+= offset;
            *u = p;
            break;
        }
        default:
            break;
        }

        io->seek += first + read;
        return  first + read;
    } else {
        uint32_t len = 10;
        char s[len];

        int p;

        switch (type) {
        case KS_VALUE_U64:
        {
            uint64_t *u = v.u64;
            u+= offset;
            p = *u;
            break;
        }
        case KS_VALUE_MAGIC_NUMBER:
        case KS_VALUE_U32:
        {
            uint32_t *u = v.u32;
            u+= offset;
            p = *u;
            break;
        }
        case KS_VALUE_U16:
        {
            uint16_t *u = v.u16;
            u+= offset;
            p = *u;
            break;
        }
        case KS_VALUE_U8:
        {
            uint8_t* u = v.u8;
            u+= offset;
            p = *u;
            break;
        }
        default:
            p=0;
            break;
        }

        snprintf(s, len, "%d", p);
        ks_string_add(io->str, s);

    }
    return 1;
}


inline uint32_t ks_io_prop_text(ks_io* io, const char* str, const char* delims, bool serialize){
    if(!serialize){
        uint32_t first = ks_string_first_not_of(io->str, io->seek, "\t\n ");
        uint32_t prop_length = ks_string_first_of(io->str, io->seek + first, delims);

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

inline uint32_t ks_io_text_len(ks_io* io, uint32_t length, const char* str, bool serialize){
    if(!serialize){
        uint32_t first = ks_string_first_not_of(io->str, io->seek, "\t\n ");
        bool ret = strncmp(io->str->data + io->seek + first, str, length) == 0;
        if(!ret){
            return false;
        }

        io->seek += first + length;
        return first + length;
    }
    else {
        ks_string_add(io->str, str == NULL ? "" : str);

    }
    return 1;
}

inline uint32_t ks_io_text(ks_io* io, const char* str, bool serialize){
    return ks_io_text_len(io, strlen(str), str, serialize);
}

inline uint32_t ks_io_fixed_text(ks_io* io, const char* str, bool serialize){
    if(!ks_io_text(io, str, serialize)){
        ks_error("Unexcepted syntax, excepted \"%s\"", str);
        return false;
    }
    return true;
}

inline bool ks_io_fixed_property(ks_io* io, const ks_io_funcs* funcs,  ks_property prop, bool serialize){
    uint32_t prop_length = funcs->key(io, funcs, prop.name,  true);
    if(prop_length == 0){
        ks_error("Property \"%s\" not found", prop.name);
        return false;
    }
    if(!ks_io_value(io, funcs, prop.value, 0, serialize)) return false;
    return true;
}

inline bool ks_io_magic_number(ks_io* io, const ks_io_funcs* funcs, const char* data){
    ks_value  val={.ptr={.str = data}, .type = KS_VALUE_MAGIC_NUMBER};
    return funcs->value(io, funcs, val, 0);
}

inline bool ks_io_array_begin(ks_io* io, const ks_io_funcs* funcs, ks_array_data* array, uint32_t offset, bool serialize){

    if(!serialize && !array->fixed_length){
        void* ptr = array->value.ptr.data;
        if(array->value.type == KS_VALUE_OBJECT){
            ks_object_data *obj = ptr;
            ks_object_data* object = malloc(sizeof(ks_object_data));
            ptr = obj->data;
            ptr += sizeof(void*)*offset;
            *object = *obj;
            object->data = *(void**)ptr = array->length == 0 ? NULL : calloc(array->length, array->elem_size);
            array->value.ptr.obj = object;

        } else if(array->value.type == KS_VALUE_ARRAY){
            // TODO
        } else {
            array->value.ptr.data = *(void**)(ptr +  sizeof(void*)*offset) = array->length == 0 ? NULL : calloc(array->length, array->elem_size);
        }
    }

    if(serialize && !array->fixed_length){
        void* ptr = array->value.ptr.data;
        if(array->value.type == KS_VALUE_OBJECT){
            ks_object_data *obj = ptr;
            ks_object_data* object = malloc(sizeof(ks_object_data));
            *object = *obj;
            object->data = *(void**)obj->data +  offset;
            array->value.ptr.obj = object;
        } else if(array->value.type == KS_VALUE_ARRAY){
            // TODO
        } else {
            array->value.ptr.data = *(array->value.ptr.vpp +  offset);
        }
    }


    if(! funcs->array_begin(io, funcs, array)) return false;
    io->indent++;

    return true;
}

inline bool ks_io_string(ks_io* io, const ks_io_funcs* funcs, ks_array_data array, uint32_t offset, bool serialize){
    ks_string * str = ks_string_new();
    if(serialize){
        if(array.length == KS_STRING_UNKNOWN_LENGTH){
            void * ptr =!array.fixed_length  ? *array.value.ptr.vpp: array.value.ptr.data;
            if(ptr != NULL ){
                array.length = strlen(ptr) + 1;
            }
        }
        ks_string_set_n(str, array.length, !array.fixed_length  ? *array.value.ptr.vpp: array.value.ptr.data);
    }
    if(!funcs->string(io, funcs, array.length,  str)) return false;

    if(!serialize){
        if(!array.fixed_length){
            array.value.ptr.data = *(array.value.ptr.vpp + offset) = str->length == 0 ? NULL : calloc(str->length, array.elem_size);
        }
        else {
            memset(array.value.ptr.data, 0, array.length*array.elem_size);
        }

        memcpy(array.value.ptr.data, str->data, str->length);
    }
    ks_string_free(str);

    return true;
}

inline bool ks_io_array(ks_io* io, const ks_io_funcs* funcs, ks_array_data array, uint32_t offset, bool serialize){

    if(!ks_io_array_begin(io, funcs, &array, offset, serialize)) return false;

    for(uint32_t i=0, e=array.length; i< e; i++){
        if(! funcs->array_elem(io, funcs, &array, i)) return false;
    }

    return ks_io_array_end(io, funcs, &array, offset, serialize);
}

bool ks_io_array_end(ks_io* io, const ks_io_funcs* funcs, ks_array_data* array, uint32_t offset, bool serialize){
    if(((!serialize && !array->fixed_length) ||
        (serialize && !array->fixed_length)) &&
            array->value.type == KS_VALUE_OBJECT){
        free(array->value.ptr.obj);
    }

    io->indent--;
    if(! funcs->array_end(io, funcs, array)) return false;

    return true;
}

inline bool ks_io_object(ks_io* io, const ks_io_funcs* funcs, ks_object_data obj, uint32_t offset, bool serialize){

    if(! funcs->object_begin(io, funcs, &obj)) return false;
    io->indent++;
    if(! (serialize ? obj.serializer : obj.deserializer)(io, funcs, obj.data, offset)) return false;
    io->indent--;
    if(! funcs->object_end(io, funcs, &obj)) return false;

    return true;
}

inline bool ks_io_key_clike(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize){
    ks_io_print_indent(io, '\t', serialize);

    if(!serialize){
        io->seek += ks_string_first_not_of(io->str, io->seek, "\t\n ");
        if( io->seek >= io->str->length || io->str->data[io->seek] == '}'){
            return false;
        }
    }
    ks_io_fixed_text(io, ".", serialize );
    uint32_t ret =   ks_io_prop_text(io, name, "\t\n =", serialize) + ks_io_fixed_text(io, "=", serialize );

    if(!serialize && fixed && strncmp(name, io->str->data + io->seek - ret, strlen(name)) != 0){
        ks_error("excepted property \"%s\"", name);
        return false;
    }

    return ret != 0;
}

inline bool ks_io_value_clike(ks_io* io, const ks_io_funcs* funcs, ks_value value, uint32_t offset,  bool serialize){
    switch(value.type){
    case KS_VALUE_MAGIC_NUMBER:
        ks_io_print_indent(io,'\t', serialize);
        ks_io_fixed_text(io, "// Magic number : ", serialize);
        return ks_io_fixed_text(io, value.ptr.str, serialize) && ks_io_print_endl(io, serialize);
    default:
        break;
    }
    uint32_t ret = ks_io_value_text(io, value.ptr, value.type, offset, serialize);
    if(ret == 0) return false;
    ret += ks_io_fixed_text(io, ",", serialize);
    ks_io_print_endl(io, serialize);
    return ret != 0;
}

bool ks_io_string_clike(ks_io* io, const ks_io_funcs* funcs, uint32_t length,ks_string*  str,  bool serialize){
    if(!ks_io_fixed_text(io, "\"", serialize)) return false;

    if(serialize){
        length = MIN(length, strlen(str->data));
        // TODO: escape sequence"
        ks_io_text_len(io, length, str->data, serialize);
    }
    else {
        uint32_t l =ks_string_first_c_of(io->str, io->seek, '\"');
        if(length != 0){
            l = MIN(l, length);
        }
        ks_string_set_n( str, l, io->str->data + io->seek);
        io->seek += l;
    }
    return ks_io_fixed_text(io, "\"", serialize) &&  ks_io_fixed_text(io, ",", serialize) && ks_io_print_endl(io, serialize);
}

inline bool ks_io_array_begin_clike(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    if(!arr->fixed_length){
        ks_io_fixed_text(io, "(", serialize);
        switch (arr->value.type) {
        case KS_VALUE_U8:
            ks_io_fixed_text(io, "uint8_t", serialize);
            break;
        case KS_VALUE_U16:
            ks_io_fixed_text(io, "uint16_t", serialize);
            break;
        case KS_VALUE_U32:
            ks_io_fixed_text(io, "uint32_t", serialize);
            break;
        case KS_VALUE_U64:
            ks_io_fixed_text(io, "uint64_t", serialize);
            break;
        case KS_VALUE_OBJECT:{
            ks_object_data *data = arr->value.ptr.obj;
            ks_io_fixed_text(io, data->type , serialize);
            break;
        }
        default:
            // TODO ARRAY
            break;
        }
        ks_io_fixed_text(io, "[", serialize);
        ks_io_value_text(io, (ks_value_ptr)&arr->length, KS_VALUE_U32, 0, serialize);
        ks_io_fixed_text(io, "]", serialize);
        ks_io_fixed_text(io, ")", serialize);
    }
    return  ks_io_fixed_text(io, "{", serialize) && ks_io_print_endl(io, serialize);
}

inline bool ks_io_array_elem_clike(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, uint32_t index, bool serialize){
     ks_io_print_indent(io,'\t', serialize);
    if(!ks_io_value(io, funcs, arr->value, index, serialize)) return false;

    return true;
}
inline bool ks_io_object_begin_clike(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj, bool serialize){
    return ks_io_fixed_text(io, "{", serialize) && ks_io_print_endl(io, serialize);
}
inline bool ks_io_array_end_clike(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return ks_io_print_indent(io,'\t', serialize)&&
            ks_io_fixed_text(io, "}", serialize)  &&
            ks_io_fixed_text(io, ",", serialize) &&
            ks_io_print_endl(io, serialize);
}

inline bool ks_io_object_end_clike(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize){
    return  ks_io_print_indent(io,'\t', serialize)&&
            ks_io_fixed_text(io, "}", serialize)  &&
            (io->indent == 0 || ks_io_fixed_text(io, ",", serialize))  &&
            ks_io_print_endl(io, serialize);
}


static inline bool little_endian(){
    const union {
        char c[sizeof(int)];
        int i;
    } v = { .c={0x1, 0x0, 0x0, 0x0} };

    return v.i == 1;
}

union endian_checker{
    char c[sizeof(int)];
    int i;
};

inline uint32_t ks_io_fixed_bin_len(ks_io* io, uint32_t length, const char* str, bool serialize){
    if(!serialize){
        if(memcmp(io->str->data + io->seek, str, length) == 0){
            io->seek += length;
            return length;
        }
        return 0;
    }
    else {
        ks_string_add_n(io->str, length, str);
    }
    return 1;
}

inline uint32_t ks_io_fixed_bin(ks_io* io, const char* str, bool serialize){
    if(!serialize){
        return ks_io_fixed_bin_len(io, ks_string_first_c_of(io->str, io->seek, 0) + 1, str, serialize);
    }
    else {
        return ks_io_fixed_bin_len(io, strlen(str) + 1, str, serialize);
    }

}

inline bool ks_io_value_bin(ks_io* io, uint32_t length, char* c, bool swap_endian,  bool serialize){
    if(!serialize){
        if(io->seek + length > io->str->length) {
            ks_error("Unexcepted file end");
            return false;
        }
        if(!swap_endian){
            memcpy(c, io->str->data + io->seek, length);
        }
        else {
            for(int32_t i= 0; i < (int32_t)length; i++){
                c[i] = io->str->data[io->seek + length - 1 - i];
            }
        }
        io->seek += length;
    }
    else {
        if(!swap_endian){
            ks_string_add_n(io->str, length, c);
        }
        else {
            ks_string_reserve(io->str, io->str->length + length);
            for(int32_t i=length-1; i>= 0; i--){
                ks_string_add_c(io->str, c[i]);
            }
        }
    }
    return true;
}

inline bool ks_io_key_binary(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool swap_endian, bool serialize){
    if(fixed) {
        return true;
    }
    uint32_t step = ks_io_fixed_bin(io, name, serialize);
    return step != 0;
}

inline bool ks_io_string_binary(ks_io* io, const ks_io_funcs* funcs, uint32_t length, ks_string* str, bool swap_endian, bool serialize){
    if(!serialize){
        uint32_t l = length == KS_STRING_UNKNOWN_LENGTH ? ks_string_first_c_of(io->str, io->seek, 0)  : length;
        ks_string_set_n(str, l, io->str->data + io->seek);
        l = length == KS_STRING_UNKNOWN_LENGTH ? l+1 : length;
        io->seek += l; // for "\0"
    }
    else {
        ks_string_add_n(io->str, length, str->data);
    }
    return true;

}


inline bool ks_io_value_binary(ks_io* io, const ks_io_funcs* funcs, ks_value value,  uint32_t offset, bool swap_endian, bool serialize){
    switch (value.type) {
    case KS_VALUE_MAGIC_NUMBER:{
        if(!serialize){
            if(memcmp(value.ptr.str, io->str->data + io->seek, 4) != 0){
                char c[5] = { 0 };
                memcpy(c, io->str->data + io->seek, 4);
                ks_error("Excepted magic number \"%s\", detected \"%s\"", value.ptr.str, c);
               return false;
            }
            io->seek += 4;
        } else {
            ks_string_add_n(io->str, 4, value.ptr.str);
        }

        break;
    }
    case KS_VALUE_U64:{
        union {
            uint64_t* u;
            char(* c)[sizeof(uint64_t)];
        } v = {.u = value.ptr.u64 + offset};
        return ks_io_value_bin(io, sizeof(*v.c), *v.c, swap_endian, serialize);
    }
    case KS_VALUE_U32:{
        union {
            uint32_t* u;
            char(* c)[sizeof(uint32_t)];
        } v = {.u = value.ptr.u32 + offset};
        return ks_io_value_bin(io, sizeof(*v.c), *v.c, swap_endian, serialize);
    }
    case KS_VALUE_U16:{
        union {
            uint16_t* u;
            char(* c)[sizeof(uint16_t)];
        } v = {.u = value.ptr.u16 + offset};
        return ks_io_value_bin(io, sizeof(*v.c), *v.c, swap_endian, serialize);
    }
    case KS_VALUE_U8:{
        union {
            uint8_t* u;
            char(* c)[sizeof(uint8_t)];
        } v = {.u = value.ptr.u8 + offset};
        return ks_io_value_bin(io, sizeof(*v.c), *v.c, swap_endian, serialize);
    }
    default:
        return false;
    }
    return true;
}

inline bool ks_io_array_begin_binary(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool swap_endian, bool serialize){
    return true;
}

inline bool ks_io_array_elem_binary(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, uint32_t index, bool swap_endian, bool serialize){
    if(!ks_io_value(io, funcs, arr->value, index, serialize)) return false;
    return true;
}
inline bool ks_io_object_begin_binary(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj, bool swap_endian, bool serialize){
    return true;
}
inline bool ks_io_array_end_binary(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool swap_endian, bool serialize){
    return true;
}

inline bool ks_io_object_end_binary(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool swap_endian, bool serialize){
    return true;
}

inline bool ks_io_key_binary_little_endian(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize){
    return ks_io_key_binary(io, funcs, name, fixed, !little_endian(), serialize);
}
inline bool ks_io_value_binary_little_endian(ks_io* io, const ks_io_funcs* funcs, ks_value value, uint32_t offset,  bool serialize){
    return ks_io_value_binary(io, funcs, value, offset, !little_endian(), serialize);
}
inline bool ks_io_string_binary_little_endian(ks_io* io, const ks_io_funcs* funcs, uint32_t length, ks_string* str, bool serialize){
    return ks_io_string_binary(io, funcs, length, str, !little_endian(), serialize);
}
inline bool ks_io_array_begin_binary_little_endian(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return ks_io_array_begin_binary(io, funcs, arr, !little_endian(), serialize);
}
inline bool ks_io_array_elem_binary_little_endian(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, uint32_t index, bool serialize){
    return ks_io_array_elem_binary(io, funcs, arr, index, !little_endian(), serialize);
}
inline bool ks_io_array_end_binary_little_endian(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return ks_io_array_end_binary(io, funcs, arr, !little_endian(), serialize);
}
inline bool ks_io_object_begin_binary_little_endian(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj,  bool serialize){
    return ks_io_object_begin_binary(io, funcs, obj, !little_endian(), serialize);
}
inline bool ks_io_object_end_binary_little_endian(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize){
    return ks_io_object_end_binary(io, funcs, obj, !little_endian(), serialize);
}

inline bool ks_io_key_binary_big_endian(ks_io* io, const ks_io_funcs* funcs, const char* name, bool fixed, bool serialize){
    return ks_io_key_binary(io, funcs, name, fixed, little_endian(), serialize);
}
inline bool ks_io_value_binary_big_endian(ks_io* io, const ks_io_funcs* funcs, ks_value value, uint32_t offset,  bool serialize){
    return ks_io_value_binary(io, funcs, value, offset, little_endian(), serialize);
}
inline bool ks_io_string_binary_big_endian(ks_io* io, const ks_io_funcs* funcs, uint32_t length, ks_string*  str, bool serialize){
    return ks_io_string_binary(io, funcs, length, str, little_endian(), serialize);
}
inline bool ks_io_array_begin_binary_big_endian(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return ks_io_array_begin_binary(io, funcs, arr, little_endian(), serialize);
}
inline bool ks_io_array_elem_binary_big_endian(ks_io* io,  const ks_io_funcs* funcs, ks_array_data* arr, uint32_t index, bool serialize){
    return ks_io_array_elem_binary(io, funcs, arr, index, little_endian(), serialize);
}
inline bool ks_io_array_end_binary_big_endian(ks_io* io, const ks_io_funcs* funcs,  ks_array_data* arr, bool serialize){
    return ks_io_array_end_binary(io, funcs, arr, little_endian(), serialize);
}
inline bool ks_io_object_begin_binary_big_endian(ks_io* io,  const ks_io_funcs* funcs, ks_object_data* obj,  bool serialize){
    return ks_io_object_begin_binary(io, funcs, obj, little_endian(), serialize);
}
inline bool ks_io_object_end_binary_big_endian(ks_io* io, const ks_io_funcs* funcs,  ks_object_data* obj, bool serialize){
    return ks_io_object_end_binary(io, funcs, obj, little_endian(), serialize);
}


ks_io_funcs_impl(binary_little_endian)
ks_io_funcs_impl(binary_big_endian)
ks_io_funcs_impl(clike)
