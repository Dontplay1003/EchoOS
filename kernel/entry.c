/* minimal kernel for loongarch64
 * it print out information passed by BIOS
 */
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration" //正式开发建议删除此行
#include "sysio/io.h"
#include "boot/boot_param.h"
#include <unistd.h>
#include "boot/boot_param.h"
#include "fs/fs.h"
#include "mm/mm.h"
#include "sched/sched.h"
#include "sched/fork.h"
#include "shell/shell.h"
//#include "serial/serial.h"

//extern void mm_init(void);
extern void trap_init(void);


int a0_temp = 0;
char** args_temp = 0;
struct bootparamsinterface* a2_temp = 0;
void kernel_entry(int a0, char **args, struct bootparamsinterface *a2)
{
    a0_temp = a0;
    args_temp = args;
    a2_temp = a2;

    //handle_bootparams(a2);
    //mm_init();
    printf("\n\n\n");
    mem_init();
    fs_init();
    trap_init();
    sched_init();
    //sata_init();


    //move_to_user_mode();	// 移到用户模式。（include/asm/system.h）
	if (!fork()) {		/* we count on this going ok */ //创建0号进程
        printf("init first process success!\n");
		// if(init()==-1){
        //     printf("init first process failed!\nsystem exit!\n");
        //     return;
        // }
	}
    else {
        printf("init first process failed!\nsystem exit!\n");
        return;
    }

    entry_shell();

    while(1) printf("");
}


// int init(void)
// {
// 	int pid,i;

// // 下面fork()用于创建一个子进程(子任务)。对于被创建的子进程，fork()将返回0 值，
// // 对于原(父进程)将返回子进程的进程号。所以if (!(pid=fork())) {...} 内是子进程执行的内容。
// // 该子进程关闭了句柄0(stdin)，以只读方式打开/etc/rc 文件，并执行/bin/sh 程序，所带参数和
// // 环境变量分别由argv_rc 和envp_rc 数组给出。参见后面的描述。
//     pid=fork(); //创建1号进程
// 	if (pid>0) {
//         /* we count on this going ok */
//     } else {
//         printf("init: fork failed\n");
//         return -1;
//     }

// // 下面是父进程执行的语句。wait()是等待子进程停止或终止，其返回值应是子进程的
// // 进程号(pid)。这三句的作用是父进程等待子进程的结束。&i 是存放返回状态信息的
// // 位置。如果wait()返回值不等于子进程号，则继续等待。
// 	// if (pid>0){
        
//     // }
// }