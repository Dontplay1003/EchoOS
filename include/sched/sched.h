#ifndef __SCHED_H_
#define __SCHED_H_

#define NR_TASKS 64		// 系统中同时最多任务（进程）数。
#define HZ 100			// 定义系统时钟滴答频率(1 百赫兹，每个滴答10ms)

#define FIRST_TASK task[0]	// 任务0 比较特殊，所以特意给它单独定义一个符号。
#define LAST_TASK task[NR_TASKS-1]	// 任务数组中的最后一项任务。

#include "head.h"		// head 头文件，定义了段描述符的简单结构，和几个选择符常量。
//#include <linux/fs.h>		// 文件系统头文件。定义文件表结构（file,buffer_head,m_inode 等）。
#include "mm/mm.h"		// 内存管理头文件。含有页面大小定义和一些页面释放函数原型。
#include "sched/signal.h"		// 信号头文件。定义信号符号常量，信号结构以及信号操作函数原型。

// 这里定义了进程运行可能处的状态。
#define TASK_RUNNING 0		// 进程正在运行或已准备就绪。
#define TASK_INTERRUPTIBLE 1	// 进程处于可中断等待状态。
#define TASK_UNINTERRUPTIBLE 2	// 进程处于不可中断等待状态，主要用于I/O 操作等待。
#define TASK_ZOMBIE 3		// 进程处于僵死状态，已经停止运行，但父进程还没发信号。
#define TASK_STOPPED 4		// 进程已停止。

#ifndef NULL
#define NULL 0	// 定义NULL 为空指针。
#endif

typedef int (*fn_ptr) ();	// 定义函数指针类型。

#define FPU_REG_WIDTH		256

#define FPU_ALIGN		__attribute__((aligned(32)))

union fpureg {
	unsigned int	val32[FPU_REG_WIDTH / 32];
	unsigned long long	val64[FPU_REG_WIDTH / 64];
};

#define NUM_FPU_REGS 32

struct loongarch_fpu {
	unsigned int	fcsr;
	unsigned int	vcsr;
	unsigned long int	fcc;	/* 8x8 */
	union fpureg fpr[NUM_FPU_REGS];
};

// 任务状态段数据结构（参见列表后的信息）。
/*
 * If you change thread_struct remember to change the #defines below too!
 */
struct thread_struct {
	/* Saved main processor registers. */
	unsigned long reg01, reg02, reg03, reg22; /* ra tp sp fp */
	unsigned long reg23, reg24, reg25, reg26; /* s0-s3 */
	unsigned long reg27, reg28, reg29, reg30, reg31; /* s4-s8 */

	/* Saved csr registers */
	unsigned long csr_prmd;
	unsigned long csr_crmd;
	unsigned long csr_euen;
	unsigned long csr_ecfg;
	unsigned long csr_badvaddr;	/* Last user fault */

	/* Saved scratch registers */
	unsigned long scr0;
	unsigned long scr1;
	unsigned long scr2;
	unsigned long scr3;

	/* Saved eflags register */
	unsigned long eflags;

	/* Other stuff associated with the thread. */
	unsigned long trap_nr;
	unsigned long error_code;
	//struct loongarch_vdso_info *vdso;

	/*
	 * Saved fpu register stuff, must be at last because
	 * it is conditionally copied at fork.
	 */
	struct loongarch_fpu fpu FPU_ALIGN;
};

#define INIT_THREAD  {						\
	/*							\
	 * Saved main processor registers			\
	 */							\
	.reg01			= 0,				\
	.reg02			= 0,				\
	.reg03			= 0,				\
	.reg22			= 0,				\
	.reg23			= 0,				\
	.reg24			= 0,				\
	.reg25			= 0,				\
	.reg26			= 0,				\
	.reg27			= 0,				\
	.reg28			= 0,				\
	.reg29			= 0,				\
	.reg30			= 0,				\
	.reg31			= 0,				\
	.csr_crmd		= 0,				\
	.csr_prmd		= 0,				\
	.csr_euen		= 0,				\
	.csr_ecfg		= 0,				\
	.csr_badvaddr		= 0,				\
	/*							\
	 * Other stuff associated with the process		\
	 */							\
	.trap_nr		= 0,				\
	.error_code		= 0,				\
	/*							\
	 * Saved fpu register stuff				\
	 */							\
	.fpu			= {				\
		.fcsr		= 0,				\
		.vcsr		= 0,				\
		.fcc		= 0,				\
		.fpr		= {{{0,},},},			\
	},							\
}
/*
// 这里是任务（进程）数据结构，或称为进程描述符。
// ==========================
// long state 任务的运行状态（-1 不可运行，0 可运行(就绪)，>0 已停止）。
// long counter 任务运行时间计数(递减)（滴答数），运行时间片。
// long priority 运行优先数。任务开始运行时counter = priority，越大运行越长。
// long signal 信号。是位图，每个比特位代表一种信号，信号值=位偏移值+1。
// struct sigaction sigaction[32] 信号执行属性结构，对应信号将要执行的操作和标志信息。
// long blocked 进程信号屏蔽码（对应信号位图）。
// --------------------------
// int exit_code 任务执行停止的退出码，其父进程会取。
// unsigned long start_code 代码段地址。
// unsigned long end_code 代码长度（字节数）。
// unsigned long end_data 代码长度 + 数据长度（字节数）。
// unsigned long brk 总长度（字节数）。
// unsigned long start_stack 堆栈段地址。
// long pid 进程标识号(进程号)。
// long father 父进程号。
// long pgrp 父进程组号。
// long session 会话号。
// long leader 会话首领。
// unsigned short uid 用户标识号（用户id）。
// unsigned short euid 有效用户id。
// unsigned short suid 保存的用户id。
// unsigned short gid 组标识号（组id）。
// unsigned short egid 有效组id。
// unsigned short sgid 保存的组id。
// long alarm 报警定时值（滴答数）。
// long utime 用户态运行时间（滴答数）。
// long stime 系统态运行时间（滴答数）。
// long cutime 子进程用户态运行时间。
// long cstime 子进程系统态运行时间。
// long start_time 进程开始运行时刻。
// unsigned short used_math 标志：是否使用了协处理器。
// --------------------------
// int tty 进程使用tty 的子设备号。-1 表示没有使用。
// unsigned short umask 文件创建属性屏蔽位。
// struct m_inode * pwd 当前工作目录i 节点结构。
// struct m_inode * root 根目录i 节点结构。
// struct m_inode * executable 执行文件i 节点结构。
// unsigned long close_on_exec 执行时关闭文件句柄位图标志。（参见include/fcntl.h）
// --------------------------
// struct desc_struct ldt[3] 本任务的局部表描述符。0-空，1-代码段cs，2-数据和堆栈段ds&ss。
// --------------------------
// struct tss_struct tss 本进程的任务状态段信息结构。
// ==========================
*/

// struct thread_info {
// 	struct task_struct	*task;		/* main task structure */
// 	unsigned long		flags;		/* low level flags */
// 	unsigned long		tp_value;	/* thread pointer */
// 	__u32			cpu;		/* current CPU */
// 	int			preempt_count;	/* 0 => preemptible, <0 => BUG */
// 	struct pt_regs		*regs;
// 	unsigned long		syscall;	/* syscall number */
// 	unsigned long		syscall_work;	/* SYSCALL_WORK_ flags */
// };

struct task_struct
{
/* these are hardcoded - don't touch */
	long state;			/* -1 unrunnable, 0 runnable, >0 stopped */
	long counter;
	long priority;
	long signal; /* bitmap of pending signals */
	struct sigaction sigaction[32]; /* 32 is _NSIG_WORDS */
	long blocked;			/* bitmap of masked signals */
/* various fields */
	int exit_code;
	unsigned long start_code, end_code, end_data, brk, start_stack;
	long pid, father, pgrp, session, leader;
	unsigned short uid, euid, suid;
	unsigned short gid, egid, sgid;
	long utime, stime, cutime, cstime, start_time;

/* ldt for this task 0 - zero 1 - cs 2 - ds&ss */
	struct desc_struct ldt[3];
/* tss for this task */
	struct thread_struct tss;
};

#define INIT_TASK \
{\
/* state etc */0,15,15, \
/* signals */0, {{0},}, 0,\
/* ec,brk... */0, 0, 0, 0, 0, 0,\
/* pid etc.. */ 0, -1, 0, 0, 0, \
/* uid etc */ 0, 0, 0, 0, 0, 0, \
/* time */ 0, 0, 0, 0, 0, \
/* ldt[3]*/	{{0, 0}, \
	{0x9f, 0xc0fa00}, /* 代码长640K，基址0x0，G=1，D=1，DPL=3，P=1 TYPE=0x0a*/  \
	{ 0x9f, 0xc0f200},}, /* 数据长640K，基址0x0，G=1，D=1，DPL=3，P=1 TYPE=0x02*/   \
/*tss*/ INIT_THREAD \
}

extern struct task_struct *task[NR_TASKS];	// 任务数组。
extern struct task_struct *current;	// 当前进程结构指针变量。
extern long volatile jiffies;	// 从开机开始算起的滴答数（10ms/滴答）。
extern long startup_time;	// 开机时间。从1970:0:0:0 开始计时的秒数。

#define CURRENT_TIME (startup_time+jiffies/HZ)	// 当前时间（秒数）。

/*
* 寻找第1 个TSS 在全局表中的入口。0-没有用nul，1-代码段cs，2-数据段ds，3-系统段syscall
* 4-任务状态段TSS0，5-局部表LTD0，6-任务状态段TSS1，等。
*/
// 全局表中第1 个任务状态段(TSS)描述符的选择符索引号。
#define FIRST_TSS_ENTRY 4
// 全局表中第1 个局部描述符表(LDT)描述符的选择符索引号。
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)
// 宏定义，计算在全局表中第n 个任务的TSS 描述符的索引号（选择符）。
#define _TSS(n) ((((unsigned long) n)<<4)+(FIRST_TSS_ENTRY<<3))
// 宏定义，计算在全局表中第n 个任务的LDT 描述符的索引号。
#define _LDT(n) ((((unsigned long) n)<<4)+(FIRST_LDT_ENTRY<<3))

// 不可中断的等待睡眠。( kernel/sched.c, 151 )
extern void sleep_on (struct task_struct **p);
// 可中断的等待睡眠。( kernel/sched.c, 167 )
extern void interruptible_sleep_on (struct task_struct **p);
// 明确唤醒睡眠的进程。( kernel/sched.c, 188 )
extern void wake_up (struct task_struct **p);

// 复制进程的页目录页表。Linus 认为这是内核中最复杂的函数之一。( mm/memory.c, 105 )
extern int copy_page_tables (unsigned long from, unsigned long to, long size);
// 释放页表所指定的内存块及页表本身。( mm/memory.c, 150 )
extern int free_page_tables (unsigned long from, unsigned long size);

// 调度程序的初始化函数。( kernel/sched.c, 385 )
extern void sched_init (void);
// 进程调度函数。( kernel/sched.c, 104 )
extern void schedule (void);
// 异常(陷阱)中断处理初始化函数，设置中断调用门并允许中断请求信号。( kernel/traps.c, 181 )
extern void trap_init (void);
// 显示内核出错信息，然后进入死循环。( kernel/panic.c, 16 )。
extern void panic (const char *str);
// 往tty 上写指定长度的字符串。( kernel/chr_drv/tty_io.c, 290 )。
extern int tty_write (unsigned minor, char *buf, int count);

//extern _inline void switch_to(int n) 
void switch_to(int n) 
{
	__switch_to();
}

#endif