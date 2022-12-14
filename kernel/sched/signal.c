/* 
* linux/kernel/signal.c
*
* (C) 1991 Linus Torvalds
*/
/*
注意：signal.c和fork.c文件的编译选项内不能有vc变量优化选项/Og，因为这两个文件
	内的函数参数内包含了函数返回地址等内容。如果加了/Og选项，编译器就会在认为
	这些参数不再使用后占用该内存，导致函数返回时出错。
	math/math_emulate.c照理也应该这样，不过好像它没有把eip等参数优化掉:)
*/
#include "sched/sched.h" // 调度程序头文件，定义了任务结构task_struct、初始任务0 的数据，
// 还有一些有关描述符参数设置和获取的嵌入式汇编函数宏语句。
#include "sched/signal.h"		// 信号头文件。定义信号符号常量，信号结构以及信号操作函数原型。

volatile void do_exit (int error_code);	// 前面的限定符volatile 要求编译器不要对其进行优化。

/*
// 获取当前任务信号屏蔽位图（屏蔽码）。
int sys_sgetmask ()
{
	return current->blocked;
}

// 设置新的信号屏蔽位图。SIGKILL 不能被屏蔽。返回值是原信号屏蔽位图。
int sys_ssetmask (int newmask)
{
	int old = current->blocked;

	current->blocked = newmask & ~(1 << (SIGKILL - 1));
	return old;
}

// 复制sigaction 数据到fs 数据段to 处。。
static _inline void save_old (char *from, char *to)
{
	int i;

	verify_area (to, sizeof (struct sigaction));	// 验证to 处的内存是否足够。
	for (i = 0; i < sizeof (struct sigaction); i++)
	{
		put_fs_byte (*from, to);	// 复制到fs 段。一般是用户数据段。
		from++;				// put_fs_byte()在include/asm/segment.h 中。
		to++;
	}
}

// 把sigaction 数据从fs 数据段from 位置复制到to 处。
static _inline void get_new (char *from, char *to)
{
	int i;

	for (i = 0; i < sizeof (struct sigaction); i++)
		*(to++) = get_fs_byte (from++);
}

// signal()系统调用。类似于sigaction()。为指定的信号安装新的信号句柄(信号处理程序)。
// 信号句柄可以是用户指定的函数，也可以是SIG_DFL（默认句柄）或SIG_IGN（忽略）。
// 参数signum --指定的信号；handler -- 指定的句柄；restorer –原程序当前执行的地址位置。
// 函数返回原信号句柄。
int sys_signal (int signum, long handler, long restorer)
{
	struct sigaction tmp;

	if (signum < 1 || signum > 32 || signum == SIGKILL)	// 信号值要在（1-32）范围内，
		return -1;			// 并且不得是SIGKILL。
	tmp.sa_handler = (void (*)(int)) handler;	// 指定的信号处理句柄。
	tmp.sa_mask = 0;		// 执行时的信号屏蔽码。
	tmp.sa_flags = SA_ONESHOT | SA_NOMASK;	// 该句柄只使用1 次后就恢复到默认值，
// 并允许信号在自己的处理句柄中收到。
	tmp.sa_restorer = (void (*)(void)) restorer;	// 保存返回地址。
	handler = (long) current->sigaction[signum - 1].sa_handler;
	current->sigaction[signum - 1] = tmp;
	return handler;
}

// sigaction()系统调用。改变进程在收到一个信号时的操作。signum 是除了SIGKILL 以外的任何
// 信号。[如果新操作(action)不为空]则新操作被安装。如果oldaction 指针不为空，则原操作
// 被保留到oldaction。成功则返回0，否则为-1。
int sys_sigaction (int signum, const struct sigaction *action,
					struct sigaction *oldaction)
{
	struct sigaction tmp;

// 信号值要在（1-32）范围内，并且信号SIGKILL 的处理句柄不能被改变。
	if (signum < 1 || signum > 32 || signum == SIGKILL)
		return -1;
// 在信号的sigaction 结构中设置新的操作（动作）。
	tmp = current->sigaction[signum - 1];
	get_new ((char *) action, (char *) (signum - 1 + current->sigaction));
// 如果oldaction 指针不为空的话，则将原操作指针保存到oldaction 所指的位置。
	if (oldaction)
		save_old ((char *) &tmp, (char *) oldaction);
// 如果允许信号在自己的信号句柄中收到，则令屏蔽码为0，否则设置屏蔽本信号。
	if (current->sigaction[signum - 1].sa_flags & SA_NOMASK)
		current->sigaction[signum - 1].sa_mask = 0;
	else
		current->sigaction[signum - 1].sa_mask |= (1 << (signum - 1));
	return 0;
}
*/

// 系统调用中断处理程序中真正的信号处理程序（在kernel/system_call.s,119 行）。
// 该段代码的主要作用是将信号的处理句柄插入到用户程序堆栈中，并在本系统调用结束
// 返回后立刻执行信号句柄程序，然后继续执行用户的程序。
void do_signal (long signr)
{
	unsigned long sa_handler;
	//long old_eip = eip;
	struct sigaction *sa = current->sigaction + signr - 1;	//current->sigaction[signu-1]。
	//int longs;
	//unsigned long *tmp_esp;

	sa_handler = (unsigned long) sa->sa_handler;
// 如果信号句柄为SIG_IGN(忽略)，则返回；如果句柄为SIG_DFL(默认处理)，则如果信号是
// SIGCHLD 则返回，否则终止进程的执行
	if (sa_handler == 1)
		return;
	if (!sa_handler)
	{
		if (signr == SIGCHLD)
			return;
		else
// 这里应该是do_exit(1<<signr))。
			do_exit (1 << (signr - 1));	// [?? 为什么以信号位图为参数？不为什么!？?]
	}

}
