#include "vector.h"
#include <stdlib.h>
#include <memory.h>

inline void ks_vector_init_base(void** data, uint32_t type_size){
    *data = calloc(1, type_size);
}

inline void ks_vector_reserve_base(void** data, uint32_t type_size,  uint32_t *capacity, uint32_t new_cap){
    if(new_cap > *capacity){
        *data = realloc(*data, new_cap*type_size);
        memset(*data + *capacity, 0, new_cap-*capacity);
        *capacity = new_cap;
    }
}

inline void  ks_vector_clear_base(void**data, uint32_t type_size, uint32_t* length){
    memset(*data, 0, *length*type_size);
    *length = 0;
}

inline void ks_vector_push_range_base(void** data, uint32_t type_size, uint32_t* length, uint32_t *capacity, uint32_t obj_length, const void*objs){
    uint32_t out_len = *length +obj_length;
    uint32_t cap = *capacity;
    while(out_len > cap){
       cap*=2;
    }
    ks_vector_reserve_base(data, type_size, capacity, cap);
    memcpy(*data + *length * type_size , objs, type_size * obj_length);
    *length += obj_length;
}

inline void ks_vector_push_base(void** data, uint32_t type_size, uint32_t* length, uint32_t *capacity, const void*obj){
    if(*length +1 > *capacity){
        ks_vector_reserve_base(data, type_size, capacity, *capacity * 2);
    }
    memcpy(*data + *length * type_size , obj, type_size);
    ++ *length;
}

inline void ks_vector_pop_base(void**data, uint32_t type_size, uint32_t* length){
    if(*length == 0) return;
    -- *length;
    memset(*data + *length*type_size, 0, type_size);
}

inline void ks_vector_resize_base(void**data, uint32_t type_size, uint32_t*length, uint32_t* capacity, uint32_t new_size){
    ks_vector_reserve_base(data, type_size, capacity, new_size);
    *length = new_size;
}
