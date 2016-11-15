
#ifndef OSS_H
#define OSS_H

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>

typedef struct { int s, ns; } myclock_t;
typedef struct
{
    pid_t pid;

    int wait_time_s;
    int wait_time_ns;
    int ttl_burst_time;
    int remaining_time;
    int is_scheduled;
    int is_finished;
    int quantum;
    int time_this_burst;

    // should both be myclock_t, but weird memory problems putting it into shared
    // memory since since they would be a struct inside of a struct
    int time_since_last_burst_s;
    int time_since_last_burst_ns;
    int arrival_time_s;
    int arrival_time__ns;
} pcb_t;

void display_time(myclock_t clck);
void advance_time(myclock_t *clck, int ns);

int spawn_process(pcb_t **pcb, int bv[]);

#endif
