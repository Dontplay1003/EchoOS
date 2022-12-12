/*
* linux/kernel/fork.c
*
* (C) 1991 Linus Torvalds
*/
/*
注意：signal.c和fork.c文件的编译选项内不能有vc变量优化选项/Og，因为这两个文件
	内的函数参数内包含了函数返回地址等内容。如果加了/Og选项，编译器就会在认为
	这些参数不再使用后占用该内存，导致函数返回时出错。
	math/math_emulate.c照理也应该这样，不过好像它没有把eip等参数优化掉:)
*/

/*
* 'fork.c'中含有系统调用'fork'的辅助子程序（参见system_call.s），以及一些其它函数
* ('verify_area')。一旦你了解了fork，就会发现它是非常简单的，但内存管理却有些难度。
* 参见'mm/mm.c'中的'copy_page_tables()'。
*/
#include "errno.h"		// 错误号头文件。包含系统中各种出错号。(Linus 从minix 中引进的)。
#include "sched/sched.h"	// 调度程序头文件，定义了任务结构task_struct、初始任务0 的数据，
#include "mm/mm.h"		// 内存管理头文件。定义了页面大小和一些页面释放函数。
#include "mm/kmalloc.h"		// 内存管理头文件。定义了页面大小和一些页面释放函数。

extern void write_verify (unsigned long address);

long last_pid = 0;

//// 进程空间区域写前验证函数。
// 对当前进程的地址addr 到addr+size 这一段进程空间以页为单位执行写操作前的检测操作。
// 若页面是只读的，则执行共享检验和复制页面操作（写时复制）。
void verify_area (void *addr, int size)
{
	unsigned long start;

	start = (unsigned long) addr;
// 将起始地址start 调整为其所在页的左边界开始位置，同时相应地调整验证区域大小。
// 此时start 是当前进程空间中的线性地址。
	size += start & 0xfff;
	start &= 0xfffff000;
	start += get_base (current->ldt[2]);// 此时start 变成系统整个线性空间中的地址位置。
	while (size > 0)
	{
		size -= 4096;
// 写页面验证。若页面不可写，则复制页面。（mm/memory.c，261 行）
		write_verify (start);
		start += 4096;
	}
}

// 设置新任务的代码和数据段基址、限长并复制页表。
// nr 为新任务号；p 是新任务数据结构的指针。
int copy_mem (int nr, struct task_struct *p)
{
	unsigned long old_data_base, new_data_base, data_limit;
	unsigned long old_code_base, new_code_base, code_limit;

	code_limit = get_limit (0x0f);	// 取局部描述符表中代码段描述符项中段限长。
	data_limit = get_limit (0x17);	// 取局部描述符表中数据段描述符项中段限长。
	old_code_base = get_base (current->ldt[1]);	// 取原代码段基址。
	old_data_base = get_base (current->ldt[2]);	// 取原数据段基址。
	if (old_data_base != old_code_base)	// 0.11 版不支持代码和数据段分立的情况。
		panic ("We don't support separate I&D");
	if (data_limit < code_limit)	// 如果数据段长度 < 代码段长度也不对。
		panic ("Bad data_limit");
	new_data_base = new_code_base = nr * 0x4000000;	// 新基址=任务号*64Mb(任务大小)。
	p->start_code = new_code_base;
	set_base (p->ldt[1], new_code_base);	// 设置代码段描述符中基址域。
	set_base (p->ldt[2], new_data_base);	// 设置数据段描述符中基址域。
	if (copy_page_tables (old_data_base, new_data_base, data_limit))
    {				// 复制代码和数据段。
		free_page_tables (new_data_base, data_limit);	// 如果出错则释放申请的内存。
		return -ENOMEM;
    }
	return 0;
}

/*
* OK，下面是主要的fork 子程序。它复制系统进程信息(task[n])并且设置必要的寄存器。
* 它还整个地复制数据段。
*/
// 复制进程。
int copy_process (int nr, struct task_struct *parent_p)
{
	struct task_struct *p;
	int i;
	// struct file *f;
	// struct i387_struct *p_i387;

	//p = (struct task_struct *) get_free_page();	// 为新任务数据结构分配内存。
	p = (struct task_struct *) kmalloc (sizeof(*p));
	if (!p)			// 如果内存分配出错，则返回出错码并退出。
		return -EAGAIN;
	task[nr] = p;			// 将新任务结构指针放入任务数组中。
// 其中nr 为任务号，由前面find_empty_process()返回。
	*p = *current;		/* NOTE! this doesn't copy the supervisor stack */
/* 注意！这样做不会复制超级用户的堆栈 （只复制当前进程内容）。*/

	p->state = TASK_UNINTERRUPTIBLE;	// 将新进程的状态先置为不可中断等待状态。
	p->pid = last_pid;		// 新进程号。由前面调用find_empty_process()得到。
	p->father = current->pid;	// 设置父进程号。
	p->counter = p->priority;
	p->signal = 0;		// 信号位图置0。
	p->leader = 0;		/* process leadership doesn't inherit */
/* 进程的领导权是不能继承的 */

	p->utime = p->stime = 0;	// 初始化用户态时间和核心态时间。
	p->cutime = p->cstime = 0;	// 初始化子进程用户态和核心态时间。
	p->start_time = jiffies;	// 当前滴答数时间。

// 以下设置任务状态段TSS所需的数据（参见列表后说明）。
	// p->tss.back_link = 0;
	// p->tss.esp0 = PAGE_SIZE + (long) p;	// 堆栈指针（由于是给任务结构p分配了1页新内存，所以此时esp0 正好指向该页顶端）。

	// p->tss.ss0 = 0x10;		// 堆栈段选择符（内核数据段）[??]。
	// p->tss.eip = eip;		// 指令代码指针。
	// p->tss.eflags = eflags;	// 标志寄存器。

	// p->tss.es = es; //& 0xffff;	// 段寄存器仅16位有效。
	// p->tss.cs = cs; //& 0xffff;
	// p->tss.ss = ss; //& 0xffff;
	// p->tss.ds = ds; //& 0xffff;
	// p->tss.fs = fs; //& 0xffff;
	// p->tss.gs = gs; //& 0xffff;

	// p->tss.ldt = _LDT (nr);	// 该新任务nr 的局部描述符表选择符（LDT 的描述符在GDT 中）。
	// p->tss.trace_bitmap = 0x80000000;
	p->tss.reg01 = parent_p->tss.reg01; //ra tp sp fp
	p->tss.reg02 = parent_p->tss.reg02;
	p->tss.reg03 = parent_p->tss.reg03;
	p->tss.reg22 = parent_p->tss.reg22;
	p->tss.reg23 = parent_p->tss.reg23; //s0-s3
	p->tss.reg24 = parent_p->tss.reg24;
	p->tss.reg25 = parent_p->tss.reg25;
	p->tss.reg26 = parent_p->tss.reg26;
	p->tss.reg27 = parent_p->tss.reg27; //s4-s8
	p->tss.reg28 = parent_p->tss.reg28;
	p->tss.reg29 = parent_p->tss.reg29;
	p->tss.reg30 = parent_p->tss.reg30;
	p->tss.reg31 = parent_p->tss.reg31;

	p->tss.csr_prmd = parent_p->tss.csr_prmd;
	p->tss.csr_crmd = parent_p->tss.csr_crmd;
	p->tss.csr_euen = parent_p->tss.csr_euen;
	p->tss.csr_ecfg = parent_p->tss.csr_ecfg;
	p->tss.csr_badvaddr = parent_p->tss.csr_badvaddr;

	p->tss.scr0 = parent_p->tss.scr0;
	p->tss.scr1 = parent_p->tss.scr1;
	p->tss.scr2 = parent_p->tss.scr2;
	p->tss.scr3 = parent_p->tss.scr3;

	p->tss.eflags = parent_p->tss.eflags;

	p->tss.trap_nr = parent_p->tss.trap_nr;
	p->tss.error_code = parent_p->tss.error_code;

	p->tss.fpu.fcsr = parent_p->tss.fpu.fcsr;
	p->tss.fpu.vcsr = parent_p->tss.fpu.vcsr;
	p->tss.fpu.fcc = parent_p->tss.fpu.fcc;
	for(int i=0;i<NUM_FPU_REGS;i++)
	{
		p->tss.fpu.fpr[i] = parent_p->tss.fpu.fpr[i];
	}



// 设置新任务的代码和数据段基址、限长并复制页表。如果出错（返回值不是0），则复位任务数组中
// 相应项并释放为该新任务分配的内存页。
	if (copy_mem (nr, p))
	{				// 返回不为0 表示出错。
		task[nr] = NULL;
		//free_page ((long) p);
		kfree(p, sizeof(*p));
		return -EAGAIN;
	}

// 在GDT中设置新任务的TSS和LDT描述符项，数据从task 结构中取。
// 在任务切换时，任务寄存器tr 由CPU 自动加载。
	set_tss_desc (gdt + (nr << 1) + FIRST_TSS_ENTRY, &(p->tss));
	set_ldt_desc (gdt + (nr << 1) + FIRST_LDT_ENTRY, &(p->ldt));
	p->state = TASK_RUNNING;	/* do this last, just in case */
/* 最后再将新任务设置成可运行状态，以防万一 */
	return last_pid;		// 返回新进程号（与任务号是不同的）。
}

// 为新进程取得不重复的进程号last_pid，并返回在任务数组中的任务号(数组index)。
int find_empty_process (void)
{
	int i;
// repeat:
// 	if ((++last_pid) < 0)
// 		last_pid = 1;
// 	for (i = 0; i < NR_TASKS; i++){
// 		if (task[i] && task[i]->pid == last_pid){
// 			goto repeat;
// 		}	
// 	}
	for (i = 1; i < NR_TASKS; i++){	// 任务0 排除在外。
		if (!task[i]){
			return i;
		}
	}
	return -EAGAIN;
}

//int copy_process (int nr, long gs, long fs, long es, long ds, long cs, long eip, long eflags, long ss)

//fork
int fork(void)
{
    int pid = find_empty_process();
    if (pid < 0)
        return -1;
    copy_process(pid, current);
	return pid;
}