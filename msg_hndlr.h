
#ifndef MSG_HNDLER_H
#define MSG_HNDLER_H

#include <time.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

typedef struct { long mtype; pid_t pid; } msgbuf_t;

int send_msg(int qid, msgbuf_t *msg);
int get_msg(int qid, int msgtype, msgbuf_t *msg);

#endif
