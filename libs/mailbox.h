#ifndef INCLUDE_MAIL_BOX_
#define INCLUDE_MAIL_BOX_

#include "type.h"
#include "sync.h"

#define MAX_MBOX_LENGTH 64
#define MAX_NAME_LENGTH 32

typedef struct mailbox
{
    char name[MAX_NAME_LENGTH];
    uint8_t buff[MAX_MBOX_LENGTH];

    int user_cnt;
    int buff_used;
    int start, end;

    mutex_lock_t lock;
    condition_t not_full;
    condition_t not_empty;
} mailbox_t;


void mbox_init();
mailbox_t *do_mbox_open(char *);
void do_mbox_close(mailbox_t *);
void do_mbox_send(mailbox_t *, void *, int);
void do_mbox_recv(mailbox_t *, void *, int);

#endif