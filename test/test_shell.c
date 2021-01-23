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
#include "fs.h"


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
    sys_spawn(test_tasks[task_id], argc, argv);
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

void cmd_dpt(int argc, char * argv[]) {
    if (argc < 2) {
        print_partition_info();
    } else {
        if (strcmp(argv[1], "read") == 0) {
            read_mbr();
        } else if (strcmp(argv[1], "write") == 0) {
            write_mbr();
        } else if (strcmp(argv[1], "set") == 0) {
            if (argc < 5) {
                printf("ERROR: argc < 5\n");
                return;
            }
            set_partition_table(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
        } else if (strcmp(argv[1], "clear") == 0) {
            if (argc < 3) {
                printf("ERROR: argc < 3\n");
                return;
            }
            clear_partition_table(atoi(argv[2]));
        }
    }
}

void cmd_fs(int argc, char * argv[]) {
    if (argc < 2) {
        print_file_system_info();
    } else {
        if (strcmp(argv[1], "init") == 0) {
            if (argc < 3) {
                printf("ERROR: argc < 3\n");
                return;
            }
            init_file_system(atoi(argv[2]));
        } else if (strcmp(argv[1], "mount") == 0) {
            if (argc < 3) {
                printf("ERROR: argc < 3\n");
                return;
            }
            mount_file_system(atoi(argv[2]));
        } else if (strcmp(argv[1], "unmount") == 0) {
            unmount_file_system();
        }
    }
}

void cmd_ls(int argc, char * argv[]) {
    sys_ls(cur_dir_inode);
}

void cmd_mkdir(int argc, char * argv[]) {
    sys_mkdir(cur_dir_inode, argv[1]);
}

void cmd_touch(int argc, char * argv[]) {
    sys_touch(cur_dir_inode, argv[1]);
}

void cmd_rmdir(int argc, char * argv[]) {
    int inode_id = sys_find(argv[1]);
    sys_rmdir(inode_id);
}

void cmd_cd(int argc, char * argv[]) {
    sys_cd(argv[1]);
}

void cmd_sink(int argc, char * argv[]) {
    sys_sink();
}

void cmd_cat(int argc, char * argv[]) {
    sys_cat(argv[1]);
}

command_t command_list[32] =
{
    {"other", cmd_other},
    {"clear", cmd_clear},
    {"exec", cmd_exec},
    {"kill", cmd_kill},
    {"ps", cmd_ps},
    {"dpt", cmd_dpt},
    {"fs", cmd_fs},
    {"ls", cmd_ls},
    {"mkdir", cmd_mkdir},
    {"rmdir", cmd_rmdir},
    {"cd", cmd_cd},
    {"touch", cmd_touch},
    {"sink", cmd_sink},
    {"cat", cmd_cat},
};

int command_cnt = 14;

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
