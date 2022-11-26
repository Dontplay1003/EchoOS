#include "utils/mem.h"
#include "boot/boot_param.h"

void memset(void *ptr, char c, unsigned long size)
{
    char *p = (char*)ptr;
    while (size--)
    {
        *p = c;
        p++;
    }
}

// void *memset(void *s, int c, unsigned int n){
//     char *xs = (char *)s;
//     while (n--)
//         *xs++ = c;
//     return s;
// }

int memcmp(void *ptr1, void *ptr2, unsigned long size)
{
    char *p1 = (char*)ptr1;
    char *p2 = (char*)ptr2;
    while (size--)
    {
        if (*p1 != *p2)
            return 0;
        p1++;
        p2++;
    }
    return 1;
}

struct loongsonlist_mem_map mem_map_list_temp = {
    {0, 0, 0, 0},
    .map_count = 15,
    .map = {
        {(u32)1, (u64)0x00200000, (u64)0x0ee00000},
        {(u32)2, (u64)0x0f000000, (u64)0x01000000},
        {(u32)1, (u64)0x00000000, (u64)0x10000000},
        {(u32)1, (u64)0x90000000, (u64)0xf0000000},
        {(u32)1, (u64)0x00000000, (u64)0x10000000},
        {(u32)1, (u64)0x90010000, (u64)0x6e6c0000},
        {(u32)1, (u64)0xfe6e0000, (u64)0x00010000},
        {(u32)1, (u64)0xfe700000, (u64)0x00190000},
        {(u32)1, (u64)0xfef10000, (u64)0x01080000},
        {(u32)1, (u64)0xfffe0000, (u64)0x80020000},
        {(u32)1, (u64)0x100d0000, (u64)0x00001000},
        {(u32)1, (u64)0xfe6f0000, (u64)0x00010000},
        {(u32)1, (u64)0xfe890000, (u64)0x002c0000},
        {(u32)1, (u64)0xfeb60000, (u64)0x003b0000},
        {(u32)1, (u64)0xfff90000, (u64)0x00050000}}};

void memcpy(void *ptr1, void *ptr2, unsigned long size)
{
    char *p1 = (char *)ptr1;
    char *p2 = (char *)ptr2;
    while (size--)
    {
        *p1 = *p2;
        p1++;
        p2++;
    }
}

void print_memmap(){
    printf("map_count : %d\n",mem_map_list_temp.map_count);
    for(int i=0;i<mem_map_list_temp.map_count;i++){
        printf("\tmem_type:%d ; mem_start:%d ; mem_size:%d\n",mem_map_list_temp.map[i].mem_type,
                mem_map_list_temp.map[i].mem_start,mem_map_list_temp.map[i].mem_size);
    }
}