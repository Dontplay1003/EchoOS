/*
 * linux/kernel/sched.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * 'sched.c'是主要的内核文件。其中包括有关调度的基本函数(sleep_on、wakeup、schedule 等)以及
 * 一些简单的系统调用函数（比如getpid()，仅从当前任务中获取一个字段）。
 */
#include "sched/sched.h"	// 调度程序头文件。定义了任务结构task_struct、第1 个初始任务
#include "sched/signal.h"		// 信号头文件。定义信号符号常量，sigaction 结构，操作函数原型。
#include "head.h"		// 内核头文件。含有一些内核常用函数的原形定义。
#include "system.h"		// 系统头文件。定义了设置或修改描述符/中断门等的嵌入式汇编宏。
#include "mm/mm.h"		// 内存管理头文件。定义了页面大小和页面掩码等。
#include <larchintrin.h>
#include "arch/loongarch.h"

#define _S(nr) (1<<((nr)-1))	// 取信号nr 在信号位图中对应位的二进制数值。信号编号1-32。
// 比如信号5 的位图数值 = 1<<(5-1) = 16 = 00010000b。
#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))	// 除了SIGKILL 和SIGSTOP 信号以外其它都是
// 可阻塞的(…10111111111011111111b)。

#define panic(s) printf("kernel panic: %s",s)

// 显示任务号nr 的进程号、进程状态和内核堆栈空闲字节数（大约）。
void show_task (int nr, struct task_struct *p)
{
	int i, j = 4096 - sizeof (struct task_struct);

	printf ("%d: pid=%d, state=%d, ", nr, p->pid, p->state);
	i = 0;
	while (i < j && !((char *) (p + 1))[i])	// 检测指定任务数据结构以后等于0 的字节数。
		i++;
	printf ("%d (of %d) chars free in kernel stack\n\r", i, j);
}
 
// 显示所有任务的任务号、进程号、进程状态和内核堆栈空闲字节数（大约）。
void show_stat (void)
{
	int i;

	for (i = 0; i < NR_TASKS; i++)// NR_TASKS 是系统能容纳的最大进程（任务）数量（64 个），
		if (task[i])		// 定义在include/kernel/sched.h 第4 行。
			show_task (i, task[i]);
}

// 定义每个时间片的滴答数?。
#define LATCH (1193180/HZ)

extern void mem_use (void);	// [??]没有任何地方定义和引用该函数。

extern int timer_interrupt (void);	// 时钟中断处理程序(kernel/system_call.s,176)。
extern int system_call (void);	// 系统调用中断处理程序(kernel/system_call.s,80)。

union task_union
{				// 定义任务联合(任务结构成员和stack 字符数组程序成员)。
	struct task_struct task;	// 因为一个任务数据结构与其堆栈放在同一内存页中，所以
	char stack[PAGE_SIZE];	// 从堆栈段寄存器ss 可以获得其数据段选择符。
};

static union task_union init_task = { INIT_TASK, };	// 定义初始任务的数据(sched.h 中)。

long volatile jiffies;	// 从开机开始算起的滴答数时间值（10ms/滴答）。
// 前面的限定符volatile，英文解释是易变、不稳定的意思。这里是要求gcc 不要对该变量进行优化
// 处理，也不要挪动位置，因为也许别的程序会来修改它的值。
long startup_time;		// 开机时间。从1970:0:0:0 开始计时的秒数。
struct task_struct *current = &(init_task.task);	// 当前任务指针（初始化为初始任务）。
struct task_struct *last_task_used_math = NULL;	// 使用过协处理器任务的指针。

struct task_struct *task[NR_TASKS] = { &(init_task.task), };	// 定义任务指针数组。

long user_stack[PAGE_SIZE >> 2];	// 定义系统堆栈指针，4K。指针指在最后一项。

// 该结构用于设置堆栈ss:esp（数据段选择符，指针），见head.s，第23 行。
struct
{
  long *a;
  short b;
}
stack_start = {&user_stack[PAGE_SIZE >> 2], 0x10};

/*
 * 'schedule()'是调度函数。这是个很好的代码！没有任何理由对它进行修改，因为它可以在所有的
 * 环境下工作（比如能够对IO-边界处理很好的响应等）。只有一件事值得留意，那就是这里的信号
 * 处理代码。
 * 注意！！任务0 是个闲置('idle')任务，只有当没有其它任务可以运行时才调用它。它不能被杀
 * 死，也不能睡眠。任务0 中的状态信息'state'是从来不用的。
 */
void schedule (void)
{
	int i, next, c;
	struct task_struct **p;	// 任务结构指针的指针。

// 如果信号位图中除被阻塞的信号外还有其它信号，并且任务处于可中断状态，则置任务为就绪状态。
// 其中'~(_BLOCKABLE & (*p)->blocked)'用于忽略被阻塞的信号，但SIGKILL 和SIGSTOP 不能被阻塞。
	for (p = &LAST_TASK; p > &FIRST_TASK; --p){
		if (*p)
		{

			if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) && (*p)->state == TASK_INTERRUPTIBLE){
				(*p)->state = TASK_RUNNING;	//置为就绪（可执行）状态。
			}
				
		}
	}

  /* 这里是调度程序的主要部分 */
	while (1)
	{
		c = -1;
		next = 0;
		i = NR_TASKS;
		p = &task[NR_TASKS];
// 这段代码也是从任务数组的最后一个任务开始循环处理，并跳过不含任务的数组槽。比较每个就绪
// 状态任务的counter（任务运行时间的递减滴答计数）值，哪一个值大，运行时间还不长，next 就
// 指向哪个的任务号。
		while (--i)
		{
			if (!*--p)
				continue;
			if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
				c = (*p)->counter, next = i;
		}
      // 如果比较得出有counter 值大于0 的结果，则退出124 行开始的循环，执行任务切换（141 行）。
		if (c)
			break;
      // 否则就根据每个任务的优先权值，更新每一个任务的counter 值，然后回到125 行重新比较。
      // counter 值的计算方式为counter = counter /2 + priority。[右边counter=0??]
		for (p = &LAST_TASK; p > &FIRST_TASK; --p)
			if (*p)
				(*p)->counter = ((*p)->counter >> 1) + (*p)->priority;
	}
	switch_to (current, task[next]);		// 切换到任务号为next 的任务，并运行之。
}

//// pause()系统调用。转换当前任务的状态为可中断的等待状态，并重新调度。
// 该系统调用将导致进程进入睡眠状态，直到收到一个信号。该信号用于终止进程或者使进程调用
// 一个信号捕获函数。只有当捕获了一个信号，并且信号捕获处理函数返回，pause()才会返回。
// 此时pause()返回值应该是-1，并且errno 被置为EINTR。这里还没有完全实现（直到0.95 版）。
int sys_pause (void)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule ();
	return 0;
}

// 把当前任务置为不可中断的等待状态，并让睡眠队列头的指针指向当前任务。
// 只有明确地唤醒时才会返回。该函数提供了进程与中断处理程序之间的同步机制。
// 函数参数*p 是放置等待任务的队列头指针。（参见列表后的说明）。
void sleep_on (struct task_struct **p)
{
	struct task_struct *tmp;

	// 若指针无效，则退出。（指针所指的对象可以是NULL，但指针本身不会为0)。
	if (!p)
		return;
	if (current == &(init_task.task))	// 如果当前任务是任务0，则死机(impossible!)。
		panic ("task[0] trying to sleep");
	tmp = *p;			// 让tmp 指向已经在等待队列上的任务(如果有的话)。
	*p = current;			// 将睡眠队列头的等待指针指向当前任务。
	current->state = TASK_UNINTERRUPTIBLE;	// 将当前任务置为不可中断的等待状态。
	schedule ();			// 重新调度。
// 只有当这个等待任务被唤醒时，调度程序才又返回到这里，则表示进程已被明确地唤醒。
// 既然大家都在等待同样的资源，那么在资源可用时，就有必要唤醒所有等待该资源的进程。该函数
// 嵌套调用，也会嵌套唤醒所有等待该资源的进程。然后系统会根据这些进程的优先条件，重新调度
// 应该由哪个进程首先使用资源。也即让这些进程竞争上岗。
	if (tmp)			// 若还存在等待的任务，则也将其置为就绪状态（唤醒）。
		tmp->state = 0;
}

// 将当前任务置为可中断的等待状态，并放入*p 指定的等待队列中。参见列表后对sleep_on()的说明。
void interruptible_sleep_on (struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task))
		panic ("task[0] trying to sleep");
	tmp = *p;
	*p = current;
repeat:
	current->state = TASK_INTERRUPTIBLE;
	schedule ();
// 如果等待队列中还有等待任务，并且队列头指针所指向的任务不是当前任务时，则将该等待任务置为
// 可运行的就绪状态，并重新执行调度程序。当指针*p 所指向的不是当前任务时，表示在当前任务被放
// 入队列后，又有新的任务被插入等待队列中，因此，既然本任务是可中断的，就应该首先执行所有
// 其它的等待任务。
	if (*p && *p != current)
	{
		(**p).state = 0;
		goto repeat;
	}
// 下面一句代码有误，应该是*p = tmp，让队列头指针指向其余等待任务，否则在当前任务之前插入
// 等待队列的任务均被抹掉了。参见图4.3。
	*p = NULL;
	if (tmp)
		tmp->state = 0;
}

// 唤醒指定任务*p。
void wake_up (struct task_struct **p)
{
	if (p && *p)
	{
		(**p).state = 0;		// 置为就绪（可运行）状态。
		*p = NULL;
	}
}

//// 时钟中断C函数处理程序，在kernel/system_call.s 中的_timer_interrupt（176 行）被调用。
// 参数cpl 是当前特权级0 或3，0 表示内核代码在执行。
// 对于一个进程由于执行时间片用完时，则进行任务切换。并执行一个计时更新工作。
void do_schedule()
{
	if ((--current->counter) > 0)
		return;			// 如果进程运行时间还没完，则退出。
	else{
		current->counter = current->priority;	// 否则重新设置进程运行时间片。
		schedule ();
	}
}

void switch_to(struct task_struct *prev, struct task_struct *next)
{
	//__switch_to(prev, next);
	//保存当前进程的prmd, ra
	asm volatile(
		"csrrd %0, 0x0\n" 
		"add.d %1, ra, r0\n"
		"add.d %2, a0, r0\n"
		"add.d %3, a1, r0\n"
		"add.d %4, tp, r0\n"
		"add.d %5, sp, r0\n"
		"csrrd %6, 0x0\n"
		"add.d ra, %7, r0\n"
		"add.d a0, %8, r0\n"
		"add.d a1, %9, r0\n"
		"add.d tp, %10, r0\n"
		"add.d sp, %11, r0\n"
		"jr ra\n"
		"nop\n"
		: "=r"(prev->tss.csr_prmd), "=r"(prev->tss.reg01), "=r"(prev->tss.reg04), "=r"(prev->tss.reg05), "=r"(prev->tss.reg02), "=r"(prev->tss.reg03)
		: "r"(next->tss.csr_prmd), "r"(next->tss.reg01), "r"(next->tss.reg04), "r"(next->tss.reg05), "r"(next->tss.reg02), "r"(next->tss.reg03)
		: "memory"
	);
	// asm volatile("csrrd %0, 0x0\n" : "=r"(prev->tss.csr_prmd));
	// asm volatile("add.d %0, ra, r0\n" : "=r"(prev->tss.reg01));
	// //保存当前进程的a0-a1
	// asm volatile("add.d %0, a0, r0\n" : "=r"(prev->tss.reg04));
	// asm volatile("add.d %0, a1, r0\n" : "=r"(prev->tss.reg05));
	// //保存当前进程的tp
	// asm volatile("add.d %0, tp, r0\n" : "=r"(prev->tss.reg02));
	// //保存当前进程的sp
	// asm volatile("add.d %0, sp, r0\n" : "=r"(prev->tss.reg03));

	// //恢复下一个进程的prmd, ra
	// asm volatile("csrrw 0x0, %0\n" : : "r"(next->tss.csr_prmd));
	// asm volatile("add.d ra, %0, r0\n" : : "r"(next->tss.reg01));
	// //恢复下一个进程的a0-a1
	// asm volatile("add.d a0, %0, r0\n" : : "r"(next->tss.reg04));
	// asm volatile("add.d a1, %0, r0\n" : : "r"(next->tss.reg05));
	// //恢复下一个进程的tp
	// asm volatile("add.d tp, %0, r0\n" : : "r"(next->tss.reg02));
	// //恢复下一个进程的sp
	// asm volatile("add.d sp, %0, r0\n" : : "r"(next->tss.reg03));

	// asm volatile("jr ra\n");

	// 	"csrrd	t1, 0x0\n" // save prmd
	// 	"stptr.d	t1, a0, THREAD_CSRPRMD\n" // save prmd
	// 	"cpu_save_nonscratch a0\n" // save a0-a1
	// 	"stptr.d	ra, a0, THREAD_REG01\n" // save ra
	// 	"move	tp, a2\n" // save tp
	// 	"cpu_restore_nonscratch a1\n" // restore a0-a1
	// 	"li.w	t0, _THREAD_SIZE - 32\n" // t0 = THREAD_SIZE - 32
	// 	"PTR_ADDU	t0, t0, tp\n" // t0 = tp + THREAD_SIZE - 32
	// 	"set_saved_sp	t0, t1, t2\n" // t1 = t0 + 32
	// 	"ldptr.d	t1, a1, THREAD_CSRPRMD\n" // load prmd
	// 	"csrwr	t1, LOONGARCH_CSR_PRMD\n" // load prmd
	// 	"jr	ra\n"
	// );
}

// 取当前进程号pid。
int sys_getpid (void)
{
	return current->pid;
}

// 取父进程号ppid。
int sys_getppid (void)
{
	return current->father;
}

// 取用户号uid。
int sys_getuid (void)
{
	return current->uid;
}

// 取euid。
int sys_geteuid (void)
{
	return current->euid;
}

// 取组号gid。
int sys_getgid (void)
{
	return current->gid;
}

// 取egid。
int sys_getegid (void)
{
	return current->egid;
}

// 调度程序的初始化子程序。
void sched_init (void)
{
	int i;
	struct desc_struct *p;	// 描述符表结构指针。

	if (sizeof (struct sigaction) != 16)	// sigaction 是存放有关信号状态的结构。
		panic ("Struct sigaction MUST be 16 bytes");
// 设置初始任务（任务0）的任务状态段描述符和局部数据表描述符(include/asm/system.h,65)。
	set_tss_desc (gdt + FIRST_TSS_ENTRY, &(init_task.task.tss));
	set_ldt_desc (gdt + FIRST_LDT_ENTRY, &(init_task.task.ldt));
// 清任务数组和描述符表项（注意i=1 开始，所以初始任务的描述符还在）。
	p = gdt + 2 + FIRST_TSS_ENTRY;
	for (i = 1; i < NR_TASKS; i++)
	{
		task[i] = NULL;
		p->a = p->b = 0;
		p++;
		p->a = p->b = 0;
		p++;
	}
	// _asm pushfd; _asm and dword ptr ss:[esp],0xffffbfff; _asm popfd;
	// ltr (0);			// 将任务0 的TSS 加载到任务寄存器tr。
	// lldt (0);			// 将局部描述符表加载到局部描述符表寄存器。
}
