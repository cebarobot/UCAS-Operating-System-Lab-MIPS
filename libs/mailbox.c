#include "string.h"
#include "mailbox.h"

#define MAX_NUM_BOX 32

mailbox_t mboxs[MAX_NUM_BOX];
mutex_lock_t alloc_lock;

static void init_mbox(mailbox_t *mailbox, char * name)
{
    strcpy(mailbox->name, name);
    mailbox->user_cnt = 0;
    mailbox->buff_used = 0;
    mailbox->start = 0;
    mailbox->end = 0;

    do_mutex_lock_init(&mailbox->lock);
    do_condition_init(&mailbox->not_full);
    do_condition_init(&mailbox->not_empty);
}

void mbox_init()
{
    
}

mailbox_t *do_mbox_open(char *name)
{
    mailbox_t * mbox = NULL;
    int hash = 0;
    for (int i = 0; name[i]; i++)
    {
        hash = (hash + name[i]) % MAX_NUM_BOX;
    }
    for (int i = 0; i < MAX_NUM_BOX; i++)
    {
        int mbox_id = (i + hash) % MAX_NUM_BOX;
        if (mboxs[mbox_id].user_cnt == 0)
        {
            init_mbox(&mboxs[mbox_id], name);
            mbox = &mboxs[mbox_id];
            break;
        }
        else if (strcmp(name, mboxs[mbox_id].name) == 0)
        {
            mbox = &mboxs[mbox_id];
            break;
        }
    }
    if (mbox)
    {
        // TODO need to record who open this mbox
        mbox->user_cnt += 1;
        return mbox;
    }
    return NULL;
}

void do_mbox_close(mailbox_t *mailbox)
{
    // TODO need to record who open this mbox
    if (mailbox->user_cnt > 0)
    {
        mailbox->user_cnt -= 1;
    }
}

void do_mbox_send(mailbox_t *mailbox, void *msg, int msg_length)
{
    do_mutex_lock_acquire(&mailbox->lock);
    while (mailbox->buff_used + msg_length > MAX_MBOX_LENGTH)
    {
        do_condition_wait(&mailbox->lock, &mailbox->not_full);
    }
    mailbox->buff_used += msg_length;

    uint8_t * src = msg;
    uint8_t * dst = mailbox->buff;
    for (int i = 0; i < msg_length; i++)
    {
        dst[(mailbox->end + i) % MAX_MBOX_LENGTH] = src[i];
    }
    mailbox->end = (mailbox->end + msg_length) % MAX_MBOX_LENGTH;

    // do_condition_signal(&mailbox->not_empty);
    do_condition_broadcast(&mailbox->not_empty);
    do_mutex_lock_release(&mailbox->lock);
}

void do_mbox_recv(mailbox_t *mailbox, void *msg, int msg_length)
{
    do_mutex_lock_acquire(&mailbox->lock);
    while (mailbox->buff_used - msg_length < 0)
    {
        do_condition_wait(&mailbox->lock, &mailbox->not_empty);
    }
    mailbox->buff_used -= msg_length;
    
    uint8_t * src = mailbox->buff;
    uint8_t * dst = msg;
    for (int i = 0; i < msg_length; i++)
    {
        dst[i] = src[(mailbox->start + i) % MAX_MBOX_LENGTH];
    }
    mailbox->start = (mailbox->start + msg_length) % MAX_MBOX_LENGTH;
    
    do_condition_broadcast(&mailbox->not_full);
    do_mutex_lock_release(&mailbox->lock);
}