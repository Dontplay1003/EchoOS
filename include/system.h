#ifndef __SYSTEM_H_
#define __SYSTEM_H_


//// 在全局表中设置任务状态段/局部表描述符。
// 参数：n - 在全局表中描述符项n 所对应的地址；addr - 状态段/局部表所在内存的基地址。
// tp - 描述符中的标志类型字节。
// %0 - eax(地址addr)；%1 - (描述符项n 的地址)；%2 - (描述符项n 的地址偏移2 处)；
// %3 - (描述符项n 的地址偏移4 处)；%4 - (描述符项n 的地址偏移5 处)；
// %5 - (描述符项n 的地址偏移6 处)；%6 - (描述符项n 的地址偏移7 处)；
extern _inline void _set_tssldt_desc(unsigned short *n,unsigned long addr,char tp) 
{ 
	*n = 104;	// 描述符长度。
	*(n + 1) = addr;	// 段基地址。
	*(n + 2) = addr >> 16;	// 段基地址。
	*((char*)n + 7) = *((char*)n + 5);	// 描述符标志字节。
	*((char*)n + 5) = tp;	// 描述符标志字节。
	*((char*)n + 6) = 0;	// 描述符标志字节。
}

//// 在全局表中设置任务状态段描述符。
// n - 是该描述符的指针；addr - 是描述符中的基地址值。任务状态段描述符的类型是0x89。
#define set_tss_desc(n,addr) \
_set_tssldt_desc((unsigned short*)(n),(unsigned long)(addr), (char)0x89)
//// 在全局表中设置局部表描述符。
// n - 是该描述符的指针；addr - 是描述符中的基地址值。局部表描述符的类型是0x82。
#define set_ldt_desc(n,addr) \
_set_tssldt_desc((unsigned short*)(n),(unsigned long)(addr), (char)0x82)

#endif