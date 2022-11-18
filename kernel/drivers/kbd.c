#pragma GCC diagnostic ignored "-Wreturn-type" //正式开发建议删除此行
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration" //正式开发建议删除此行
#include "drivers/kbd.h"
#include "shell/shell.h"

//char kbd_buf[KBD_BUF_SIZE];
//unsigned int kbd_start_ptr = 0;
//unsigned int kbd_end_ptr = 0;
int kbd_status[128] = {0}; // 0 up 1 down
int capslock_state = CAPS_LOWER;


int kbd_callback_table_num = 0;
void (*kbd_callback_table[KBD_CALLBACK_TABLE])(char,int);


int kbd_event_register(void* f_entry){
    if(f_entry == 0){
        return -1;
    }
    else kbd_callback_table[kbd_callback_table_num]=f_entry;
    kbd_callback_table_num+=1;
    return kbd_callback_table_num-1;
}

int kbd_event_invoke(char issue_c, int key_state){
    int kbd_event_num = kbd_callback_table_num;
    for(int i=0;i<kbd_event_num;i++){
        //(kbd_callback_table[i])(kbd_buf[(kbd_end_ptr-1)%KBD_BUF_SIZE]);
        (kbd_callback_table[i])(issue_c, key_state);
    }
}

unsigned char kbd_irq(){
    char issue_c=0;
    int kbd_n=0;
    while (kbd_has_data()) {
        //printf("%x ", kbd_read_byte());
        kbd_n = keymap[ (unsigned int)kbd_read_byte() ];
        //printf("%d\n",kbd_n);
        //解析位置
        kbd_status[kbd_n]=KEY_PUSH;
        if(kbd_n==KEY_INSERT){//??
            kbd_n = keymap[ (unsigned int)kbd_read_byte() ];
            kbd_status[kbd_n]=KEY_RELEASE;
        }
        if(kbd_n == KEY_CAPSLOCK && kbd_status[kbd_n] == KEY_PUSH){
            capslock_state = !capslock_state;
        }
        //else{
        //    kbd_status[kbd_n] = !kbd_status[kbd_n];
        //    continue;
        //}
        //解析内容并保存到键盘缓冲区
        if ((kbd_status[KEY_LEFTSHIFT] == KEY_PUSH || kbd_status[KEY_RIGHTSHIFT] == KEY_PUSH)){
            //kbd_buf[(kbd_end_ptr++) % KBD_BUF_SIZE] = kbd_US_up[ kbd_n ];
            issue_c = kbd_US_up[ kbd_n ];
        }
        else{
            //kbd_buf[(kbd_end_ptr++)% KBD_BUF_SIZE] = kbd_US[ kbd_n ];
            issue_c = kbd_US[ kbd_n ];
        }
        //kbd_end_ptr %= KBD_BUF_SIZE;
        //printf("%c", kbd_US[ keymap[ (unsigned int)kbd_read_byte() ] ]);
    }
    
    //kbd_event_invoke(issue_c, kbd_status[kbd_n]);
    shell_buf_update(issue_c, kbd_status[kbd_n]);
    
    //printf("key done\n");
}