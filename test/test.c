#include "test.h"

#include "string.h"
#include "stdio.h"
#include "syscall.h"

// use shell to test kill a process
struct task_info shell_task = {"shell", (uint64_t)&test_shell, USER_PROCESS, 1};
struct task_info *shell_tasks[16] = {&shell_task};
int num_shell_tasks = 1;

#define SHELL_BEGIN 15

int get_command(char * buff, int buff_size, char symbol)
{
    printf("%c ", symbol);

    memset(buff, 0, buff_size);

    char * p_buff = buff;
    char ch;
    int len = 0;

    while (1)
    {
        if ((ch = sys_serial_read()) > 0)
        {
            if (ch == '\r' || ch == '\n')
            {
                printf("\n");
                break;
            }
            else if (ch == '\b' || ch == 127)
            {
                if (p_buff > buff)
                {
                    printf("\b");
                    p_buff --;
                }
            }
            else
            {
                // TODO: Need to control overflow
                printf("%c", ch);
                *p_buff = ch;
                p_buff ++;
            }
        }
    }
    return p_buff - buff;
}

void cmd_clear()
{
    sys_screen_clear(0, 30);
    sys_move_cursor(1, SHELL_BEGIN);
    printf("----------------------------------------\n");
}

int handle_command(char * buff, int buff_size)
{
    // TODO compare should be improve
    if (strcmp("clear", buff) == 0) {
        cmd_clear();
    }
}

void test_shell()
{
    cmd_clear();
    static char buff[100];
    int cmd_len = 0;

    while (1)
    {
        cmd_len = get_command(buff, sizeof(buff), '$');
        if (cmd_len <= 0)
        {
            // panic;
        }
        while (buff[cmd_len - 1] == '/')
        {
            buff[cmd_len - 1] = ' ';
            cmd_len += get_command(buff + cmd_len, sizeof(buff) - cmd_len, '>');
        }

        printf("cmd is %s\n", buff);
        handle_command(buff, sizeof(buff));
        
    }
}
