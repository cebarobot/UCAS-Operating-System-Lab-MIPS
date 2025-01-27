#include "mac.h"
#include "irq.h"
#include "type.h"
#include "screen.h"
#include "syscall.h"
#include "sched.h"
#include "string.h"
#include "stdio.h"
#include "test5.h"

#define SHM_KEY 1

#define HDR_OFFSET 54
#define SHMP_KEY 0x42
#define MAGIC 0xbeefbeefbeefbeeflu
#define FIFO_BUF_MAX 2048

#define MAX_RECV_CNT 32
char recv_buffer[MAX_RECV_CNT * PSIZE];
uint64_t recv_length[MAX_RECV_CNT] = {0};

uint32_t send_buffer[PSIZE] = {0xffffffff, 0x5500ffff, 0xf77db57d, 0x00450008, 0x0000d400, 0x11ff0040, 0xa8c073d8, 0x00e00101, 0xe914fb00, 0x0004e914, 0x0000, 0x005e0001, 0x2300fb00, 0x84b7f28b, 0x00450008, 0x0000d400, 0x11ff0040, 0xa8c073d8, 0x00e00101, 0xe914fb00, 0x0801e914, 0x0000};

const char response[] = "Response: ";

/* this should not exceed a page */
typedef struct echo_shm_vars
{
    uint64_t magic_number;
    uint64_t available;
    char fifo_buffer[FIFO_BUF_MAX];
} echo_shm_vars_t;

void shm_read(char *shmbuf, uint64_t *_available, char *buf, uint64_t size)
{
    while (size > 0)
    {
        while (_available != 1)
            ;

        int sz = size > FIFO_BUF_MAX ? FIFO_BUF_MAX : size;
        memcpy(buf, shmbuf, sz);
        size -= sz;

        _available = 0;
    }
}

void shm_write(char *shmbuf, uint64_t *_available, char *buf, uint64_t size)
{
    while (size > 0)
    {
        while (_available != 0)
            ;

        int sz = size > FIFO_BUF_MAX ? FIFO_BUF_MAX : size;
        memcpy(shmbuf, buf, sz);
        size -= sz;

        _available = 1;
    }
}

void test_send()
{
    int mode = 0;
    int size = PNUM;

    int send_num = 0;
    int i;

    int resp_len = strlen(response);
    int print_location = 10;
    sys_move_cursor(1, print_location);
    printf("[ECHO SEND SIDE]\n");
#ifdef P5_C_CORE
    char *cur = recv_buffer;
    uint64_t shmid;
    shmid = shmget(SHM_KEY);
    echo_shm_vars_t *vars = (echo_shm_vars_t *)shmat(shmid);
    if (vars == NULL)
    {
        sys_move_cursor(1, 1);
        printf("shmpageget failed!\n");
        return -1;
    }

    shm_read(vars->fifo_buffer, &vars->available, recv_buffer, size * sizeof(PSIZE));
    shm_read(vars->fifo_buffer, &vars->available, recv_length, size * sizeof(uint64_t));

    for (int i = 0; i < size; ++i)
    {
        sys_move_cursor(1, print_location);
        printf("No.%d packet, recv_length[i] = %d ...\n", i, recv_length[i]);
        memcpy(cur + HDR_OFFSET, response, resp_len);

        sys_net_send(cur, recv_length[i], 1);
        send_num += 1;
        sys_move_cursor(1, print_location + 1);
        printf("[ECHO TASK] Echo no.%d packets ...\n", i);
        cur += recv_length[i];
    }

    shmpagedt((void *)vars);
#else

    for (int i = 0; i < 1; ++i)
    {
        sys_move_cursor(1, print_location);
        printf("No.%d packet, recv_length[i] = %d ...\n", i, PSIZE);

        do_net_send((uint64_t)send_buffer, PSIZE, PNUM);
        // sys_net_send(buffer, PSIZE, PNUM);
        send_num += 1;
        sys_move_cursor(1, print_location + 1);
        printf("[ECHO TASK] Echo no.%d packets ...\n", i);
        sys_sleep(5);
    }
#endif
    sys_exit();
}

void test_recv()
{
    int mode = 0;
    int size = PNUM;
#ifdef P5_C_CORE
    uint64_t shmid;
    shmid = shmget(SHM_KEY);
    echo_shm_vars_t *vars = (echo_shm_vars_t *)shmat(shmid);
    if (vars == NULL)
    {
        sys_move_cursor(1, 1);
        printf("shmpageget failed!\n");
        return -1;
    }

    sys_move_cursor(1, 1);

    vars->available = 0;

    sys_move_cursor(1, 1);
    printf("[ECHO TASK] start recv(%d):                    \n", size);

    int ret = sys_net_recv(recv_buffer, size * PSIZE, size, recv_length);
    shm_write(vars->fifo_buffer, &vars->available, recv_buffer, size * PSIZE);
    shm_write(vars->fifo_buffer, &vars->available, recv_length, size * sizeof(uint64_t));

    shmdt((void *)vars);
#else
    sys_move_cursor(1, 1);
    printf("[ECHO TASK] start recv(%d):                    \n", size);

    int ret = do_net_recv(recv_buffer, size * PSIZE, size, recv_length);
    // int ret = sys_net_recv(recv_buffer, size * PSIZE, size, recv_length);

#endif
    sys_exit();
}
