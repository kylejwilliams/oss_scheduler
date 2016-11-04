#ifndef OSS_H
#define OSS_H

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

#include "queue.h"

typedef struct { int s, ns; } myclock_t;
typedef struct { pid_t pid; int time_created; int quantum; int ttl_burst_time; } pcb_t;
typedef struct { long mtype; pid_t pid; } msgbuf_t;

void display_time(myclock_t clck);
void advance_time(myclock_t *clck, int ns);
// void clean_and_exit(int exit_status,
//     int *id_clock,
//     int bv[],
//     myclock_t *clck,
//     int *id_pcb,
//     pcb_t *pcb);

int spawn_process(pcb_t *pcb, int bv[]);

#endif
