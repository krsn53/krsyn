#pragma once

#include <stdint.h>
#include "./math.h"

void    ks_vector_init_base         (void** data,  u32 type_size, u32 *length, u32 *capacity);
void    ks_vector_reserve_base      (void** data, u32 type_size, u32 *capacity, u32 new_cap);
void    ks_vector_clear_base        (void**data, u32 type_size, u32* length);
void    ks_vector_push_range_base   (void** data, u32 type_size, u32* length, u32 *capacity, u32 obj_length, const void*objs);
void    ks_vector_push_base         (void** data, u32 type_size, u32* length, u32 *capacity, const void*obj);
void    ks_vector_pop_base          (void**data, u32 type_size, u32* length);
void    ks_vector_resize_base       (void**data, u32 type_size, u32*length, u32* capacity, u32 new_size);
void    ks_vector_insert_base       (void**data, u32 type_size, u32*length, u32* capacity, u32 index, const void *obj);
void    ks_vector_remove_base       (void**data, u32 type_size, u32*length, u32 index);

#define ks_vector_data(data)        (void**)&data, sizeof(*data)

#define ks_vector_init_var(data, length, capacity)                          ks_vector_init_base(ks_vector_data(data), &length, &capacity)
#define ks_vector_reserve_var(data, capacity, new_cap)                      ks_vector_reserve_base(ks_vector_data(data), &capacity, new_cap)
#define ks_vector_clear_var(data, length)                                   ks_vector_clear_base(ks_vector_data(data), &length)
#define ks_vector_push_var(data, length, capacity, var)                     ks_vector_push_base(ks_vector_data(data), &length, &capacity, &var)
#define ks_vector_push_range_var(data, length, capacity, var_length, var)   ks_vector_push_range_base(ks_vector_data(data), &length, &capacity, var_length, var)
#define ks_vector_pop_var(data, length)                                     ks_vector_pop(ks_vector_data(data), &length)
#define ks_vector_resize_var(data, length, capacity, new_size)              ks_vector_resize_base(ks_vector_data(data), &length, &capacity, new_size)
#define ks_vector_insert_var(data, length, capacity, index, obj)            ks_vector_insert_base(ks_vector_data(data), &length, &capacity, index, &obj)
#define ks_vector_remove_var(data, length, index)                           ks_vector_remove_base(ks_vector_data(data), &length, index)

#define ks_vector_init(stct)                                            ks_vector_init_var((stct)->data, (stct)->length, (stct)->capacity)
#define ks_vector_reserve(stct, new_cap)                                ks_vector_reserve_var((stct)->data, (stct)->capacity, new_cap)
#define ks_vector_clear(stct)                                           ks_vector_clear_var((stct)->data, (stct)->length)
#define ks_vector_push(stct, var)                                       ks_vector_push_var((stct)->data, (stct)->length, (stct)->capacity, var)
#define ks_vector_push_range(stct, var_length, var)                     ks_vector_push_range_var((stct)->data, (stct)->length, (stct)->capacity, var_length, var)
#define ks_vector_pop(stct)                                             ks_vector_pop_var((stct)->data, (stct)->length)
#define ks_vector_resize(stct, new_size)                                ks_vector_resize_var((stct)->data, (stct)->length, (stct)->capacity, new_size)
#define ks_vector_insert(stct, index, obj)                              ks_vector_insert_var((stct)->data, (stct)->length, (stct)->capacity, index, obj)
#define ks_vector_remove(stct, index)                                   ks_vector_remove_var((stct)->data, (stct)->length, index)
