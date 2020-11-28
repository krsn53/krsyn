#include "../krsyn.h"
#include <stdio.h>

typedef struct Test{
    u8 u8;
    u16 u16;
    u32 u32;
    u64 u64;
    u8 arr[10];
    u32 len_arr;
    u16 *arr_len;
    char strarr[20];
    char* str_p;
    u32 len_str;
    char* str_len;

}Test;

static Test test ={
    .u8 = 1,
    .u16 = 2,
    .u32 = 3,
    .u64 = 4,
    .arr = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
    .len_arr = 3,
    .arr_len = (u16[]){15, 16, 17},
    .strarr = "test18",
    .str_p = "test19",
    .len_str = 6,
    .str_len = "test20",
};

bool Test_equals(const Test *t1, const Test *t2){
    bool ret = t1->u8 == t2->u8 &&
            t1->u16 == t2->u16 &&
            t1->u32 == t2->u32 &&
            t1->u64 == t2->u64 &&
            memcmp(t1->arr, t2->arr, sizeof (t1->arr)) == 0 &&
            t1->len_arr == t2->len_arr &&
            memcmp(t1->arr_len, t2->arr_len, sizeof (*t1->arr_len) * t1->len_arr) == 0 &&
            memcmp(t1->strarr, t2->strarr, sizeof (t1->strarr)) == 0 &&
            memcmp(t1->str_p, t2->str_p, strlen(t1->str_p)) == 0 &&
            t1->len_str == t2->len_str &&
            memcmp(t1->str_len, t2->str_len, t1->len_str*sizeof(char)) == 0;
    return ret;
}

ks_io_decl_custom_func(Test);

ks_io_begin_custom_func(Test)
    ks_fp_u8(u8);
    ks_fp_u16(u16);
    ks_fp_u32(u32);
    ks_fp_u64(u64);
    ks_fp_arr_u8(arr);
    ks_fp_u32(len_arr);
    ks_fp_arr_u16_len(arr_len, ks_access(len_arr));
    ks_fp_str(strarr);
    ks_fp_str_p(str_p);
    ks_fp_u32(len_str);
    ks_fp_str_len(str_len, ks_access(len_str));
ks_io_end_custom_func(Test)

int main ( void ){

    ks_io *io = ks_io_new();
    {
        ks_io_begin_serialize(io, clike, ks_prop_root(test, Test));
        printf("--- C like serialize test ---\n");
        printf("test2:\n%s", io->str->data);

        Test test2;

        printf("--- C like deserialize test ---\n");
        ks_io_begin_deserialize(io, clike, ks_prop_root(test2, Test));
        printf("result: test is equals test2 = %s\n", Test_equals(&test, &test2) ? "True" : "False");

        free(test2.arr_len);
        free(test2.str_len);
        free(test2.str_p);
    }

    {
        ks_io_begin_serialize(io, binary_little_endian, ks_prop_root(test, Test));
        printf("\n--- binary little endian serialize test ---\ntest3:\n");
        for(u32 i=0; i< io->str->length; i++){
            printf("0x%02x, ", io->str->data[i]);
        }
        printf("\n");

        Test test3;

        printf("--- binary little endian deserialize test ---\n");
        ks_io_begin_deserialize(io, binary_little_endian, ks_prop_root(test3, Test));
        printf("result: test is equals test3 = %s\n", Test_equals(&test, &test3) ? "True" : "False");

        free(test3.arr_len);
        free(test3.str_len);
        free(test3.str_p);
    }


    {
        ks_io_begin_serialize(io, binary_big_endian, ks_prop_root(test, Test));
        printf("\n--- binary big endian serialize test ---\ntest4:\n");
        for(u32 i=0; i< io->str->length; i++){
            printf("0x%02x, ", io->str->data[i]);
        }
        printf("\n");

        Test test4;

        printf("--- binary big endian deserialize test ---\n");
        ks_io_begin_deserialize(io, binary_big_endian, ks_prop_root(test4, Test));
        printf("result: test is equals test4 = %s\n", Test_equals(&test, &test4) ? "True" : "False");

        free(test4.arr_len);
        free(test4.str_len);
        free(test4.str_p);
    }
    ks_io_free(io);

    return 0;
}
