/* minimal kernel for loongarch64
 * it print out information passed by BIOS
 */
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration" //正式开发建议删除此行
#include "sysio/io.h"
#include "boot/boot_param.h"
#include <unistd.h>
#include "config/info.h"
#include "boot/boot_param.h"
#include "fs/fs.h"
//#include "serial/serial.h"

//extern void mm_init(void);
extern void trap_init(void);


void kernel_entry(int a0, char **args, struct bootparamsinterface *a2)
{
    // char *p=0x00000000;
    // while(*p<0x10000000){
    //     p+=1024;
    //     //p+=1;
    //     //*p = 31;
    //     printf("%x ",p);
    //     printf("%x\n",*p);
    // }
    
    //while(1)printf("end");

    //printf("%p\n",&a0);
    //printf("%p\n",args);
    //printf("%p\n",a2);
    // char digits[] = "0123456789abcdef";
    // for(int i=0;i<1000;i++){
    //     //printf("%p ",((char*)a2+i));
    //     //printf("%d",(int)(*((char*)a2+i)) & 0b00001111);
    //     putc(digits[ (int)(*((char*)a2+i)) & 0b00001111 ]);
    //     putc(digits[ (int)(*((char*)a2+i)) & 0b11110000 ]);
    //     printf(" ");
    // }
    //env_init(a2);
    // int i;

    // printf("There is %d args for kernel:\n", a0);
    // for (i=0; i < a0; i++) {
    //     printf("cmd arg %d: %s\n", i, args[i]);
    // }

    // printf("efi system table at %p\n", a2->systemtable);
    // printf("efi extend list at %p\n", a2->extlist);


    /* ... read the linux kernel source for how to decode these data */

    //handle_bootparams(a2);
    //mm_init();
    printf("\n\n\n");
    fs_init();
    trap_init();

    // int x=0,y=0;
    // //printf("\ec");
    // printf("--------------");
    // printf("\e[6n");

    // //printf("[%d;%dR", &x, &y);
    // printf("row = %d, col = %d", x, y);
    //MOVETO(x,y);
    //printf("\nx:%d,y:%d\n");
    //while(1);

    print_info();
    entry_shell(a0, args, a2);
    //sata_init();
    int xx;
    while(1){
        xx++;
    }
}
