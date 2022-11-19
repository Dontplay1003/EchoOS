#pragma GCC diagnostic ignored "-Wimplicit-function-declaration" //正式开发建议删除此行
#include "shell/shell.h"
#include "sysio/io.h"
#include "drivers/kbd.h"
#include "util/util.h"
#include "cpu/loongarch.h"
#include "cpu/ls7a.h"
#include "boot/boot_param.h"
#include "fs/fs.h"
#include "app/vim.h"

int enable_shell = 0;
int handle_cmd_flag = 0;
char shell_buf[SHELL_BUF_SIZE];
unsigned int shell_buf_start_ptr = 0;
unsigned int shell_buf_end_ptr = 0;
int shell_kbd_status[128] = {0}; // 0 up 1 down

char shell_path[256]="/";

void show_help()
{
    puts("\thelp: show help\n");
    puts("\texit: exit the shell\n");
    puts("\tmemmap: print the memory map\n");
    puts("\tclock: a tool to show time interrupt\n");
    puts("\t\t-s    reset clock\n");
    puts("\t\t-p    print present clock\n");
    puts("\t\t-f    modify the speed level\n");
    puts("\tbootparam: show boot parameter\n");
}

void shell_buf_update(char c, int state, int kbd_n){
    if(enable_shell == 0) return;
    if(state == KEY_PUSH){
        if(c == '\n'){
            putc(c);
            handle_cmd_flag = 1;
        }
        else if(c != '\b' && c != 0){
            putc(c);
            shell_buf[shell_buf_end_ptr++]=c;
            shell_buf_end_ptr %= SHELL_BUF_SIZE;
        }
        else if(c == '\b' && shell_buf_end_ptr!=shell_buf_start_ptr){
            putc(c);
            putc(' ');
            putc(c);
            shell_buf_end_ptr--;
            shell_buf_end_ptr %= SHELL_BUF_SIZE;
        }
    }
}

int time_n = 0;
int normal_freq = 0x00800000UL;
int freq = 0x00800000UL;
int handle_cmd(int a0, char **args, struct bootparamsinterface *a2){
    char cmd_content[2048]={0};
    // for(int i=0;i<2048;i++){
    //     cmd_content[i]=0;
    // }
    int i=0;
    while(shell_buf_start_ptr != shell_buf_end_ptr){
        cmd_content[i++]=shell_buf[(shell_buf_start_ptr)%SHELL_BUF_SIZE];
        shell_buf[(shell_buf_start_ptr)%SHELL_BUF_SIZE] = 0;
        shell_buf_start_ptr+=1;
        shell_buf_start_ptr%=SHELL_BUF_SIZE;
    }
    cmd_content[i]='\0';
    char cmd[128]={0};
    i=0;
    while(cmd_content[i] != '\0' && cmd_content[i] != ' '){
        cmd[i] = cmd_content[i];
        i++;
    }
    cmd[i++]='\0';
    if (strcmp(cmd, "help") == 0){
        show_help();
        handle_cmd_flag = 0;
        return 1;
    }
    else if (strcmp(cmd, "exit") == 0){
        printf("shell has exited!");
        handle_cmd_flag = 0;
        return -1;
    }
    else if(strcmp(cmd, "clear") == 0){
        //printf("12345678901234567890");
        //printf("\e[H\e[2J");
        printf("\ec");
        //for(int i=0;i<10;i++) putc(1);
        //for(int i=0;i<10000;i++) printf(" ");
        //for(int i=0;i<10000;i++) printf("\b");
        handle_cmd_flag = 0;
        return 1;
    }
    else if (strcmp(cmd, "memmap") == 0){
        print_memmap();
        handle_cmd_flag = 0;
        return 1;
    }
    else if (strcmp(cmd, "clock") == 0){
        if(cmd_content[i++] == '-'){
            char para = cmd_content[i++];
            if(para == 's'){
                unsigned long tcfg = freq | CSR_TCFG_EN | CSR_TCFG_PER;
                time_n = 0;
                w_csr_tcfg(tcfg);
                printf("\tClock start!\n");
            }
            else if(para == 'p'){
                printf("\tPresent clock:%d\n",time_n);
            }
            else if(para == 'f'){

                freq = freq << (int)(cmd_content[i+1]-'0');
                printf("\tSpeed level has been set to %c level!\n",cmd_content[i+1]);
            }
            handle_cmd_flag = 0;
            return 1;
        }            
        else{
            printf("invalid parameter!\n");
            printf("\t-s    reset clock\n");
            printf("\t-p    print present clock\n");
            printf("\t-f    modify the speed level\n");
            handle_cmd_flag = 0;
            return 0;
        }
        
        handle_cmd_flag = 0;
        return 1;
    }
    else if (strcmp(cmd, "bootparam") == 0){
            int i;
            printf("\tThere is %d args for kernel:\n", a0);
            for (i=0; i < a0; i++) {
                printf("\tcmd arg %d: %s\n", i, args[i]);
            }
            printf("\tefi system table at %p\n", a2->systemtable);
            printf("\tefi extend list at %p\n", a2->extlist);
        handle_cmd_flag = 0;
        return 1;
    }
    else if (strcmp(cmd, "ls") == 0){
        struct file_folder *path = (struct file_folder *)path_als(shell_path, 0);
        printf("file name | address | file size | file type\n");
        for(int i=0;i<16;i++){
            if(path->file_list[i]->file_p!=0){
                printf("%s    ",path->file_list[i]->file_name);
                printf("%p    ",path->file_list[i]->file_p);
                printf("%d    ",path->file_list[i]->file_size);
                printf("%s\n",path->file_list[i]->file_type);
            }
        }
        for(int i=0;i<16;i++){
            if(path->file_folder_list[i]!=0){
                printf("%s    ",path->file_folder_list[i]->file_folder_name);
                printf("%p    ",path->file_folder_list[i]);
                printf("----    ");
                printf("dir\n");
            }
        }
        handle_cmd_flag = 0;
        return 1;
    }
    else if (strcmp(cmd, "mkdir") == 0){
        struct file_folder *path = (struct file_folder *)path_als(shell_path, 0);
        char result[100][100]={0};
        int result_len = 0;
        split(cmd_content," ",result,&result_len);
        //split("hxx/gyw/hx/lc","/",result,&result_len);
        if(result_len<=1){
            printf("invalid parameter!\n");
            handle_cmd_flag = 0;
            return 0;
        }
        int j = 0;
        // while(j < result_len){
        //     printf("%s",result[j++]);
        // }
        create_folder(path,&cmd_content[i]);
        handle_cmd_flag = 0;
        return 1;
    }
    else if (strcmp(cmd, "touch") == 0){
        struct file_folder *path = (struct file_folder *)path_als(shell_path, 0);
        char result[100][100]={0};
        int result_len = 0;
        split(cmd_content," ",result,&result_len);
        //split("hxx/gyw/hx/lc","/",result,&result_len);
        if(result_len<=1){
            printf("invalid parameter!\n");
            handle_cmd_flag = 0;
            return 0;
        }
        int j = 0;
        // while(j < result_len){
        //     printf("%s",result[j++]);
        // }
        create_file(path,&cmd_content[i]);
        handle_cmd_flag = 0;
        return 1;
    }
    else if (strcmp(cmd, "vim") == 0){
        struct file_folder *path = (struct file_folder *)path_als(shell_path, 0);
        char result[100][100]={0};
        int result_len = 0;
        split(cmd_content," ",result,&result_len);
        //split("hxx/gyw/hx/lc","/",result,&result_len);
        if(result_len<=1){
            printf("invalid parameter!\n");
            handle_cmd_flag = 0;
            return 0;
        }
        enable_shell = 0;
        vim_entry(path,&cmd_content[i]);
        clear_screen();
        enable_shell = 1;
        handle_cmd_flag = 0;
        return 1;
    }
    else{
        printf("No such command!\n");
        handle_cmd_flag = 0;
        return 0;
    }
}

void entry_shell(int a0, char **args, struct bootparamsinterface *a2){
    enable_shell = 1;
    for(int i =0;i<SHELL_BUF_SIZE;i++){
        shell_buf[i]=0;
    }
    kbd_event_register(shell_buf_update);
    while (1)
    {
        printf("EchoOS:%s $ ",shell_path);
        while(handle_cmd_flag == 0) printf("");
        int state = handle_cmd(a0, args, a2);
        if(state == -1) break;
    }
    enable_shell = 0;
}