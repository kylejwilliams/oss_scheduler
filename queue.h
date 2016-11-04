
#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>

typedef struct node node_t;
struct node { pid_t pid; int time_slice; node_t *ptr; };

pid_t front_element();
void enq(pid_t pid);
int deq();
int empty();
void create();
int queue_size();

#endif
