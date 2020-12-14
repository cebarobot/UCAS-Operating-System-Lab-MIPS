#include "sched.h"
#include "screen.h"
#include "stdio.h"
#include "syscall.h"
#include "time.h"
#include "string.h"

#include "test4.h"

#define RW_TIMES 3

uint64_t rand_seed;
void srand(int x) {
	rand_seed = x;
}

int rand() {
	rand_seed = rand_seed * 22695477 + 1;
	return rand_seed;
}

void rw_task1(int argc, char *argv[])
{
	// sys_move_cursor(1, 1);
	// printf("user argc: %d", argc);
	// sys_move_cursor(1, 2);
	// printf("user argv: %s %s %s", argv[0], argv[1], argv[2]);
	// sys_move_cursor(1, 3);
	// printf("%x", argv);
	
	int mem1, mem2 = 0;
	int curs = 0;
	int memory[RW_TIMES * 2];

	int i = 0;

	srand((uint32_t)sys_get_timer());
	for (i = 0; i < RW_TIMES; i++)
	{
		sys_move_cursor(1, curs + i);
		mem1 = atoi(argv[i + 2]);

		memory[i] = mem2 = rand();
		*(int *)mem1 = mem2;
		printf("Write: 0x%x,%d", mem1, mem2);
	}
	curs = RW_TIMES;
	for (i = 0; i < RW_TIMES; i++)
	{
		sys_move_cursor(1, curs + i);
		mem1 = atoi(argv[RW_TIMES + i + 2]);

		memory[i + RW_TIMES] = *(int *)mem1;
		if (memory[i + RW_TIMES] == memory[i])
			printf("Read succeed: 0x%x,%d", mem1, memory[i + RW_TIMES]);
		else
			printf("Read error: 0x%x,%d", mem1, memory[i + RW_TIMES]);
	}
	sys_exit();
}
