#include"config/info.h"
#include"sysio/io.h"

char LABEL[] = "EchoOS 0.0.1\n";

char LOGO[] =
"    ______     __          ____  _____\n"
"   / ____/____/ /_  ____  / __ \\/ ___/\n"
"  / __/ / ___/ __ \\/ __ \\/ / / /\\__ \\ \n"
" / /___/ /__/ / / / /_/ / /_/ /___/ / \n"
"/_____/\\___/_/ /_/\\____/\\____//____/  \n"
"\n";

char BIRTHDAY[] = "2022-10-03";

char COPYRIGHT[] = "Copyright (C) 2022 Xu Xian. All Rights Resevered.\n";

char ORIGIN[] = "October 3, 2022 is an awesome day.\n";

char THANKS[] = "Thanks to the Support of Beijing Jiaotong University.\n";

void print_info()
{
    puts(LOGO);
    printf("\n");
    puts(LABEL);
    printf("\n");
    puts(ORIGIN);
    puts(COPYRIGHT);
    puts(THANKS);
    printf("\n");
}