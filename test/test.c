#include "test.h"

#include "screen.h"
#include "string.h"
#include "stdio.h"
#include "syscall.h"

// use shell to test kill a process
struct task_info shell_task = {"shell", (uint64_t)&test_shell, USER_PROCESS, 1};
struct task_info *shell_tasks[16] = {&shell_task};
int num_shell_tasks = 1;

#ifdef P3_TEST

struct task_info task1 = {"task1", (uint64_t)&ready_to_exit_task, USER_PROCESS};
struct task_info task2 = {"task2", (uint64_t)&wait_lock_task, USER_PROCESS};
struct task_info task3 = {"task3", (uint64_t)&wait_exit_task, USER_PROCESS};

// struct task_info task4 = {"task4", (uint64_t)&semaphore_add_task1, USER_PROCESS};
// struct task_info task5 = {"task5", (uint64_t)&semaphore_add_task2, USER_PROCESS};
// struct task_info task6 = {"task6", (uint64_t)&semaphore_add_task3, USER_PROCESS};

struct task_info task7 = {"task7", (uint64_t)&producer_task, USER_PROCESS};
struct task_info task8 = {"task8", (uint64_t)&consumer_task1, USER_PROCESS};
struct task_info task9 = {"task9", (uint64_t)&consumer_task2, USER_PROCESS};

struct task_info task10 = {"task10", (uint64_t)&barrier_task1, USER_PROCESS};
struct task_info task11 = {"task11", (uint64_t)&barrier_task2, USER_PROCESS};
struct task_info task12 = {"task12", (uint64_t)&barrier_task3, USER_PROCESS};

struct task_info task13 = {"SunQuan", (uint64_t)&SunQuan, USER_PROCESS};
struct task_info task14 = {"LiuBei", (uint64_t)&LiuBei, USER_PROCESS};
struct task_info task15 = {"CaoCao", (uint64_t)&CaoCao, USER_PROCESS};
#endif


#ifdef P4_TEST
struct task_info task16 = {"mem_test1", (uint64_t)&rw_task1, USER_PROCESS};
struct task_info task17 = {"plane", (uint64_t)&drawing_task1, USER_PROCESS};
#endif

#ifdef P5_TEST
struct task_info task18 = {"mac_send", (uint64_t)&test_send, USER_PROCESS};
struct task_info task19 = {"mac_recv", (uint64_t)&test_recv, USER_PROCESS};
#endif

#ifdef P6_TEST

struct task_info task19 = {"fs_test", (uint64_t)&test_fs, USER_PROCESS};
#endif
// struct task_info task16 = {"multcore", (uint64_t)&test_multicore, USER_PROCESS};
struct task_info *test_tasks[NUM_MAX_TASK] = {
    &task19,
};
