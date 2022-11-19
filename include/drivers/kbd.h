#include "drivers/kbdmap.h"

//#define KBD_BUF_SIZE 2048
#define KBD_CALLBACK_TABLE 32
enum{
    KEY_PUSH = 1,
    KEY_RELEASE = 0
};
enum{
    CAPS_UPER = 1,
    CAPS_LOWER = 0
};

//typedef void (*kbd_callback_table)(char*)[KBD_CALLBACK_TABLE];

int kbd_event_register(void *f_entry);
int kbd_event_invoke(char issue_c, int key_state, int kbd_n);
unsigned char kbd_irq();