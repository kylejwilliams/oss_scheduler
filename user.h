#ifndef USER_H
#define USER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>

#include "msg_hndlr.h"

typedef struct { int s, ns; } myclock_t;
typedef struct { pid_t pid; int time_created; int quantum; int ttl_burst_time; } pcb_t;

#endif
