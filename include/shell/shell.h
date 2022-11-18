#include "boot/boot_param.h"

#define SHELL_BUF_SIZE 2048
#define SHELL_CALLBACK_TABLE 32

extern int time_n;

void show_help();
void shell_buf_update(char c, int state);
int handle_cmd(int a0, char **args, struct bootparamsinterface *a2);
void entry_shell(int a0, char **args, struct bootparamsinterface *a2);