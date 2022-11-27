#include "app/vim.h"
#include "sysio/io.h"
#include "utils/mem.h"
#include "utils/string.h"
#include "drivers/kbd.h"
#include "cpu/loongarch.h"

int enable_vim = 0;
int exit_flag = 0;
text_i = 0;
char file_copy[1000][80];

struct file_info *vim_file = 0;
int c_x=0,c_y=0,file_x=0,file_y=0,file_ln=0,offset = 0;

void clear_screen(){
    printf("\ec");
    printf("\e[0;0H");
}

void print2screen(int offset){
    clear_screen();
    for(int i=0+offset;i<25;i++){
        for(int j=0;j<80;j++){
            if(file_copy[i][j]!=1) printf("%c",file_copy[i][j]);
        }
    }
}

void save_file(){
    char *text_p = vim_file->file_p;
    for(int i=0;i<1000;i++){
        for(int j=0;j<80;j++){
            if(file_copy[i][j]!=0 && file_copy[i][j]!=1){
                *text_p = file_copy[i][j];
                text_p++;
            }
            else continue;
        }
    }
}

void vim_input_handle(char c, int state, int kbd_n){
    //printf("%d\n",enable_vim);
    if(enable_vim == 0) return;
    printf("");
    if(state == KEY_PUSH){
        if(kbd_n == KEY_ESC){
            enable_vim = 0;
            save_file();
            exit_flag = 1;
            return;
        }
        else if(kbd_n == 72){
            printf("\e[1A");
            if(c_y>3){
                c_y--;
            }
            else if(offset>0){
                offset--;
            }
            else if(offset <= 0 && c_y>0){
                offset=0;
                c_y--;
            }
        }
        else if(kbd_n == 80){
            printf("\e[1B");
            if(c_y<22){
                c_y++;
            }
            else if(offset<file_ln-23){
                offset++;
            }
            else if(offset >= file_ln-23 && c_y<=25){
                offset=file_ln-23;
                c_y++;
            }
        }
        else if(kbd_n == 77){
            printf("\e[1C");
            if(c_x<79) c_x++;
            else if(file_copy[c_y+offset+1][0]==1){
                printf("\e[B");
                printf("\e[80D");
            }
        }
        else if(kbd_n == 75){
            printf("\e[1D");
            if(c_x>0) c_x--;
        }
        else{
            file_copy[c_y+offset][c_x]=c;
            //file_copy[0][0]=c;
            if(c=='\n'){
                c_y++;
                c_x=0;
            }
            if(c_x<80)c_x++;
            else{
                c_x=0;
                c_y++;
            }
        }
    }
    //printf("\e[s");
    print2screen(offset);
    //printf("\e[u");
    if(c_y==0)printf("\033[%d;%dH", c_y+1, c_x+1);
    else printf("\033[%d;%dH", c_y+1, c_x);
    return;
}

void create_copy(){
    c_x=0,c_y=0,file_y=0,file_x=0;file_ln=0;
    char *text_p = vim_file->file_p;
    for(int i=0;i<1000;i++){
        for(int j=0;j<80;j++){
            file_copy[i][j]=0;
        }
    }
    char temp_c = 0;
    while(*text_p!=0){
        temp_c = *text_p;
        if(temp_c == '\n'){
            file_copy[file_y][file_x] = temp_c;
            file_y++;
            file_ln++;
            file_x=0;
        }
        else{
            file_copy[file_y][file_x] = temp_c;
            file_x++;
            if(file_x>=80){
                file_x=0;
                file_y++;
                file_copy[file_y][file_x] = 1;
            }
        }
        text_p++;
    }
}

int vim_entry(struct file_folder *relative_path, char *file_name){
    enable_vim = 1;
    vim_file = 0;
    text_i = 0;
    exit_flag=0;
    c_x=0,c_y=0,file_x=0,file_y=0,file_ln=0,offset = 0;

    int list_n=-1;
    for(int i=0;i<16;i++){
        if(strcmp(relative_path->file_list[i]->file_name, file_name) == 0){
            list_n = i;
            break;
        }
    }

    if(list_n==-1){
        list_n = create_file(relative_path,file_name);
    }

    if(list_n==-1){
        return -1;
    }
    else{
        vim_file = relative_path->file_list[list_n];
        create_copy();
        clear_screen();
        print2screen(0);

        while(1){
            if(exit_flag == 1) break;
            printf("");
        }

        enable_vim = 0;
        clear_screen();
        //printf("%c %p ",file_copy[0][0],(vim_file->file_p));
        return 1;
    }
    
}