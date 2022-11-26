#ifndef _ECHO_UTILS_MEM_
#define _ECHO_UTILS_MEM_

void memset(void *ptr, char c, unsigned long size);
int memcmp(void *ptr1, void *ptr2, unsigned long size);
void memcpy(void *ptr1, void *ptr2, unsigned long size);

void print_memmap();

#endif /* !_ECHO_UTILS_MEM_ */