/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode. 
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit 
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include "test.h"

#include "screen.h"
#include "stdio.h"
#include "syscall.h"
#include "string.h"


#define SHELL_BEGIN 15

typedef struct command
{
    char name[32];
    void (* function)(int argc, char * argv[]);
} command_t;

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
                // ! WARNING: overflow will cause fault
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

void cmd_exec(int argc, char * argv[])
{
    if (argc < 2)
    {
        printf("ERROR: need task id.\n");
        return;
    }
    int task_id = atoi(argv[1]);
    printf("start task %d\n", task_id);
    sys_spawn(test_tasks[task_id]);
}

void cmd_kill(int argc, char * argv[])
{
    if (argc < 2)
    {
        printf("ERROR: need pid.\n");
        return;
    }
    int pid = atoi(argv[1]);
    printf("kill pid %d\n", pid);
    if (sys_kill(pid))
    {
        printf("ERROR: no such proc.\n");
    }
}

void cmd_other(int argc, char * argv[])
{
    printf("ERROR: unknow command\n");
    for (int i = 0; i < argc; i++)
    {
        printf("    %d %s\n", i, argv[i]);
    }
}

void cmd_ps(int argc, char * argv[])
{
    sys_process_show();
}

command_t command_list[32] =
{
    {"other", cmd_other},
    {"clear", cmd_clear},
    {"exec", cmd_exec},
    {"kill", cmd_kill},
    {"ps", cmd_ps},
};

int command_cnt = 5;

void prase_command(char * buff, int buff_size, int * argc, char * argv[])
{
    int arg_cnt = 0;
    int space = 1;
    for (int i = 0; i < buff_size && buff[i]; i++)
    {
        if (buff[i] == ' ')         // TODO: should be replaced by is_space()
        {
            buff[i] = 0;
            space = 1;
        }
        else
        {
            if (space)
            {
                argv[arg_cnt] = buff + i;
                arg_cnt += 1;
            }
            space = 0;
        }
    }
    *argc = arg_cnt;
}

void handle_command(int argc, char * argv[])
{
    for (int i = 0; i < command_cnt; i++)
    {
        if (strcmp(command_list[i].name, argv[0]) == 0)
        {
            command_list[i].function(argc, argv);
            return;
        }
    }
    cmd_other(argc, argv);
}

void test_shell()
{
    cmd_clear();
    static char buff[100];
    int cmd_len = 0;

    int argc;
    char * argv[100];

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

        // printf("cmd is %s\n", buff);

        prase_command(buff, cmd_len, &argc, argv);
        
        handle_command(argc, argv);
    }
    test_tasks;
}
