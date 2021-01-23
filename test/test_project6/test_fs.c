#include "test_fs.h"
#include "fs.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"

static char buff[64];

void test_fs(void)
{
    int i, j;
    int fd = sys_fopen("1.txt", O_RDWR);
    // printf("===============%d\n", fd);

    sys_fseek(fd, 0x409F6A);
    // write 'hello world!' * 10
    for (i = 0; i < 5; i++)
    {
        sys_fwrite(fd, "hello world!\n", 13);
    }

    sys_move_cursor(0, 0);

    sys_fseek(fd, 0x409F6A);
    // read
    for (i = 0; i < 5; i++)
    {
        sys_fread(fd, buff, 13);
        for (j = 0; j < 13; j++)
        {
            printf("%c", buff[j]);
        }
    }
    
    sys_move_cursor(0, 1);

    sys_fclose(fd);
    sys_exit();
}