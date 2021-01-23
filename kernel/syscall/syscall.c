#include "fs.h"
#include "sem.h"
#include "irq.h"
#include "cond.h"
#include "lock.h"
#include "sched.h"
#include "common.h"
#include "screen.h"
#include "barrier.h"
#include "syscall.h"

uint64_t system_call_helper(uint64_t fn, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    return syscall[fn](arg0, arg1, arg2);
}

pid_t sys_spawn(task_info_t *info, int argc, char** argv)
{
    return invoke_syscall(SYSCALL_SPAWN, (uint64_t)info, argc, (uint64_t)argv);
}

void sys_exit(void)
{
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE);
}

void sys_sleep(uint32_t time)
{
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE);
}

int sys_kill(pid_t pid)
{
    return invoke_syscall(SYSCALL_KILL, pid, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid)
{
    invoke_syscall(SYSCALL_WAITPID, pid, IGNORE, IGNORE);
}

pid_t sys_getpid()
{
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE);
}

void sys_yield()
{
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE);
}


void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, (uint64_t)buff, IGNORE, IGNORE);
}

void sys_reflush()
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE);
}

char sys_serial_read()
{
    invoke_syscall(SYSCALL_SERIAL_READ, IGNORE, IGNORE, IGNORE);
}

void sys_serial_write(char ch)
{
    invoke_syscall(SYSCALL_SERIAL_WRITE, ch, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE);
}

void mutex_lock_init(mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_INIT, (uint64_t)lock, IGNORE, IGNORE);
}

void mutex_lock_acquire(mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_ACQUIRE, (uint64_t)lock, IGNORE, IGNORE);
}

void mutex_lock_release(mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_RELEASE, (uint64_t)lock, IGNORE, IGNORE);
}

void condition_init(condition_t *condition)
{
    invoke_syscall(SYSCALL_CONDITION_INIT, (uint64_t)condition, IGNORE, IGNORE);
}

void condition_wait(mutex_lock_t *lock, condition_t *condition)
{
    invoke_syscall(SYSCALL_CONDITION_WAIT, (uint64_t)lock, (uint64_t)condition, IGNORE);
}
void condition_signal(condition_t *condition)
{
    invoke_syscall(SYSCALL_CONDITION_SIGNAL, (uint64_t)condition, IGNORE, IGNORE);
}

void condition_broadcast(condition_t *condition)
{
    invoke_syscall(SYSCALL_CONDITION_BROADCAST, (uint64_t)condition, IGNORE, IGNORE);
}

void semaphore_init(semaphore_t *s, int val)
{
}
void semaphore_up(semaphore_t *s)
{
}

void semaphore_down(semaphore_t *s)
{
}

void barrier_init(barrier_t *barrier, int goal)
{
    invoke_syscall(SYSCALL_BARRIER_INIT, (uint64_t)barrier, goal, IGNORE);
}

void barrier_wait(barrier_t *barrier)
{
    invoke_syscall(SYSCALL_BARRIER_WAIT, (uint64_t)barrier, IGNORE, IGNORE);
}

uint64_t binsem_get(int key)
{
    return invoke_syscall(SYSCALL_BINSEM_GET, key, IGNORE, IGNORE);
}

void binsem_op(uint64_t binsem_id, int op)
{
    invoke_syscall(SYSCALL_BINSEM_OP, binsem_id, op, IGNORE);
}

mailbox_t *mbox_open(char *name)
{
    return (void *)invoke_syscall(SYSCALL_MAILBOX_OPEN, (uint64_t)name, IGNORE, IGNORE);
}

void mbox_close(mailbox_t *mailbox)
{
    invoke_syscall(SYSCALL_MAILBOX_CLOSE, (uint64_t)mailbox, IGNORE, IGNORE);
}

void mbox_send(mailbox_t *mailbox, void *msg, int msg_length)
{
    invoke_syscall(SYSCALL_MAILBOX_SEND, (uint64_t)mailbox, (uint64_t)msg, msg_length);
}

void mbox_recv(mailbox_t *mailbox, void *msg, int msg_length)
{
    invoke_syscall(SYSCALL_MAILBOX_RECV, (uint64_t)mailbox, (uint64_t)msg, msg_length);
}

int sys_read_shell_buff(char *buff)
{
}

void sys_process_show(void)
{
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE);
}

void sys_screen_clear(int line1, int line2)
{
    invoke_syscall(SYSCALL_SCREEN_CLEAR, line1, line2, IGNORE);
}

int sys_init_file_system(int part_id) {
    invoke_syscall(SYSCALL_FS_INIT, part_id, IGNORE, IGNORE);
}
int sys_mount_file_system(int part_id) {
    invoke_syscall(SYSCALL_FS_MOUNT, part_id, IGNORE, IGNORE);
}
int sys_unmount_file_system() {
    invoke_syscall(SYSCALL_FS_UNMOUNT, IGNORE, IGNORE, IGNORE);
}
void sys_print_file_system_info() {
    invoke_syscall(SYSCALL_FS_INFO, IGNORE, IGNORE, IGNORE);
}
void sys_sink() {
    invoke_syscall(SYSCALL_FS_SINK, IGNORE, IGNORE, IGNORE);
}

void sys_rmdir(uint32_t inode_id) {
    invoke_syscall(SYSCALL_FS_RMDIR, inode_id, IGNORE, IGNORE);
}
uint32_t sys_find(char * test_file_name) {
    invoke_syscall(SYSCALL_FS_FIND_FILE, (uint64_t)test_file_name, IGNORE, IGNORE);
}
uint32_t sys_find_dir(char * test_file_name) {
    invoke_syscall(SYSCALL_FS_FIND_DIR, (uint64_t)test_file_name, IGNORE, IGNORE);
}
void sys_cd(char * test_file_name) {
    invoke_syscall(SYSCALL_FS_CD, (uint64_t)test_file_name, IGNORE, IGNORE);
}
void sys_mkdir(uint32_t fa_inode_id, char * name) {
    invoke_syscall(SYSCALL_FS_MKDIR, fa_inode_id, (uint64_t)name, IGNORE);
}
void sys_touch(uint32_t fa_inode_id, char * name) {
    invoke_syscall(SYSCALL_FS_TOUCH, fa_inode_id, (uint64_t)name, IGNORE);
}
void sys_ls(uint32_t inode_id) {
    invoke_syscall(SYSCALL_FS_LS, inode_id, IGNORE, IGNORE);
}

int sys_fopen(char *name, uint32_t access) {
    invoke_syscall(SYSCALL_FS_OPEN, (uint64_t)name, access, IGNORE);
}
int sys_fwrite(uint32_t fd, char *buff, uint32_t size) {
    invoke_syscall(SYSCALL_FS_WRITE, fd, (uint64_t)buff, size);
}
int sys_fread(uint32_t fd, char *buff, uint32_t size) {
    invoke_syscall(SYSCALL_FS_READ, fd, (uint64_t)buff, size);
}
int sys_fseek(uint32_t fd, int offset) {
    invoke_syscall(SYSCALL_FS_SEEK, fd, offset, IGNORE);
}
int sys_fclose(uint32_t fd) {
    invoke_syscall(SYSCALL_FS_CLOSE, fd, IGNORE, IGNORE);
}
int sys_cat(char *name) {
    invoke_syscall(SYSCALL_FS_CAT, (uint64_t)name, IGNORE, IGNORE);
}

uint32_t sys_net_recv(uint64_t rd, uint64_t rd_phy, uint64_t daddr)
{
}

void sys_net_send(uint64_t td, uint64_t td_phy)
{
}

void sys_init_mac()
{
}

uint64_t sys_get_timer() {
    invoke_syscall(SYSCALL_GET_TIMER, IGNORE, IGNORE, IGNORE);
}