#ifndef __FORK_H_
#define __FORK_H_


void verify_area (void *addr, int size);
int copy_mem (int nr, struct task_struct *p);
int copy_process (int nr, long ebp, long edi, long esi, long gs, long none,
				  long ebx, long ecx, long edx,
				  long fs, long es, long ds,
				  long eip, long cs, long eflags, long esp, long ss);
int find_empty_process (void);
int fork(void);


#endif